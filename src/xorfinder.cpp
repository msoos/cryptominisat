/******************************************
Copyright (C) 2009-2020 Authors of CryptoMiniSat, see AUTHORS file

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

#include "xorfinder.h"
#include "constants.h"
#include "frat.h"
#include "time_mem.h"
#include "solver.h"
#include "occsimplifier.h"
#include "clauseallocator.h"
#include "sqlstats.h"
#include "varreplacer.h"

#include <limits>
#include <iostream>
//#define XOR_DEBUG

using namespace CMSat;
using std::cout;
using std::endl;

XorFinder::XorFinder(OccSimplifier* _occsimplifier, Solver* _solver) :
    occsimplifier(_occsimplifier)
    , solver(_solver)
    , toClear(_solver->toClear)
    , seen(_solver->seen)
    , seen2(_solver->seen2)
{
    tmp_vars_xor_two.reserve(2000);
}

// Adds found XOR clauses to solver->xorclauses
void XorFinder::find_xors_based_on_long_clauses() {
    DEBUG_MARKED_CLAUSE_DO(assert(solver->no_marked_clauses()));

    vector<Lit> lits;
    for (const auto & offset: occsimplifier->clauses) {
        if (xor_find_time_limit <= 0) break;

        Clause* cl = solver->cl_alloc.ptr(offset);
        xor_find_time_limit -= 1;

        //Already freed
        if (cl->freed() || cl->get_removed() || cl->red()) continue;

        //Too large -> too expensive
        if (cl->size() > solver->conf.maxXorToFind) continue;

        //If not tried already, find an XOR with it
        if (!cl->stats.marked_clause ) {
            cl->stats.marked_clause = 1;
            assert(!cl->get_removed());

            size_t needed_per_ws = 1ULL << (cl->size()-2);
            //let's allow shortened clauses
            needed_per_ws >>= 1;

            for(const Lit lit: *cl) {
                if (solver->watches[lit].size() < needed_per_ws) goto next;
                if (solver->watches[~lit].size() < needed_per_ws) goto next;
            }

            lits.resize(cl->size());
            std::copy(cl->begin(), cl->end(), lits.begin());
            findXor(lits, offset, cl->abst);
            next:;
        }
    }
}

// NOTE: all in `xorclauses` must be detached at this point
void XorFinder::clean_equivalent_xors(vector<Xor>& txors) {
    if (!txors.empty()) {
        size_t orig_size = txors.size();
        for(Xor& x: txors) std::sort(x.begin(), x.end());
        std::sort(txors.begin(), txors.end());

        size_t sz = 1;
        vector<Xor>::iterator i = txors.begin();
        vector<Xor>::iterator j = i;
        ++i;
        for(vector<Xor>::iterator end = txors.end(); i != end; ++i) {
            if (j->vars == i->vars && j->rhs == i->rhs) {
                if (solver->frat->enabled()) {
                    assert(false && "TODO FRAT");
                }
            } else {
                j++;
                *j = *i;
                sz++;
            }
        }
        txors.resize(sz);

        if (solver->conf.verbosity) {
            cout << "c [xor-clean-equiv] removed equivalent xors: "
            << (orig_size-txors.size()) << " left with: " << txors.size()
            << endl;
        }
    }
}

// Finds the XORs based on long clauses, and adds them to xorclauses
// does NOT detach or delete the corresponding clauses
bool XorFinder::find_xors() {
    assert(solver->gmatrices.empty());
    const auto orig_num_xors = solver->xorclauses.size();

    runStats.clear();
    runStats.numCalls = 1;
    grab_mem();

    for(auto& gw: solver->gwatches) gw.clear();
    if (!solver->okay()) return false;

    double my_time = cpuTime();
    const int64_t orig_xor_find_time_limit =
        1000LL*1000LL*solver->conf.xor_finder_time_limitM
        *solver->conf.global_timeout_multiplier;

    xor_find_time_limit = orig_xor_find_time_limit;

    occsimplifier->sort_occurs_and_set_abst();
    verb_print(1, "[occ-xor] sort occur list T: " << (cpuTime()-my_time));
    DEBUG_MARKED_CLAUSE_DO(assert(solver->no_marked_clauses()));

    find_xors_based_on_long_clauses();
    assert(orig_num_xors + runStats.foundXors == solver->xorclauses.size());
    // TODO FRAT
    /* clean_equivalent_xors(solver->xorclauses); */

    //Cleanup
    for(ClOffset offset: occsimplifier->clauses) {
        Clause* cl = solver->cl_alloc.ptr(offset);
        cl->stats.marked_clause = 0;
    }

    //Print stats
    const bool time_out = (xor_find_time_limit < 0);
    const double time_remain = float_div(xor_find_time_limit, orig_xor_find_time_limit);
    runStats.findTime = cpuTime() - my_time;
    runStats.time_outs += time_out;
    solver->print_xors(solver->xorclauses);

    if (solver->conf.verbosity) runStats.print_short(solver, time_remain);
    globalStats += runStats;

    if (solver->sqlStats) {
        solver->sqlStats->time_passed(
            solver
            , "xor-find"
            , cpuTime() - my_time
            , time_out
            , time_remain
        );
    }
    return solver->okay();
}


