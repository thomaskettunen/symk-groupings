#include "top_k_uniform_cost_search.h"

#include "../closed_list.h"
#include "../search_algorithms/symbolic_search.h"
#include "../cost.h"

namespace symbolic {
/// @brief (Citation Needed) Checks if the frontier has new custs, and if so, adds new solution
/// @param bucket The frontier maybe?
/// @param g The cost of the bucket
void TopkUniformCostSearch::checkFrontierCut(Bucket &bucket, Cost g) {
    for (BDD &bucketBDD : bucket) {
        auto all_sols = perfectHeuristic->getAllCuts(bucketBDD, g, fw, engine->getMinG());
        for (auto &sol : all_sols) {
            engine->new_solution(sol);
        }
    }
}

void TopkUniformCostSearch::filterFrontier() {
    frontier.filter(closed);
    mgr->filter_mutex(frontier.bucket(), fw, initialization());
    remove_zero(frontier.bucket());
}
}