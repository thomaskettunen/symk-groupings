#ifndef SYMBOLIC_SEARCHES_TOP_K_UNIFORM_COST_SEARCH_H
#define SYMBOLIC_SEARCHES_TOP_K_UNIFORM_COST_SEARCH_H

#include "uniform_cost_search.h"
#include "../cost.h"

namespace symbolic {
class TopkUniformCostSearch : public UniformCostSearch {
protected:
    virtual void checkFrontierCut() override;
public:
    TopkUniformCostSearch(SymbolicSearch *eng, const SymParameters &params): UniformCostSearch(eng, params) {}
};
} // namespace symbolic

#endif
