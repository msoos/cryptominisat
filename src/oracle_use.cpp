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

#include "ccnr_cms.h"
#include "ccnr_oracle_pre.h"
#include "constants.h"
#include "frat.h"
#include "oracle/utils.h"
#include "solver.h"
#include "oracle/oracle.h"
#include "solvertypes.h"
#include "subsumeimplicit.h"
#include "distillerlongwithimpl.h"
#include "occsimplifier.h"
#include "time_mem.h"
#include "varreplacer.h"
#include "distillerbin.h"

using namespace CMSat;

namespace CMSat {
    struct VarPair {
        uint32_t v1;
        uint32_t v2;
        uint32_t score;
    };
}

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

vector<vector<int>> Solver::get_irred_cls_for_oracle() const {
    vector<vector<int>> clauses;
    vector<int> tmp;
    for (const auto& off: longIrredCls) {
        const Clause& cl = *cl_alloc.ptr(off);
        tmp.clear();
        for (auto const& l: cl) tmp.push_back(orclit(l));
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

bool Solver::oracle_vivif(int fast, bool& backbone_found) {
    using sspp::PosLit;
    using sspp::NegLit;
    using sspp::oracle::TriState;

    assert(!frat->enabled());
    assert(solver->okay());
    execute_inprocess_strategy(false, "must-renumber");
    if (!okay()) return okay();
    if (nVars() < 10) return okay();
    double start_vivif_time = cpuTime();

    auto clauses = get_irred_cls_for_oracle();
    std::shuffle(clauses.begin(), clauses.end(), mtrand);
    for(auto& cl: clauses) std::shuffle(cl.begin(), cl.end(), mtrand);
    detach_and_free_all_irred_cls();

    sspp::oracle::Oracle oracle(nVars(), clauses, {});
    oracle.SetVerbosity(conf.verbosity);

    int64_t tot_vivif_mems = solver->conf.global_timeout_multiplier*533LL*1000LL*1000LL;
    if (fast > 0) tot_vivif_mems /= (3*fast);
    bool early_aborted_vivif = true;
    uint32_t bin_added = 0;
    uint32_t equiv_added = 0;
    for (int i = 0; i < (int)clauses.size(); i++) {
        if (backbone_found && clauses[i].size() == 2) {
            // Backbone has been found, this will never be shorter
            continue;
        }
        for (int j = 0; j < (int)clauses[i].size(); j++) {
            if (oracle.getStats().mems > tot_vivif_mems) goto end1;
            auto assump = negate(clauses[i]);
            swapdel(assump, j);
            int64_t mems2 =  500LL*1000LL*1000LL;
            if (fast > 0) mems2 /= (3*fast);
            auto ret = oracle.Solve(assump, true,mems2);
            if (ret.isUnknown()) goto end1;
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
            }
        }
    }
    early_aborted_vivif = false;
    backbone_found = true;

    // Do equiv check
    end1:
    const auto oracle_vivif_mems_used = oracle.getStats().mems;
    const double end_vivif_time = cpuTime();
    const auto tot_bin_mems = (int64_t)conf.oracle_find_bins*solver->conf.global_timeout_multiplier*7LL*1000LL*1000LL;
    bool early_aborted_bin = true;
    oracle.reset_mems();
    double start_bin_time = cpuTime();
    if (conf.oracle_find_bins && nVars() < 10ULL*1000ULL) {
        vector<vector<uint32_t>> pg(nVars());
        for (uint32_t v = 0; v < nVars(); v++) pg[v].resize(nVars(), 0);
        for (const auto& clause : clauses) {
            for (auto l1 : clause) {
                for (auto l2 : clause) {
                    uint32_t v1 = orc_to_lit(l1).var();
                    uint32_t v2 = orc_to_lit(l2).var();
                    if (v1 < v2) pg[v1][v2]++;
                }
            }
        }
        vector<VarPair> varp;
        for (uint32_t v1 = 0; v1 < nVars(); v1++)
            for (uint32_t v2 = 0; v2 < nVars(); v2++)
                if (v1 < v2 && pg[v1][v2] > 0) varp.push_back({v1, v2, pg[v1][v2]});
        pg.clear();
        pg.shrink_to_fit();

        // Actually seems to slow it down. Strange. TODO
        /* std::sort(varp.begin(), varp.end(), [](const VarPair& a, const VarPair& b) { */
        /*         return a.score > b.score;}); */
        verb_print(1, "[oracle-bin] potential pairs: " << varp.size());

        auto mem = solver->conf.global_timeout_multiplier*333LL*1000LL;
        for (const auto& vp: varp) {
            if (varData[vp.v1].removed != Removed::none) continue;
            if (varData[vp.v2].removed != Removed::none) continue;
            if (value(vp.v1) != l_Undef) continue;
            if (value(vp.v2) != l_Undef) continue;
            if (oracle.getStats().mems > tot_bin_mems) goto end2;

            TriState ret;
            Clause* cl;
            const Lit l1 = Lit(vp.v1, false);
            const Lit l2 = Lit(vp.v2, false);
            ret = oracle.Solve({orclit(l1), orclit(l2)}, true, mem);
            if (ret.isUnknown()) goto end2;
            if (ret.isTrue()) goto next;
            cl = add_clause_int({~l1, ~l2}, true);
            assert(!cl);
            if (!okay()) return false;
            bin_added++;

            ret = oracle.Solve({orclit(~l1), orclit(~l2)}, true, mem);
            if (ret.isUnknown()) goto end2;
            if (ret.isTrue()) goto next;
            assert(ret.isFalse() && ret.isFalse());
            cl = add_clause_int({l1, l2}, true);
            assert(!cl);
            if (!okay()) return false;
            bin_added++;
            equiv_added++;
            continue;

            next:
            ret = oracle.Solve({orclit(~l1), orclit(l2)}, true, mem);
            if (ret.isUnknown()) goto end2;
            if (ret.isTrue()) continue;
            cl = add_clause_int({l1, ~l2}, true);
            assert(!cl);
            if (!okay()) return false;
            bin_added++;

            ret = oracle.Solve({orclit(l1), orclit(~l2)}, true, mem);
            if (ret.isUnknown()) goto end2;
            if (ret.isTrue()) continue;
            cl = add_clause_int({~l1, l2}, true);
            assert(!cl);
            if (!okay()) return false;
            bin_added++;
            equiv_added++;
        }
    }
    early_aborted_bin = false;

    end2:
    const double end_bin_tme = cpuTime();
    const auto oracle_bin_mems_used = oracle.getStats().mems;

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
            s.id = ++clauseID;
            s.glue = cl.size();
            Clause* cl2 = solver->add_clause_int(tmp2, true, &s);
            if (cl2) longRedCls[2].push_back(cl_alloc.get_offset(cl2));
            if (!okay()) return false;
        }
    }
    execute_inprocess_strategy(false, "must-scc-vrepl");
    if (!okay()) return okay();

    verb_print(1, "[oracle-vivif]"
            << " learnt-units: " << oracle.getStats().learned_units
            << " T-out: " << (early_aborted_vivif ? "Y" : "N")
            << " T-remain: " << stats_line_percent(tot_vivif_mems-oracle_vivif_mems_used, tot_vivif_mems) << "%"
            << " T: " << std::setprecision(2) << (end_vivif_time-start_vivif_time));

    verb_print(1, "[oracle-bin]"
            << " bin-added: " << bin_added
            << " equiv-added: " << equiv_added
            << " T-out: " << (early_aborted_bin ? "Y" : "N")
            << " T-remain: " << stats_line_percent(tot_bin_mems-oracle_bin_mems_used, tot_bin_mems) << "%"
            << " T: " << std::setprecision(2) << (end_bin_tme - start_bin_time));

    verb_print(1, "[oracle-vivif-bin]"
            << " cache-used: " << oracle.getStats().cache_useful
            << " cache-added: " << oracle.getStats().cache_added
            << " total T: " << std::setprecision(2) << (cpuTime() - start_vivif_time));
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
                cs.push_back(OracleDat(ww, OracleBin(l, ws.lit2(), ws.get_id())));
            }
        }
    }
    std::sort(cs.begin(), cs.end());
    /* print_cs_ordering(cs); */
    return cs;
}

