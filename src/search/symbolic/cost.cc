#include "cost.h"
#include "grouping.h"
#include "../plan_manager.h"
#include "../task_proxy.h"
#include "../utils/hash.h"
#include "../abstract_task.h"

#include <algorithm>
#include <limits>
#include <ostream>
#include <string>
#include <memory>
#include <set>

template<typename K, typename V> V map_has(std::unordered_map<K, V> map, K key) {
    return map.find(key) != map.end();
}

template<typename K, typename V> V map_get_or(std::unordered_map<K, V> map, K key, V default_val) {
    if (map_has(map, key)) return map.at(key);
    else return default_val;
}

namespace symbolic {
// NOTE: P10: Hvorfor kan jeg ikke definere den her function i headeren?!?!
std::string magic_to_string(CostMagicFlags flag) {
    switch (flag)
    {
        case NORMAL: return std::string("NORMAL");
        case INVALID: return std::string("INVALID");
        case MAX: return std::string("MAX");
        default: std::cerr << "Unknown CostMagicFlags: " + std::to_string((int)flag) + ", cannot convert to string" << std::endl; assert(false);
    }
};

Cost::Cost(CostMagicFlags flag) : magic(flag), sum(-1) { }
Cost::Cost(std::unordered_map<grouping::GroupID, int> map, int sum) : magic(CostMagicFlags::NORMAL), value(map), sum(sum) { }
Cost::Cost(std::shared_ptr<AbstractTask> task, OperatorID op) : magic(CostMagicFlags::NORMAL), value({{(*grouping::g_grouping_function())(op), 1}}), sum(task->get_operator_cost(op.get_index(), false)) {}
Cost::Cost(TaskProxy task, OperatorID op) : magic(CostMagicFlags::NORMAL), value({{(*grouping::g_grouping_function())(op), 1}}), sum(task.get_operators()[op].get_cost()) {}

const Cost Cost::INVALID = Cost(CostMagicFlags::INVALID);
const Cost Cost::MIN = Cost(std::unordered_map<grouping::GroupID, int>(), 0);
const Cost Cost::MAX = Cost(CostMagicFlags::MAX);

Cost &Cost::operator+=(const Cost &other) {
    if (other.magic == CostMagicFlags::INVALID || this->magic == CostMagicFlags::INVALID) { throw std::runtime_error("Addition with Cost(INVALID)"); }
    for (const auto& [group, amount] : other.value) {
        this->value[group] = map_get_or(this->value, group, 0) + amount;
    }
    
    this->sum += other.sum;
    return *this;
}

Cost &Cost::operator-=(const Cost &other) {
    if (other.magic == CostMagicFlags::INVALID || this->magic == CostMagicFlags::INVALID) { throw std::runtime_error("Subtraction with Cost(INVALID)"); }
    for (const auto& [group, amount] : other.value) {
        auto result = map_get_or(this->value, group, 0) - amount;
        this->value[group] = result;
        if (result < 0) this->magic = CostMagicFlags::INVALID;
    }
    
    this->sum -= other.sum;
    return *this;
}

Cost Cost::operator+(const Cost other) const {
    Cost tmp = *this;
    tmp += other;
    return tmp;
}

Cost Cost::operator-(const Cost other) const {
    Cost tmp = *this;
    tmp -= other;
    return tmp;
}

bool Cost::operator>=(const Cost &other) const {
    switch (this->magic)
    {
        case CostMagicFlags::MAX: return true;
        case CostMagicFlags::INVALID: return false; //. Invalid values were originally represented with -1, ussure if they should be equal
        case CostMagicFlags::NORMAL: break;
        default: throw std::runtime_error("P10: Unsure how to handle >= for cost with lhs->magic:" + magic_to_string(this->magic));
    }

    switch (other.magic)
    {
        case CostMagicFlags::MAX: return (this->magic == CostMagicFlags::MAX);
        case CostMagicFlags::INVALID: return true; //. Invalid values were originally represented with -1, an so should be "less" than every valid value I think
        case CostMagicFlags::NORMAL: break;
        default: throw std::runtime_error("P10: Unsure how to handle >= for cost with rhs->magic:" + magic_to_string(other.magic));
    }

    if(this->sum < other.sum) return false;
    if(this->sum > other.sum) return true;

    // NOTE: P10: Assumes the keys are ordereable
    std::set<grouping::GroupID> keys;
    for(auto &[key, _] : this->value) {
        keys.insert(key);
    }
    for(auto &[key, _] : other.value) {
        keys.insert(key);
    }

    for (const auto& group : keys) { // NOTE: P10: Iterated in order of keys
        //. Assumes none of the values are negative (Should be INVALID, and handled above)
        auto lhs = map_get_or(this->value, group, 0);
        auto rhs = map_get_or(other.value, group, 0);
        if (lhs < rhs) return false;
        if (lhs > rhs) return true;
        //. else, lhs == rhs, continue
    }
    return true; //. they are equal
}

bool Cost::operator<=(const Cost &other) const {
    switch (this->magic)
    {
        case CostMagicFlags::MAX: return true;
        case CostMagicFlags::INVALID: return true; //. Invalid values were originally represented with -1, an so should be "less" than every valid value I think
        case CostMagicFlags::NORMAL: break;
        default: throw std::runtime_error("P10: Unsure how to handle <= for cost with lhs->magic:" + magic_to_string(this->magic));
    }

    switch (other.magic)
    {
        case CostMagicFlags::MAX: return true;
        case CostMagicFlags::INVALID: return false; //. Invalid values were originally represented with -1, an so should be "less" than every valid value I think
        case CostMagicFlags::NORMAL: break;
        default: throw std::runtime_error("P10: Unsure how to handle <= for cost with rhs->magic:" + magic_to_string(other.magic));
    }

    if(this->sum < other.sum) return true;
    if(this->sum > other.sum) return false;

    // NOTE: P10: Assumes the keys are ordereable
    std::set<grouping::GroupID> keys;
    for(auto &[key, _] : this->value) {
        keys.insert(key);
    }
    for(auto &[key, _] : other.value) {
        keys.insert(key);
    }

    for (const auto& group : keys) { // NOTE: P10: Iterated in order of keys
        //. Assumes none of the values are negative (Should be INVALID, and handled above)
        auto lhs = map_get_or(this->value, group, 0);
        auto rhs = map_get_or(other.value, group, 0);
        if (lhs < rhs) return true;
        if (lhs > rhs) return false;
        //. else, lhs == rhs, continue
    }
    return true; //. they are equal
}

bool Cost::operator>(const Cost &other) const {
    switch (this->magic)
    {
        case CostMagicFlags::MAX: return !(other.magic == CostMagicFlags::MAX);
        case CostMagicFlags::INVALID: return false; //. Invalid values were originally represented with -1, an so should be "less" than every valid value I think
        case CostMagicFlags::NORMAL: break;
        default: throw std::runtime_error("P10: Unsure how to handle > for cost with lhs->magic:" + magic_to_string(this->magic));
    }

    switch (other.magic)
    {
        case CostMagicFlags::MAX: return false;
        case CostMagicFlags::INVALID: return true; //. Invalid values were originally represented with -1, an so should be "less" than every valid value I think
        case CostMagicFlags::NORMAL: break;
        default: throw std::runtime_error("P10: Unsure how to handle > for cost with rhs->magic:" + magic_to_string(other.magic));
    }

    if(this->sum < other.sum) return false;
    if(this->sum > other.sum) return true;

    // NOTE: P10: Assumes the keys are ordereable
    std::set<grouping::GroupID> keys;
    for(auto &[key, _] : this->value) {
        keys.insert(key);
    }
    for(auto &[key, _] : other.value) {
        keys.insert(key);
    }

    for (const auto& group : keys) { // NOTE: P10: Iterated in order of keys
        //. Assumes none of the values are negative (Should be INVALID, and handled above)
        auto lhs = map_get_or(this->value, group, 0);
        auto rhs = map_get_or(other.value, group, 0);
        if (lhs < rhs) return false;
        if (lhs > rhs) return true;
        //. else, lhs == rhs, continue
    }
    return false; //. they are equal
}

bool Cost::operator<(const Cost &other) const {
    switch (this->magic)
    {
        case CostMagicFlags::MAX: return false; //. max < nothing
        case CostMagicFlags::INVALID: return true; //. Invalid values were originally represented with -1, an so should be "less" than every valid value I think
        case CostMagicFlags::NORMAL: break;
        default: throw std::runtime_error("P10: Unsure how to handle < for cost with lhs->magic:" + magic_to_string(this->magic));
    }

    switch (other.magic)
    {
        case CostMagicFlags::MAX: return !(this->magic == CostMagicFlags::MAX); //. (anything other than max) < max
        case CostMagicFlags::INVALID: return false; //. Invalid values were originally represented with -1, an so should be "less" than every valid value I think
        case CostMagicFlags::NORMAL: break;
        default: throw std::runtime_error("P10: Unsure how to handle < for cost with rhs->magic:" + magic_to_string(other.magic));
    }

    if(this->sum < other.sum) return true;
    if(this->sum > other.sum) return false;

    // NOTE: P10: Assumes the keys are ordereable
    std::set<grouping::GroupID> keys;
    for(auto &[key, _] : this->value) {
        keys.insert(key);
    }
    for(auto &[key, _] : other.value) {
        keys.insert(key);
    }

    for (const auto& group : keys) { // NOTE: P10: Iterated in order of keys
        //. Assumes none of the values are negative (Should be INVALID, and handled above)
        auto lhs = map_get_or(this->value, group, 0);
        auto rhs = map_get_or(other.value, group, 0);
        if (lhs < rhs) return true;
        if (lhs > rhs) return false;
        //. else, lhs == rhs, continue
    }
    return false; //. they are equal
}

bool Cost::operator==(const Cost &other) const {
    return *this >= other && *this <= other;
}

bool Cost::operator!=(const Cost &other) const {
    return !(*this == other);
}

bool Cost::dominates(const Cost &other) const {
    switch (this->magic)
    {
        case CostMagicFlags::MAX: return true;
        case CostMagicFlags::NORMAL: break;
        default: throw std::runtime_error("P10: Unsure how to handle <= for cost with lhs->magic:" + magic_to_string(this->magic));
    }

    switch (other.magic)
    {
        case CostMagicFlags::MAX: return true;
        case CostMagicFlags::NORMAL: break;
        default: throw std::runtime_error("P10: Unsure how to handle <= for cost with rhs->magic:" + magic_to_string(other.magic));
    }

    for (const auto& [group, lhs] : this->value) { //. Iterate lhs's (this's) keys
        //. Assumes none of the values are negative (Should be INVALID, and handled above)
        auto rhs = map_get_or(other.value, group, 0);
        if (rhs < lhs) return false; //. for some index, rhs is better (smaller) than lhs, so therefore lhs does NOT dominate rhs
    }
    for (const auto& [group, rhs] : other.value) { //. Iterate rhs's (other's) keys
        //. Assumes none of the values are negative (Should be INVALID, and handled above)
        auto lhs = map_get_or(this->value, group, 0);
        if (rhs < lhs) return false; //. for some index, rhs is better (smaller) than lhs, so therefore lhs does NOT dominate rhs
    }
    return true; //. there are none that are larger in the lhs
}

Cost Cost::min(Cost first, Cost second) {
    return first < second ? first : second;
}

Cost Cost::max(Cost first, Cost second) {
    return first < second ? second : first;
}

Cost Cost::plan_cost(const Plan &plan, const TaskProxy &task) {
    Cost plan_cost = Cost::MIN;
    for (OperatorID op_id : plan) {
        plan_cost += Cost(task, op_id);
    }
    return plan_cost;
}

std::string to_string(const Cost c) {
    switch (c.magic)
    {
        case CostMagicFlags::INVALID: return std::string("Cost(INVALID)");
        case CostMagicFlags::MAX: return std::string("Cost(MAX)");
        default: break;
    }

    std::string outputString = "";

    std::set<grouping::GroupID> ids;
    for(auto &id : (grouping::g_grouping_function())->get_groups()) {
        ids.insert(id);
    }

#define SHORT_PRINT
    for (auto &group : ids) {
        int amount = map_get_or(c.value, group, 0);
#ifdef SHORT_PRINT
        outputString += std::to_string(amount) + " ";
#else
        outputString += "{(" + std::to_string(group) + ") " + (grouping::g_grouping_function())->get_group_name(group) + ": " + std::to_string(amount) + "}, ";
#endif // SHORT_PRINT
    }
    return "Cost[" + std::to_string(c.sum) + "]( " + outputString + ")";
}

std::ostream &operator<<(std::ostream &os, const Cost &c) {
    return os << to_string(c);
}
}