void XorFinder::findXor(vector<Lit>& lits, const ClOffset offset, cl_abst_type abst)
{
    //Set this clause as the base for the XOR, fill 'seen'
    xor_find_time_limit -= lits.size()/4+1;
    poss_xor.setup(lits, offset, abst, occ_cnt);

    //Run findXorMatch for the 2 smallest watchlists
    Lit slit = lit_Undef;
    Lit slit2 = lit_Undef;
    uint32_t smallest = numeric_limits<uint32_t>::max();
    uint32_t smallest2 = numeric_limits<uint32_t>::max();
    for (size_t i = 0, end = lits.size(); i < end; i++) {
        const Lit lit = lits[i];
        uint32_t num = solver->watches[lit].size();
        num += solver->watches[~lit].size();
        if (num < smallest) {
            slit2 = slit;
            smallest2 = smallest;

            slit = lit;
            smallest = num;
        } else if (num < smallest2) {
            slit2 = lit;
            smallest2 = num;
        }
    }
    findXorMatch(solver->watches[slit], slit);
    findXorMatch(solver->watches[~slit], ~slit);

    if (!solver->frat->enabled() && lits.size() <= solver->conf.maxXorToFindSlow) {
        findXorMatch(solver->watches[slit2], slit2);
        findXorMatch(solver->watches[~slit2], ~slit2);
    }

    if (poss_xor.foundAll()) {
        std::sort(lits.begin(), lits.end());
        for(auto& l: lits) l = l.unsign();
        Xor found_xor(lits, poss_xor.getRHS());
        SLOW_DEBUG_DO(for(Lit lit: lits) assert(solver->varData[lit.var()].removed == Removed::none));

        add_found_xor(found_xor);
        assert(poss_xor.get_fully_used().size() == poss_xor.get_offsets().size());
        for(uint32_t i = 0; i < poss_xor.get_offsets().size() ; i++) {
            ClOffset offs = poss_xor.get_offsets()[i];
            Clause* cl = solver->cl_alloc.ptr(offs);
            assert(!cl->get_removed());
        }
    }
    poss_xor.clear_seen(occ_cnt);
}

void XorFinder::add_found_xor(const Xor& found_xor)
{
    frat_func_start();
    solver->xorclauses.push_back(found_xor);
    Xor& added = solver->xorclauses.back();
    runStats.foundXors++;
    runStats.sumSizeXors += found_xor.size();
    runStats.maxsize = std::max<uint32_t>(runStats.maxsize, found_xor.size());
    runStats.minsize = std::min<uint32_t>(runStats.minsize, found_xor.size());
    solver->xorclauses_updated = true;
    if (solver->frat->enabled()) {
        solver->chain.clear();
        INC_XID(added);
        for(const auto& off: poss_xor.get_offsets()) {
            auto cl = *solver->cl_alloc.ptr(off);
            assert(!cl.freed());
            assert(!cl.get_removed());
            solver->chain.push_back(cl.stats.ID);
        }
        *solver->frat << implyxfromcls << added; solver->add_chain(); *solver->frat << fin;
    }
    frat_func_end();
}

