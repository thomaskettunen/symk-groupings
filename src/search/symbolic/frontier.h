#ifndef SYMBOLIC_FRONTIER_H
#define SYMBOLIC_FRONTIER_H

#include "sym_bucket.h"

#include "searches/sym_search.h"

#include <cassert>
#include <map>

#include "cost.h"
#include "closed_list.h"

namespace symbolic {
class SymStateSpaceManager;

class Result {
public:
    bool ok;
    TruncatedReason truncated_reason;
    double time_spent;

    Result(double t) : ok(true), time_spent(t) {}
    Result(TruncatedReason reason, double t) : ok(false), truncated_reason(reason), time_spent(t) {}
};

class ExpansionResult : public Result {
public:
    bool step_zero;
    std::vector<std::map<Cost, Bucket>> buckets;
    ExpansionResult(bool step_zero_, std::vector<std::map<Cost, Bucket>> &buckets_, double t) : Result(t), step_zero(step_zero_) {
        buckets.swap(buckets_);
    }

    ExpansionResult(bool step_zero_, TruncatedReason reason, double t)
        : Result(reason, t), step_zero(step_zero_) {
    }
};

class Frontier { // Current states extracted from the open list
    SymStateSpaceManager *mgr;

    Bucket states; 
    Cost g_value;

    ExpansionResult expand_zero(int maxTime, int maxNodes, bool fw);
    ExpansionResult expand_cost(int maxTime, int maxNodes, bool fw);

    bool silent = false;
public:
    Cost last_g;
    Frontier(bool silent) : mgr(nullptr), g_value(Cost::INVALID), last_g(Cost::INVALID), silent(silent) { }

    void init(SymStateSpaceManager *mgr_, const BDD &bdd) {
        mgr = mgr_;
        states.push_back(bdd);
        g_value = Cost::MIN;
    }

    void set(Cost g, Bucket &bdd) {
        assert(empty());
        g_value = g;
        states.swap(bdd);
    }

    void clear() { Bucket().swap(states); }

    bool empty() const {return states.empty(); }
    int nodes() const { return nodeCount(states); }
    int buckets() const { return states.size(); }
    Cost g() const { return g_value; }
    Bucket &bucket() { return states; }

    void filter(const std::shared_ptr<ClosedList> closed);

    ExpansionResult expand(int maxTime, int maxNodes, bool fw) {
        if (false) { // TODO: P10: Ignore zero cost for now
            return expand_zero(maxTime, maxNodes, fw);
        }

        // Image with respect to cost actions
        return expand_cost(maxTime, maxNodes, fw);
    }

    friend std::ostream &operator<<(std::ostream &os, const Frontier &frontier);
};
}
#endif
