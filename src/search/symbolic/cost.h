#ifndef COST_H
#define COST_H

#include "cost.h"
#include "../plan_manager.h"
#include "../utils/hash.h"
#include "../abstract_task.h"

#include <algorithm>
#include <limits>
#include <ostream>
#include <string>
#include <memory>
#include <unordered_map>

using GroupID = int;

// namespace symbolic {
//     class Cost;
// }

// namespace std {
//     template <> class hash<symbolic::Cost> {
//         public:
//         size_t operator()(const symbolic::Cost& cost) const;
//     };
// }

namespace symbolic {
    enum CostMagicFlags {
        NORMAL,
        EMPTY_CONSTRUCTOR,
        INVALID,
        MIN,
        MAX,
    };

    std::string magic_to_string(CostMagicFlags flag);

    class Cost {
        public:
            explicit Cost();
            explicit Cost(CostMagicFlags);
            Cost(std::shared_ptr<AbstractTask> task, OperatorID op);
            Cost(TaskProxy task, OperatorID op);

            static const Cost INVALID;
            static const Cost MIN;
            static const Cost MAX;
            
            Cost lower_bound();
            Cost upper_bound();
            Cost &operator+=(const Cost &other);
            Cost &operator-=(const Cost &other);
            Cost operator+(const Cost other) const;
            Cost operator-(const Cost other) const;
            // Cost operator*(const double other) const;
            bool operator>=(const Cost &other) const;
            bool operator<=(const Cost &other) const;
            bool operator>(const Cost &other) const;
            bool operator<(const Cost &other) const;
            bool operator==(const Cost &other) const;
            bool operator!=(const Cost &other) const;

            static Cost min(Cost first, Cost second);
            static Cost max(Cost first, Cost second);
            static Cost plan_cost(const Plan &plan, const TaskProxy &task_proxy);

            friend std::string to_string(const Cost c);
            friend std::ostream &operator<<(std::ostream &os, const Cost &c);
            // friend std::size_t std::hash<Cost>::operator()(const Cost& cost) const;

        private:
            std::string get_group_name(int group_no);
            static std::unordered_map<std::string, int> group_name_to_group_id;
            GroupID get_group_id(const TaskProxy task, OperatorID op_id);

            CostMagicFlags magic;
            std::unordered_map<GroupID, int> value;
    };
}

#endif