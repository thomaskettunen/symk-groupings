#include "cost.h"
#include "../utils/hash.h"
#include "../abstract_task.h"

#include <algorithm>
#include <limits>
#include <ostream>
#include <string>
#include <memory>

namespace symbolic {
Cost::Cost() : value(0) {};
Cost::Cost(int value) : value(value) {};

const Cost Cost::NO_VALUE = Cost(-2);
const Cost Cost::UNINITIALIZED = Cost(-2);
const Cost Cost::INVALID = Cost(-1);
const Cost Cost::DEAD_END = Cost(-1);
const Cost Cost::MIN = Cost(0);
const Cost Cost::ONE = Cost(1);
const Cost Cost::MAX = Cost(std::numeric_limits<int>::max());
const Cost Cost::INFTY = Cost(std::numeric_limits<int>::max());

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
    // ASS: map from operator name (string) to group name (string)
    std::function<std::string(std::string)> op_name_to_group_name = [](std::string op_name) {
        // ASS: This is prefix
        // return op_name.substr(0, op_name.find(' ')); //. Prefix_1 (remember to change the name!!)
        return op_name.substr(0, op_name.find(' ', op_name.find(' ') + 1)); //. Prefix_2 (remember to change the name!!)
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
