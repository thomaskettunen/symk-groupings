#ifndef SYMBOLIC_SEARCH_ALGORITHMS_TOP_K_SYMBOLIC_UNIFORM_COST_SEARCH_H
#define SYMBOLIC_SEARCH_ALGORITHMS_TOP_K_SYMBOLIC_UNIFORM_COST_SEARCH_H

#include "symbolic_search.h"
#include "../grouping.h"

namespace symbolic {
class TopkSymbolicUniformCostSearch : public SymbolicSearch {
private:
    bool fw;
    bool bw;
    bool alternating;
    virtual void initialize() override;
    
public:
    std::shared_ptr<grouping::GroupingFunction> grouping_function;
    TopkSymbolicUniformCostSearch(const plugins::Options &opts, bool fw, bool bw, std::shared_ptr<grouping::GroupingFunction> grouping_function, bool alternating = false);
    ~TopkSymbolicUniformCostSearch() = default;

    void new_solution(const SymSolutionCut &sol) override;
};
}

#endif
