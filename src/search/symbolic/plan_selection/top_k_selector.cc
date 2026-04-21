#include "top_k_selector.h"

#include "../../state_registry.h"
#include "../../task_utils/task_properties.h"

#include "../cost.h"

using namespace std;

namespace symbolic {
TopKSelector::TopKSelector(int k, bool dump_plans, bool write_plans)
    : sym_vars(nullptr),
      state_registry(nullptr),
      anytime_completness(false),
      dump_plans(dump_plans),
      write_plans(write_plans),
      num_desired_plans(k),
      num_accepted_plans(0),
      num_rejected_plans(0),
      plan_mgr_task_proxy(*tasks::g_root_task),
      first_accepted_plan_cost(Cost::MAX) {
}

void TopKSelector::init(shared_ptr<SymVariables> sym_vars, const shared_ptr<AbstractTask> &task, PlanManager &plan_manager) {
    this->sym_vars = sym_vars;
    state_registry = make_shared<StateRegistry>(TaskProxy(*task));
    plan_mgr = plan_manager;
    states_accepted_goal_paths = sym_vars->zeroBDD();
}

bool TopKSelector::has_accepted_plan(const Plan &plan) const {
    size_t plan_seed = get_hash_value(plan);
    if (hashes_accepted_plans.count(plan_seed) == 0) {
        return false;
    }
    if (different(hashes_accepted_plans.at(plan_seed), plan)) {
        return false;
    }
    return true;
}

bool TopKSelector::has_rejected_plan(const Plan &plan) const {
    size_t plan_seed = get_hash_value(plan);
    if (hashes_rejected_plans.count(plan_seed) == 0) {
        return false;
    }
    if (different(hashes_rejected_plans.at(plan_seed), plan)) {
        return false;
    }
    return true;
}

const Plan &TopKSelector::get_first_accepted_plan() const {
    return first_accepted_plan;
}

void TopKSelector::print_options() const {
    utils::g_log << "Plan files: " << plan_mgr.get_plan_filename() << endl;
}

size_t TopKSelector::different(
    const vector<Plan> &plans, const Plan &plan) const {
    for (auto &cur : plans) {
        if (cur.size() == plan.size()) {
            bool same = true;
            for (size_t i = 0; i < cur.size(); ++i) {
                if (cur.at(i) != plan.at(i)) {
                    same = false;
                    break;
                }
            }
            if (same) {
                return false;
            }
        }
    }
    return true;
}

BDD TopKSelector::get_final_state(const Plan &plan) const {
    State cur = state_registry->get_initial_state();
    for (auto &op : plan) {
        assert(task_properties::is_applicable(
            state_registry->get_task_proxy().get_operators()[op.get_index()],
            cur));
        cur = state_registry->get_successor_state(
            cur, state_registry->get_task_proxy().get_operators()[op]);
    }
    return sym_vars->getStateBDD(cur);
}

// The FD successor generator does sometimes has issues with conditional effects
// e.g., in settlers-opt18-adl + p02.pddl.
// In the long run we want to change it here to use our symbolic data structures
BDD TopKSelector::states_on_path(const Plan &plan) {
    State cur = state_registry->get_initial_state();
    BDD path_states = sym_vars->getStateBDD(cur);
    for (auto &op : plan) {
        cur = state_registry->get_successor_state(
            cur, state_registry->get_task_proxy().get_operators()[op]);
        path_states += sym_vars->getStateBDD(cur);
    }
    return path_states;
}

// Hashes a vector of ints (= a plan)
// According to the following link this is the hash function used by boost
// for hashing vector<int>. Experience: really good function
// https://stackoverflow.com/questions/20511347/a-good-hash-function-for-a-vector

size_t TopKSelector::get_hash_value(const Plan &plan) const {
    size_t seed = plan.size();
    for (auto &op : plan) {
        seed ^= op.get_index() + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
    return seed;
}

void TopKSelector::save_accepted_plan(const Plan &plan) {
    if (num_accepted_plans == 0) {
        first_accepted_plan = plan;
        first_accepted_plan_cost =
            Cost::plan_cost(plan, state_registry->get_task_proxy());

        if (!write_plans) {
            plan_mgr.save_plan(
                plan, plan_mgr_task_proxy, false, num_desired_plans > 1);
        }
    }

    size_t plan_seed = get_hash_value(plan);
    if (hashes_accepted_plans.count(plan_seed) == 0) {
        hashes_accepted_plans[plan_seed] = vector<Plan>();
    }
    hashes_accepted_plans[plan_seed].push_back(plan);
    states_accepted_goal_paths += states_on_path(plan);
    num_accepted_plans++;

    utils::g_log << "Found plan [" << num_accepted_plans << "/" << num_desired_plans << "] with cost: " << Cost::plan_cost(plan, plan_mgr_task_proxy) << endl;
    if (write_plans) {
        plan_mgr.save_plan(plan, plan_mgr_task_proxy, dump_plans, num_desired_plans > 1);
    } else if (dump_plans) {
        plan_mgr.dump_plan(plan, plan_mgr_task_proxy);
    }
}

void TopKSelector::save_rejected_plan(const Plan &plan) {
    size_t plan_seed = get_hash_value(plan);
    if (hashes_rejected_plans.count(plan_seed) == 0) {
        hashes_rejected_plans[plan_seed] = vector<Plan>();
    }
    hashes_rejected_plans[plan_seed].push_back(plan);
    states_accepted_goal_paths += states_on_path(plan);
    num_rejected_plans++;
}

bool TopKSelector::has_zero_cost_loop(const Plan &plan) const {
    State cur = state_registry->get_initial_state();
    BDD zero_reachable = sym_vars->getStateBDD(cur);
    for (auto &op : plan) {
        cur = state_registry->get_successor_state(
            cur, state_registry->get_task_proxy().get_operators()[op]);
        BDD new_state = sym_vars->getStateBDD(cur);

        if (state_registry->get_task_proxy().get_operators()[op].get_cost() !=
            0) {
            zero_reachable = new_state;
        } else {
            BDD intersection = zero_reachable * new_state;
            if (!intersection.IsZero()) {
                return true;
            }
            zero_reachable += new_state;
        }
    }

    return false;
}

pair<int, int> TopKSelector::get_first_zero_cost_loop(const Plan &plan) const {
    pair<int, int> zero_cost_op_seq(-1, -1);
    int last_zero_op_state = 0;
    vector<State> states;
    states.push_back(state_registry->get_initial_state());
    for (size_t op_i = 0; op_i < plan.size(); ++op_i) {
        State succ = state_registry->get_successor_state(
            states.back(),
            state_registry->get_task_proxy().get_operators()[plan[op_i]]);

        for (size_t state_i = last_zero_op_state; state_i < states.size();
             ++state_i) {
            if (states[state_i].get_id() == succ.get_id()) {
                zero_cost_op_seq.first = state_i;
                zero_cost_op_seq.second = op_i;
                break;
            }
        }
        if (state_registry->get_task_proxy()
                .get_operators()[plan[op_i]]
                .get_cost() != 0) {
            last_zero_op_state = states.size() - 1;
        }

        if (zero_cost_op_seq.first != -1) {
            break;
        }
        states.push_back(succ);
    }

    if (zero_cost_op_seq.first == -1) {
        cerr << "Zero loop goes wrong!" << endl;
        exit(0);
    }
    return zero_cost_op_seq;
}

vector<Plan> TopKSelector::get_accepted_plans() const {
    vector<Plan> res;
    for (auto &it : hashes_accepted_plans) {
        res.insert(res.end(), it.second.begin(), it.second.end());
    }
    return res;
}

void TopKSelector::add_plan(const Plan &plan) {
    if (!has_accepted_plan(plan)) {
        save_accepted_plan(plan);
    }
}
}
