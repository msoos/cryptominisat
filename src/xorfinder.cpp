/******************************************
Copyright (c) 2016, Mate Soos

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

#include <limits>

using namespace CMSat;
using std::cout;
using std::endl;

XorFinder::XorFinder(OccSimplifier* _occsimplifier, Solver* _solver) :
    occsimplifier(_occsimplifier)
    , solver(_solver)
    , seen(_solver->seen)
    , toClear(_solver->toClear)
{
}

void XorFinder::find_xors_based_on_long_clauses()
{
    vector<Lit> lits;
    for (vector<ClOffset>::iterator
        it = occsimplifier->clauses.begin()
        , end = occsimplifier->clauses.end()
        ; it != end && xor_find_time_limit > 0
        ; ++it
    ) {
        ClOffset offset = *it;
        Clause* cl = solver->cl_alloc.ptr(offset);
        xor_find_time_limit -= 3;

        //Already freed
        if (cl->freed() || cl->getRemoved()) {
            continue;
        }

        //Too large -> too expensive
        if (cl->size() > solver->conf.maxXorToFind) {
            continue;
        }

        //If not tried already, find an XOR with it
        if (!cl->stats.marked_clause ) {
            cl->stats.marked_clause = true;
            assert(!cl->getRemoved());

            const size_t needed_per_ws = 1ULL << (cl->size()-2);
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

            //TODO check if already inside in some clever way
            findXor(lits, offset, cl->abst);
            next:;
        }
    }
}

void XorFinder::find_xors()
{
    runStats.clear();
    runStats.numCalls = 1;

    cls_of_xors.clear();
    xors.clear();
    double myTime = cpuTime();
    const int64_t orig_xor_find_time_limit =
        1000LL*1000LL*solver->conf.xor_finder_time_limitM
        *solver->conf.global_timeout_multiplier;

    xor_find_time_limit = orig_xor_find_time_limit;

    #ifdef DEBUG_MARKED_CLAUSE
    assert(solver->no_marked_clauses());
    #endif

    find_xors_based_on_long_clauses();

    //Cleanup
    for(ClOffset offset: occsimplifier->clauses) {
        Clause* cl = solver->cl_alloc.ptr(offset);
        cl->stats.marked_clause = false;
    }

    //Print stats
    const bool time_out = (xor_find_time_limit < 0);
    const double time_remain = float_div(xor_find_time_limit, orig_xor_find_time_limit);
    runStats.findTime = cpuTime() - myTime;
    runStats.time_outs += time_out;
    assert(runStats.foundXors == xors.size());
    solver->sumSearchStats.num_xors_found_last = xors.size();
    print_found_xors();

    if (solver->conf.verbosity) {
        runStats.print_short(solver);
    }
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
}

void XorFinder::print_found_xors()
{
    if (solver->conf.verbosity >= 5) {
        cout << "c Found XORs: " << endl;
        for(vector<Xor>::const_iterator
            it = xors.begin(), end = xors.end()
            ; it != end
            ; ++it
        ) {
            cout << "c " << *it << endl;
        }
    }
}

void XorFinder::add_xors_to_gauss()
{
    solver->xorclauses = xors;
    #ifdef SLOW_DEBUG
    for(const Xor& x: xors) {
        for(uint32_t v: x) {
            assert(solver->varData[v].removed == Removed::none);
        }
    }
    #endif

    for(ClOffset offs: cls_of_xors) {
        Clause* cl = solver->cl_alloc.ptr(offs);
        assert(!cl->getRemoved());
        cl->set_used_in_xor(true);
    }
}

void XorFinder::findXor(vector<Lit>& lits, const ClOffset offset, cl_abst_type abst)
{
    //Set this clause as the base for the XOR, fill 'seen'
    xor_find_time_limit -= 50;
    poss_xor.setup(lits, offset, abst, seen);

    Lit slit = lit_Undef;
    size_t snum = std::numeric_limits<size_t>::max();
    for (const Lit lit: lits) {
        size_t num = solver->watches[lit].size() + solver->watches[~lit].size();
        if (snum > num) {
            snum = num;
            slit = lit;
        }
    }

    xor_find_time_limit -= 20;
    findXorMatch(solver->watches[slit]);
    findXorMatch(solver->watches[~slit]);

    if (poss_xor.foundAll()) {
        std::sort(lits.begin(), lits.end());
        Xor found_xor(lits, poss_xor.getRHS());
        #ifdef SLOW_DEBUG
        for(Lit lit: lits) {
            assert(solver->varData[lit.var()].removed == Removed::none);
        }
        #endif

        //TODO check if already inside in some clever way
        add_found_xor(found_xor);
        for(ClOffset off: poss_xor.get_offsets()) {
            cls_of_xors.push_back(off);
        }
    }

    //Clear 'seen'
    for (const Lit tmp_lit: lits) {
        seen[tmp_lit.var()] = 0;
    }
}

void XorFinder::add_found_xor(const Xor& found_xor)
{
    xors.push_back(found_xor);
    runStats.foundXors++;
    runStats.sumSizeXors += found_xor.size();
}

void XorFinder::findXorMatch(watch_subarray_const occ)
{
    xor_find_time_limit -= (int64_t)occ.size();
    for (const Watched& w: occ) {
        if (w.isIdx() || w.isBin()) {
            continue;
        }

        //Deal with clause

        //Clause will be at least 3 long
        if (poss_xor.getSize() <= 2) {
            continue;
        }

        const ClOffset offset = w.get_offset();
        Clause& cl = *solver->cl_alloc.ptr(offset);
        xor_find_time_limit -= 20; //deref penalty
        if (cl.freed() || cl.getRemoved())
            continue;

        //Could be smaller, but it would be expensive
        if (cl.size() != poss_xor.getSize())
            continue;

        //Doesn't contain variables not in the original clause
        if ((cl.abst | poss_xor.getAbst()) != poss_xor.getAbst())
            continue;

        //Check RHS, vars inside
        bool rhs = true;
        for (const Lit cl_lit :cl) {
            //early-abort, contains literals not in original clause
            if (!seen[cl_lit.var()])
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
            cl.stats.marked_clause = true;;
        }

        xor_find_time_limit-=50;
        poss_xor.add(cl, offset, varsMissing);
        if (poss_xor.foundAll())
            break;

        end:;
    }
}

void XorFinder::clean_up_xors()
{
    assert(toClear.empty());

    //Fill seen with vars used
    for(const Xor& x: xors) {
        for(uint32_t v: x) {
            if (seen[v] == 0) {
                toClear.push_back(Lit(v, false));
            }

            if (seen[v] < 2) {
                seen[v]++;
            }
        }
    }

    vector<Xor>::iterator it = xors.begin();
    vector<Xor>::iterator it2 = xors.begin();
    size_t num = 0;
    for(vector<Xor>::iterator end = xors.end()
        ; it != end
        ; it++
    ) {
        if (xor_has_interesting_var(*it)) {
            *it2 = *it;
            it2++;
            num++;
        }
    }
    xors.resize(num);

    for(Lit l: toClear) {
        seen[l.var()] = 0;
    }
    toClear.clear();
}

void XorFinder::xor_together_xors()
{
    assert(solver->watches.get_smudged_list().empty());
    uint32_t xored = 0;
    const double myTime = cpuTime();
    assert(toClear.empty());
    //count how many times a var is used
    for(const Xor& x: xors) {
        for(uint32_t v: x) {
            if (seen[v] == 0) {
                toClear.push_back(Lit(v, false));
            }

            //Don't roll around
            if (seen[v] != std::numeric_limits<uint16_t>::max()) {
                seen[v]++;
            }
        }
    }

    //Link in xors into watchlist
    for(size_t i = 0; i < xors.size(); i++) {
        const Xor& x = xors[i];
        for(uint32_t v: x) {
            Lit l(v, false);
            assert(solver->watches.size() > l.toInt());
            solver->watches[l].push(Watched(i)); //Idx watch
            solver->watches.smudge(l);
        }
    }

    //Only when a var is used exactly twice it's interesting
    interesting.clear();
    for(const Lit l: toClear) {
        if (seen[l.var()] == 2) {
            interesting.push_back(l.var());
        }
    }

    while(!interesting.empty()) {
        const uint32_t v = interesting.back();
        interesting.resize(interesting.size()-1);

        Xor x[2];
        size_t idxes[2];
        unsigned at = 0;
        size_t i2 = 0;
        assert(solver->watches.size() > Lit(v, false).toInt());
        watch_subarray ws = solver->watches[Lit(v, false)];
        for(size_t i = 0; i < ws.size(); i++) {
            const Watched& w = ws[i];
            if (!w.isIdx()) {
                ws[i2] = ws[i];
                i2++;
            } else if (xors[w.get_idx()] != Xor()) {

                //Rollaround in 'seen' -- probably will never happen
                if (at > 2) {
                    assert(false && "Rollaround in 'seen'? May happen, but weird");
                    continue;
                }

                x[at] = xors[w.get_idx()];
                idxes[at] = w.get_idx();
                at++;
            }
        }
        ws.resize(i2);
        if (at < 2) {
            //Has been removed thanks to some XOR-ing together, skip
            continue;
        }

        vector<uint32_t> vars = xor_two(x[0], x[1]);
        Xor x_new(vars, x[0].rhs ^ x[1].rhs);
        xors.push_back(x_new);
        for(uint32_t v: x_new) {
            Lit l(v, false);
            solver->watches[l].push(Watched(xors.size()-1));
            solver->watches.smudge(l);
        }
        xors[idxes[0]] = Xor();
        xors[idxes[1]] = Xor();
        xored++;
    }

    for(const Lit l: toClear) {
        seen[l.var()] = 0;
    }
    toClear.clear();

    solver->clean_occur_from_idx_types_only_smudged();
    clean_xors_from_empty();
    double recur_time = cpuTime() - myTime;
        if (solver->conf.verbosity) {
        cout
        << "c [occ-xor] xored together " << xored*2
        << " clauses "
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

    #ifdef SLOW_DEBUG
    //Make sure none is 2.
    for(const Xor& x: xors) {
        for(uint32_t v: x) {
            if (seen[v] == 0) {
                toClear.push_back(Lit(v, false));
            }

            //Don't roll around
            if (seen[v] != std::numeric_limits<uint16_t>::max()) {
                seen[v]++;
            }
        }
    }

    for(const Lit c: toClear) {
        assert(seen[c.var()] != 2);
        seen[c.var()] = 0;
    }
    toClear.clear();
    #endif
}

void XorFinder::clean_xors_from_empty()
{
    size_t i2 = 0;
    for(size_t i = 0;i < xors.size(); i++) {
        Xor& x = xors[i];
        if (x.size() == 0
            && x.rhs == false
        ) {
            //nothing, skip
        } else {
            xors[i2] = xors[i];
            i2++;
        }
    }
    xors.resize(i2);
}

bool XorFinder::add_new_truths_from_xors()
{
    size_t origTrailSize  = solver->trail_size();
    size_t origBins = solver->binTri.redBins;
    double myTime = cpuTime();

    assert(solver->ok);
    size_t i2 = 0;
    for(size_t i = 0;i < xors.size(); i++) {
        Xor& x = xors[i];
        if (x.size() > 2) {
            xors[i2] = xors[i];
            i2++;
            continue;
        }

        switch(x.size() ) {
            case 0: {
                if (x.rhs == true) {
                    solver->ok = false;
                    return false;
                }
                break;
            }

            case 1: {
                vector<Lit> lits = {Lit(x[0], !x.rhs)};
                solver->add_clause_int(lits, true, ClauseStats(), false);
                if (!solver->ok) {
                    return false;
                }
                break;
            }

            case 2: {
                //RHS == 1 means both same is not allowed
                vector<Lit> lits{Lit(x[0], false), Lit(x[1], true^x.rhs)};
                solver->add_clause_int(lits, true, ClauseStats(), false);
                if (!solver->ok) {
                    return false;
                }
                lits = {Lit(x[0], true), Lit(x[1], false^x.rhs)};
                solver->add_clause_int(lits, true, ClauseStats(), false);
                if (!solver->ok) {
                    return false;
                }
                break;
            }

            default: {
                assert(false && "Not possible");
            }
        }
    }
    xors.resize(i2);

    double add_time = cpuTime() - myTime;
    uint32_t num_bins_added = solver->binTri.redBins - origBins;
    uint32_t num_units_added = solver->trail_size() - origTrailSize;

    if (solver->conf.verbosity) {
        cout
        << "c [occ-xor] added unit " << num_units_added
        << " bin " << num_bins_added
        << solver->conf.print_times(add_time)
        << endl;
    }


    if (solver->sqlStats) {
        solver->sqlStats->time_passed_min(
            solver
            , "xor-add-new-bin-unit"
            , add_time
        );
    }

    return true;
}

vector<uint32_t> XorFinder::xor_two(
    Xor& x1, Xor& x2
) {
    x1.sort();
    x2.sort();
    vector<uint32_t> ret;
    size_t x1_at = 0;
    size_t x2_at = 0;
    while(x1_at < x1.size() || x2_at < x2.size()) {
        if (x1_at == x1.size()) {
            ret.push_back(x2[x2_at]);
            x2_at++;
            continue;
        }

        if (x2_at == x2.size()) {
            ret.push_back(x1[x1_at]);
            x1_at++;
            continue;
        }

        const uint32_t a = x1[x1_at];
        const uint32_t b = x2[x2_at];
        if (a == b) {
            x1_at++;
            x2_at++;

            seen[a] -= 2;
            if (seen[a] == 2) {
                interesting.push_back(a);
            }
            continue;
        }

        if (a < b) {
            ret.push_back(a);
            x1_at++;
            continue;
        } else {
            ret.push_back(b);
            x2_at++;
            continue;
        }
    }

    return ret;
}

bool XorFinder::xor_has_interesting_var(const Xor& x)
{
    for(uint32_t v: x) {
        if (seen[v] > 1) {
            return true;
        }
    }
    return false;
}

size_t XorFinder::mem_used() const
{
    size_t mem = 0;
    mem += xors.capacity()*sizeof(Xor);

    //Temporary
    mem += tmpClause.capacity()*sizeof(Lit);
    mem += varsMissing.capacity()*sizeof(uint32_t);

    return mem;
}

void XorFinder::Stats::print_short(const Solver* solver) const
{
    cout
    << "c [occ-xor] found " << std::setw(6) << foundXors
    << " avg sz " << std::setw(4) << std::fixed << std::setprecision(1)
    << float_div(sumSizeXors, foundXors)
    << solver->conf.print_times(findTime, time_outs)
    << endl;
}

void XorFinder::Stats::print() const
{
    cout << "c --------- XOR STATS ----------" << endl;
    print_stats_line("c num XOR found on avg"
        , float_div(foundXors, numCalls)
        , "avg size"
    );

    print_stats_line("c XOR avg size"
        , float_div(sumSizeXors, foundXors)
    );

    print_stats_line("c XOR finding time"
        , findTime
        , float_div(time_outs, numCalls)*100.0
        , "time-out"
    );
    cout << "c --------- XOR STATS END ----------" << endl;
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
