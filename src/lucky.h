/******************************************
Copyright (C) 2009-2020 Authors of CryptoMiniSat, see AUTHORS file
Originally from CaDiCaL's "lucky.cpp", Copyright by Armin Biere, 2019

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
***********************************************/

#ifndef LUCKY_PHASES_H_
#define LUCKY_PHASES_H_

#include "solvertypes.h"

namespace CMSat {

class Solver;
class Xor;

// CaDiCaL's 'lucky_phases'
class Lucky
{
public:
    Lucky(Solver* solver);

    // l_True if one of the checks satisfied the formula, in which case the
    // model has been stored and the trail unwound. l_Undef otherwise.
    lbool doit();

    // 10 = satisfied, 0 = not lucky, -1 = terminated. As in CaDiCaL.
    // Public so the tests can drive them one at a time.
    int trivially_satisfiable(const bool polar);
    int forward_satisfiable(const bool polar);
    int backward_satisfiable(const bool polar);
    int horn_satisfiable(const bool polar);

private:
    int unlucky(const int res);
    // False if we are back at level 0 because of a conflict, or because
    // 'aborted' was set by an asynchronous termination.
    bool assume(const Lit lit);
    bool assume_rest(const bool polar);
    bool all_same_sat_xors(const bool polar) const;
    bool xors_satisfied() const;
    bool terminated() const;
    void found_model(const char* what, const bool polar);

    Solver* solver;
    vector<Lit> to_set;
    bool aborted = false;
};

}

#endif
