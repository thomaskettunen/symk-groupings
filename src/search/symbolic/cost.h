#ifndef COST_H
#define COST_H

#include "cost.h"
#include "../utils/hash.h"

#include <algorithm>
#include <limits>
#include <ostream>
#include <string>

namespace symbolic {
    class Cost;
}

namespace utils { //. Specialization of utils::feed for this class
    void feed(HashState &hash_state, const symbolic::Cost &value);
}

namespace symbolic {
    class Cost {
        public:
        explicit Cost();
        explicit Cost(int value);

        static const Cost NO_VALUE;
        static const Cost UNINITIALIZED;
        static const Cost INVALID;
        static const Cost DEAD_END;
        static const Cost MIN;
        static const Cost ONE;
        static const Cost MAX;
        static const Cost INFTY;

        Cost &operator+=(const Cost &other);
        Cost &operator-=(const Cost &other);
        Cost operator+(const Cost other) const;
        Cost operator-(const Cost other) const;
        Cost operator*(const double other) const;
        bool operator>=(const Cost &other) const;
        bool operator<=(const Cost &other) const;
        bool operator>(const Cost &other) const;
        bool operator<(const Cost &other) const;
        bool operator==(const Cost &other) const;
        bool operator!=(const Cost &other) const;

        static Cost min(Cost first, Cost second);
        static Cost max(Cost first, Cost second);

        friend std::string to_string(const Cost c);
        friend std::ostream &operator<<(std::ostream &os, const Cost &c);
        friend void utils::feed(HashState &hash_state, const Cost &value);
        
        private:
        int value;
    };
}

#endif