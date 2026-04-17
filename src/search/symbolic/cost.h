#ifndef COST_H
#define COST_H

#include "grouping.h"
#include "../plan_manager.h"
#include "../utils/hash.h"
#include "../abstract_task.h"

#include <algorithm>
#include <limits>
#include <ostream>
#include <string>
#include <memory>
#include <unordered_map>

namespace symbolic {
    enum CostMagicFlags {
        NORMAL,
        INVALID,
        MAX,
    };

    std::string magic_to_string(CostMagicFlags flag);

    class Cost {
        public:
            explicit Cost() = delete;
            explicit Cost(std::unordered_map<grouping::GroupID, int> map);
            Cost(std::shared_ptr<AbstractTask> task, OperatorID op);
            Cost(TaskProxy task, OperatorID op);

            static const Cost INVALID;
            static const Cost MIN;
            static const Cost MAX;

            Cost &operator+=(const Cost &other);
            Cost &operator-=(const Cost &other);
            Cost operator+(const Cost other) const;
            Cost operator-(const Cost other) const;
            bool operator>=(const Cost &other) const;
            bool operator<=(const Cost &other) const;
            bool operator>(const Cost &other) const;
            bool operator<(const Cost &other) const;
            bool operator==(const Cost &other) const;
            bool operator!=(const Cost &other) const;
            bool dominates(const Cost &other) const;
            
            static Cost min(Cost first, Cost second);
            static Cost max(Cost first, Cost second);
            static Cost plan_cost(const Plan &plan, const TaskProxy &task_proxy);

            friend std::string to_string(const Cost c);
            friend std::ostream &operator<<(std::ostream &os, const Cost &c);

        private:
            explicit Cost(CostMagicFlags);
            CostMagicFlags magic;
            std::unordered_map<grouping::GroupID, int> value;
            int sum;
    };
}

#endif