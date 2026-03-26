#include "found_plans.h"

namespace found_plans
{
  found_plans::found_plans() {}

  bool found_plans::is_dominated(symbolic::Cost g) {
    for (auto cost : this->paretto_frontier){
      if(g >= cost) return true;
    }
    return false;
  };

  found_plans global_instance;
} 
