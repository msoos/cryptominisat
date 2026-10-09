/******************************************
Copyright (C) 2009-2026 Authors of CryptoMiniSat, see AUTHORS file

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

#pragma once

#include <cstdint>
#include <vector>
#include "solvertypesmini.h"
#include "cloffset.h"

namespace CMSat {

class Solver;

// Congruence closure over AND and XOR gates, as kissat's: gates of the same
// type whose inputs are equivalent have equivalent outputs. Merging bottom-up
// to a fixpoint makes the definitions of merged outputs identical, so BVE does
// not face one var with two different definitions. Equivalences are added as
// binary clauses with propagation-derived hints; var replacement substitutes.
class Congruence {
public:
    explicit Congruence(Solver* solver);
    bool run();

private:
    struct Gate {
        Lit out;
        std::vector<Lit> ins; //XOR: positive literals of the input vars
        bool is_xor;
    };
    enum class Norm { skip, key, out_false, out_equiv };
    struct TmpCl {
        ClOffset offs;
        Lit a, b; //binary: offs unused
        int32_t id;
        bool is_bin;
    };

    Solver* solver;
    std::vector<Gate> gates;
    std::vector<Lit> repr;
    std::vector<uint32_t> stamp;
    uint32_t cur_stamp = 0;
    std::vector<TmpCl> tmps;

    uint64_t num_and = 0;
    uint64_t num_xor = 0;
    uint64_t num_merged = 0;
    uint64_t num_units = 0;
    uint64_t num_failed = 0;

    void extract_ands();
    void extract_xors();
    Lit find(Lit l);
    void unite(Lit a, Lit b);
    Norm normalize(const Gate& g, std::vector<uint32_t>& key, Lit& out, Lit& equiv);
    bool prove(const std::vector<Lit>& cl, std::vector<uint32_t> split);
    bool add_implied(const std::vector<Lit>& cl, const std::vector<uint32_t>& split);
    void delete_tmps();
    bool equate(Lit a, Lit b, const std::vector<uint32_t>& split);
};

}
