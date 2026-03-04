#ifndef SYMBOLIC_SYM_TRANSITION_RELATIONS_H
#define SYMBOLIC_SYM_TRANSITION_RELATIONS_H

#include "sym_mutexes.h"
#include "sym_parameters.h"
#include "sym_variables.h"

#include "transition_relations/conjunctive_transition_relation.h"
#include "transition_relations/disjunctive_transition_relation.h"
#include "transition_relations/transition_relation.h"

#include <algorithm>

#include "cost.h"

namespace extra_tasks {
class SdacTask;
}

namespace symbolic {
class SymTransitionRelations {
    SymVariables *sym_vars;
    const SymParameters &sym_params;

    std::map<Cost, std::vector<ConjunctiveTransitionRelation>>
        individual_conj_transitions;
    std::map<Cost, std::vector<DisjunctiveTransitionRelation>>
        individual_disj_transitions;
    std::map<Cost, std::vector<TransitionRelationPtr>> individual_transitions;

    std::map<Cost, std::vector<DisjunctiveTransitionRelation>>
        disj_transitions; // Merged TRs
    std::map<Cost, std::vector<TransitionRelationPtr>> transitions; // Merged TRs
    Cost min_transition_cost; // minimum cost of non-zero cost transitions

    void init_individual_transitions(
        const std::shared_ptr<AbstractTask> &task,
        const SymMutexes &sym_mutexes);
    void create_single_trs(
        const std::shared_ptr<AbstractTask> &task,
        const SymMutexes &sym_mutexes);
    void create_merged_transitions();
    void move_monolithic_conj_transitions();

    template<class T>
    int get_size(std::map<Cost, std::vector<T>> transitions) const;

public:
    SymTransitionRelations(
        SymVariables *sym_vars, const SymParameters &sym_params);
    void init(
        const std::shared_ptr<AbstractTask> &task,
        const SymMutexes &sym_mutexes);

    Cost get_min_transition_cost() const;
    bool has_zero_cost_transition() const;
    bool has_unit_cost() const;

    const std::map<Cost, std::vector<TransitionRelationPtr>> &
    get_transition_relations() const;
    const std::map<Cost, std::vector<TransitionRelationPtr>> &
    get_individual_transition_relations() const;
};
}

#endif
