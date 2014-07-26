#ifndef __MAIN_COMMON_H__
#define __MAIN_COMMON_H__

#include "cryptominisat.h"
#include <iostream>
#include <cmath>

void print_model(std::ostream* os, CMSat::SATSolver* solver)
{
    *os << "v ";
    size_t line_size = 2;
    for (uint32_t var = 0; var < solver->nVars(); var++) {
        if (solver->get_model()[var] != CMSat::l_Undef) {
            const bool value_is_positive = (solver->get_model()[var] == CMSat::l_True);
            const size_t this_var_size = std::ceil(std::log10(var+1)) + 1 + !value_is_positive;
            line_size += this_var_size;
            if (line_size > 80) {
                *os << std::endl << "v ";
                line_size = 2 + this_var_size;
            }
            *os << (value_is_positive? "" : "-") << var+1 << " ";
        }
    }

    *os << "0" << std::endl;
}

#endif //__MAIN_COMMON_H__
