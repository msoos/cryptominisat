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

#include "solver.h"
#include "oracle/oracle.h"

using namespace CMSat;

inline vector<int> negate(vector<int> vec) {
	for (int& lit : vec) lit = sspp::Neg(lit);
	return vec;
}

template<typename T>
void swapdel(vector<T>& vec, size_t i) {
	assert(i < vec.size());
	std::swap(vec[i], vec.back());
	vec.pop_back();
}

inline int orclit(const Lit x) {
    return (x.sign() ? ((x.var()+1)*2+1) : (x.var()+1)*2);
}

inline Lit orc_to_lit(int x) {
    uint32_t var = x/2-1;
    bool neg = x&1;
    return Lit(var, neg);
}

vector<vector<int>> Solver::get_irred_cls_for_oracle() const
{
    vector<vector<int>> clauses;
    vector<int> tmp;
    for (const auto& off: longIrredCls) {
        const Clause& cl = *cl_alloc.ptr(off);
        tmp.clear();
        for (auto const& l: cl) {
            tmp.push_back(orclit(l));
        }
        clauses.push_back(tmp);
    }
    for (uint32_t i = 0; i < nVars()*2; i++) {
        Lit l = Lit::toLit(i);
        for(auto const& ws: watches[l]) {
            if (!ws.isBin() || ws.red())  continue;
            if (l < ws.lit2()) {
                tmp.clear();
                tmp.push_back(orclit(l));
                tmp.push_back(orclit(ws.lit2()));
                clauses.push_back(tmp);
            }
        }
    }
    return clauses;
}

bool Solver::oracle_vivif(bool& finished)
{
    assert(!frat->enabled());
    assert(solver->okay());
    finished = false;

    backbone_simpl(300LL*1000LL, finished);
    execute_inprocess_strategy(false, "must-renumber");
    if (!okay()) return okay();
    if (nVars() < 10) return okay();
    double my_time = cpuTime();

    auto clauses = get_irred_cls_for_oracle();
    detach_and_free_all_irred_cls();

    sspp::oracle::Oracle oracle(nVars(), clauses, {});
    oracle.SetVerbosity(conf.verbosity);
    bool sat = false;
    for (int i = 0; i < (int)clauses.size(); i++) {
        for (int j = 0; j < (int)clauses[i].size(); j++) {
            if (oracle.getStats().mems > 1600LL*1000LL*1000LL) goto end;
            auto assump = negate(clauses[i]);
            swapdel(assump, j);
            auto ret = oracle.Solve(assump, true, 500LL*1000LL*1000LL);
            if (ret.isUnknown()) {
                goto end;
            }
            if (ret.isFalse()) {
                sort(assump.begin(), assump.end());
                auto clause = negate(assump);
                oracle.AddClauseIfNeededAndStr(clause, true);
                clauses[i] = clause;
                j = -1; //start from beginning
                if (clause.empty()) {
                    ok = false;
                    return false;
                }
            } else if(!sat) {
                sat = true;
            }
        }
    }
    finished |= true;

    end:
    vector<Lit> tmp2;
    for(const auto& cl: clauses) {
        tmp2.clear();
        for(const auto& l: cl) tmp2.push_back(orc_to_lit(l));
        Clause* cl2 = solver->add_clause_int(tmp2);
        if (cl2) longIrredCls.push_back(cl_alloc.get_offset(cl2));
        if (!okay()) return false;
    }

    if (conf.oracle_get_learnts) {
        for (const auto& cl: oracle.GetLearnedClauses()) {
            tmp2.clear();
            for(const auto& l: cl) tmp2.push_back(orc_to_lit(l));
            ClauseStats s;
            s.which_red_array = 2;
            s.ID = ++clauseID;
            s.glue = cl.size();
            Clause* cl2 = solver->add_clause_int(tmp2, true, &s);
            if (cl2) longRedCls[2].push_back(cl_alloc.get_offset(cl2));
            if (!okay()) return false;
        }
    }

    verb_print(1, "[oracle-vivif] finished: " << finished
            << " cache-used: " << oracle.getStats().cache_useful
            << " cache-added: " << oracle.getStats().cache_added
            << " learnt-units: " << oracle.getStats().learned_units
            << " finished (vivif or backbone): " << finished
            << " T: " << std::setprecision(2) << (cpuTime()-my_time));
    return solver->okay();
}

void Solver::dump_cls_oracle(const string fname, const vector<OracleDat>& cs)
{
    vector<sspp::Lit> tmp;
    std::ofstream fout(fname.c_str());
    fout << nVars() << endl;
    for(uint32_t i = 0; i < cs.size(); i++) {
        const auto& c = cs[i];
        tmp.clear();
        if (!c.binary) {
            Clause& cl = *cl_alloc.ptr(c.off);
            for(auto const& l: cl) assert(l.var() < nVars());
            for(auto const& l: cl) tmp.push_back(orclit(l));
        } else {
            const OracleBin& b = c.bin;
            assert(b.l1.var() < nVars());
            assert(b.l2.var() < nVars());
            tmp.push_back(orclit(b.l1));
            tmp.push_back(orclit(b.l2));
        }
        for(auto const& l: tmp) fout << l << " ";
        fout << endl;
    }
}

