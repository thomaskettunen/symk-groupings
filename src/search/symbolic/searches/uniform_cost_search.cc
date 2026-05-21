#include "uniform_cost_search.h"

#include "../closed_list.h"
#include "../frontier.h"
#include "../sym_utils.h"

#include "../../utils/timer.h"
#include "../plan_reconstruction/sym_solution_cut.h"
#include "../search_algorithms/symbolic_search.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "../cost.h"
#include "../pareto_front.h"

#define log(x) do { if(!engine->is_silent()) utils::g_log << "[" << (fw ? "->" : "<-") << "]: " << x << std::endl; } while(0)

using namespace std;

namespace symbolic {
UniformCostSearch::UniformCostSearch(SymbolicSearch *eng, const SymParameters &params)
    : SymSearch(eng, params),
      fw(true),
      closed(make_shared<ClosedList>(engine->is_silent())),
      open_list(make_shared<OpenList>(engine->is_silent())),
      frontier(make_shared<Frontier>(engine->is_silent())),
      lastStepCost(true),
      last_g_cost(Cost::MIN) {
}

bool UniformCostSearch::init(
    shared_ptr<SymStateSpaceManager> manager, bool forward,
    UniformCostSearch *opposite_search) {
    mgr = manager;
    fw = forward;
    lastStepCost = true;
    last_g_cost = Cost::MIN;
    assert(mgr);

    BDD init_bdd = fw ? mgr->get_initial_state() : mgr->get_goal();
    frontier->init(manager.get(), init_bdd);

    closed->init(mgr.get());
    closed->insert(Cost::MIN, init_bdd, fw);

    if (opposite_search) {
        perfectHeuristic = opposite_search->getClosedShared();
        oppositeOpenList = opposite_search->open_list;
    } else {
        perfectHeuristic = make_shared<ClosedList>(engine->is_silent());
        perfectHeuristic->init(mgr.get());
        if (fw) {
            perfectHeuristic->insert(Cost::MIN, mgr->get_goal(), fw);
        } else {
            perfectHeuristic->insert(Cost::MIN, mgr->get_initial_state(), fw);
        }
    }

    advanceFrontier();

    return true;
}

void UniformCostSearch::checkFrontierCut() {
    if (sym_params.non_stop) return;

    for (BDD &bucketBDD : frontier->bucket()) {
        auto sol = perfectHeuristic->getCheapestCut(bucketBDD, frontier->g(), fw);
        if (sol.get_f().is_valid()) {
            engine->new_solution(sol);
            log("found solution " << sol);
        }
        bucketBDD *= perfectHeuristic->notClosed(); // Prune everything closed in opposite direction
    }
}

/// @brief Advances the frontier to the next (valid) state
void UniformCostSearch::advanceFrontier() {
    Cost last_g = frontier->g();
    while (frontier->empty()) {
        if(open_list->empty()) { // NOTE: P10: hacky solution to stop when frontier is empty do not forge
            engine->search_done = true;
            utils::g_log << "Completed search, open list empty" << std::endl;
            return;
        }
        open_list->pop(*frontier);
        last_g_cost = frontier->g();
        if (oppositeOpenList) {
            bool dominated = !oppositeOpenList->open.empty();
            for (auto &[cost, bucket] : oppositeOpenList->open) {
                if (!pareto_front::dominates(last_g_cost + cost)) {
                    dominated = false;
                    break;
                }
            }
            if (dominated){
                frontier->clear();
                continue;
            }
        }
        filterFrontier();
    }
    frontier->last_g = last_g;
    log("advanced frontier from " << frontier->last_g << " to " << frontier->g());
}

// Here we filter states: remove closed states and mutex states
// This procedure is delayed in comparision to explicit search
// Idea: no need to "change" BDDs until we actually process them
void UniformCostSearch::filterFrontier() {
    frontier->filter(closed);
    mgr->filter_mutex(frontier->bucket(), fw, false);
    remove_zeroBDDs(frontier->bucket());
}

/// @brief Expands the current frontier into the open list; empties the frontiers state
void UniformCostSearch::expandFrontier(int maxTime, int maxNodes) {
    ExpansionResult res_expansion = frontier->expand(maxTime, maxNodes, fw);
    assert(res_expansion.ok);

    lastStepCost = false; /// TODO: P10: Must be set to flase before check cut?
    for (auto &resImage : res_expansion.buckets) {
        for (auto &[imageCost, bucket] : resImage) {
            Cost cost = frontier->g() + imageCost;
            
            mgr->merge_bucket(bucket); // NOTE: P10: (Potentially) merges some of the BDDs in the bucket

            for (auto &bdd : bucket) {
                assert(!bdd.IsZero()); // NOTE: P10: merge_bucket above removes zeroBDDs
                open_list->insert(bdd, cost);
            }
        }
    }
    log("expanded frontier: " << frontier->g());
}

void UniformCostSearch::stepImage(int maxTime, int maxNodes) {
    log("UniformCostSearch::stepImage");
    checkFrontierCut();
    for (const BDD &states : frontier->bucket()) closed->insert(frontier->g(), states, fw);
    if (engine->solved()) return;
    expandFrontier(maxTime, maxNodes);
    advanceFrontier();
}
}
