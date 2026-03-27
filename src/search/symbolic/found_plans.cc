#include "found_plans.h"

namespace found_plans
{
  found_plans::found_plans() { std::cout << "THIS SHOULD ONLY APPEAR ONCE" << std::endl; }

  bool found_plans::is_dominated(symbolic::Cost g) {
    for (auto cost : this->paretto_frontier) {
      if (cost.dominates(g)) {
        std::cout << g << " is dominated by " << cost << std::endl;
        return true;
      } else {
        std::cout << g << " is NOT dominated by " << cost << std::endl;
      }
    }
    return false;
  };

  found_plans global_instance;
} 
