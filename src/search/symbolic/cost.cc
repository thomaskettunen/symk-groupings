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

namespace symbolic {
Cost::Cost() : magic(CostMagicFlags::EMPTY_CONSTRUCTOR), value() {};
Cost::Cost(CostMagicFlags flag) : magic(flag) {};
Cost::Cost(std::shared_ptr<AbstractTask> task, OperatorID op) : magic(CostMagicFlags::NORMAL), value({{get_group_id(task, op), 1}}) {}

// TODO: P10: Fake all operators in group 0 for now
//. We probably need to unify the whole std::shared_ptr<AbstracTast> vs TaskProxy dichotomy
Cost::Cost(TaskProxy task, OperatorID op) : magic(CostMagicFlags::NORMAL), value({{0, 1}}) /*value({{get_group_id(task, op), 1}})*/ {}

const Cost Cost::INVALID = Cost(CostMagicFlags::INVALID);
const Cost Cost::MIN = Cost(CostMagicFlags::MIN);
const Cost Cost::ONE = Cost(CostMagicFlags::ONE);
const Cost Cost::MAX = Cost(CostMagicFlags::MAX);

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
        default: throw std::runtime_error("P10: Unsure how to handle >= for cost with lhs->magic:" + this->magic);
    }

    switch (other.magic)
    {
        case CostMagicFlags::MAX: return (this->magic == CostMagicFlags::MAX);
        case CostMagicFlags::MIN: return true;
        case CostMagicFlags::NORMAL: break;
        case CostMagicFlags::EMPTY_CONSTRUCTOR: break;
        default: throw std::runtime_error("P10: Unsure how to handle >= for cost with rhs->magic:" + other.magic);
    }

    for (const auto& [group, amount] : other.value) {
        if (this->value.find(group) != this->value.end()) {
            if (!(this->value.at(group) >= amount)) return false;
        } else {
            return false;
        }
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
        default: throw std::runtime_error("P10: Unsure how to handle <= for cost with lhs->magic:" + this->magic);
    }

    switch (other.magic)
    {
        case CostMagicFlags::MAX: return true;
        case CostMagicFlags::MIN: return (this->magic == CostMagicFlags::MIN);
        case CostMagicFlags::NORMAL: break;
        case CostMagicFlags::EMPTY_CONSTRUCTOR: break;
        default: throw std::runtime_error("P10: Unsure how to handle <= for cost with rhs->magic:" + other.magic);
    }

    for (const auto& [group, amount] : this->value) {
        if (other.value.find(group) != other.value.end()) {
            if (!(this->value.at(group) <= amount)) return false;
        } else {
            return false;
        }
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
        default: throw std::runtime_error("P10: Unsure how to handle > for cost with lhs->magic:" + this->magic);
    }

    switch (other.magic)
    {
        case CostMagicFlags::MAX: return false;
        case CostMagicFlags::MIN: return !(this->magic == CostMagicFlags::MIN);
        case CostMagicFlags::NORMAL: break;
        case CostMagicFlags::EMPTY_CONSTRUCTOR: break;
        default: throw std::runtime_error("P10: Unsure how to handle > for cost with rhs->magic:" + other.magic);
    }

    if (!(*this >= other)) {
        return false;
    }

    for (const auto& [group, amount] : this->value) {
        if (other.value.find(group) != other.value.end()) {
            if (this->value.at(group) > other.value.at(group)) return true;
        } else {
            return true;
        }
    }
    return false;
}

bool Cost::operator<(const Cost &other) const {
    switch (this->magic)
    {
        case CostMagicFlags::MAX: return false;
        case CostMagicFlags::MIN: return !(other.magic == CostMagicFlags::MIN);
        case CostMagicFlags::NORMAL: break;
        case CostMagicFlags::EMPTY_CONSTRUCTOR: break;
        default: throw std::runtime_error("P10: Unsure how to handle < for cost with lhs->magic:" + this->magic);
    }

    switch (other.magic)
    {
        case CostMagicFlags::MAX: return !(this->magic == CostMagicFlags::MAX);
        case CostMagicFlags::MIN: return false;
        case CostMagicFlags::NORMAL: break;
        case CostMagicFlags::EMPTY_CONSTRUCTOR: break;
        default: throw std::runtime_error("P10: Unsure how to handle < for cost with rhs->magic:" + other.magic);
    }

    if (!(*this <= other)){
        return false;
    }

    for (const auto& [group, amount] : other.value) {
        if (this->value.find(group) != this->value.end()) {
            if (this->value.at(group) < amount) return true;
        } else {
            return true;
        }
    }
    return false;
}

bool Cost::operator==(const Cost &other) const {
    return *this >= other && *this <= other;
}

bool Cost::operator!=(const Cost &other) const {
    return !(*this == other);
}

Cost Cost::min(Cost first, Cost second) {
    throw std::runtime_error("min not implemented no total order");
}

Cost Cost::max(Cost first, Cost second) {
    throw std::runtime_error("max not implemented no total order");
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
