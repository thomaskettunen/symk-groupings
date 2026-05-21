#ifndef SYMBOLIC_SEARCH_ALGORITHMS_SYMBOLIC_SEARCH_H
#define SYMBOLIC_SEARCH_ALGORITHMS_SYMBOLIC_SEARCH_H

#include "../sym_enums.h"
#include "../sym_parameters.h"
#include "../sym_state_space_manager.h"

#include "../../plugins/plugin.h"
#include "../../search_algorithm.h"
#include "../plan_reconstruction/sym_solution_registry.h"

#include <memory>
#include <vector>

#include "../cost.h"

namespace options {
class Options;
}

namespace symbolic {
class SymStateSpaceManager;
class SymSearch;
class TopKSelector;
class SymVariables;

class SymbolicSearch : public SearchAlgorithm {
private:
protected:
    // Hold a reference to the task implementation and pass it to objects that
    // need it.
    const std::shared_ptr<AbstractTask> task;
    std::shared_ptr<AbstractTask> search_task;
    // Use task_proxy to access task information.
    TaskProxy task_proxy;

    // Symbolic manager to perform bdd operations
    std::shared_ptr<SymStateSpaceManager> mgr;

    std::unique_ptr<SymSearch> search;

    std::shared_ptr<SymVariables> vars; // The symbolic variables are declared

    SymParameters sym_params; // Parameters for symbolic search

    int step_num;

    std::shared_ptr<TopKSelector> plan_data_base;
    std::shared_ptr<SymSolutionRegistry> solution_registry; // Solution registry

    bool silent;

    virtual void initialize() override;

    virtual SearchStatus step() override;

public:
    SymbolicSearch(const plugins::Options &opts);
    virtual ~SymbolicSearch() = default;

    bool is_silent(){
        return silent;
    }

    virtual bool solved() const {
        return search_done;
    }

    virtual BDD get_states_on_goal_paths() const {
        return solution_registry->get_states_on_goal_paths();
    }

    virtual void new_solution(const SymSolutionCut &sol);

    virtual void print_statistics() const override;

    static void add_options_to_feature(plugins::Feature &feature);

    bool search_done = false;

    std::shared_ptr<OpenList> fw_open = nullptr;
    std::shared_ptr<OpenList> bw_open = nullptr;
    std::shared_ptr<Frontier> fw_frontier = nullptr;
    std::shared_ptr<Frontier> bw_frontier = nullptr;
};
}

#endif
