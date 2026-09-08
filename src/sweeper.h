/******************************************
Copyright (C) 2026 Authors of CryptoMiniSat, see AUTHORS file

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
#include <limits>
#include <vector>
#include <unordered_set>

#include "solvertypesmini.h"
#include "cloffset.h"

struct kitten;

namespace CMSat {

class Solver;
class OccSimplifier;
class Clause;

///SAT sweeping, ported from CaDiCaL/Kissat. Extracts a local environment of
///clauses around a variable, uses kitten to prove environment-level units
///(backbone) and literal equivalences. Units are added with FRAT chains from
///kitten cores; equivalences become FRAT-chained irred binaries that
///scc-vrepl/VarReplacer pick up later.
class Sweeper
{
public:
    Sweeper(OccSimplifier* occs, Solver* solver);
    ~Sweeper();
    bool sweep();

    //kitten core traversal callbacks, public for the extern "C" trampolines
    void core_cb_plain(unsigned id, bool learned, size_t sz, const unsigned* lits);
    void core_cb_chain(unsigned cid, unsigned id, bool learned, size_t sz,
                       const unsigned* lits, size_t chsz, const unsigned* chain);

private:
    struct EnvCl {
        int32_t id;             ///< CMS clause ID
        std::vector<Lit> lits;  ///< original literals, including falsified
    };
    struct ProofCl {
        uint32_t kit_id = 0;
        uint32_t sweep_id = std::numeric_limits<uint32_t>::max(); ///< index into env_cls
        int32_t cms_id = 0;
        bool learned = false;
        std::vector<Lit> lits;
        std::vector<unsigned> chain; ///< kitten clause ids, reversed
    };

    void sweep_variable(uint32_t v);
    void collect_environment(uint32_t v);
    void add_lit_to_env(uint32_t depth, Lit l);
    bool env_add_long(uint32_t depth, ClOffset off);
    void env_add_bin(uint32_t depth, Lit l1, Lit l2, int32_t id);
    void clear_env();
    Lit repr(Lit l);

    void init_backbone_and_partition();
    void refine();
    void refine_backbone();
    void refine_partition();
    void flip_backbone_literals();
    void flip_partition_literals();
    bool backbone_candidate(Lit l);
    bool extract_fixed(Lit l);
    bool equivalence_candidates(Lit lit, Lit other);
    void partition_remove(Lit l);
    void partition_squash_or_drop_last(Lit lit, Lit other, bool drop_first);

    int solve();
    bool flip(Lit l);
    bool ticks_out() const;
    void charge_kitten_ticks();

    void save_core(uint32_t which);
    void add_core(uint32_t which);
    void clear_core(uint32_t which);
    void save_add_clear_core();
    bool add_equiv_bin(const ProofCl* pc, Lit a, Lit b);

    OccSimplifier* occs;
    Solver* solver;
    kitten* kit = nullptr;
    bool fr = false; ///< FRAT enabled for this run

    //environment, valid during one sweep_variable
    std::vector<uint32_t> depths;       ///< per var, 0 = not in env
    std::vector<uint32_t> env_vars;
    std::vector<EnvCl> env_cls;
    std::vector<ClOffset> env_marked;   ///< longs with marked_clause set
    std::unordered_set<int32_t> env_bin_ids;
    uint32_t env_encoded = 0;
    std::vector<Lit> backbone;
    std::vector<Lit> partition;         ///< classes separated by lit_Undef
    std::vector<Lit> reprs;             ///< union-find over literals
    std::vector<uint8_t> unit_at_collect; ///< var was assigned when its clause was collected
    std::vector<ProofCl> core[2];
    uint32_t core_save = 0;

    //limits for this run
    struct {
        uint32_t vars;
        uint32_t clauses;
        uint32_t depth;
    } limit;

    //persistent across runs
    std::vector<uint8_t> done_outer; ///< outer var already swept this epoch
    uint32_t epoch = 0;

    //stats for this run
    uint32_t swept_vars = 0;
    uint32_t found_units = 0;
    uint32_t found_equivs = 0;
    uint64_t solves = 0;
    uint64_t flips = 0;

    //temporaries
    std::vector<unsigned> kit_cl_tmp;
    std::vector<Lit> tmp_lits;
    std::vector<int32_t> tmp_hints;
    std::vector<Lit> tmp_bin;
    std::vector<Lit> tmp_fin;
};

}
