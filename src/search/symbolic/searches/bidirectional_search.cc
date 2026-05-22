#include "bidirectional_search.h"

#include "../search_algorithms/symbolic_search.h"

#include <memory>

using namespace std;

namespace symbolic {
BidirectionalSearch::BidirectionalSearch( SymbolicSearch *eng, const SymParameters &params, shared_ptr<UniformCostSearch> _fw, shared_ptr<UniformCostSearch> _bw, bool _alternating)
    : SymSearch(eng, params),
      fw(_fw),
      bw(_bw),
      cur_dir(nullptr),
      alternating(_alternating) {
    assert(fw->getStateSpace() == bw->getStateSpace());
    mgr = fw->getStateSpaceShared();
}

string BidirectionalSearch::get_last_dir() const {
    return cur_dir ? cur_dir->get_last_dir() : "";
}

UniformCostSearch *BidirectionalSearch::selectBestDirection() {
    if (!engine->is_silent()) utils::g_log << "selecting best direction, ";
    if (alternating) {
        if (!cur_dir) {
            if (!engine->is_silent()) utils::g_log << "(alternating) no current direction: " << (fw.get()->fw ? "[->]" : "[<-]") << " fw.g(): " << fw->frontier->g() << ", bw.g(): " << bw->frontier->g() << std::endl;
            cur_dir = fw;
        } else {
            if (cur_dir == fw) {
                if (!engine->is_silent()) utils::g_log << "(alternating): " << (bw.get()->fw ? "[->]" : "[<-]") << " fw.g(): " << fw->frontier->g() << ", bw.g(): " << bw->frontier->g() << std::endl;
                cur_dir = bw;
            } else {
                if (!engine->is_silent()) utils::g_log << "(alternating): " << (fw.get()->fw ? "[->]" : "[<-]") << " fw.g(): " << fw->frontier->g() << ", bw.g(): " << bw->frontier->g() << std::endl;
                cur_dir = fw;
            }
        }
    } else {
        Estimation &fw_est = *fw->get_step_estimator();
        Estimation &bw_est = *bw->get_step_estimator();
        if (fw_est.get_failed() && bw_est.get_failed()) {
            sym_params.increase_bound();
            bw_est.set_data(bw_est.get_time(), bw_est.get_nodes(), false);
            if (!engine->is_silent()) utils::g_log << (fw.get()->fw ? "[->]" : "[<-]") << " fw.g(): " << fw->frontier->g() << ", bw.g(): " << bw->frontier->g() << std::endl;
            return fw.get();
        }
        cur_dir = (bw_est < fw_est) ? bw : fw;
    }
    if (!engine->is_silent()) utils::g_log << (cur_dir.get()->fw ? "[->]" : "[<-]") << " fw.g(): " << fw->frontier->g() << ", bw.g(): " << bw->frontier->g() << std::endl;
    return cur_dir.get();
}

bool BidirectionalSearch::finished() const {
    return fw->finished() || bw->finished();
}

void BidirectionalSearch::stepImage(int maxTime, int maxNodes) {
    selectBestDirection()->stepImage(maxTime, maxNodes);
}
}