void Solver::print_cs_ordering(const vector<OracleDat>& cs) const
{
    for(const auto& c: cs) {
        cout << "c.bin:" << c.binary;
        if (!c.binary) cout << " offs: " << c.off;
        else {
            cout << " bincl: " << c.bin.l1 << "," << c.bin.l2;
        }

        cout << " c.val: ";
        for(const auto& v: c.val) cout << v << " ";
        cout << endl;
    }
}

vector<vector<uint16_t>> Solver::compute_edge_weights() const
{
    vector<vector<uint16_t>> edgew(nVars());
    for (uint32_t i = 0; i < nVars(); i++) edgew[i].resize(nVars(), 0);
    for (const auto& off: longIrredCls) {
        Clause& cl = *cl_alloc.ptr(off);
        for (auto const& l1 : cl) { for (auto const& l2 : cl) {
            uint32_t v1 = l1.var();
            uint32_t v2 = l2.var();
            if (v1 < v2) edgew[v1][v2]++;
        } }
    }
    for (uint32_t i = 0; i < nVars()*2; i++) {
        Lit l = Lit::toLit(i);
        for(auto const& ws: watches[l]) {
            if (!ws.isBin() || ws.red())  continue;
            uint32_t v1 = l.var();
            uint32_t v2 = ws.lit2().var();
            if (v1 < v2) edgew[v1][v2]++;
        }
    }
    return edgew;
}

vector<Solver::OracleDat> Solver::order_clauses_for_oracle() const
{
    vector<vector<uint16_t>> edgew;
    bool edgew_avail = false;
    // Cutoff to limit memory usage. Otherwise it's n**2 memory usage
    if (nVars() < 35000) { edgew = compute_edge_weights(); edgew_avail = true; }
    vector<OracleDat> cs;
    array<int, ORACLE_DAT_SIZE> ww;
    for (const auto& off: longIrredCls) {
        Clause& cl = *cl_alloc.ptr(off);
        assert(!cl.red());
        ww = {};
        if (edgew_avail) {
            for (auto const& l1 : cl) { for (auto const& l2: cl) {
                const uint32_t v1 = l1.var();
                const uint32_t v2 = l2.var();
                if (v1 < v2) {
                    assert(edgew[v1][v2] >= 1);
                    if (edgew[v1][v2] <= ww.size()) ww[edgew[v1][v2]-1]--;
                }
            } }
        } else ww[0] = cl.size();
        cs.push_back(OracleDat(ww, off));
    }

    for (uint32_t i = 0; i < nVars()*2; i++) {
        Lit l = Lit::toLit(i);
        for(auto const& ws: watches[l]) {
            if (!ws.isBin() || ws.red())  continue;
            const uint32_t v1 = l.var();
            const uint32_t v2 = ws.lit2().var();
            if (v1 < v2) {
                ww = {};
                if (edgew_avail) {
                    assert(edgew[v1][v2] >= 1);
                    if (edgew[v1][v2] <= ww.size()) ww[edgew[v1][v2]-1]--;
                } else ww[0] = 2;
                cs.push_back(OracleDat(ww, OracleBin(l, ws.lit2(), ws.get_ID())));
            }
        }
    }
    std::sort(cs.begin(), cs.end());
    /* print_cs_ordering(cs); */
    return cs;
}

