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

#include "gatefinder.h"
#include "time_mem.h"
#include "solver.h"
#include "occsimplifier.h"
#include "subsumestrengthen.h"
#include "clauseallocator.h"
#include <array>
#include <utility>
#include "sqlstats.h"

using namespace CMSat;
using std::cout;
using std::endl;

GateFinder::GateFinder(OccSimplifier *_simplifier, Solver *_solver) :
    numDotPrinted(0)
    , simplifier(_simplifier)
    , solver(_solver)
    , seen(_solver->seen)
    , seen2(_solver->seen2)
    , toClear(solver->toClear)
{
//     sizeSortedOcc.resize(solver->conf.maxGateBasedClReduceSize+1);
}

void GateFinder::cleanup()
{
    solver->clean_occur_from_idx_types_only_smudged();
    orGates.clear();
}

void GateFinder::find_all()
{
    runStats.clear();
    orGates.clear();

    assert(solver->watches.get_smudged_list().empty());
    find_or_gates_and_update_stats();
    if (solver->conf.doPrintGateDot) {
        print_graphviz_dot();
    }
//     if (true) {
//         for(auto g: orGates) {
//             cout << "found: " << g << endl;
//         }
//     }

    if (solver->conf.verbosity >= 3) {
        runStats.print(solver->nVars());
    }
    globalStats += runStats;
    solver->sumSearchStats.num_gates_found_last = orGates.size();
}

void GateFinder::find_or_gates_and_update_stats()
{
    assert(solver->ok);

    double myTime = cpuTime();
    const int64_t orig_numMaxGateFinder =
        solver->conf.gatefinder_time_limitM*100LL*1000LL
        *solver->conf.global_timeout_multiplier;
    numMaxGateFinder = orig_numMaxGateFinder;
    simplifier->limit_to_decrease = &numMaxGateFinder;

    find_or_gates();

    for(const auto orgate: orGates) {
        runStats.gatesSize += 2;
        runStats.num++;
    }
    const double time_used = cpuTime() - myTime;
    const bool time_out = (numMaxGateFinder <= 0);
    const double time_remain = float_div(numMaxGateFinder, orig_numMaxGateFinder);
    runStats.findGateTime = time_used;
    runStats.find_gate_timeout = time_out;
    if (solver->sqlStats) {
        solver->sqlStats->time_passed(
            solver
            , "gate find"
            , time_used
            , time_out
            , time_remain
        );
    }

    if (solver->conf.verbosity) {
        cout << "c [occ-gates]"
        << " found: " << print_value_kilo_mega(runStats.num)
        << " avg-s: " << std::fixed << std::setprecision(1)
        << float_div(runStats.gatesSize, runStats.num)
        /*<< " avg-s: " << std::fixed << std::setprecision(1)
        << float_div(learntGatesSize, numRed)*/
        << solver->conf.print_times(time_used, time_out, time_remain)
        << endl;
    }
}

struct IncidenceSorter
{
    IncidenceSorter(const vector<uint32_t>& _inc) :
        inc(_inc)
    {}

    bool operator()(const uint32_t a, const uint32_t b) {
        return inc[a] < inc[b];
    }

    const vector<uint32_t>& inc;
};

vector<uint32_t> GateFinder::get_definability(vector<uint32_t>& vars)
{
    double myTime = cpuTime();
    vector<uint32_t> definable;

    for(const auto var: vars) {
        assert(var < solver->nVars() && "get_definability must not be passed variables that no longer exist");
        assert(var < seen.size());
        seen[var] = 1;
    }


    //Set up incidence
    vector<uint32_t> inc(solver->nVars(), 0);
    for(const auto& g: orGates) {
        if (seen[g.lit1.var()] &&
            seen[g.lit2.var()] &&
            seen[g.rhs.var()])
        {
            inc[g.rhs.var()]++;
        }
    }
    for(const auto var: vars) {
        assert(var < inc.size());
    }
    std::stable_sort(vars.begin(), vars.end(), IncidenceSorter(inc));

    for(auto var: vars) {
        //already defined, let's not do circular dependency
        if (!seen[var]) {
            continue;
        }

        bool defined = false;
        for(int inv = 0; inv < 2 && !defined; inv++) {
            Lit lit = Lit(var, inv);
            const auto& ws = solver->watches[lit];
            for(auto w: ws) {
                if (!w.isIdx()) {
                    continue;
                }
                const OrGate& g = orGates[w.get_idx()];
                assert(g.rhs.var() == var);

                //must be part of original set, and its ingredients must not have been defined
                if (seen[g.rhs.var()] &&
                    seen[g.lit1.var()] &&
                    seen[g.lit2.var()])
                {
                        seen[g.rhs.var()] = 0;
                        definable.push_back(var);
                        defined = true;
                        break;
                }
            }
        }
    }

    for(auto var: vars) {
        seen[var] = 0;
    }

    double time_passed = cpuTime() - myTime;
    if (solver->conf.verbosity) {
        cout << "c [occ-gates] definable: "
        << print_value_kilo_mega(definable.size())
        << solver->conf.print_times(time_passed)
        << endl;
    }

    return definable;
}