bool Solver::oracle_sparsify(bool fast)
{
    assert(!frat->enabled());
    execute_inprocess_strategy(false, "sub-impl, occ-backw-sub, must-renumber");
    if (!okay()) return okay() ;
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
    vector<vector<sspp::Lit>> cls;
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
        cls.push_back(tmp);
    }
    vector<int8_t> assumps_map(nVars()+tot_cls+1, 2);
    CCNROraclePre ccnr(solver);
    ccnr.init(cls, nVars()+tot_cls, &assumps_map);
    const double build_time = cpuTime() - my_time;

    // Set all assumptions to FALSE, i.e. all clauses are active
    vector<int> assumps_changed;
    for (uint32_t i = 0; i < tot_cls; i++) {
        auto l = orclit(Lit(nVars()+i, true));
        oracle.SetAssumpLit(l, false);
        assumps_map[sspp::VarOf(l)] = sspp::IsPos(l);
        assumps_changed.push_back(sspp::VarOf(l));
    }

    // Now try to remove clauses one-by-one
    uint32_t last_printed = 0;
    uint32_t ccnr_useful = 0;
    uint32_t unknown = 0;
    int64_t mems = solver->conf.global_timeout_multiplier*100LL*1000LL*1000LL;
    if (fast) mems /= 3;
    int64_t mems2 = solver->conf.global_timeout_multiplier*333LL*1000LL*1000LL;
    if (fast) mems2 /= 3;
    sspp::oracle::TriState ret;
    for (uint32_t i = 0; i < tot_cls; i++) {
        if ((10*i)/(tot_cls) != last_printed) {
            verb_print(1, "[oracle-sparsify] done with " << ((10*i)/(tot_cls))*10 << " %"
                << " oracle mems: " << print_value_kilo_mega(oracle.getStats().mems)
                << " ccnr useful: " << (double)ccnr_useful/(double)i*100.0 << "%"
                << " T: " << (cpuTime()-my_time));
            last_printed = (10*i)/(tot_cls);
        }

        // Try removing this clause, making its indicator TRUE (i.e. removed)
        {
            auto l = orclit(Lit(nVars()+i, false));
            oracle.SetAssumpLit(l, false);
            assumps_map[sspp::VarOf(l)] = sspp::IsPos(l);
            assumps_changed.push_back(sspp::VarOf(l));
        }

        tmp.clear();
        const auto& c = cs[i];
        if (!c.binary) {
            Clause& cl = *cl_alloc.ptr(c.off);
            for(auto const& l: cl) tmp.push_back(orclit(~l));
        } else {
            tmp.push_back(orclit(~(c.bin.l1)));
            tmp.push_back(orclit(~(c.bin.l2)));
        }

        for(const auto& l: tmp) {
            assumps_map[sspp::VarOf(l)] = sspp::IsPos(l);
            assumps_changed.push_back(sspp::VarOf(l));
        }
        ccnr.adjust_assumps(assumps_changed);
        assumps_changed.clear();
        int ret_ccnr = ccnr.run(6000);
        /* int ret_ccnr = false; */
        for(const auto& l: tmp) {
            assumps_map[sspp::VarOf(l)] = 2;
            assumps_changed.push_back(sspp::VarOf(l));
        }
        if (ret_ccnr) {
            verb_print(3, "[oracle-sparsify] ccnr-oracle determined SAT");
            ccnr_useful++;
            goto need;
        } else {
            verb_print(3, "[oracle-sparsify] ccnr-oracle UNKNOWN");
        }

        ret = oracle.Solve(tmp, false, mems);
        if (ret.isUnknown()) {
            /*out of time*/
            unknown++;
            goto need;
        }

        if (ret.isTrue()) {
            need:
            // We need this clause, can't remove
            auto l = orclit(Lit(nVars()+i, true));
            oracle.SetAssumpLit(l, true);
            /* assumps_map[sspp::VarOf(l)] = sspp::IsPos(l); */
            /* assumps_changed.push_back(sspp::VarOf(l)); */
        } else {
            assert(ret.isFalse());
            // We can freeze(!) this clause to be disabled.
            auto l = orclit(Lit(nVars()+i, false));
            oracle.SetAssumpLit(l, true);
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

        if (oracle.getStats().mems > mems2) {
            verb_print(1, "[oracle-sparsify] too many mems in oracle, aborting");
            goto fin;
        }
    }

    fin:
    if (fast) conf.oracle_removed_is_learnt = true;
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
        << " ccnr useful: " << ccnr_useful
        << " oracle uknown: " << unknown
        << " cache-used: " << oracle.getStats().cache_useful
        << " cache-added: " << oracle.getStats().cache_added
        << " learnt-units: " << oracle.getStats().learned_units
        << " T: " << (cpuTime()-my_time) << " buildT: " << build_time);

    return solver->okay();
}