bool Solver::oracle_sparsify()
{
    assert(!frat->enabled());
    execute_inprocess_strategy(false, "sub-impl, occ-backw-sub, must-renumber");
    if (!okay()) return okay();
    if (nVars() < 10) return okay();

    double my_time = cpuTime();
    uint32_t removed = 0;
    uint32_t removed_bin = 0;
    auto cs = order_clauses_for_oracle();
    const uint32_t tot_cls = longIrredCls.size() + binTri.irredBins;
    assert(cs.size() == tot_cls);
    //dump_cls_oracle("debug.xt", cs);

    // The "+tot_cls" is for indicator variables
    sspp::oracle::Oracle oracle(nVars()+tot_cls, {});
    oracle.SetVerbosity(conf.verbosity);
    vector<sspp::Lit> tmp;
    for(uint32_t i = 0; i < cs.size(); i++) {
        const auto& c = cs[i];
        tmp.clear();
        if (!c.binary) {
            Clause& cl = *cl_alloc.ptr(c.off);
            for(auto const& l: cl) assert(l.var() < nVars());
            for(auto const& l: cl) tmp.push_back(orclit(l));
        } else {
            const OracleBin& b = c.bin;
            assert(b.l1.var() < nVars());
            assert(b.l2.var() < nVars());
            tmp.push_back(orclit(b.l1));
            tmp.push_back(orclit(b.l2));
        }
        // Indicator variable
        tmp.push_back(orclit(Lit(nVars()+i, false)));
        oracle.AddClause(tmp, false);
    }
    const double build_time = cpuTime() - my_time;

    // Set all assumptions to FALSE, i.e. all clauses are active
    for (uint32_t i = 0; i < tot_cls; i++) {
        oracle.SetAssumpLit(orclit(Lit(nVars()+i, true)), false);
    }

    // Now try to remove clauses one-by-one
    uint32_t last_printed = 0;
    for (uint32_t i = 0; i < tot_cls; i++) {
        if ((10*i)/(tot_cls) != last_printed) {
            verb_print(1, "[oracle-sparsify] done with " << ((10*i)/(tot_cls))*10 << " %"
                << " oracle mems: " << print_value_kilo_mega(oracle.getStats().mems)
                << " T: " << (cpuTime()-my_time));
            last_printed = (10*i)/(tot_cls);
        }

        // Try removing this clause, making its indicator TRUE (i.e. removed)
        oracle.SetAssumpLit(orclit(Lit(nVars()+i, false)), false);
        tmp.clear();
        const auto& c = cs[i];
        if (!c.binary) {
            Clause& cl = *cl_alloc.ptr(c.off);
            for(auto const& l: cl) tmp.push_back(orclit(~l));
        } else {
            tmp.push_back(orclit(~(c.bin.l1)));
            tmp.push_back(orclit(~(c.bin.l2)));
        }

        auto ret = oracle.Solve(tmp, false, 600LL*1000LL*1000LL);
        if (ret.isUnknown()) { /*out of time*/ goto fin; }

        if (ret.isTrue()) {
            // We need this clause, can't remove
            oracle.SetAssumpLit(orclit(Lit(nVars()+i, true)), true);
        } else {
            assert(ret.isFalse());
            // We can freeze(!) this clause to be disabled.
            oracle.SetAssumpLit(orclit(Lit(nVars()+i, false)), true);
            removed++;
            if (!c.binary) {
                Clause& cl = *cl_alloc.ptr(c.off);
                assert(!cl.stats.marked_clause);
                cl.stats.marked_clause = 1;
            } else {
                removed_bin++;
                Lit lit1 = c.bin.l1;
                Lit lit2 = c.bin.l2;
                findWatchedOfBin(watches, lit1, lit2, false, c.bin.ID).mark_bin_cl();
                findWatchedOfBin(watches, lit2, lit1, false, c.bin.ID).mark_bin_cl();
            }
        }

        if (oracle.getStats().mems > 900LL*1000LL*1000LL) {
            verb_print(1, "[oracle-sparsify] too many mems in oracle, aborting");
            goto fin;
        }
    }

    fin:
    uint32_t bin_red_added = 0;
    uint32_t bin_irred_removed = 0;
    for(auto& ws: watches) {
        uint32_t j = 0;
        for(uint32_t i = 0; i < ws.size(); i++) {
            if (ws[i].isBNN()) {
                ws[j++] = ws[i];
                continue;
            } else if (ws[i].isBin()) {
                if (!ws[i].bin_cl_marked()) {
                    ws[j++] = ws[i];
                    continue;
                }
                bin_irred_removed++;
                if (conf.oracle_removed_is_learnt) {
                    ws[i].unmark_bin_cl();
                    ws[i].setReallyRed();
                    bin_red_added++;
                    ws[j++] = ws[i];
                }
                continue;
            } else if (ws[i].isClause()) {
                Clause* cl = cl_alloc.ptr(ws[i].get_offset());
                if (conf.oracle_removed_is_learnt || !cl->stats.marked_clause) ws[j++] = ws[i];
                continue;
            }
        }
        ws.shrink(ws.size()-j);
    }
    binTri.redBins+=bin_red_added/2;
    binTri.irredBins-=bin_irred_removed/2;

    uint32_t j = 0;
    for(uint32_t i = 0; i < longIrredCls.size(); i++) {
        ClOffset off = longIrredCls[i];
        Clause* cl = cl_alloc.ptr(off);
        if (!cl->stats.marked_clause) {
            longIrredCls[j++] = longIrredCls[i];
        } else {
            litStats.irredLits -= cl->size();
            if (conf.oracle_removed_is_learnt) {
                cl->stats.marked_clause = false;
                litStats.redLits += cl->size();
                longRedCls[2].push_back(off);
                cl->stats.which_red_array = 2;
                cl->isRed = true;
            } else {
                cl_alloc.clauseFree(off);
            }
        }
    }
    longIrredCls.resize(j);

    //cout << "New cls size: " << clauses.size() << endl;
    //Subsume();

    verb_print(1, "[oracle-sparsify] removed: " << removed
        << " of which bin: " << removed_bin
        << " tot considered: " << tot_cls
        << " cache-used: " << oracle.getStats().cache_useful
        << " cache-added: " << oracle.getStats().cache_added
        << " learnt-units: " << oracle.getStats().learned_units
        << " T: " << (cpuTime()-my_time) << " buildT: " << build_time);

    return solver->okay();
}