void XorFinder::findXorMatch(watch_subarray_const occ, const Lit wlit)
{
    xor_find_time_limit -= (int64_t)occ.size()/8+1;
    for (const Watched& w: occ) {
        if (w.isIdx()) continue;
        assert(poss_xor.getSize() > 2);

        if (w.isBin()) {
            // FRAT-XOR cannot have different sized clauses for the moment
            if (solver->frat->enabled()) continue;

            SLOW_DEBUG_DO(assert(occ_cnt[wlit.var()]));
            if (w.red()) continue;
            if (!occ_cnt[w.lit2().var()]) goto end;

            binvec.clear();
            binvec.resize(2);
            binvec[0] = w.lit2();
            binvec[1] = wlit;
            if (binvec[0] > binvec[1]) {
                std::swap(binvec[0], binvec[1]);
            }

            xor_find_time_limit -= 1;
            poss_xor.add(binvec, numeric_limits<ClOffset>::max(), varsMissing);
            if (poss_xor.foundAll())
                break;
        } else {
            if (w.getBlockedLit().toInt() == lit_Undef.toInt())
                //Clauses are ordered, lit_Undef means it's larger than maxXorToFind
                break;

            if (w.getBlockedLit().toInt() == lit_Error.toInt())
                //lit_Error means it's freed or removed, and it's ordered so no more
                break;

            if ((w.getBlockedLit().toInt() | poss_xor.getAbst()) != poss_xor.getAbst())
                continue;

            xor_find_time_limit -= 3;
            const ClOffset offset = w.get_offset();
            Clause& cl = *solver->cl_alloc.ptr(offset);
            if (cl.freed() || cl.get_removed() || cl.red()) {
                //Clauses are ordered!!
                break;
            }

            // FRAT cannot handle mix of sizes
            if (solver->frat->enabled() && cl.size() != poss_xor.getSize()) {
                //clauses are ordered!!
                break;
            }

            //Allow the clause to be smaller or equal in size
            if (cl.size() > poss_xor.getSize()) {
                //clauses are ordered!!
                break;
            }

            //For longer clauses, don't the the fancy algo that can
            //deal with incomplete XORs
            if (cl.size() != poss_xor.getSize()
                && poss_xor.getSize() > solver->conf.maxXorToFindSlow
            ) {
                break;
            }

            //Doesn't contain variables not in the original clause
            SLOW_DEBUG_DO(assert(cl.abst == calcAbstraction(cl)));
            if ((cl.abst | poss_xor.getAbst()) != poss_xor.getAbst())
                continue;

            //Check RHS, vars inside
            bool rhs = true;
            for (const Lit cl_lit :cl) {
                //early-abort, contains literals not in original clause
                if (!occ_cnt[cl_lit.var()])
                    goto end;

                rhs ^= cl_lit.sign();
            }
            //either the invertedness has to match, or the size must be smaller
            if (rhs != poss_xor.getRHS() && cl.size() == poss_xor.getSize())
                continue;

            //If the size of this clause is the same of the base clause, then
            //there is no point in using this clause as a base for another XOR
            //because exactly the same things will be found.
            if (cl.size() == poss_xor.getSize()) {
                cl.stats.marked_clause = 1;
            }

            xor_find_time_limit -= cl.size()/4+1;
            poss_xor.add(cl, offset, varsMissing);
            if (poss_xor.foundAll())
                break;
        }
        end:;
    }
}

uint32_t XorFinder::xor_two(Xor const* x1_p, Xor const* x2_p, uint32_t& clash_var) {
    SLOW_DEBUG_DO(for(const auto& s: seen) assert(s == 0));
    tmp_vars_xor_two.clear();
    if (x1_p->size() > x2_p->size()) std::swap(x1_p, x2_p);
    const Xor& x1 = *x1_p;
    const Xor& x2 = *x2_p;

    uint32_t clash_num = 0;
    for(uint32_t v: x1) {
        assert(seen[v] == 0);
        seen[v] = 1;
    }

    for(const auto& v: x2) {
        assert(seen[v] != 2);
        if (seen[v] == 0) tmp_vars_xor_two.push_back(v);
        else {
            clash_var = v;
            clash_num++;
        }
        seen[v] = 2;
    }

    for(const auto& v: x1) {
        if (seen[v] != 2) tmp_vars_xor_two.push_back(v);
        seen[v] = 0;
    }

    for(const auto& v: x2) seen[v] = 0;
    return clash_num;
}

bool XorFinder::xor_has_interesting_var(const Xor& x)
{
    for(uint32_t v: x) {
        if (solver->seen[v] > 1) {
            return true;
        }
    }
    return false;
}

size_t XorFinder::mem_used() const
{
    size_t mem = 0;
    mem += solver->xorclauses.capacity()*sizeof(Xor);

    //Temporary
    mem += tmpClause.capacity()*sizeof(Lit);
    mem += varsMissing.capacity()*sizeof(uint32_t);

    return mem;
}

void XorFinder::grab_mem()
{
    occ_cnt.clear();
    occ_cnt.resize(solver->nVars(), 0);
}

void XorFinder::Stats::print_short(const Solver* solver, double time_remain) const
{
    cout
    << "c [occ-xor] found " << std::setw(6) << foundXors
    ;
    if (foundXors > 0) {
        cout
        << " avg sz " << std::setw(3) << std::fixed << std::setprecision(1)
        << float_div(sumSizeXors, foundXors)
        << " min sz " << std::setw(2) << std::fixed << std::setprecision(1)
        << minsize
        << " max sz " << std::setw(2) << std::fixed << std::setprecision(1)
        << maxsize;
    }
    cout
    << solver->conf.print_times(findTime, time_outs, time_remain)
    << endl;
}

XorFinder::Stats& XorFinder::Stats::operator+=(const XorFinder::Stats& other)
{
    //Time
    findTime += other.findTime;

    //XOR
    foundXors += other.foundXors;
    sumSizeXors += other.sumSizeXors;

    //Usefulness
    time_outs += other.time_outs;

    return *this;
}
