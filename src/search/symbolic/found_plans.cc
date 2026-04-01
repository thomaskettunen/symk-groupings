#include "found_plans.h"

namespace found_plans
{
  found_plans::found_plans() { std::cout << "THIS SHOULD ONLY APPEAR ONCE" << std::endl; }

  bool found_plans::is_dominated(symbolic::Cost g) {
    for (auto cost : this->paretto_frontier) {
      if (cost.dominates(g)) {
        return true;
      }
    }
    return false;
  };

  found_plans global_instance;
} 
