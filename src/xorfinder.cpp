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
#include "time_mem.h"
#include "solver.h"
#include "occsimplifier.h"
#include "clauseallocator.h"
#include "sqlstats.h"
#include "varreplacer.h"

#include <limits>
#include <iostream>
#ifdef USE_TBUDDY
#include <pseudoboolean.h>
#endif
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

void XorFinder::find_xors_based_on_long_clauses()
{
    #ifdef DEBUG_MARKED_CLAUSE
    assert(solver->no_marked_clauses());
    #endif

    vector<Lit> lits;
    for (const auto & offset: occsimplifier->clauses) {
        if (xor_find_time_limit <= 0) break;

        Clause* cl = solver->cl_alloc.ptr(offset);
        xor_find_time_limit -= 1;

        //Already freed
        if (cl->freed() || cl->getRemoved() || cl->red()) {
            continue;
        }

        //Too large -> too expensive
        if (cl->size() > solver->conf.maxXorToFind) {
            continue;
        }

        //If not tried already, find an XOR with it
        if (!cl->stats.marked_clause ) {
            cl->stats.marked_clause = 1;
            assert(!cl->getRemoved());

            size_t needed_per_ws = 1ULL << (cl->size()-2);
            //let's allow shortened clauses
            needed_per_ws >>= 1;

            for(const Lit lit: *cl) {
                if (solver->watches[lit].size() < needed_per_ws) {
                    goto next;
                }
                if (solver->watches[~lit].size() < needed_per_ws) {
                    goto next;
                }
            }

            lits.resize(cl->size());
            std::copy(cl->begin(), cl->end(), lits.begin());
            findXor(lits, offset, cl->abst);
            next:;
        }
    }
}

