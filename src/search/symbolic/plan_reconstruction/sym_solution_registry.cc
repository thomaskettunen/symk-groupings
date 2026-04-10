#include "sym_solution_registry.h"

#include "../searches/uniform_cost_search.h"

#include "../cost.h"

#include "../pareto_front.h"

using namespace std;

namespace symbolic {
void SymSolutionRegistry::add_plan(const Plan &plan) const { 
    assert(!(plan_data_base->has_zero_cost_loop(plan)));
    plan_data_base->add_plan(plan);
}

void SymSolutionRegistry::reconstruct_plans( // NOTE: P10: When this is called we already know what some plan cost is
    const vector<SymSolutionCut> &sym_cuts) {
    assert(queue.empty());

    for (const SymSolutionCut &sym_cut : sym_cuts) {
        assert(fw_closed || sym_cut.get_g() == Cost::MIN);
        assert(bw_closed || sym_cut.get_h() == Cost::MIN);

        if(pareto_front::dominates(sym_cut.get_f())) continue; // NOTE: P10: if the current plan is dominated we should just not even push it to the queue

        ReconstructionNode cur_node(
            sym_cut.get_g(), 
            sym_cut.get_h(), 
            std::numeric_limits<int>::max(), 
            sym_cut.get_cut(), 
            sym_vars->zeroBDD(), 
            fw_closed != nullptr, 0
        );
        queue.push(cur_node);

        // In the bidirectional case we might can directly swap the direction
        if (swap_to_bwd_phase(cur_node)) {
            ReconstructionNode bw_node = cur_node;
            bw_node.set_states(fw_closed->get_start_states());
            bw_node.set_visited_states(fw_closed->get_start_states());
            bw_node.set_fwd_phase(false);
            queue.push(bw_node);
        }
    }

    if (!queue.empty()) {
        symbolic::Cost cost = queue.top().get_f();

        if (pareto_front::dominates(cost)) { utils::g_log << "dominated " << cost << std::endl; } 
        else { utils::g_log << "found non dominated plan: " << cost << std::endl; }

        pareto_front::insert(cost); // NOTE: P10: we get the total price for the current plan we want
        // NOTE: P10: the queue for some reason finds all possible reorderings, where the hell does it do this
    }

    while (!queue.empty()) { // NOTE: P10: for some reason the queue counts down until it reaches the cost[0,0,0] ??? and none of these will ever be dominated obviously
        ReconstructionNode cur_node = queue.top();
        queue.pop();

        // We extract a single state form the relevant states and process it (because we do simple solutions only?)
        if (sym_vars->numStates(cur_node.get_states()) > 1) {
            BDD state_bdd = sym_vars->getSinglePrimaryStateFrom(cur_node.get_states());
            ReconstructionNode remaining_node = cur_node;
            remaining_node.set_states(remaining_node.get_states() * !state_bdd);
            if (sym_vars->numStates(remaining_node.get_states()) > 0) { queue.push(remaining_node); }
            assert(sym_vars->numStates(remaining_node.get_states()) > 0 || remaining_node.get_states() == sym_vars->zeroBDD());
            cur_node.set_states(state_bdd);
        }
        cur_node.add_visited_states(cur_node.get_states());

        assert(sym_vars->numStates(cur_node.get_states()) > 0);
        assert(sym_vars->numStates(cur_node.get_states()) == 1);
        assert(cur_node.get_plan_length() + 1 == sym_vars->numStates(cur_node.get_visitied_states()));

        // Check if we have found a solution with this cut
        if (is_solution(cur_node)) { // NOTE: P10: Here we check if the current node we are looking at is a solution, maybe checking if not dominated here would result in getting the plans we want
            Plan cur_plan;
            cur_node.get_plan(cur_plan); // we get the current plan for the node we are looking for
            add_plan(cur_plan);

            // Plan data base tells us if we need to continue
            // We can stop early if we, e.g., have found enough plans
            if (!plan_data_base->reconstruct_solutions(sym_cuts[0].get_f())) { // NOTE: P10: at this point the first plan has already been found
                queue = ReconstructionQueue(CompareReconstructionNodes(ReconstructionPriority::REMAINING_COST));
                return;
            }

            queue = ReconstructionQueue(CompareReconstructionNodes(ReconstructionPriority::REMAINING_COST));
            return; // NOTE: P10: this return probably shouldnt be here and we should instead check for other ways to break out of here
            // NOTE: P10: with the above return we always end up with only one plan and never come in here again
        }
        expand_actions(cur_node); // NOTE: P10: this is obviously important somehow 
    }
    assert(queue.empty());
}

void SymSolutionRegistry::expand_actions(const ReconstructionNode &node) {
    bool fwd = node.is_fwd_phase();
    Cost cur_cost = Cost::INVALID; // NOTE: P10: Placeholder, overwritten in each of the two cases below.
    shared_ptr<ClosedList> cur_closed_list;

    if (fwd) {
        cur_cost = node.get_g();
        cur_closed_list = fw_closed;
    } else {
        cur_cost = node.get_h();
        cur_closed_list = bw_closed;
    }

    // Traverse in oposite direction to first consider actions with higher costs
    // Mostly relevant for single solution reconstruction
    const auto &trs = sym_transition_relations->get_individual_transition_relations();
    for (auto it = trs.rbegin(); it != trs.rend(); ++it) {
        auto &[op_cost, transition_relations] = *it;
        Cost new_cost = cur_cost - op_cost;

        // new cost can not be negative
        if (new_cost < Cost::MIN) { continue; }

        for (auto tr : transition_relations) {
            BDD closed_states = cur_closed_list->get_closed_at(new_cost);
            BDD succ = fwd ? tr->preimage(node.get_states(), closed_states)
                           : tr->image(node.get_states());

            BDD intersection = succ * closed_states;
            int layer_id = 0;
            if (op_cost == Cost::MIN)
                layer_id = cur_closed_list->get_zero_cut(new_cost, intersection);

            // Ignore states we have already visited
            intersection *= !node.get_visitied_states();

            if (intersection.IsZero()) { continue; }

            ReconstructionNode new_node(
                Cost::INVALID,
                 Cost::INVALID,
                 layer_id,
                 intersection,
                 node.get_visitied_states(),
                 fwd,
                 node.get_plan_length() + 1
            );

            if (fwd) {
                new_node.set_g(new_cost);
                new_node.set_h(node.get_h());
                new_node.set_predecessor(make_shared<ReconstructionNode>(node), tr);
            } else {
                new_node.set_g(node.get_g());
                new_node.set_h(new_cost);
                new_node.set_successor(make_shared<ReconstructionNode>(node), tr);
            }

            // We have sucessfully reconstructed to the initial state
            if (swap_to_bwd_phase(new_node)) {
                assert(fw_closed->get_start_states() * new_node.get_states() != sym_vars->zeroBDD());
                BDD middle_state = new_node.get_middle_state(fw_closed->get_start_states());
                ReconstructionNode bw_node(
                    Cost::MIN, 
                    new_node.get_h(),
                    0,
                    middle_state,
                    new_node.get_visitied_states(),
                    false,
                    node.get_plan_length() + 1
                );

                bw_node.set_predecessor(make_shared<ReconstructionNode>(node), tr);

                // Add init state to visited states
                bw_node.add_visited_states(fw_closed->get_start_states());

                if (!pareto_front::dominates(bw_node.get_f())) { queue.push(bw_node); }
            } else {
                if (!pareto_front::dominates(new_node.get_f())) { queue.push(new_node); }
            }
        }
    }
}

bool SymSolutionRegistry::swap_to_bwd_phase(const ReconstructionNode &node) const {
    return bw_closed && node.is_fwd_phase() && node.get_g() == Cost::MIN && !(node.get_states() * fw_closed->get_start_states()).IsZero();
}

bool SymSolutionRegistry::is_solution(const ReconstructionNode &node) const {
    if (node.get_f() > Cost::MIN) return false;
    if (bw_closed && node.is_fwd_phase()) return false;
    shared_ptr<ClosedList> closed = node.is_fwd_phase() ? fw_closed : bw_closed;
    return !(node.get_states() * closed->get_start_states()).IsZero();
}

SymSolutionRegistry::SymSolutionRegistry(): fw_closed(nullptr), bw_closed(nullptr), plan_data_base(nullptr) {
    queue = ReconstructionQueue(CompareReconstructionNodes(ReconstructionPriority::REMAINING_COST));
}

void SymSolutionRegistry::init(
    shared_ptr<SymVariables> sym_vars,
    shared_ptr<symbolic::ClosedList> fw_closed,
    shared_ptr<symbolic::ClosedList> bw_closed,
    shared_ptr<SymTransitionRelations> sym_transition_relations,
    shared_ptr<TopKSelector> plan_data_base
) {
    this->sym_vars = sym_vars;
    this->plan_data_base = plan_data_base;
    this->fw_closed = fw_closed;
    this->bw_closed = bw_closed;
    this->sym_transition_relations = sym_transition_relations;

    // If unit costs we simple use sort by remaining cost
    if (sym_transition_relations->has_unit_cost()) {
        queue = ReconstructionQueue(CompareReconstructionNodes(ReconstructionPriority::REMAINING_COST));
    }

    reconstruction_timer.stop();
    reconstruction_timer.reset();
}

void SymSolutionRegistry::register_solution(const SymSolutionCut &solution) {
    bool merged = false;
    for (size_t pos = 0; pos < sym_cuts[solution.get_f()].size(); ++pos) {
        // a cut with same g and h values exist
        // => we combine the cut to avoid multiple cuts with same solutions
        if (sym_cuts[solution.get_f()][pos] == solution) {
            sym_cuts[solution.get_f()][pos].merge(solution);
            merged = true;
            break;
        }
    }
    if (!merged) { sym_cuts[solution.get_f()].push_back(solution); }
}

void SymSolutionRegistry::construct_cheaper_solutions() {
    for (auto it = sym_cuts.begin(); it != sym_cuts.end();) {
        const auto& [cost, cuts] = *it;
        if (found_k_plans()) break;

        reconstruction_timer.resume();
        reconstruct_plans(cuts);
        reconstruction_timer.stop();

        it = sym_cuts.erase(it);
    }
}
}
