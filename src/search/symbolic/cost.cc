#include "cost.h"
#include "../utils/hash.h"

#include <algorithm>
#include <limits>
#include <ostream>
#include <string>

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

    Cost &Cost::operator+=(const Cost &other) { this->value += other.value; return *this; }
    Cost &Cost::operator-=(const Cost &other) { this->value -= other.value; return *this; }
    Cost Cost::operator+(const Cost other) const { return Cost(this->value + other.value); }
    Cost Cost::operator-(const Cost other) const { return Cost(this->value - other.value); }
    bool Cost::operator>=(const Cost &other) const { return this->value >= other.value; }
    bool Cost::operator<=(const Cost &other) const { return this->value <= other.value; }
    bool Cost::operator>(const Cost &other) const { return this->value > other.value; }
    bool Cost::operator<(const Cost &other) const { return this->value < other.value; }
    bool Cost::operator==(const Cost &other) const { return this->value == other.value; }
    bool Cost::operator!=(const Cost &other) const { return this->value != other.value; }

    Cost Cost::min(Cost first, Cost second) { return (first.value - second.value < 0) ? second : first; }
    Cost Cost::max(Cost first, Cost second) { return (first.value - second.value < 0) ? first : second; }

    std::string to_string(const Cost c) { return "Cost(" + std::to_string(c.value) + ")"; }
    std::ostream &operator<<(std::ostream &os, const Cost &c) { return os << "Cost(" << c.value << ")"; }
}

namespace utils {
    void feed(HashState &hash_state, const symbolic::Cost &value) { hash_state.feed(value.value); }
}
