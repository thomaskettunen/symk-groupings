#include "frontier.h"
#include "sym_state_space_manager.h"
#include "../utils/timer.h"
#include "cost.h"
#include "closed_list.h"

#include "../utils/logging.h"

namespace symbolic {
/// @brief Filters dominated states from the closed-list out of the frontier.
///
///        I.e. if the frontier has cost (1 2 3) and contains state s1, which is found in the closed-list with any cost dominating (1 2 3), e.g. cost (1 1 1), s1 will be removed from the frontier.
/// @param closed The closed-list to base the filtering on.
void Frontier::filter(const std::shared_ptr<ClosedList> closed) {

    //utils::g_log << "nodes before filtering frontier " << nodes() << std::endl;

    if (states.empty()) { return; }
    for (auto &[cost, bdd] : closed->getClosedList()) {
        if (cost.dominates(this->g())) {
            for (BDD &b : states) {
                b -= bdd;
            }
        }
    }

    //utils::g_log << "nodes after filtering frontier " << nodes() << std::endl;
}

ExpansionResult Frontier::expand_zero(int maxTime, int maxNodes, bool fw) {
    assert(false); // TODO: P10: Ignore zero const actions for now
    // // Image with respect to 0-cost actions
    // utils::Timer image_time;

    // mgr->set_time_limit(maxTime);
    // // Compute image, storing the result on Simg
    // try {
    //     for (size_t i = 0; i < Szero.size(); i++) {
    //         Simg.push_back(std::map<Cost, Bucket>());
    //         mgr->zero_image(fw, Szero[i], Simg[i][Cost::MIN], maxNodes);
    //     }
    //     mgr->unset_time_limit();
    // } catch (const BDDError &e) {
    //     mgr->unset_time_limit();
    //     return ExpansionResult(true, TruncatedReason::IMAGE_ZERO, image_time());
    // }

    // Bucket().swap(Szero); // Delete Szero because it has been expanded

    // return ExpansionResult(true, Simg, image_time());
}

ExpansionResult Frontier::expand_cost(int maxTime, int maxNodes, bool fw) {
    std::vector<std::map<Cost, Bucket>> result;
    utils::Timer image_time;
    mgr->set_time_limit(maxTime);
    // cout << maxTime << " + " << maxNodes << endl;
    try {
        for (size_t i = 0; i < states.size(); i++) {
            result.push_back(std::map<Cost, Bucket>());
            mgr->cost_image(fw, states[i], result[i], maxNodes);
        }
        mgr->unset_time_limit();
    } catch (const BDDError &e) {
        // Update estimation
        mgr->unset_time_limit();

        return ExpansionResult(false, TruncatedReason::IMAGE_COST, image_time());
    }

    Bucket().swap(states);
    return ExpansionResult(false, result, image_time());
}

std::ostream &operator<<(std::ostream &os, const Frontier &frontier) {
    if (!frontier.empty()) {
        os << "Frontier " << frontier.g() << " : nodes:" << !frontier.nodes();
    } else {
        os << "Frontier : empty;";
    }
    return os;
}
}
