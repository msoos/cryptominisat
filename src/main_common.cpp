/*************************************************************
MiniSat       --- Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
CryptoMiniSat --- Copyright (C) 2009-2020 Authors of CryptoMiniSat, see AUTHORS file

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
***************************************************************/

#include "main_common.h"
#include "solverconf.h"
#include <iostream>
#include <fstream>

using std::endl;

using namespace CMSat;

void MainCommon::handle_frat_option() {
    FILE* fratfTmp = fopen(frat_fname.c_str(), "wb");
    if (fratfTmp == nullptr) {
        std::cerr
        << "ERROR: Could not open FRAT file '" << frat_fname << "' for writing"
        << endl;

        std::exit(-1);
    }
    fratf = fratfTmp;
}

void MainCommon::handle_idrup_option() {
    assert(conf.idrup);
    FILE* idrupfTmp = fopen(idrup_fname.c_str(), "w");
    if (idrupfTmp == NULL) {
        std::cerr
        << "ERROR: Could not open IDRUP file '" << idrup_fname << "' for writing"
        << endl;

        std::exit(-1);
    }
    idrupf = idrupfTmp;
}

uint32_t MainCommon::print_model(CMSat::SATSolver* solver, std::ostream* os, const std::vector<uint32_t>* only)
{
    *os << "v ";
    size_t line_size = 2;
    size_t num_undef = 0;

    auto fun = [&](uint32_t var) {
        if (solver->get_model()[var] != CMSat::l_Undef) {
            const bool value_is_positive = (solver->get_model()[var] == CMSat::l_True);
            const size_t this_var_size = std::ceil(std::log10(var+1)) + 1 + !value_is_positive;
            line_size += this_var_size;
            if (line_size > 80) {
                *os << std::endl << "v ";
                line_size = 2 + this_var_size;
            }
            *os << (value_is_positive? "" : "-") << var+1 << " ";
        } else {
            num_undef++;
        }
    };

    if (only == nullptr) {
        for (uint32_t var = 0; var < solver->nVars(); var++) {
            fun(var);
        }
    } else {
        for(uint32_t var: *only) {
            fun(var);
        }
    }
    *os << "0" << std::endl;
    return num_undef;
}