void GateFinder::find_or_gates()
{
    if (solver->nVars() < 1)
        return;

    const size_t offs = solver->mtrand.randInt(solver->nVars()*2-1);
    for(size_t i = 0
        ; i < solver->nVars()*2
            && *simplifier->limit_to_decrease > 0
            && !solver->must_interrupt_asap()
        ; i++
    ) {
        const size_t at = (offs + i) % (solver->nVars()*2);
        const Lit lit = Lit::toLit(at);
        find_or_gates_in_sweep_mode(lit);
        find_or_gates_in_sweep_mode(~lit);
    }
}

void GateFinder::find_or_gates_in_sweep_mode(const Lit lit)
{
    assert(toClear.empty());

    //From the clauses
    //a V -b
    //a V -c
    //mark b and c with seen[b]=1 and seen[c]=1
    watch_subarray_const ws = solver->watches[lit];
    *simplifier->limit_to_decrease -= ws.size();
    for(const Watched w: ws) {
        if (w.isBin() && !w.red()) {
            seen[(~w.lit2()).toInt()] = 1;
            toClear.push_back(~w.lit2());
        }
    }
    //avoid loops
    seen[(~lit).toInt()] = 0;

    //Now look for
    //b V c V -a
    //so look through watches[-a] and check for 3-long clauses
    watch_subarray_const ws2 = solver->watches[~lit];
    *simplifier->limit_to_decrease -= ws2.size();
    for(const Watched w: ws2) {
        //Looking for tri or longer
        if (!w.isClause()) {
            continue;
        }

        ClOffset offset = w.get_offset();
        const Clause& cl = *solver->cl_alloc.ptr(offset);
        if (cl.size() > 3 || cl.red() || cl.getRemoved()) {
            continue;
        }
        assert(cl.size() == 3);


        bool ok = true;
        int at = 0;
        Lit lits[2];
        for(int i = 0; i < 3; i++) {
            if (cl[i] != ~lit && !seen[cl[i].toInt()]) {
                ok = false;
                break;
            }
            if (cl[i] != ~lit) {
                assert(at < 2);
                lits[at++] = cl[i];
            }
        }
        if (!ok) {
            break;
        }

        add_gate_if_not_already_inside(lit, lits[0], lits[1]);
    }

    *simplifier->limit_to_decrease -= toClear.size();
    for(const Lit toclear: toClear) {
        seen[toclear.toInt()] = 0;
    }
    toClear.clear();
}


void GateFinder::add_gate_if_not_already_inside(
    const Lit rhs
    , const Lit lit1
    , const Lit lit2
) {
    OrGate gate(rhs, lit1, lit2);
    for (Watched ws: solver->watches[gate.rhs]) {
        if (ws.isIdx()
            && orGates[ws.get_idx()] == gate
        ) {
            return;
        }
    }
    link_in_gate(gate);
}

void GateFinder::link_in_gate(const OrGate& gate)
{
    const size_t at = orGates.size();
    orGates.push_back(gate);
    solver->watches[gate.rhs].push(Watched(at));
    solver->watches.smudge(gate.rhs);
}

////////////////////////// PRINTING //////////////////////////////

