#ifndef PARETO_FRONT_H
#define PARETO_FRONT_H

#include "cost.h"

namespace pareto_front {
    bool dominates(const symbolic::Cost& g);
    void insert(const symbolic::Cost& g);
}

#endif