#include "top_k_uniform_cost_search.h"

#include "../closed_list.h"
#include "../search_algorithms/symbolic_search.h"
#include "../cost.h"

#define log(x) do { if(!engine->is_silent()) utils::g_log << "[" << (fw ? "->" : "<-") << "]: " << x << std::endl; } while(0)

namespace symbolic {
/// @brief (Citation Needed) Checks if the frontier has new custs, and if so, adds new solution
/// @param bucket The frontier maybe?
/// @param g The cost of the bucket
void TopkUniformCostSearch::checkFrontierCut() {
    log("checking frontier: " << frontier->g());
    for (BDD &bucketBDD : frontier->bucket()) {
        auto all_sols = perfectHeuristic->getAllCuts(bucketBDD, frontier->g(), fw);
        for (auto &sol : all_sols) {
            engine->new_solution(sol);
            log("found solution " << sol);
        }
    }
}
}