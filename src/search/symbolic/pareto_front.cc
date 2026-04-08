#include "pareto_front.h"
#include <set>

namespace pareto_front {
    static std::set<symbolic::Cost> paretto_frontier;

    bool dominates(const symbolic::Cost& g) {
        for (auto cost : paretto_frontier) {
            if (cost.dominates(g)) return true;
        }
        return false;
    }

    void insert(const symbolic::Cost& g) { paretto_frontier.insert(g); }
}