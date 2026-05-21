#include "bidirectional_search.h"

#include "../search_algorithms/symbolic_search.h"

#include <memory>

using namespace std;

namespace symbolic {
BidirectionalSearch::BidirectionalSearch( SymbolicSearch *eng, const SymParameters &params, shared_ptr<UniformCostSearch> _fw, shared_ptr<UniformCostSearch> _bw, bool _alternating)
    : SymSearch(eng, params),
      fw(_alternating ? _bw : _fw), // NOTE: P10: Use "alternating" to swap the order of comparisons laster
      bw(_alternating ? _fw : _bw),
      cur_dir(nullptr),
      alternating(_alternating) {
    assert(fw->getStateSpace() == bw->getStateSpace());
    mgr = fw->getStateSpaceShared();
}

string BidirectionalSearch::get_last_dir() const {
    return cur_dir ? cur_dir->get_last_dir() : "";
}

UniformCostSearch *BidirectionalSearch::selectBestDirection() {
    if (!engine->is_silent()) utils::g_log << "selecting best direction: ";
    if (fw->frontier->g() < bw->frontier->g()) {
        if (!engine->is_silent()) utils::g_log << (fw.get()->fw ? "[->]" : "[<-]") << " fw.g(): " << fw->frontier->g() << ", bw.g(): " << bw->frontier->g() << std::endl;
        return fw.get();
    } else {
        if (!engine->is_silent()) utils::g_log << (bw.get()->fw ? "[->]" : "[<-]") << " fw.g(): " << fw->frontier->g() << ", bw.g(): " << bw->frontier->g() << std::endl;
        return bw.get();
    }
}

bool BidirectionalSearch::finished() const {
    return fw->finished() || bw->finished();
}

void BidirectionalSearch::stepImage(int maxTime, int maxNodes) {
    selectBestDirection()->stepImage(maxTime, maxNodes);
}
}
