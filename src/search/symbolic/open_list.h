#ifndef SYMBOLIC_OPEN_LIST_H
#define SYMBOLIC_OPEN_LIST_H

#include "sym_bucket.h"

#include <cassert>
#include <iostream>
#include <map>

#include "cost.h"

namespace symbolic {
class SymStateSpaceManager;
class Frontier;

class OpenList {
    std::map<symbolic::Cost, Bucket> open; // States in open with unkwown h-value

    // At any point in the search we can close all the states in
    // open[minG()] because they cannot be generated with lower
    // cost. Doing that we can set hNotClosed to the next bucket.
    void closeMinOpen();

public:
    bool empty() const {
        assert(open.empty() || !open.begin()->second.empty());
        return open.empty();
    }

    void insert(const Bucket &bucket, symbolic::Cost g);
    void insert(const BDD &bdd, symbolic::Cost g);

    Cost minG() const;

    Cost minNextG(const Frontier &frontier, Cost min_action_cost) const;
    void pop(Frontier &frontier);

    bool contains_any_state(const BDD &bdd) const;

    friend std::ostream &operator<<(std::ostream &os, const OpenList &open);
};
}
#endif
