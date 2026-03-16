#include "cost.h"
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

namespace symbolic {

// NOTE: P10: Hvorfor kan jeg ikke definere den her function i headeren?!?!
std::string magic_to_string(CostMagicFlags flag) {
    switch (flag)
    {
        case NORMAL: return std::string("NORMAL");
        case EMPTY_CONSTRUCTOR: return std::string("EMPTY_CONSTRUCTOR");
        case INVALID: return std::string("INVALID");
        case MIN: return std::string("MIN");
        case ONE: return std::string("ONE");
        case MAX: return std::string("MAX");
        default: std::cerr << "Unknown CostMagicFlags: " + std::to_string((int)flag) + ", cannot convert to string" << std::endl; assert(false);
    }
};

Cost::Cost() : magic(CostMagicFlags::EMPTY_CONSTRUCTOR), value() {};
Cost::Cost(CostMagicFlags flag) : magic(flag) {};
Cost::Cost(std::shared_ptr<AbstractTask> task, OperatorID op) : magic(CostMagicFlags::NORMAL), value({{0, 1}}) {}

// TODO: P10: Fake all operators in group 0 for now
//. We probably need to unify the whole std::shared_ptr<AbstracTast> vs TaskProxy dichotomy
Cost::Cost(TaskProxy task, OperatorID op) : magic(CostMagicFlags::NORMAL), value({{0, 1}}) /*value({{get_group_id(task, op), 1}})*/ {}

const Cost Cost::INVALID = Cost(CostMagicFlags::INVALID);
const Cost Cost::MIN = Cost(CostMagicFlags::MIN);
const Cost Cost::MAX = Cost(CostMagicFlags::MAX);

/// @return The largest lower bound, i.e. a, where x < this -> x <= a
Cost Cost::lower_bound() {
    // NOTE: P10: Assumes the keys are ordereable
    std::set<GroupID> keys;
    for(auto &[key, _] : this->value) {
        keys.insert(key);
    }
    
    Cost result;
    int remaining = keys.size();
    for (const GroupID group : keys) { // NOTE: P10: Iterated in order of keys
        --remaining;
        int val = this->value.at(group);
        if (remaining == 0) --val; //. Decrement the last value (least significant in lexicographical ordering)
        result.value.insert({group, val});
    }
    return result;
}

/// @return The smallest upper bound, i.e. a, where x > this -> x >= a
Cost Cost::upper_bound() {
    Cost result;
    for (const auto &[group, val] : this->value) { // NOTE: P10: Is there a "right" way to copy this?
        result.value.insert({group, val});
    }
    GroupID max_group = std::numeric_limits<GroupID>::max();
    result.value.insert({max_group, this->value.find(max_group) == this->value.end() ? 1 : this->value.at(max_group) + 1});
    return result;
}

Cost &Cost::operator+=(const Cost &other) {
    for (const auto& [group, amount] : other.value) {
        if (this->value.find(group) != this->value.end()) {
            this->value[group] += amount;
        } else {
            this->value[group] = amount;
        }
    }
    return *this;
}

Cost &Cost::operator-=(const Cost &other) {
    for (const auto& [group, amount] : other.value) {
        if (this->value.find(group) != this->value.end()) {
            this->value[group] -= amount;
        } else {
            this->value[group] = -amount;
        }
    }
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

Cost Cost::operator*(const double other) const {
    throw std::runtime_error("there are no doubles");
}

bool Cost::operator>=(const Cost &other) const {
    switch (this->magic)
    {
        case CostMagicFlags::MAX: return true;
        case CostMagicFlags::MIN: return (other.magic == CostMagicFlags::MIN);
        case CostMagicFlags::NORMAL: break;
        case CostMagicFlags::EMPTY_CONSTRUCTOR: break;
        default: throw std::runtime_error("P10: Unsure how to handle >= for cost with lhs->magic:" + magic_to_string(this->magic));
    }

    switch (other.magic)
    {
        case CostMagicFlags::MAX: return (this->magic == CostMagicFlags::MAX);
        case CostMagicFlags::MIN: return true;
        case CostMagicFlags::NORMAL: break;
        case CostMagicFlags::EMPTY_CONSTRUCTOR: break;
        default: throw std::runtime_error("P10: Unsure how to handle >= for cost with rhs->magic:" + magic_to_string(other.magic));
    }

    // NOTE: P10: Assumes the keys are ordereable
    std::set<GroupID> keys;
    for(auto &[key, _] : this->value) {
        keys.insert(key);
    }
    for(auto &[key, _] : other.value) {
        keys.insert(key);
    }

    for (const auto& group : keys) { // NOTE: P10: Iterated in order of keys
        if (this->value.find(group) == this->value.end()) return false; //. "this" has zero of somethign that "other" has >0 of, hence "this" < "other"
        if (other.value.find(group) == other.value.end()) return true; //. "other" has zero of somethign that "this" has >0 of, hence "other" < "this"
        //. Both has the key
        if (this->value.at(group) < other.value.at(group)) return false;
    }
    return true;
}

bool Cost::operator<=(const Cost &other) const {
    switch (this->magic)
    {
        case CostMagicFlags::MAX: return true;
        case CostMagicFlags::MIN: return (other.magic == CostMagicFlags::MIN);
        case CostMagicFlags::NORMAL: break;
        case CostMagicFlags::EMPTY_CONSTRUCTOR: break;
        default: throw std::runtime_error("P10: Unsure how to handle <= for cost with lhs->magic:" + magic_to_string(this->magic));
    }

    switch (other.magic)
    {
        case CostMagicFlags::MAX: return true;
        case CostMagicFlags::MIN: return (this->magic == CostMagicFlags::MIN);
        case CostMagicFlags::NORMAL: break;
        case CostMagicFlags::EMPTY_CONSTRUCTOR: break;
        default: throw std::runtime_error("P10: Unsure how to handle <= for cost with rhs->magic:" + magic_to_string(other.magic));
    }

    // NOTE: P10: Assumes the keys are ordereable
    std::set<GroupID> keys;
    for(auto &[key, _] : this->value) {
        keys.insert(key);
    }
    for(auto &[key, _] : other.value) {
        keys.insert(key);
    }

    for (const auto& group : keys) { // NOTE: P10: Iterated in order of keys
        if (this->value.find(group) == this->value.end()) return true; //. "this" has zero of somethign that "other" has >0 of, hence "this" < "other"
        if (other.value.find(group) == other.value.end()) return false; //. "other" has zero of somethign that "this" has >0 of, hence "other" < "this"
        //. Both has the key
        if (this->value.at(group) > other.value.at(group)) return false;
    }
    return true;
}

bool Cost::operator>(const Cost &other) const {
    switch (this->magic)
    {
        case CostMagicFlags::MAX: return !(other.magic == CostMagicFlags::MAX);
        case CostMagicFlags::MIN: return false;
        case CostMagicFlags::NORMAL: break;
        case CostMagicFlags::EMPTY_CONSTRUCTOR: break;
        default: throw std::runtime_error("P10: Unsure how to handle > for cost with lhs->magic:" + magic_to_string(this->magic));
    }

    switch (other.magic)
    {
        case CostMagicFlags::MAX: return false;
        case CostMagicFlags::MIN: return !(this->magic == CostMagicFlags::MIN);
        case CostMagicFlags::NORMAL: break;
        case CostMagicFlags::EMPTY_CONSTRUCTOR: break;
        default: throw std::runtime_error("P10: Unsure how to handle > for cost with rhs->magic:" + magic_to_string(other.magic));
    }

    // NOTE: P10: Assumes the keys are ordereable
    std::set<GroupID> keys;
    for(auto &[key, _] : this->value) {
        keys.insert(key);
    }
    for(auto &[key, _] : other.value) {
        keys.insert(key);
    }

    for (const GroupID group : keys) { // NOTE: P10: Iterated in order of keys
        if (this->value.find(group) == this->value.end()) return false; //. "this" has zero of somethign that "other" has >0 of, hence "this" < "other"
        if (other.value.find(group) == other.value.end()) return true; //. "other" has zero of somethign that "this" has >0 of, hence "other" < "this"
        //. Both has the key
        if (this->value.at(group) < other.value.at(group)) return false;
    }
    return false; //. they are equal
}

bool Cost::operator<(const Cost &other) const {
    switch (this->magic)
    {
        case CostMagicFlags::MAX: return false; //. max < nothing
        case CostMagicFlags::MIN: return !(other.magic == CostMagicFlags::MIN); //. min < (anything other than min)
        case CostMagicFlags::NORMAL: break;
        case CostMagicFlags::EMPTY_CONSTRUCTOR: break;
        default: throw std::runtime_error("P10: Unsure how to handle < for cost with lhs->magic:" + magic_to_string(this->magic));
    }

    switch (other.magic)
    {
        case CostMagicFlags::MAX: return !(this->magic == CostMagicFlags::MAX); //. (anything other than max) < max
        case CostMagicFlags::MIN: return false; //. nothing < min
        case CostMagicFlags::NORMAL: break;
        case CostMagicFlags::EMPTY_CONSTRUCTOR: break;
        default: throw std::runtime_error("P10: Unsure how to handle < for cost with rhs->magic:" + magic_to_string(other.magic));
    }

    // NOTE: P10: Assumes the keys are ordereable
    std::set<GroupID> keys;
    for(auto &[key, _] : this->value) {
        keys.insert(key);
    }
    for(auto &[key, _] : other.value) {
        keys.insert(key);
    }

    for (const GroupID group : keys) { // NOTE: P10: Iterated in order of keys
        if (this->value.find(group) == this->value.end()) return true; //. "this" has zero of somethign that "other" has >0 of, hence "this" < "other"
        if (other.value.find(group) == other.value.end()) return false; //. "other" has zero of somethign that "this" has >0 of, hence "other" < "this"
        //. Both has the key
        if (this->value.at(group) > other.value.at(group)) return false;
    }
    return false; //. they are equal
}

bool Cost::operator==(const Cost &other) const {
    return *this >= other && *this <= other;
}

bool Cost::operator!=(const Cost &other) const {
    return !(*this == other);
}

Cost Cost::min(Cost first, Cost second) {
    return first < second ? first : second;
}

Cost Cost::max(Cost first, Cost second) {
    return first < second ? second : first;
}

Cost Cost::plan_cost(const Plan &plan, const TaskProxy &task) {
    Cost plan_cost = Cost();
    for (OperatorID op_id : plan) {
        plan_cost += Cost(task, op_id);
    }
    return plan_cost;
}

std::string to_string(const Cost c) {
    std::string outputString = "";
    for (const auto& [group, amount] : c.value) {
        outputString += "\t( " + std::to_string(group) + ": " + std::to_string(amount) + ")\n";
    }
    
    return "Cost(\n" + outputString + ")";
}

std::ostream &operator<<(std::ostream &os, const Cost &c) {
    return os << to_string(c);
}

std::unordered_map<std::string, int> Cost::group_name_to_group_id; // NOTE: P10: may cause secret spooky error check here if ghosts appear

std::string Cost::get_group_name(int group_no) {
    for (auto &it : Cost::group_name_to_group_id) {
        if (it.second == group_no) return it.first;
    }
    return std::string("No matching group");
}

GroupID Cost::get_group_id(const std::shared_ptr<AbstractTask> task, OperatorID op_id) {
    std::function<std::string(std::string)> op_name_to_group_name = [](std::string op_name) {
        return op_name.substr(0, op_name.find(' '));
    };

    auto op_name = task->get_operator_name(op_id.get_index(), false);
    auto group_name = op_name_to_group_name(op_name);
    GroupID group_id;
    auto it = Cost::group_name_to_group_id.find(group_name);
    if (it != Cost::group_name_to_group_id.end()) {
        group_id = (group_name_to_group_id)[group_name];
    } else {
        group_id = group_name_to_group_id.size();
        (group_name_to_group_id)[group_name] = group_id;
    }
    return group_id;
};
}

namespace std {
    size_t hash<symbolic::Cost>::operator()(const symbolic::Cost& cost) const {
        // TODO: P10: make a real human hash function
        return 1;
    }
}