void GateFinder::print_graphviz_dot()
{
    std::stringstream ss;
    ss << "Gates" << (numDotPrinted++) << ".dot";
    std::string filenename = ss.str();
    std::ofstream file(filenename.c_str(), std::ios::out);
    file << "digraph G {" << endl;
    vector<bool> gateUsed;
    gateUsed.resize(orGates.size(), false);
    size_t index = 0;
    for (const OrGate orGate: orGates) {
        index++;
        for (const Lit lit: orGate.getLits()) {
            for (Watched ws: solver->watches[lit]) {
                if (!ws.isIdx()) {
                    continue;
                }
                uint32_t at = ws.get_idx();

                //The same one, skip
                if (at == index)
                    continue;

                file << "Gate" << at;
                gateUsed[at] = true;
                file << " -> ";

                file << "Gate" << index;
                gateUsed[index] = true;

                file << "[arrowsize=\"0.4\"];" << endl;
            }

            /*vector<uint32_t>& occ2 = gateOccEq[(~*it2).toInt()];
            for (vector<uint32_t>::const_iterator it3 = occ2.begin(), end3 = occ2.end(); it3 != end3; it3++) {
                if (*it3 == index) continue;

                file << "Gate" << *it3;
                gateUsed[*it3] = true;
                file << " -> ";

                file << "Gate" << index;
                gateUsed[index] = true;

                file << "[style = \"dotted\", arrowsize=\"0.4\"];" << endl;
            }*/
        }
    }

    index = 0;
    for (const OrGate orGate: orGates) {
        index++;

        if (gateUsed[index]) {
            file << "Gate" << index << " [ shape=\"point\"";
            file << ", size = 0.8";
            file << ", style=\"filled\"";
            file << ", color=\"darkseagreen\"";
            file << "];" << endl;
        }
    }

    file  << "}" << endl;
    file.close();
    cout << "c Printed gate structure to file " << filenename << endl;
}

GateFinder::Stats& GateFinder::Stats::operator+=(const Stats& other)
{
    findGateTime += other.findGateTime;
    find_gate_timeout += other.find_gate_timeout;
    orBasedTime += other.orBasedTime;
    or_based_timeout += other.or_based_timeout;
    varReplaceTime += other.varReplaceTime;
    andBasedTime += other.andBasedTime;
    and_based_timeout += other.and_based_timeout;
    erTime += other.erTime;

    //OR-gate
    orGateUseful += other.orGateUseful;
    numLongCls += other.numLongCls;
    numLongClsLits += other.numLongClsLits;
    litsRem += other.litsRem;
    varReplaced += other.varReplaced;

    //And-gate
    andGateUseful += other.andGateUseful;
    clauseSizeRem += other.clauseSizeRem;

    //ER
    numERVars += other.numERVars;

    //Gates
    gatesSize += other.gatesSize;
    num += other.num;

    return *this;
}

void GateFinder::Stats::print(const size_t nVars) const
{
    cout << "c -------- GATE FINDING ----------" << endl;
    print_stats_line("c time"
        , total_time()
    );

    print_stats_line("c find gate time"
        , findGateTime
        , stats_line_percent(findGateTime, total_time())
        , "% time"
    );

    print_stats_line("c gate-based cl-sh time"
        , orBasedTime
        , stats_line_percent(orBasedTime, total_time())
        , "% time"
    );

    print_stats_line("c gate-based cl-rem time"
        , andBasedTime
        , stats_line_percent(andBasedTime, total_time())
        , "% time"
    );

    print_stats_line("c gate-based varrep time"
        , varReplaceTime
        , stats_line_percent(varReplaceTime, total_time())
        , "% time"
    );

    print_stats_line("c gatefinder cl-short"
        , orGateUseful
        , stats_line_percent(orGateUseful, numLongCls)
        , "% long cls"
    );

    print_stats_line("c gatefinder lits-rem"
        , litsRem
        , stats_line_percent(litsRem, numLongClsLits)
        , "% long cls lits"
    );

    print_stats_line("c gatefinder cl-rem"
        , andGateUseful
        , stats_line_percent(andGateUseful, numLongCls)
        , "% long cls"
    );

    print_stats_line("c gatefinder cl-rem's lits"
        , clauseSizeRem
        , stats_line_percent(clauseSizeRem, numLongClsLits)
        , "% long cls lits"
    );

    print_stats_line("c gatefinder var-rep"
        , varReplaced
        , stats_line_percent(varReplaced, nVars)
        , "% vars"
    );

    cout << "c -------- GATE FINDING END ----------" << endl;
}

