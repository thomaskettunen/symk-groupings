#include "closed_list.h"

#include "sym_state_space_manager.h"
#include "sym_utils.h"

#include "plan_reconstruction/sym_solution_registry.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "cost.h"

using namespace std;

namespace symbolic {
ClosedList::ClosedList() : mgr(nullptr) {
}

void ClosedList::init(SymStateSpaceManager *manager) {
    mgr = manager;
    map<Cost, vector<BDD>>().swap(zeroCostClosed);
    map<Cost, BDD>().swap(closed);
    closedTotal = mgr->zeroBDD();
}

void ClosedList::init(SymStateSpaceManager *manager, const ClosedList &other) {
    mgr = manager;
    map<Cost, vector<BDD>>().swap(zeroCostClosed);
    map<Cost, BDD>().swap(closed);
    closedTotal = mgr->zeroBDD();

    closedTotal = other.closedTotal;
    closed[Cost::MIN] = closedTotal;
}

void ClosedList::insert(Cost h, BDD S) {
    if (closed.count(h)) {
        closed[h] += S;
    } else {
        closed[h] = S;
    }

    if (mgr->has_zero_cost_transition()) {
        zeroCostClosed[h].push_back(S);
    }
    closedTotal += S;
}

SymSolutionCut ClosedList::getCheapestCut(BDD states, Cost g, bool fw) const {
    BDD cut_candidate = states * closedTotal;
    if (cut_candidate.IsZero()) {
        return SymSolutionCut();
    }

    for (const auto &closedH : closed) {
        Cost h = closedH.first;

        BDD cut = closedH.second * cut_candidate;
        if (!cut.IsZero()) {
            if (fw) {
                return SymSolutionCut(g, h, cut);
            } else {
                return SymSolutionCut(h, g, cut);
            }
        }
    }
    cerr << "Inconsitent cut result" << endl;
    exit(0);
    return SymSolutionCut();
}

/// @brief For a set of (newly expanded) states, expanded with a given g, find all cuts with the closed list
/// @param states Newly expanded states
/// @param g The cost of reachign them
/// @param fw Whether we are perfoming forward search
/// @param lower_bound The highest cost for a full plan allowed
/// @return The different cuts, at different costs
vector<SymSolutionCut> ClosedList::getAllCuts(BDD states, Cost g, bool fw, Cost lower_bound) const {
    vector<SymSolutionCut> result;
    BDD cut_candidate = states * closedTotal;
    if (!cut_candidate.IsZero()) {
        for (const auto &[h, bdd] : closed) {
            //* Here we also need to consider higher costs due to the architecture of symBD. Otherwise their occur problems in
            if (g + h < lower_bound) { continue; }

            // utils::g_log << "Check cut of g=" << g << " with h=" << h << endl;
            BDD cut = bdd * cut_candidate;
            if (!cut.IsZero()) {
                if (fw) {
                    result.emplace_back(g, h, cut);
                } else {
                    result.emplace_back(h, g, cut);
                }
            }
        }
    }
    return result;
}
}
