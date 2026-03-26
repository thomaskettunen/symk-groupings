#ifndef GLOBAL_PARETTO_H
#define GLOBAL_PARETTO_H

#include "cost.h"
#include <stdlib.h>
#include "set"

namespace found_plans {
  class found_plans{
    public:
      found_plans();
      std::set<symbolic::Cost> paretto_frontier;

      bool is_dominated(symbolic::Cost g);
  };

  extern found_plans global_instance;
}

#endif