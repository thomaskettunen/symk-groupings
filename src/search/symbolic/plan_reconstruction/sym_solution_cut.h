#ifndef SYMBOLIC_PLAN_RECONSTRUCTION_SYM_SOLUTION_H
#define SYMBOLIC_PLAN_RECONSTRUCTION_SYM_SOLUTION_H

#include "../sym_variables.h"

#include "../../task_proxy.h"

#include <vector>

#include "../cost.h"

namespace symbolic {
class SymSolutionCut {
protected:
    Cost g;
    Cost h;
    BDD cut;

public:
    SymSolutionCut(); // dummy for no solution
    SymSolutionCut(Cost g, Cost h, BDD cut);

    Cost get_g() const;
    void set_g(Cost g);

    Cost get_h() const;
    void set_h(Cost h);

    Cost get_f() const;

    BDD get_cut() const;
    void set_cut(BDD cut);

    void merge(const SymSolutionCut &other);

    // Here we only compare g and h values!!!
    bool operator<(const SymSolutionCut &other) const;
    bool operator>(const SymSolutionCut &other) const;
    bool operator==(const SymSolutionCut &other) const;
    bool operator!=(const SymSolutionCut &other) const;

    friend std::ostream &operator<<(std::ostream &os, const SymSolutionCut &sym_cut) {
        return os << "symcut{" 
                  << "g=" << sym_cut.get_g() 
                  << ", h=" << sym_cut.get_h()
                  << ", f=" << sym_cut.get_f()
                  << ", nodes=" << sym_cut.get_cut().nodeCount() 
                  << "}";
    }
};
} // namespace symbolic
#endif
