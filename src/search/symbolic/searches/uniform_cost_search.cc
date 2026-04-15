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

using namespace std;

namespace symbolic {
UniformCostSearch::UniformCostSearch(
    SymbolicSearch *eng, const SymParameters &params)
    : SymSearch(eng, params),
      fw(true),
      step_estimation(0, 0, false),
      closed(make_shared<ClosedList>()),
      open_list(make_shared<OpenList>()),
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
    frontier.init(manager.get(), init_bdd);

    closed->init(mgr.get());
    closed->insert(Cost::MIN, init_bdd);

    if (opposite_search) {
        perfectHeuristic = opposite_search->getClosedShared();
        oppositeOpenList = opposite_search->open_list;
    } else {
        perfectHeuristic = make_shared<ClosedList>();
        perfectHeuristic->init(mgr.get());
        if (fw) {
            perfectHeuristic->insert(Cost::MIN, mgr->get_goal());
        } else {
            perfectHeuristic->insert(Cost::MIN, mgr->get_initial_state());
        }
    }

    prepareBucket();

    engine->setMinG(getG());

    return true;
}

void UniformCostSearch::checkFrontierCut(Bucket &bucket, Cost g) {
    if (sym_params.non_stop) {
        return;
    }

    for (BDD &bucketBDD : bucket) {
        auto sol = perfectHeuristic->getCheapestCut(bucketBDD, g, fw);
        if (sol.get_f() >= Cost::MIN) {
            engine->new_solution(sol);
        }
        // Prune everything closed in opposite direction
        bucketBDD *= perfectHeuristic->notClosed();
    }
}

bool UniformCostSearch::provable_no_more_plans() {
    return open_list->empty();
}

bool UniformCostSearch::prepareBucket() {
    if (!frontier.bucketReady()) { // NOTE: P10: Is this check really required?
        while (frontier.empty()) {
            if(open_list->empty()) { // NOTE: P10: hacky solution to stop when frontier is empty do not forge
                engine->search_done = true;
                utils::g_log << "Completed search, open list empty" << std::endl;
                return true;
            }
            open_list->pop(frontier);
            last_g_cost = frontier.g();
            if (oppositeOpenList) {
                bool dominated = !oppositeOpenList->open.empty();
                for (auto &[cost, bucket] : oppositeOpenList->open) {
                    if (!pareto_front::dominates(last_g_cost + cost)) {
                        dominated = false;
                        break;
                    }
                }
                if (dominated){
                    frontier.clear();
                    continue;
                }
            }
            checkFrontierCut(frontier.bucket(), frontier.g()); // TODO: P10: What this do?
            filterFrontier();
        }

        // Close and move to reopen
        if (!lastStepCost || frontier.g() != Cost::MIN) { // Avoid closing init twice
            for (const BDD &states : frontier.bucket()) {
                closed->insert(frontier.g(), states);
            }
        }
        engine->setMinG(getG());
    }

    return false;
}

// Here we filter states: remove closed states and mutex states
// This procedure is delayed in comparision to explicit search
// Idea: no need to "change" BDDs until we actually process them
void UniformCostSearch::filterFrontier() {
    frontier.filter(closed);
    mgr->filter_mutex(frontier.bucket(), fw, initialization());
    remove_zero(frontier.bucket());
}

void UniformCostSearch::stepImage(int maxTime, int maxNodes) {
    utils::Timer step_timer;
    bool done = prepareBucket();
    if (done) {
        return;
    }

    Result prepare_res = frontier.prepare(maxTime, maxNodes, fw, initialization());
    if (!prepare_res.ok) {
        step_estimation.set_data(step_timer(), frontier.nodes(), !prepare_res.ok);
        return;
    }

    if (engine->solved()) {
        return; // Skip image if we are done
    }

    int stepNodes = frontier.nodes();
    ResultExpansion res_expansion = frontier.expand(maxTime, maxNodes, fw);

    if (res_expansion.ok) {
        lastStepCost = false; // Must be set to false before calling checkCut
        // Process Simg, removing duplicates and computing h. Store in Sfilter
        // and reopen. Include new states in the open list
        for (auto &resImage : res_expansion.buckets) {
            for (auto &[imageCost, bucket] : resImage) {
                Cost cost = frontier.g() + imageCost;
                
                mgr->merge_bucket(bucket);

                checkFrontierCut(bucket, cost);

                for (auto &bdd : bucket) {
                    if (!bdd.IsZero()) {
                        stepNodes = max(stepNodes, bdd.nodeCount());
                        open_list->insert(bdd, cost);
                    }
                }
            }
        }
        utils::g_log << "expanded frontier [" << (fw ? "->" : "<-") << "]: " << frontier.g() << " frontier nodes: " << stepNodes << std::endl;
    }

    while (!frontier.bucketReady() && !open_list->empty()) {
        prepareBucket();
    }

    step_estimation.set_data(step_timer(), stepNodes, !res_expansion.ok);
}
}
