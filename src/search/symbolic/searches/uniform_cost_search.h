#ifndef SYMBOLIC_UNIFORM_COST_SEARCH_H
#define SYMBOLIC_UNIFORM_COST_SEARCH_H

#include "sym_search.h"

#include "../closed_list.h"
#include "../frontier.h"
#include "../open_list.h"
#include "../sym_bucket.h"
#include "../sym_estimate.h"
#include "../sym_state_space_manager.h"
#include "../sym_utils.h"

#include <memory>

#include "../cost.h"

namespace symbolic {
class ClosedList;

class UniformCostSearch : public SymSearch {
public:
    // Current state of the search:
    bool fw; // Direction of the search. true=forward, false=backward
    std::shared_ptr<ClosedList> closed; // Closed list is a shared ptr to share
    std::shared_ptr<OpenList> open_list;
    std::shared_ptr<Frontier> frontier;
protected:

    // Opposite direction. Mostly relevant when bidirectional search is used
    std::shared_ptr<ClosedList> perfectHeuristic;
    std::shared_ptr<OpenList> oppositeOpenList; 
    std::shared_ptr<Frontier> oppositeFrontier;

    bool lastStepCost; // If the last step was a cost step (to know if we are in estimationDisjCost or Zero)

    Cost last_g_cost;

    virtual void checkFrontierCut();

    void advanceFrontier();

    virtual void filterFrontier();

    void expandFrontier(int maxTime, int maxNodes);

    //////////////////////////////////////////////////////////////////////////////
public:
    UniformCostSearch(SymbolicSearch *eng, const SymParameters &params);
    UniformCostSearch(const UniformCostSearch &) = delete;
    UniformCostSearch(UniformCostSearch &&) = default;
    UniformCostSearch &operator=(const UniformCostSearch &) = delete;
    UniformCostSearch &operator=(UniformCostSearch &&) = default;
    virtual ~UniformCostSearch() = default;

    virtual bool finished() const override {
        return open_list->empty() && frontier->empty();
    }

    void step() override {
        /*if (step_estimation.get_failed()) {
            p.increase_bound();
        }
        stepImage(p.max_alloted_time, p.max_alloted_nodes);*/
        stepImage(0, 0);
    }

    virtual std::string get_last_dir() const override {
        return fw ? "FW" : "BW";
    }

    virtual void stepImage(int maxTime, int maxNodes) override;

    bool init(
        std::shared_ptr<SymStateSpaceManager> manager, bool fw,
        UniformCostSearch *opposite_search); // Init forward or backward search

    virtual Cost getF() const override {
        return open_list->minNextG(*frontier, mgr->get_min_transition_cost());
    }

    virtual Cost getG() const {
        return frontier->empty() ? open_list->minG() : frontier->g();
    }

    std::shared_ptr<ClosedList> getClosedShared() const {
        return closed;
    }

    void filterDuplicates(Bucket &bucket);

    // Returns the nodes that have been expanded by the algorithm (closed
    // without the current frontier)
    BDD getExpanded() const;
    void getNotExpanded(Bucket &res) const;

    // void write(const std::string & file) const;

    void filter_mutex(Bucket &bucket) {
        mgr->filter_mutex(bucket, fw, false);
    }
};
}
#endif
