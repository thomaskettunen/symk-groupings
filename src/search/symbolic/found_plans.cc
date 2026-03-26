#include "found_plans.h"

namespace found_plans
{
  found_plans::found_plans() {}

  bool found_plans::is_dominated(symbolic::Cost g) {
    for (auto cost : this->paretto_frontier){

      std::cout << "this may be dominated:\n" << g << "\n" << cost << std::endl;
      if(g >= cost){
        std::cout << "dominated" << std::endl;
      }else{
        std::cout << "not dominated" << std::endl;
      }

      if(g >= cost) return true;
    }
    return false;
  };

  found_plans global_instance;
} 