void XorFinder::clean_equivalent_xors(vector<Xor>& txors)
{
    if (!txors.empty()) {
        size_t orig_size = txors.size();
        for(Xor& x: txors) {
            std::sort(x.begin(), x.end());
        }
        std::sort(txors.begin(), txors.end());

        size_t sz = 1;
        vector<Xor>::iterator i = txors.begin();
        vector<Xor>::iterator j = i;
        ++i;
        for(vector<Xor>::iterator end = txors.end(); i != end; ++i) {
            if (j->vars == i->vars && j->rhs == i->rhs) {
                j->merge_clash(*i, seen);
                j->detached |= i->detached;
                if (solver->frat->enabled()) {
                    verb_print(5, "Cleaning equivalent XOR at: " << (i - txors.begin()) << " xor: " << *i);
                    TBUDDY_DO(solver->frat->flush());
                    TBUDDY_DO(delete i->bdd);
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

void XorFinder::find_xors()
{
    runStats.clear();
    runStats.numCalls = 1;
    grab_mem();
    if ((solver->conf.xor_var_per_cut + 2) > solver->conf.maxXorToFind) {
        if (solver->conf.verbosity) {
            cout << "c WARNING updating max XOR to find to "
            << (solver->conf.xor_var_per_cut + 2)
            << " as the current number was lower than the cutting number" << endl;
        }
        solver->conf.maxXorToFind = solver->conf.xor_var_per_cut + 2;
    }

    //Clear flags. This is super-important.
    for(auto& offs: occsimplifier->clauses) {
        Clause* cl = solver->cl_alloc.ptr(offs);
        if (cl->getRemoved() || cl->freed()) {
            continue;
        }

        cl->set_used_in_xor(false);
        cl->set_used_in_xor_full(false);
    }

    if (solver->frat->enabled()) {
        solver->frat->flush();
        TBUDDY_DO(for (auto const& x: solver->xorclauses) delete x.bdd);
        TBUDDY_DO(for (auto const& x: solver->xorclauses_unused) delete x.bdd);
        TBUDDY_DO(for (auto const& x: solver->xorclauses_orig) delete x.bdd);
    }
    solver->xorclauses.clear();
    solver->xorclauses_orig.clear();
    solver->xorclauses_unused.clear();

    double myTime = cpuTime();
    const int64_t orig_xor_find_time_limit =
        1000LL*1000LL*solver->conf.xor_finder_time_limitM
        *solver->conf.global_timeout_multiplier;

    xor_find_time_limit = orig_xor_find_time_limit;

    occsimplifier->sort_occurs_and_set_abst();
    if (solver->conf.verbosity) {
        cout << "c [occ-xor] sort occur list T: " << (cpuTime()-myTime) << endl;
    }

    #ifdef DEBUG_MARKED_CLAUSE
    assert(solver->no_marked_clauses());
    #endif

    find_xors_based_on_long_clauses();
    assert(runStats.foundXors == solver->xorclauses.size());

    //clean them of equivalent XORs
    clean_equivalent_xors(solver->xorclauses);
    solver->xorclauses_orig = solver->xorclauses;

    // Need to do this due to XORs encoding new info
    //    see NOTE in cnf.h
    TBUDDY_DO(solver->frat->flush());
    TBUDDY_DO(for(auto& x: solver->xorclauses) if (solver->frat->enabled()) x.create_bdd_xor());
    TBUDDY_DO(for(auto& x: solver->xorclauses_orig) if (solver->frat->enabled()) x.create_bdd_xor());

    //Cleanup
    for(ClOffset offset: occsimplifier->clauses) {
        Clause* cl = solver->cl_alloc.ptr(offset);
        cl->stats.marked_clause = 0;
    }

    //Print stats
    const bool time_out = (xor_find_time_limit < 0);
    const double time_remain = float_div(xor_find_time_limit, orig_xor_find_time_limit);
    runStats.findTime = cpuTime() - myTime;
    runStats.time_outs += time_out;
    solver->sumSearchStats.num_xors_found_last = solver->xorclauses.size();
    print_found_xors();

    if (solver->conf.verbosity) runStats.print_short(solver, time_remain);
    globalStats += runStats;

    if (solver->sqlStats) {
        solver->sqlStats->time_passed(
            solver
            , "xor-find"
            , cpuTime() - myTime
            , time_out
            , time_remain
        );
    }
    solver->xor_clauses_updated = true;

    #ifdef SLOW_DEBUG
    for(const Xor& x: solver->xorclauses)
        for(uint32_t v: x)
            assert(solver->varData[v].removed == Removed::none);
    #endif
}

void XorFinder::print_found_xors()
{
    if (solver->conf.verbosity >= 5) {
        cout << "c Found XORs: " << endl;
        for(auto const& x: solver->xorclauses) cout << "c " << x << endl;
        cout << "c -> Total: " << solver->xorclauses.size() << " xors" << endl;
    }
}

void XorFinder::findXor(vector<Lit>& lits, const ClOffset offset, cl_abst_type abst)
{
    //Set this clause as the base for the XOR, fill 'seen'
    xor_find_time_limit -= lits.size()/4+1;
    poss_xor.setup(lits, offset, abst, occcnt);

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

    if (lits.size() <= solver->conf.maxXorToFindSlow) {
        findXorMatch(solver->watches[slit2], slit2);
        findXorMatch(solver->watches[~slit2], ~slit2);
    }

    if (poss_xor.foundAll()) {
        std::sort(lits.begin(), lits.end());
        Xor found_xor(lits, poss_xor.getRHS(), vector<uint32_t>());
        SLOW_DEBUG_DO(for(Lit lit: lits) assert(solver->varData[lit.var()].removed == Removed::none));

        add_found_xor(found_xor);
        assert(poss_xor.get_fully_used().size() == poss_xor.get_offsets().size());
        for(uint32_t i = 0; i < poss_xor.get_offsets().size() ; i++) {
            ClOffset offs = poss_xor.get_offsets()[i];
            bool fully_used = poss_xor.get_fully_used()[i];

            Clause* cl = solver->cl_alloc.ptr(offs);
            assert(!cl->getRemoved());
            cl->set_used_in_xor(true);
            cl->set_used_in_xor_full(fully_used);
        }
    }
    poss_xor.clear_seen(occcnt);
}

void XorFinder::add_found_xor(const Xor& found_xor)
{
    solver->xorclauses.push_back(found_xor);
    runStats.foundXors++;
    runStats.sumSizeXors += found_xor.size();
    runStats.maxsize = std::max<uint32_t>(runStats.maxsize, found_xor.size());
    runStats.minsize = std::min<uint32_t>(runStats.minsize, found_xor.size());
}

void XorFinder::findXorMatch(watch_subarray_const occ, const Lit wlit)
{
    xor_find_time_limit -= (int64_t)occ.size()/8+1;
    for (const Watched& w: occ) {
        if (w.isIdx()) {
            continue;
        }
        assert(poss_xor.getSize() > 2);

        if (w.isBin()) {
            SLOW_DEBUG_DO(assert(occcnt[wlit.var()]));
            if (w.red()) continue;
            if (!occcnt[w.lit2().var()]) goto end;

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
            if (cl.freed() || cl.getRemoved() || cl.red()) {
                //Clauses are ordered!!
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
                if (!occcnt[cl_lit.var()])
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

void XorFinder::move_xors_without_connecting_vars_to_unused()
{
    if (solver->xorclauses.empty()) return;

    double myTime = cpuTime();
    vector<Xor> cleaned;
    assert(toClear.empty());

    //Fill "seen" with vars used
    uint32_t non_empty = 0;
    for(const Xor& x: solver->xorclauses) {
        if (x.size() != 0) {
            non_empty++;
        }

        for(uint32_t v: x) {
            if (solver->seen[v] == 0) {
                toClear.push_back(Lit(v, false));
            }

            if (solver->seen[v] < 2) {
                solver->seen[v]++;
            }
        }
    }

    //has at least 1 var with occur of 2
    for(const Xor& x: solver->xorclauses) {
        if (xor_has_interesting_var(x) || x.detached) {
            #ifdef VERBOSE_DEBUG
            cout << "XOR has connecting var: " << x << endl;
            #endif
            cleaned.push_back(x);
        } else {
            #ifdef VERBOSE_DEBUG
            cout << "XOR has no connecting var: " << x << endl;
            #endif
            solver->xorclauses_unused.push_back(x);
        }
    }

    //clear "seen"
    for(Lit l: toClear) solver->seen[l.var()] = 0;
    toClear.clear();

    solver->xorclauses = cleaned;

    double time_used = cpuTime() - myTime;
    if (solver->conf.verbosity) {
        cout << "c [xor-rem-unconnected] left with " <<  solver->xorclauses.size()
        << " xors from " << non_empty << " non-empty xors"
        << solver->conf.print_times(time_used)
        << endl;
    }
    if (solver->sqlStats) {
        solver->sqlStats->time_passed_min(
            solver
            , "xor-rem-no-connecting-vars"
            , time_used
        );
    }
}

bool XorFinder::xor_together_xors(vector<Xor>& this_xors)
{
    if (occcnt.size() != solver->nVars())
        grab_mem();

    if (this_xors.empty())
        return solver->okay();

    #ifdef SLOW_DEBUG
    for(auto x: occcnt) {
        assert(x == 0);
    }
    #endif

    if (solver->conf.verbosity >= 5) {
        cout << "c XOR-ing together XORs. Starting with: " << endl;
        for(const auto& x: this_xors) {
            cout << "c XOR before xor-ing together: " << x << endl;
        }
    }

    assert(solver->okay());
    assert(solver->decisionLevel() == 0);
    assert(solver->watches.get_smudged_list().empty());
    const size_t origsize = this_xors.size();

    uint32_t xored = 0;
    const double myTime = cpuTime();
    assert(toClear.empty());

    //Link in xors into watchlist
    for(size_t i = 0; i < this_xors.size(); i++) {
        const Xor& x = this_xors[i];
        for(uint32_t v: x) {
            if (occcnt[v] == 0) {
                toClear.push_back(Lit(v, false));
            }
            occcnt[v]++;

            Lit l(v, false);
            assert(solver->watches.size() > l.toInt());
            solver->watches[l].push(Watched(i, WatchType::watch_idx_t));
            solver->watches.smudge(l);
        }
    }

    //Don't XOR together over the sampling vars
    //or variables that are in regular clauses
    vector<uint32_t> to_clear_2;
    if (solver->conf.sampling_vars) {
        for(uint32_t outside_var: *solver->conf.sampling_vars) {
            uint32_t outer_var = solver->map_to_with_bva(outside_var);
            outer_var = solver->varReplacer->get_var_replaced_with_outer(outer_var);
            uint32_t int_var = solver->map_outer_to_inter(outer_var);
            if (int_var < solver->nVars()) {
                if (!seen2[int_var]) {
                    seen2[int_var] = 1;
                    to_clear_2.push_back(int_var);
                    //cout << "sampling var: " << int_var+1 << endl;
                }
            }
        }
    }

    for(const auto& ws: solver->watches) {
        for(const auto& w: ws) {
            if (w.isBin() && !w.red()) {
                uint32_t v = w.lit2().var();
                if (!seen2[v]) {
                    seen2[v] = 1;
                    to_clear_2.push_back(v);
                }
            }
        }
    }

    for(const auto& offs: solver->longIrredCls) {
        Clause* cl = solver->cl_alloc.ptr(offs);
        if (cl->red() || cl->used_in_xor()) {
            continue;
        }
        for(Lit l: *cl) {
            if (!seen2[l.var()]) {
                seen2[l.var()] = 1;
                to_clear_2.push_back(l.var());
                //cout << "Not XORing together over var: " << l.var()+1 << endl;
            }
        }
    }

    //until fixedpoint
    bool changed = true;
    while(changed) {
        changed = false;
        interesting.clear();
        for(const Lit l: toClear) {
            if (occcnt[l.var()] == 2 && !seen2[l.var()]) {
                interesting.push_back(l.var());
            }
        }

        while(!interesting.empty()) {
            #ifdef SLOW_DEBUG
            {
                vector<uint32_t> check;
                check.resize(solver->nVars(), 0);
                for(size_t i = 0; i < this_xors.size(); i++) {
                    const Xor& x = this_xors[i];
                    for(uint32_t v: x) {
                        check[v]++;
                    }
                }
                for(size_t i = 0; i < solver->nVars(); i++) {
                    assert(check[i] == occcnt[i]);
                }
            }
            #endif

            //Pop and check if it can be XOR-ed together
            const uint32_t v = interesting.back();
            interesting.resize(interesting.size()-1);
            if (occcnt[v] != 2)
                continue;

            size_t idxes[2];
            unsigned at = 0;
            size_t i2 = 0;
            assert(solver->watches.size() > Lit(v, false).toInt());
            watch_subarray ws = solver->watches[Lit(v, false)];

            //Remove the 2 indexes from the watchlist
            for(size_t i = 0; i < ws.size(); i++) {
                const Watched& w = ws[i];
                if (!w.isIdx()) {
                    ws[i2++] = ws[i];
                } else if (!this_xors[w.get_idx()].empty()) {
                    assert(at < 2);
                    idxes[at] = w.get_idx();
                    at++;
                }
            }
            assert(at == 2);
            ws.resize(i2);

            Xor& x0 = this_xors[idxes[0]];
            Xor& x1 = this_xors[idxes[1]];
            uint32_t clash_var;
            uint32_t clash_num = xor_two(&x0, &x1, clash_var);

            //If they are equivalent
            if (x0.size() == x1.size()
                && x0.rhs == x1.rhs
                && clash_num == x0.size()
            ) {
                VERBOSE_PRINT("x1: " << x0 << " -- at idx: " << idxes[0]);
                VERBOSE_PRINT("x2: " << x1 << " -- at idx: " << idxes[1]);
                VERBOSE_PRINT("equivalent. ");

                //Update clash values & detached values
                x1.merge_clash(x0, seen);
                x1.detached |= x0.detached;

                VERBOSE_PRINT("after merge: " << x1 <<  " -- at idx: " << idxes[1]);

                //Equivalent, so delete one
                if (solver->frat->enabled()) {
                    TBUDDY_DO(solver->frat->flush());
                    TBUDDY_DO(delete x0.bdd);
                }
                x0 = Xor();

                //Re-attach the other, remove the occur of the one we deleted
                solver->watches[Lit(v, false)].push(Watched(idxes[1], WatchType::watch_idx_t));

                for(uint32_t v2: x1) {
                    Lit l(v2, false);
                    assert(occcnt[l.var()] >= 2);
                    occcnt[l.var()]--;
                    if (occcnt[l.var()] == 2 && !seen2[l.var()]) {
                        interesting.push_back(l.var());
                    }
                }
            } else if (clash_num > 1 || x0.detached || x1.detached) {
                //add back to ws, can't do much
                ws.push(Watched(idxes[0], WatchType::watch_idx_t));
                ws.push(Watched(idxes[1], WatchType::watch_idx_t));
                continue;
            } else {
                occcnt[v] -= 2;
                assert(occcnt[v] == 0);

                Xor x_new(tmp_vars_xor_two, x0.rhs ^ x1.rhs, clash_var);
                x_new.merge_clash(x0, seen);
                x_new.merge_clash(x1, seen);
                #ifdef USE_TBUDDY
                if (solver->frat->enabled()) {
                    solver->frat->flush();
                    tbdd::xor_set xs;
                    xs.add(*x0.create_bdd_xor());
                    xs.add(*x1.create_bdd_xor());
                    x_new.bdd = xs.sum();
                }
                #endif

                VERBOSE_PRINT("x1: " << x0 << " -- at idx: " << idxes[0]);
                VERBOSE_PRINT("x2: " << x1 << " -- at idx: " << idxes[1]);
                VERBOSE_PRINT("clashed on var: " << clash_var+1);
                VERBOSE_PRINT("final: " << x_new <<  " -- at idx: " << this_xors.size());

                changed = true;
                this_xors.push_back(x_new);
                for(uint32_t v2: x_new) {
                    Lit l(v2, false);
                    solver->watches[l].push(Watched(this_xors.size()-1, WatchType::watch_idx_t));
                    assert(occcnt[l.var()] >= 1);
                    if (occcnt[l.var()] == 2 && !seen2[l.var()]) {
                        interesting.push_back(l.var());
                    }
                }
                if (solver->frat->enabled()) {
                    TBUDDY_DO(solver->frat->flush());
                    TBUDDY_DO(delete this_xors[idxes[0]].bdd);
                    TBUDDY_DO(delete this_xors[idxes[1]].bdd);
                }
                this_xors[idxes[0]] = Xor();
                this_xors[idxes[1]] = Xor();
                xored++;
            }
        }
    }

    if (solver->conf.verbosity >= 5) {
        cout << "c Finished XOR-ing together XORs. " << endl;
        size_t at = 0;
        for(const auto& x: this_xors) {
            cout << "c XOR after xor-ing together: " << x << " -- at idx: " << at << endl;
            at++;
        }
    }

    //Clear
    for(const Lit l: toClear) occcnt[l.var()] = 0;
    toClear.clear();
    for(const auto& x: to_clear_2) seen2[x] = 0;

    solver->clean_occur_from_idx_types_only_smudged();
    clean_xors_from_empty(this_xors);
    double recur_time = cpuTime() - myTime;
    if (solver->conf.verbosity) {
        cout
        << "c [xor-together] xored together: " << xored
        << " orig xors: " << origsize
        << " new xors: " << this_xors.size()
        << solver->conf.print_times(recur_time)
        << endl;
    }


    if (solver->sqlStats) {
        solver->sqlStats->time_passed_min(
            solver
            , "xor-xor-together"
            , recur_time
        );
    }

    #if defined(SLOW_DEBUG) || defined(XOR_DEBUG)
    //Make sure none is 2.
    assert(toClear.empty());
    for(const Xor& x: this_xors) {
        for(uint32_t v: x) {
            if (occcnt[v] == 0) {
                toClear.push_back(Lit(v, false));
            }

            //Don't roll around
            occcnt[v]++;
        }
    }

    for(const Lit c: toClear) {
        /*This is now possible because we don't XOR them together
        in case they clash on more than 1 variable */
        //assert(occcnt[c.var()] != 2);

        occcnt[c.var()] = 0;
    }
    toClear.clear();
    #endif

    return solver->okay();
}

void XorFinder::clean_xors_from_empty(vector<Xor>& thisxors)
{
    size_t j = 0;
    for(size_t i = 0;i < thisxors.size(); i++) {
        Xor& x = thisxors[i];
        if (x.size() == 0 && x.rhs == false) {
            if (!x.clash_vars.empty()) {
                solver->xorclauses_unused.push_back(x);
            } else {
                TBUDDY_DO(solver->frat->flush());
                TBUDDY_DO(delete x.bdd);
            }
        } else {
            verb_print(4, "xor after clean: " << thisxors[i]);
            thisxors[j++] = thisxors[i];
        }
    }
    thisxors.resize(j);
}

uint32_t XorFinder::xor_two(Xor const* x1_p, Xor const* x2_p, uint32_t& clash_var)
{
    tmp_vars_xor_two.clear();
    if (x1_p->size() > x2_p->size()) {
        std::swap(x1_p, x2_p);
    }
    const Xor& x1 = *x1_p;
    const Xor& x2 = *x2_p;

    uint32_t clash_num = 0;
    for(uint32_t v: x1) {
        assert(seen[v] == 0);
        seen[v] = 1;
    }

    uint32_t i_x2;
    bool early_abort = false;
    for(i_x2 = 0; i_x2 < x2.size(); i_x2++) {
        uint32_t v = x2[i_x2];
        assert(seen[v] != 2);
        if (seen[v] == 0) {
            tmp_vars_xor_two.push_back(v);
        } else {
            clash_var = v;
            if (clash_num > 0 &&
                clash_num != i_x2 //not equivalent by chance
            ) {
                //early abort, it's never gonna be good
                clash_num++;
                early_abort = true;
                break;
            }
            clash_num++;
        }
        seen[v] = 2;
    }

    if (!early_abort) {
        #ifdef SLOW_DEBUG
        uint32_t other_clash = 0;
        #endif
        for(uint32_t v: x1) {
            if (seen[v] != 2) {
                tmp_vars_xor_two.push_back(v);
            } else {
                #ifdef SLOW_DEBUG
                other_clash++;
                #endif
            }
            seen[v] = 0;
        }
        #ifdef SLOW_DEBUG
        assert(other_clash == clash_num);
        #endif
    } else {
        for(uint32_t v: x1) {
            seen[v] = 0;
        }
    }

    for(uint32_t i = 0; i < i_x2; i++) {
        seen[x2[i]] = 0;
    }

    #ifdef SLOW_DEBUG
    for(uint32_t v: x1) {
        assert(seen[v] == 0);
    }
    #endif

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
    occcnt.clear();
    occcnt.resize(solver->nVars(), 0);
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
