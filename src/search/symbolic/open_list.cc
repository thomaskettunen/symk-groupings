#include "open_list.h"

#include "frontier.h"

#include <cassert>

#include "cost.h"

#include "found_plans.h"


using namespace std;

namespace symbolic {
void OpenList::insert(const Bucket &bucket, symbolic::Cost g) {
    assert(!bucket.empty());
    copy_bucket(bucket, open[g]);
}

void OpenList::insert(const BDD &bdd, symbolic::Cost g) {
    assert(!bdd.IsZero());
    open[g].push_back(bdd);
}

symbolic::Cost OpenList::minNextG(const Frontier &frontier, symbolic::Cost min_action_cost) const {
    symbolic::Cost next_g =
        (frontier.empty() ? symbolic::Cost::MAX
                          : frontier.g() + min_action_cost);
    if (!open.empty()) {
        return min(next_g, open.begin()->first);
    }
    return next_g;
}

void OpenList::pop(Frontier &frontier) {
    assert(frontier.empty());
    symbolic::Cost g = open.begin()->first;

    
    // found_plans::global_instance.paretto_frontier.insert(g);

    while(found_plans::global_instance.is_dominated(g)){ 

        open.erase(g);

        if(open.empty()) return;

        g = open.begin()->first;
    }

    frontier.set(g, open.begin()->second);
    open.erase(g);
}

symbolic::Cost OpenList::minG() const {
    return open.empty() ? symbolic::Cost::MAX : open.begin()->first;
}

bool OpenList::contains_any_state(const BDD &bdd) const {
    for (auto &key : open) {
        if (bucket_contains_any_state(key.second, bdd)) {
            return true;
        }
    }
    return false;
}

ostream &operator<<(ostream &os, const OpenList &exp) {
    os << " open{";
    for (auto &o : exp.open) {
        os << o.first << " ";
    }
    return os << "}";
}
}
