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

#include <cstdint>
#include <fstream>
#include <cmath>
#include <fcntl.h>
#include <functional>
#include <limits>
#include <string>
#include <algorithm>
#include <vector>
#include <complex>
#include <locale>
#include <random>
#include <unordered_map>
#include "constants.h"
#include "solvertypes.h"
#include "solvertypesmini.h"

#include "varreplacer.h"
#include "time_mem.h"
#include "searcher.h"
#include "occsimplifier.h"
#include "distillerlong.h"
#include "distillerbin.h"
#include "distillerlitrem.h"
#include "clausecleaner.h"
#include "solutionextender.h"
#include "varupdatehelper.h"
#include "completedetachreattacher.h"
#include "subsumestrengthen.h"
#include "watchalgos.h"
#include "clauseallocator.h"
#include "subsumeimplicit.h"
#include "distillerlongwithimpl.h"
#include "str_impl_w_impl.h"
#include "datasync.h"
#include "reducedb.h"
#include "sccfinder.h"
#include "intree.h"
#include "satzilla_features_calc.h"
#include "GitSHA1.h"
#include "trim.h"
#include "streambuffer.h"
#include "gaussian.h"
#include "sqlstats.h"
#include "frat.h"
#include "idrup.h"
#include "xorfinder.h"
#include "cardfinder.h"
#include "sls.h"
#include "matrixfinder.h"
#include "lucky.h"
#include "get_clause_query.h"
#include "community_finder.h"
extern "C" {
#include "mpicosat/mpicosat.h"
}
#include "cryptominisat.h"

#ifdef USE_BREAKID
#include "cms_breakid.h"
#endif

#ifdef USE_BOSPHORUS
#include "cms_bosphorus.h"
#endif

using namespace CMSat;
using std::cout;
using std::endl;

#ifdef USE_SQLITE3
#include "sqlitestats.h"
#endif

//#define DEBUG_RENUMBER
//#define DEBUG_IMPLICIT_PAIRS_TRIPLETS

Solver::Solver(const SolverConf *_conf, std::atomic<bool>* _must_interrupt_inter) :
    Searcher(_conf, this, _must_interrupt_inter)
{
    sqlStats = nullptr;
    intree = new InTree(this);

#ifdef USE_BREAKID
    if (conf.doBreakid) breakid = new BreakID(this);
#endif

    if (conf.perform_occur_based_simp) {
        occsimplifier = new OccSimplifier(this);
    }
    if (conf.doFindCard) {
        card_finder = new CardFinder(this);
    }
    distill_long_cls = new DistillerLong(this);
    distill_bin_cls = new DistillerBin(this);
    distill_lit_rem = new DistillerLitRem(this);
    dist_long_with_impl = new DistillerLongWithImpl(this);
    dist_impl_with_impl = new StrImplWImpl(this);
    clauseCleaner = new ClauseCleaner(this);
    varReplacer = new VarReplacer(this);
    if (conf.doStrSubImplicit) {
        subsumeImplicit = new SubsumeImplicit(this);
    }
    datasync = new DataSync(this, nullptr);
    Searcher::solver = this;
    reduceDB = new ReduceDB(this);

    set_up_sql_writer();
    next_lev1_reduce = conf.every_lev1_reduce;
    next_lev2_reduce =  conf.every_lev2_reduce;
    #if defined(STATS_NEEDED) || defined(FINAL_PREDICTOR)
    next_pred_reduce =  conf.every_pred_reduce;
    #endif

    check_xor_cut_config_sanity();
}

Solver::~Solver()
{
    delete sqlStats;
    delete intree;
    delete occsimplifier;
    delete distill_long_cls;
    delete distill_lit_rem;
    delete distill_bin_cls;
    delete dist_long_with_impl;
    delete dist_impl_with_impl;
    delete clauseCleaner;
    delete varReplacer;
    delete subsumeImplicit;
    delete datasync;
    delete reduceDB;
#ifdef USE_BREAKID
    delete breakid;
#endif
    delete card_finder;
}

void Solver::set_sqlite(
    [[maybe_unused]] const string filename
) {
    #ifdef USE_SQLITE3
    sqlStats = new SQLiteStats(filename);
    if (!sqlStats->setup(this)) exit(-1);
    if (conf.verbosity >= 4) {
        cout << "c Connected to SQLite server" << endl;
    }
    if (frat->enabled()) frat->set_sqlstats_ptr(sqlStats);
    #else
    std::cerr << "SQLite support was not compiled in, cannot use it. Exiting."
    << endl;
    std::exit(-1);
    #endif
}

void Solver::set_shared_data(SharedData* shared_data) { datasync->set_shared_data(shared_data); }

// Only used for unsat, unit, and binary xors during initalization
void Solver::add_clause_int_frat(const vector<Lit>& cl, const uint32_t id) {
    assert(cl.size() <= 2);
    ClauseStats s;
    s.ID = id;
    Clause* c = solver->add_clause_int(
            cl,
            false, //red
            &s,
            true, // attach long
            nullptr, //finalLits
            true, //add_frat
            lit_Undef, //frat_first
            false, //sorted
            true); // remove_frat -- this is what we need
    assert(c == nullptr && "Only used for unsat, unit, and binary xors");
}

bool Solver::add_xor_clause_inter(
    const vector<Lit>& lits
    , bool rhs
    , const bool attach
    , const int32_t XID
) {
    frat_func_start_raw();
    VERBOSE_PRINT("add_xor_clause_inter: " << lits << " rhs: " << rhs);
    assert(okay());
    assert(!attach || qhead == trail.size());
    assert(decisionLevel() == 0);

    auto ps = lits;
    auto XID2 = clean_xor_vars_no_prop(ps, rhs, XID);
    if (ps.size() >= (0x01UL << 28)) throw CMSat::TooLongClauseError();

    if (ps.empty()) {
        if (rhs) {
            *frat << implyclfromx << ++clauseID << fratchain << XID2 << fin;
            set_unsat_cl_id(clauseID);
            ok = false;
        } else assert(XID2 == 0); // we return 0 otherwise from clean_xor_vars_no_prop
        return okay();
    } else if (ps.size() == 1) {
        ps[0] ^= !rhs;
        const auto ID = ++clauseID;
        *frat << implyclfromx << ID << ps << fratchain << XID2 << fin;
        *frat << delx << XID2 << fin;
        add_clause_int_frat(ps, ID);
    } else if (ps.size() == 2) {
        ps[0] ^= !rhs;
        const auto ID1 = ++clauseID;
        *frat << implyclfromx << ID1 << ps << fratchain << XID2 << fin;
        add_clause_int_frat(ps, ID1);
        ps[0] ^= true; ps[1] ^= true;
        const auto ID2 = ++clauseID;
        *frat << implyclfromx << ID2 << ps << fratchain << XID2 << fin;
        add_clause_int_frat(ps, ID2);
        ps[0] ^= true; ps[1] ^= true;
        *frat << delx << XID2 << fin;
    } else {
        assert(ps.size() > 2);
        xorclauses_updated = true;
        xorclauses.emplace_back(ps, rhs);
        Xor& x = xorclauses.back();
        x.XID = XID2;
        attach_xor_clause(xorclauses.size()-1);
    }
    frat_func_end_raw();
    return okay();
}

//Deals with INTERNAL variables
bool Solver::sort_and_clean_clause(
    vector<Lit>& ps
    , const vector<Lit>& origCl
    , const bool red
    , const bool sorted
) {
    if (!sorted) {
        std::sort(ps.begin(), ps.end());
    }
    Lit p = lit_Undef;
    uint32_t i, j;
    for (i = j = 0; i != ps.size(); i++) {
        if (value(ps[i]) == l_True) {
            return false;
        } else if (ps[i] == ~p) {
            if (!red) {
                uint32_t var = p.var();
                var = map_inter_to_outer(var);
                if (undef_must_set_vars.size() < var+1) {
                    undef_must_set_vars.resize(var+1, false);
                }
                undef_must_set_vars[var] = true;
            }
            return false;
        } else if (value(ps[i]) != l_False && ps[i] != p) {
            ps[j++] = p = ps[i];

            if (varData[p.var()].removed != Removed::none) {
                cout << "ERROR: clause " << origCl << " contains literal "
                << p << " whose variable has been removed (removal type: "
                << removed_type_to_string(varData[p.var()].removed)
                << " var-updated lit: "
                << varReplacer->get_var_replaced_with(p)
                << ")"
                << endl;

                //Variables that have been eliminated cannot be added internally
                //as part of a clause. That's a bug
                assert(varData[p.var()].removed == Removed::none);
            }
        }
    }
    ps.resize(ps.size() - (i - j));
    return true;
}

/**
@brief Adds a clause to the problem. MUST only be called internally

This code is very specific in that it must NOT be called with variables in
"ps" that have been replaced, eliminated, etc. Also, it must not be called
when the wer are in an UNSAT (!ok) state, for example. Use it carefully,
and only internally

Deals with INTERNAL variables
*/
Clause* Solver::add_clause_int(
    const vector<Lit>& lits
    , const bool red
    , const ClauseStats* const cl_stats
    , const bool attach_long
    , vector<Lit>* finalLits
    , bool add_frat
    , const Lit frat_first
    , const bool sorted
    , const bool remove_frat
) {
    assert(okay());
    assert(decisionLevel() == 0);
    assert(!attach_long || qhead == trail.size());
    VERBOSE_PRINT("add_clause_int clause " << lits);

    add_clause_int_tmp_cl = lits;
    vector<Lit>& ps = add_clause_int_tmp_cl;
    if (!sort_and_clean_clause(ps, lits, red, sorted)) {
        if (finalLits) finalLits->clear();
        if (remove_frat) *frat << del << cl_stats->ID << lits << fin;
        return nullptr;
    }
    VERBOSE_PRINT("add_clause_int final clause: " << ps);

    //If caller required final set of lits, return it.
    if (finalLits) *finalLits = ps;

    int32_t ID;
    if (remove_frat) {
        assert(cl_stats);
        assert(frat_first == lit_Undef);
        assert(add_frat);
        ID = cl_stats->ID;
        if (ps != lits) {
            ID = ++clauseID;
            *frat << add << ID << ps << fin;
            *frat << del << cl_stats->ID << lits << fin;
        }
    } else {
        ID = ++clauseID;
        if (add_frat) {
            size_t i = 0;
            if (frat_first != lit_Undef) {
                assert(ps.size() > 0);
                if (frat_first != lit_Undef) {
                    for(i = 0; i < ps.size(); i++) {
                        if (ps[i] == frat_first) break;
                    }
                }
                std::swap(ps[0], ps[i]);
            }

            *frat << add << ID << ps << fin;
            if (frat_first != lit_Undef) {
                std::swap(ps[0], ps[i]);
            }
        }
    }

    //Handle special cases
    switch (ps.size()) {
        case 0:
            set_unsat_cl_id(ID);
            ok = false;
            if (conf.verbosity >= 6) {
                cout << "c solver received clause through addClause(): " << lits
                << " that became an empty clause at toplevel --> UNSAT" << endl;
            }
            return nullptr;
        case 1:
            assert(decisionLevel() == 0);
            enqueue<false>(ps[0]);
            *frat << del << ID << ps[0] << fin; // double unit delete
            if (attach_long) ok = (propagate<true>().isnullptr());
            return nullptr;
        case 2:
            attach_bin_clause(ps[0], ps[1], red, ID);
            return nullptr;
        default:
            Clause* c = cl_alloc.Clause_new(ps, sumConflicts, ID);
            c->isRed = red;
            if (cl_stats) {
                c->stats = *cl_stats;
                STATS_DO(if (ID != c->stats.ID && sqlStats && c->stats.is_tracked)
                        sqlStats->update_id(c->stats.ID, ID));
                c->stats.ID = ID;
            }
            if (red && cl_stats == nullptr) {
                assert(false && "does this happen at all? should it happen??");
                #if defined(STATS_NEEDED) || defined(FINAL_PREDICTOR)
                //TODO red_stats_extra setup: glue, size, introduced_at_conflict
                #endif
            }

            //In class 'OccSimplifier' we don't need to attach normall
            if (attach_long) {
                attachClause(*c);
            } else {
                if (red) litStats.redLits += ps.size();
                else litStats.irredLits += ps.size();
            }
            return c;
    }
}

//Deals with INTERNAL variables
// return TRUE if needs to be removed
void Solver::sort_and_clean_bnn(BNN& bnn)
{
    std::sort(bnn.begin(), bnn.end());
    Lit p = lit_Undef;
    uint32_t i, j;
    for (i = j = 0; i < bnn.size(); i++) {
        if (value(bnn[i]) == l_True) {
            bnn.cutoff --;
            continue;
        } else if (value(bnn[i]) == l_False) {
            continue;
        } else if (bnn[i].var() == p.var()
            && bnn[i].sign() == !p.sign()
        ) {
            p = lit_Undef;
            bnn.cutoff--; //either way it's a +1 on the LHS
            j--;
            continue;
        } else {
            bnn[j++] = p = bnn[i];

            if (varData[p.var()].removed != Removed::none) {
                cout << "ERROR: BNN " << bnn << " contains literal "
                << p << " whose variable has been removed (removal type: "
                << removed_type_to_string(varData[p.var()].removed)
                << " var-updated lit: "
                << varReplacer->get_var_replaced_with(p)
                << ")"
                << endl;

                //Variables that have been eliminated cannot be added internally
                //as part of a clause. That's a bug
                assert(varData[p.var()].removed == Removed::none);
            }
        }
    }
    bnn.resize(j);

    if (!bnn.set && value(bnn.out) != l_Undef) {
        if (value(bnn.out) == l_False) {
            for(auto& l: bnn) {
                l = ~l;
            }
            bnn.cutoff = (int)bnn.size()+1-bnn.cutoff;
        }
        bnn.set = true;
        bnn.out = lit_Undef;
    }
}

void Solver::attach_bnn(const uint32_t bnn_idx)
{
    BNN* bnn = bnns[bnn_idx];

//     cout << "Attaching BNN: " << *bnn << endl;

    for(const auto& l: *bnn) {
        watches[l].push(Watched(bnn_idx, WatchType::watch_bnn_t, bnn_pos_t));
        watches[~l].push(Watched(bnn_idx, WatchType::watch_bnn_t, bnn_neg_t));

    }
    if (!bnn->set)  {
        watches[bnn->out].push(Watched(bnn_idx, WatchType::watch_bnn_t, bnn_out_t));
        watches[~bnn->out].push(Watched(bnn_idx, WatchType::watch_bnn_t, bnn_out_t));
    }
}

//Input BNN *must* be already clean
bool Solver::bnn_to_cnf(BNN& bnn)
{
    // It must have already been evaluated
    assert(bnn.set || value(bnn.out) == l_Undef);

    vector<Lit> lits;
    if (bnn.set && bnn.cutoff == 1) {
        assert(bnn.size() > 1);
        lits.clear();
        lits.insert(lits.end(), bnn.begin(), bnn.end());
        Clause* cl = add_clause_int(lits);
        assert(ok);
        if (cl != nullptr) {
            longIrredCls.push_back(cl_alloc.get_offset(cl));
        }
        return true;
    }

    if (!bnn.set && bnn.cutoff == 1) {
        lits.clear();
        lits.insert(lits.end(), bnn.begin(), bnn.end());
        lits.push_back(~bnn.out);
        Clause* cl = add_clause_int(lits);
        if (cl != nullptr) {
            longIrredCls.push_back(cl_alloc.get_offset(cl));
        }
        for(Lit l: bnn) {
            lits.clear();
            lits.push_back(~l);
            lits.push_back(bnn.out);
            Clause* cl2 = add_clause_int(lits);
            assert(cl2 == nullptr);
        }
        return true;
    }

    if (!bnn.set && bnn.cutoff == (int)bnn.size()) {
        lits.clear();
        for(const Lit& l: bnn) {
            lits.push_back(~l);
        }
        lits.push_back(bnn.out);
        Clause* cl = add_clause_int(lits);
        if (cl != nullptr) {
            longIrredCls.push_back(cl_alloc.get_offset(cl));
        }
        for(const Lit& l: bnn) {
            lits.clear();
            lits.push_back(l);
            lits.push_back(~bnn.out);
            Clause* cl2 = add_clause_int(lits);
            assert(cl2 == nullptr);
        }
        return true;
    }

    if (bnn.cutoff == 2 && bnn.size() == 3) {
        //input is a v b v c <-> d
        //creates:
        //a v b v -d
        //a v c v -d
        //b v c v -d
        //----
        //-a v -b v d
        //-a v -c v d
        //-b v -c v d
        //------
        //when bnn.set, we don't need the 2nd part
        ///    (and -d is not in 1st part)

        for(uint32_t rev = 0; rev < 2; rev++) {
            //if it's set, don't do the rev
            if (bnn.set && rev == 1) {
                break;
            }
            for(uint32_t i = 0; i < 3; i++) {
                lits.clear();
                for (uint32_t i2 = 0; i2 < 3; i2++) {
                    if (i != i2) {
                        lits.push_back(bnn[i2] ^ (bool)rev);
                    }
                }
                if (!bnn.set) {
                    lits.push_back(~bnn.out ^ (bool)rev);
                }
                Clause* cl2 = add_clause_int(lits);
                if (cl2 != nullptr)
                    longIrredCls.push_back(cl_alloc.get_offset(cl2));
            }
        }
        return true;
    }


    return false;
}

void Solver::add_bnn_clause_inter(
    vector<Lit>& lits,
    const int32_t cutoff,
    Lit out)
{
    assert(ok);
    uint32_t num_req = sizeof(BNN) + lits.size()*sizeof(Lit);
    void* mem = malloc(num_req);
    BNN* bnn = new (mem) BNN(lits, cutoff, out);

    sort_and_clean_bnn(*bnn);
    bnn->undefs = bnn->size();
    bnn->ts = 0;
    lbool ret = bnn_eval(*bnn);
    if (ret != l_Undef) {
        if (ret == l_False) {
            ok = false;
            free(bnn);
            return;
        }
        free(bnn);
        bnn = nullptr;
    }

    if (bnn != nullptr) {
        assert(check_bnn_sane(*bnn));
        if (bnn_to_cnf(*bnn)) {
            free(bnn);
            bnn = nullptr;
        } else {
            bnns.push_back(bnn);
            attach_bnn(bnns.size()-1);
        }
    }
    ok = propagate<true>().isnullptr();
}

void Solver::attachClause(
    const Clause& cl
    , const bool checkAttach
) {
    //Update stats
    if (cl.red()) litStats.redLits += cl.size();
    else litStats.irredLits += cl.size();

    //Call Solver's function for heavy-lifting
    PropEngine::attachClause(cl, checkAttach);
}

void Solver::attach_bin_clause(
    const Lit lit1
    , const Lit lit2
    , const bool red
    , const int32_t ID
    , [[maybe_unused]] const bool checkUnassignedFirst
) {
    //Update stats
    if (red) binTri.redBins++;
    else binTri.irredBins++;

    //Call Solver's function for heavy-lifting
    PropEngine::attach_bin_clause(lit1, lit2, red, ID, checkUnassignedFirst);
}

void Solver::detachClause(const Clause& cl, const bool remove_frat)
{
    if (remove_frat) *frat << del << cl << fin;
    assert(cl.size() > 2);
    detach_modified_clause(cl[0], cl[1], cl.size(), &cl);
}

void Solver::detachClause(const ClOffset offset, const bool remove_frat)
{
    Clause* cl = cl_alloc.ptr(offset);
    detachClause(*cl, remove_frat);
}

void Solver::detach_modified_clause(
    const Lit lit1
    , const Lit lit2
    , const uint32_t origSize
    , const Clause* address
) {
    if (address->red()) litStats.redLits -= origSize;
    else litStats.irredLits -= origSize;
    PropEngine::detach_modified_clause(lit1, lit2, address);
}

//Takes OUTSIDE variables and makes them INTERNAL, replaces them, etc.
bool Solver::add_clause_helper(vector<Lit>& ps) {
    if (!ok) return false;

    //Sanity checks
    assert(decisionLevel() == 0);
    assert(qhead == trail.size());

    //Check for too long clauses
    if (ps.size() > (0x01UL << 28)) {
        cout << "Too long clause!" << endl;
        throw CMSat::TooLongClauseError();
    }

    for (Lit& lit: ps) {
        //Check for too large variable number
        if (lit.var() >= nVarsOuter()) {
            cout << "ERROR: Variable " << lit.var() + 1
            << " inserted, but max var is " << nVarsOuter() << endl;
            assert(false);
            std::exit(-1);
        }

        //Undo var replacement
        const Lit updated_lit = varReplacer->get_lit_replaced_with_outer(lit);
        if (conf.verbosity >= 12 && lit != updated_lit)
            cout << "EqLit updating outer lit " << lit << " to outer lit " << updated_lit << endl;
        lit = updated_lit;

        //Map outer to inter, and add re-variable if need be
        if (map_outer_to_inter(lit).var() >= nVars()) new_var(false, lit.var(), false);
    }
    renumber_outer_to_inter_lits(ps);

    #ifdef SLOW_DEBUG
    //Check renumberer
    for (const Lit lit: ps) {
        const Lit updated_lit = varReplacer->get_lit_replaced_with(lit);
        assert(lit == updated_lit);
    }
    #endif

    //Uneliminate vars
    if (get_num_vars_elimed() != 0) {
        for (const Lit& lit: ps) {
            if (varData[lit.var()].removed == Removed::elimed && !occsimplifier->uneliminate(lit.var()))
                return false;

            assert(varData[lit.var()].removed == Removed::none);
        }
    }

    #ifdef SLOW_DEBUG
    //Check
    for (Lit& lit: ps) {
        const Lit updated_lit = varReplacer->get_lit_replaced_with(lit);
        assert(lit == updated_lit);
    }
    #endif

    return true;
}

// Takes OUTER (NOT *outside*) variables
// Input is ORIGINAL clause.
bool Solver::add_clause_outer(vector<Lit>& ps, const vector<Lit>& outer_ps, bool red, bool restore)
{
    ClauseStats clstats;
    clstats.ID = ++clauseID;
    if (!restore)
      *frat << "add_clause_outer\n" << origcl << clstats.ID << outer_ps << fin;
    if (red) clstats.which_red_array = 2;

    VERBOSE_PRINT("Adding clause " << ps);
    const size_t origTrailSize = trail.size();

    if (!add_clause_helper(ps)) {
        *frat << del << clstats.ID << ps << fin;
        return false;
    }

    if (frat->incremental()) // import the "inner version with duplicates removed"
      *frat << "learning renumbered\n" << add << clstats.ID << ps << fin;

    std::sort(ps.begin(), ps.end());
    if (red) assert(!frat->enabled() && "Cannot have both FRAT and adding of redundant clauses");
    Clause *cl = add_clause_int(
        ps
        , red //redundant?
        , &clstats
        , true //yes, attach
        , nullptr
        , true //add frat?
        , lit_Undef
        , true //sorted
        , !frat->incremental() //remove old clause from proof if we changed it
    );

    if (frat->incremental()) {// del the "inner version with duplicates removed"
      if (cl) {
	*frat << "learning renumbered clause\n" << add << *cl << fin;
      }
      *frat << "deleting old\n" << del << clstats.ID << ps << fin;
      if (!restore)
	*frat << "deleting old i\n" << del << clstats.ID << outer_ps << fin;
    }

    if (cl != nullptr) {
        ClOffset offset = cl_alloc.get_offset(cl);
        if (!red) longIrredCls.push_back(offset);
        else longRedCls[2].push_back(offset);
    }

    zeroLevAssignsByCNF += trail.size() - origTrailSize;

    return ok;
}

void Solver::test_renumbering() const
{
    //Check if we renumbered the variables in the order such as to make
    //the unknown ones first and the known/eliminated ones second
    bool uninteresting_seen = false;
    bool problem = false;
    for(size_t i = 0; i < nVars(); i++) {
        /* cout << "val[" << i << "]: " << value(i); */
        if (varData[i].removed == Removed::elimed
            || varData[i].removed == Removed::replaced
            || value(i) != l_Undef
        ) {
            uninteresting_seen = true;
            /* cout << " set/removed" << endl; */
        } else {
            /* cout << " non-removed" << endl; */
        }

        if (value(i) == l_Undef
            && varData[i].removed != Removed::elimed
            && varData[i].removed != Removed::replaced
            && uninteresting_seen
        ) {
            problem = true;
        }
    }
    assert(!problem && "We renumbered the variables in the wrong order!");
}

void Solver::renumber_clauses(const vector<uint32_t>& outer_to_inter)
{
    //Clauses' abstractions have to be re-calculated
    for(ClOffset offs: longIrredCls) {
        Clause* cl = cl_alloc.ptr(offs);
        updateLitsMap(*cl, outer_to_inter);
        cl->set_strengthened();
    }

    for(auto& lredcls: longRedCls) {
        for(ClOffset off: lredcls) {
            Clause* cl = cl_alloc.ptr(off);
            updateLitsMap(*cl, outer_to_inter);
            cl->set_strengthened();
        }
    }

    //Clauses' abstractions have to be re-calculated
    for(Xor& x: xorclauses) updateVarsMap(x.vars, outer_to_inter);
    for(auto& bnn: bnns) {
        if (bnn == nullptr) continue;
        assert(!bnn->isRemoved);
        updateLitsMap(*bnn, outer_to_inter);
        if (!bnn->set) bnn->out = getUpdatedLit(bnn->out, outer_to_inter);
    }
}

size_t Solver::calculate_inter_to_outer_and_outer_to_inter(
    vector<uint32_t>& outer_to_inter
    , vector<uint32_t>& inter_to_outer
) {
    static constexpr uint32_t none = std::numeric_limits<uint32_t>::max();
    vector<uint32_t> fin(nVars(), none);
    uint32_t at = 0;
    for(uint32_t b = 0; b < 2; b++)
        for(uint32_t i = 0; i < nVars(); i++) {
            Lit l(i, b);
            for(const auto& ws: watches[l]) {
                if (ws.isBin() && !ws.red()) {
                    if (fin[l.var()] == none) fin[l.var()] = at++;
                }
            }
        }
    for(const auto& off: longIrredCls) {
        Clause& cl = *cl_alloc.ptr(off);
        for(const auto& l: cl) {
            if (fin[l.var()] == none) fin[l.var()] = at++;
        }
    }

    size_t num_effective_vars = 0;
    for(size_t i = 0; i < nVars(); i++) {
        if (value(i) != l_Undef
            || varData[i].removed == Removed::elimed
            || varData[i].removed == Removed::replaced
        ) {
        } else {
            if (fin[i] == none) fin[i] = at++;
            num_effective_vars++;
        }
    }
    assert(at == num_effective_vars);
    for(size_t i = 0; i < nVars(); i++) if (fin[i] == none) fin[i] = at++;


    for(size_t i = 0; i < nVars(); i++) {
        outer_to_inter[i] = fin[i];
        inter_to_outer[fin[i]] = i;
    }

    //Extend to nVarsOuter() --> these are just the identity transformation
    for(size_t i = nVars(); i < nVarsOuter(); i++) {
        outer_to_inter[i] = i;
        inter_to_outer[i] = i;
    }

    return num_effective_vars;
}

double Solver::calc_renumber_saving()
{
    uint32_t num_used = 0;
    for(size_t i = 0; i < nVars(); i++) {
        if (value(i) != l_Undef
            || varData[i].removed == Removed::elimed
            || varData[i].removed == Removed::replaced
        ) {
            continue;
        }
        num_used++;
    }
    double saving = 1.0-(double)num_used/(double)nVars();
    return saving;
}

//Beware. Cannot be called while Searcher is running.
bool Solver::renumber_variables(bool must_renumber)
{
    assert(okay());
    assert(decisionLevel() == 0);
    SLOW_DEBUG_DO(for(const auto& x: xorclauses) for(const auto& v: x) assert(v < nVars()));

    if (nVars() == 0) return okay();
    if (!must_renumber && calc_renumber_saving() < 0.2) return okay();
    if (!clear_gauss_matrices(false)) return false;

    double my_time = cpuTime();
    if (!clauseCleaner->remove_and_clean_all()) return false;

    //outer_to_inter[10] = 0 ---> what was 10 is now 0.
    vector<uint32_t> outer_to_inter(nVarsOuter());
    vector<uint32_t> inter_to_outer(nVarsOuter());

    size_t numEffectiveVars = calculate_inter_to_outer_and_outer_to_inter(outer_to_inter, inter_to_outer);

    //Create temporary outer_to_inter2
    vector<uint32_t> inter_to_outer2(nVarsOuter()*2);
    for(size_t i = 0; i < nVarsOuter(); i++) {
        inter_to_outer2[i*2] = inter_to_outer[i]*2;
        inter_to_outer2[i*2+1] = inter_to_outer[i]*2+1;
    }

    renumber_clauses(outer_to_inter);
    CNF::update_vars(outer_to_inter, inter_to_outer, inter_to_outer2);
    PropEngine::updateVars(outer_to_inter, inter_to_outer);
    Searcher::updateVars(outer_to_inter, inter_to_outer);
    USE_BREAKID_DO(if (breakid) breakid->updateVars(outer_to_inter, inter_to_outer));

    //Update sub-elements' vars
    varReplacer->updateVars(outer_to_inter, inter_to_outer);
    datasync->updateVars(outer_to_inter, inter_to_outer);

    //Tests
    test_renumbering();
    test_reflectivity_of_renumbering();

    //Print results
    const double time_used = cpuTime() - my_time;
    if (conf.verbosity) {
        cout
        << "c [renumber]"
        << conf.print_times(time_used)
        << endl;
    }
    if (sqlStats) {
        sqlStats->time_passed_min(
            solver
            , "renumber"
            , time_used
        );
    }
    if (conf.doSaveMem) save_on_var_memory(numEffectiveVars);

    SLOW_DEBUG_DO(for(const auto& x: xorclauses) for(const auto& v: x.vars) assert(v < nVars()));

    //NOTE order_heap is now wrong, but that's OK, it will be restored from
    //backed up activities and then rebuilt at the start of Searcher
    return okay();
}

void Solver::new_vars(size_t n)
{
    if (n == 0) {
        return;
    }

    Searcher::new_vars(n);
    varReplacer->new_vars(n);

    if (conf.perform_occur_based_simp) {
        occsimplifier->new_vars(n);
    }

    datasync->new_vars(n);
}

void Solver::new_var(
    const bool bva,
    const uint32_t orig_outer,
    const bool insert_varorder)
{
    Searcher::new_var(bva, orig_outer, insert_varorder);

    varReplacer->new_var(orig_outer);

    if (conf.perform_occur_based_simp) {
        occsimplifier->new_var(orig_outer);
    }

    if (orig_outer == numeric_limits<uint32_t>::max()) {
        datasync->new_var(bva);
    }

    //Too expensive
    //test_reflectivity_of_renumbering();
}

void Solver::save_on_var_memory(const uint32_t newNumVars)
{
    //print_mem_stats();

    const double my_time = cpuTime();
    minNumVars = newNumVars;
    Searcher::save_on_var_memory();

    varReplacer->save_on_var_memory();
    if (occsimplifier) {
        occsimplifier->save_on_var_memory();
    }
    datasync->save_on_var_memory();

    const double time_used = cpuTime() - my_time;
    if (sqlStats) {
        sqlStats->time_passed_min(
            this
            , "save var mem"
            , time_used
        );
    }
    //print_mem_stats();
}

void Solver::set_assumptions() {
    SLOW_DEBUG_DO(for(const auto& x: varData) assert(x.assumption == l_Undef));
    conflict.clear();

    vector<Lit> tmp;
    tmp = assumptions;
    add_clause_helper(tmp); // unelimininates, sanity checks
    fill_assumptions_set();
    SLOW_DEBUG_DO(check_assumptions_sanity());
}

void Solver::uneliminate_sampling_set() {
    if (!conf.sampling_vars_set) return;

    vector<Lit> tmp;
    for(const auto& v: conf.sampling_vars) tmp.push_back(Lit(v, false));
    add_clause_helper(tmp);
}

void Solver::add_assumption(const Lit assump)
{
    assert(varData[assump.var()].assumption == l_Undef);
    assert(varData[assump.var()].removed == Removed::none);
    assert(value(assump) == l_Undef);

    Lit outer_lit = map_inter_to_outer(assump);
    assumptions.push_back(outer_lit);
    varData[assump.var()].assumption = assump.sign() ? l_False : l_True;
}

void Solver::check_model_for_assumptions() const {
    for(Lit p: assumptions) {
        assert(p.var() < model.size());

        if (model_value(p) == l_Undef)
            cout << "ERROR, lit " << p << " is in assumptions, but it wasn't set" << endl;
        assert(model_value(p) != l_Undef);

        if (model_value(p) != l_True) {
            cout << "ERROR, lit " << p << " is in assumptions, but it was set to: "
            << model_value(p) << endl;
        }
        assert(model_value(p) == l_True);
    }
}

void Solver::check_recursive_minimization_effectiveness(const lbool status)
{
    const SearchStats& srch_stats = Searcher::get_stats();
    if (status == l_Undef
        && conf.doRecursiveMinim
        && srch_stats.recMinLitRem + srch_stats.litsRedNonMin > 100000
    ) {
        double remPercent = float_div(srch_stats.recMinLitRem, srch_stats.litsRedNonMin)*100.0;
        double costPerGained = float_div(srch_stats.recMinimCost, remPercent);
        if (costPerGained > 200ULL*1000ULL*1000ULL) {
            conf.doRecursiveMinim = false;
            if (conf.verbosity) {
                cout
                << "c recursive minimization too costly: "
                << std::fixed << std::setprecision(0) << (costPerGained/1000.0)
                << "Kcost/(% lits removed) --> disabling"
                << std::setprecision(2)
                << endl;
            }
        } else {
            if (conf.verbosity) {
                cout
                << "c recursive minimization cost OK: "
                << std::fixed << std::setprecision(0) << (costPerGained/1000.0)
                << "Kcost/(% lits removed)"
                << std::setprecision(2)
                << endl;
            }
        }
    }
}

void Solver::check_minimization_effectiveness(const lbool status)
{
    const SearchStats& search_stats = Searcher::get_stats();
    if (status == l_Undef
        && conf.doMinimRedMore
        && search_stats.moreMinimLitsStart > 100000
    ) {
        double remPercent = float_div(
            search_stats.moreMinimLitsStart-search_stats.moreMinimLitsEnd,
            search_stats.moreMinimLitsStart)*100.0;

        //TODO take into account the limit on the number of first literals, too
        if (remPercent < 1.0) {
            conf.doMinimRedMore = false;
            if (conf.verbosity) {
                cout
                << "c more minimization effectiveness low: "
                << std::fixed << std::setprecision(2) << remPercent
                << " % lits removed --> disabling"
                << endl;
            }
        } else if (remPercent > 7.0) {
            more_red_minim_limit_binary_actual = 3*conf.more_red_minim_limit_binary;
            if (conf.verbosity) {
                cout
                << "c more minimization effectiveness good: "
                << std::fixed << std::setprecision(2) << remPercent
                << " % --> increasing limit to 3x"
                << endl;
            }
        } else {
            more_red_minim_limit_binary_actual = conf.more_red_minim_limit_binary;
            if (conf.verbosity) {
                cout
                << "c more minimization effectiveness OK: "
                << std::fixed << std::setprecision(2) << remPercent
                << " % --> setting limit to norm"
                << endl;
            }
        }
    }
}

bool Solver::check_xor_clause_satisfied_model(const Xor& x) const {
    bool good = true;

    bool rhs = false;
    for(const auto& v: x) {
        if (model_value(v) == l_Undef) {
            cout << "ERROR: variable " << v+1 << " in xorclauses: " << x << " is UNDEF!" << endl;
            good = false;
        } else rhs ^= model_value(v) == l_True;
    }
    if (rhs != x.rhs) {
        cout << "ERROR XOR in xorclauses not satisfied: " << x << endl;
        good = false;
    }

    return good;
}

void Solver::extend_solution(const bool only_sampling_solution) {
    DEBUG_IMPLICIT_STATS_DO(check_stats());

    #ifdef SLOW_DEBUG
    //Check that sampling vars are all assigned
    if (conf.sampling_vars_set) {
        for(uint32_t outer_var: conf.sampling_vars) {
            outer_var = varReplacer->get_var_replaced_with_outer(outer_var);
            uint32_t int_var = map_outer_to_inter(outer_var);

            assert(varData[int_var].removed == Removed::none);
            if (int_var < nVars() && varData[int_var].removed == Removed::none)
                assert(model[int_var] != l_Undef);
        }
    }
    #endif

    const double my_time = cpuTime();
    updateArrayRev(model, inter_to_outerMain);

    if (!only_sampling_solution) {
        SolutionExtender extender(this, occsimplifier);
        extender.extend();
    } else varReplacer->extend_model_already_set();

    if (only_sampling_solution && conf.sampling_vars_set) {
        for(uint32_t var: conf.sampling_vars) {
            if (model[var] == l_Undef) {
                cout << "ERROR: variable " << var+1 << " is set as sampling but is unset!" << endl;
                cout << "NOTE: var " << var + 1 << " has removed value: "
                << removed_type_to_string(varData[var].removed)
                << " and is set to " << value(var) << endl;

                if (varData[var].removed == Removed::replaced) {
                    uint32_t v2 = varReplacer->get_var_replaced_with(var);
                    cout << " --> replaced with var " << v2 + 1 << " whose value is: " << value(v2) << endl;
                }
            }
            assert(model[var] != l_Undef);
        }
    }

    check_model_for_assumptions();
    if (sqlStats) sqlStats->time_passed_min( this , "extend solution" , cpuTime()-my_time);
}

void Solver::set_up_sql_writer()
{
    if (!sqlStats) {
        return;
    }

    bool ret = sqlStats->setup(this);
    if (!ret) {
        std::cerr
        << "c ERROR: SQL was required (with option '--sql 2'), but couldn't connect to SQL server." << endl;
        std::exit(-1);
    }
}

void Solver::check_xor_cut_config_sanity() const
{
    if (MAX_XOR_RECOVER_SIZE < 4) {
        std::cerr << "ERROR: MAX_XOR_RECOVER_SIZE  must be at least 4. It's currently: " << MAX_XOR_RECOVER_SIZE << endl;
        exit(-1);
    }
}

void Solver::check_and_upd_config_parameters()
{
    if (conf.max_glue_cutoff_gluehistltlimited > 1000) {
        cout << "ERROR: 'Maximum supported glue size is currently 1000" << endl;
        exit(-1);
    }

    if (conf.shortTermHistorySize <= 0) {
        std::cerr << "ERROR: You MUST give a short term history size (\"--gluehist\")  greater than 0!" << endl;
        exit(-1);
    }

    if ((frat->enabled()))  {
        if (!conf.do_hyperbin_and_transred) {
            if (conf.verbosity) {
                cout
                << "c OTF hyper-bin is needed for BProp in FRAT, turning it back"
                << endl;
            }
            conf.do_hyperbin_and_transred = true;
        }

        #ifdef USE_BREAKID
        if (conf.doBreakid) {
            if (conf.verbosity) {
                cout
                << "c BreakID is not supported with FRAT, turning it off"
                << endl;
            }
            conf.doBreakid = false;
        }
        #endif

        #ifdef USE_BOSPHORUS
        if (conf.do_bosphorus) {
            if (conf.verbosity) {
                cout
                << "c Bosphorus is not supported with FRAT, turning it off"
                << endl;
            }
            conf.do_bosphorus = false;
        }
        #endif
    }

    if (conf.sampling_vars_set) {
        SLOW_DEBUG_DO(for(uint32_t v: conf.sampling_vars) assert(v < nVarsOuter()));
    }

    if (conf.blocking_restart_trail_hist_length == 0) {
        std::cerr << "ERROR: Blocking restart length must be at least 0" << endl;
        exit(-1);
    }

    check_xor_cut_config_sanity();
}

lbool Solver::simplify_problem_outside(const string* strategy)
{
    #ifdef SLOW_DEBUG
    if (ok) {
        assert(check_order_heap_sanity());
        check_implicit_stats();
        check_wrong_attach();
        find_all_attached();
        check_all_clause_attached();
    }
    #endif

    conf.global_timeout_multiplier = conf.orig_global_timeout_multiplier;
    solveStats.num_simplify_this_solve_call = 0;
    set_assumptions();
    uneliminate_sampling_set();

    lbool status = l_Undef;
    if (!ok) {
        status = l_False;
        goto end;
    }
    check_and_upd_config_parameters();
    USE_BREAKID_DO(if (breakid) breakid->start_new_solving());

    //ignore "no simplify" if explicitly called
    if (nVars() > 0 /*&& conf.do_simplify_problem*/) {
        bool backup_sls = conf.doSLS;
        bool backup_breakid = conf.doBreakid;
        conf.doSLS = false;
        conf.doBreakid = false;
        status = simplify_problem(false, strategy ? *strategy : conf.simplify_schedule_nonstartup);
        conf.doSLS = backup_sls;
        conf.doBreakid = backup_breakid;
    }

    end:
    unfill_assumptions_set();
    assumptions.clear();
    conf.conf_needed = true;
    return status;
}

void Solver::reset_for_solving() {
    longest_trail_ever_best = 0;
    longest_trail_ever_inv = 0;
    polarity_strategy_change = 0;
    increasing_phase_size = conf.restart_first;
    set_assumptions();
    uneliminate_sampling_set();
    #ifdef SLOW_DEBUG
    if (ok) {
        assert(check_order_heap_sanity());
        check_implicit_stats();
        /* find_all_attached(); */
        check_all_clause_attached();
        check_no_duplicate_lits_anywhere();
    }
    #endif

    solveStats.num_solve_calls++;
    check_and_upd_config_parameters();

    //Reset parameters
    luby_loop_num = 0;
    conf.global_timeout_multiplier = conf.orig_global_timeout_multiplier;
    solveStats.num_simplify_this_solve_call = 0;
    verb_print(6, __func__ << " called");
}

lbool Solver::solve_with_assumptions(
    const vector<Lit>* _assumptions,
    const bool only_sampling_solution
) {
    if (frat->enabled()) frat->set_sqlstats_ptr(sqlStats);
    copy_assumptions(_assumptions);
    reset_for_solving();

    //Check if adding the clauses caused UNSAT
    lbool status = l_Undef;
    if (!okay()) {
        assert(conflict.empty());
        status = l_False;
        verb_print(6, "Solver status " << status << " on startup of solve()");
        goto end;
    } else {
        SLOW_DEBUG_DO(check_wrong_attach());
        SLOW_DEBUG_DO(check_all_clause_attached());
    }
    assert(prop_at_head());
    assert(okay());
    USE_BREAKID_DO(if (breakid) breakid->start_new_solving());

    //Simplify in case simplify_at_startup is set
    if (status == l_Undef
        && nVars() > 0
        && conf.do_simplify_problem
        && conf.simplify_at_startup
        && (solveStats.num_simplify == 0 || conf.simplify_at_every_startup)
    ) {
        status = simplify_problem(
            !conf.full_simplify_at_startup,
            !conf.full_simplify_at_startup ? conf.simplify_schedule_startup : conf.simplify_schedule_nonstartup);
    }

    #ifdef STATS_NEEDED
    if (status == l_Undef) {
        CommunityFinder comm_finder(this);
        comm_finder.compute();
    }
    #endif

    if (status == l_Undef) status = iterate_until_solved();

    end:
    if (sqlStats) sqlStats->finishup(status);
    handle_found_solution(status, only_sampling_solution);
    unfill_assumptions_set();
    assumptions.clear();
    conf.max_confl = numeric_limits<uint64_t>::max();
    conf.maxTime = numeric_limits<double>::max();
    datasync->finish_up_mpi();
    conf.conf_needed = true;
    set_must_interrupt_asap();
    assert(decisionLevel()== 0);
    assert(!ok || prop_at_head());
    if (_assumptions == nullptr || _assumptions->empty()) {
        #ifdef USE_BREAKID
        if (assumptions.empty()) {
            verb_print(1, "[breakid] Under BreakID it's UNSAT. Assumed lit: " << breakid->get_assumed_lit());
        } else
        #endif
        {
            if (status == l_False)  assert(!okay());
        }
    }

    write_final_frat_clauses();
    conclude_idrup(status);
    return status;
}

void Solver::write_final_frat_clauses() {
    if (!frat->enabled()) return;
    if (frat->incremental()) return;
    assert(decisionLevel() == 0);
    frat_func_start();

    if (varReplacer) varReplacer->delete_frat_cls();

    *frat << "empty clause next (if we found it)\n";
    if (!okay() && unsat_cl_ID != -1) {
        assert(!frat->enabled() || unsat_cl_ID != 0);
        *frat << finalcl << unsat_cl_ID << fin;
    }


    *frat << "gmatrix finalize frat begin\n";
    for(auto& g: gmatrices) g->finalize_frat();

    *frat << "free bdds begin\n";
    for(auto& x: xorclauses) {
        if (x.reason_cl_ID != 0) *frat << finalcl << x.reason_cl_ID << x.reason_cl << fin;
        x.reason_cl_ID = 0;
        if (x.XID != 0) *frat << finalx << x.XID << fin;
        x.XID = 0;
    }

    *frat << "finalization of unit clauses next\n";
    for(uint32_t i = 0; i < nVars(); i ++) {
        if (unit_cl_IDs[i] != 0) {
            assert(unit_cl_XIDs[i] != 0);
            assert(value(i) != l_Undef);
            Lit l = Lit(i, value(i) == l_False);
            *frat << finalcl << unit_cl_IDs[i] << l << fin;
            *frat << finalx << unit_cl_XIDs[i] << l << fin;
        }
    }

    *frat << "finalization of binary clauses next\n";
    for(uint32_t i = 0; i < nVars()*2; i++) {
        Lit l = Lit::toLit(i);
        for(const auto& w: watches[l]) {
            //only do once per binary
            if (w.isBin() && w.lit2() < l) {
                *frat << finalcl << w.get_ID() << l << w.lit2() << fin;
            }
        }
    }

    *frat << "finalization of redundant clauses next\n";
    for(const auto& cls: longRedCls) {
        for(const auto& offs: cls) {
            Clause* cl = cl_alloc.ptr(offs);
            *frat << finalcl << *cl << fin;
        }
    }
    *frat << "finalization of irredundant clauses next\n";
    for(const auto& offs: longIrredCls) {
        Clause* cl = cl_alloc.ptr(offs);
        *frat << finalcl << *cl << fin;
    }
    frat_func_end();
}

void Solver::dump_memory_stats_to_sql()
{
    if (!sqlStats) {
        return;
    }

    const double my_time = cpuTime();

    sqlStats->mem_used(
        this
        , "solver"
        , my_time
        , mem_used()/(1024*1024)
    );

    sqlStats->mem_used(
        this
        , "vardata"
        , my_time
        , mem_used_vardata()/(1024*1024)
    );

    sqlStats->mem_used(
        this
        , "longclauses"
        , my_time
        , CNF::mem_used_longclauses()/(1024*1024)
    );

    sqlStats->mem_used(
        this
        , "watch-alloc"
        , my_time
        , watches.mem_used_alloc()/(1024*1024)
    );

    sqlStats->mem_used(
        this
        , "watch-array"
        , my_time
        , watches.mem_used_array()/(1024*1024)
    );

    sqlStats->mem_used(
        this
        , "renumber"
        , my_time
        , CNF::mem_used_renumberer()/(1024*1024)
    );

    if (occsimplifier) {
        sqlStats->mem_used(
            this
            , "occsimplifier"
            , my_time
            , occsimplifier->mem_used()/(1024*1024)
        );
    }

    sqlStats->mem_used(
        this
        , "varreplacer"
        , my_time
        , varReplacer->mem_used()/(1024*1024)
    );

    double vm_mem_used = 0;
    const uint64_t rss_mem_used = memUsedTotal(vm_mem_used);
    sqlStats->mem_used(
        this
        , "rss"
        , my_time
        , rss_mem_used/(1024*1024)
    );
    sqlStats->mem_used(
        this
        , "vm"
        , my_time
        , vm_mem_used/(1024*1024)
    );
}

uint64_t Solver::calc_num_confl_to_do_this_iter(const size_t iteration_num) const
{
    double iter_num = std::min<size_t>(iteration_num, 100ULL);
    double mult = std::pow(conf.num_conflicts_of_search_inc, iter_num);
    mult = std::min(mult, conf.num_conflicts_of_search_inc_max);
    uint64_t num_conflicts_of_search = (double)conf.num_conflicts_of_search*mult;
    if (conf.never_stop_search) {
        num_conflicts_of_search = 600ULL*1000ULL*1000ULL;
    }
    if (conf.max_confl >= sumConflicts) {
        num_conflicts_of_search = std::min<uint64_t>(
            num_conflicts_of_search
            , conf.max_confl - sumConflicts
        );
    } else {
        num_conflicts_of_search = 0;
    }

    return num_conflicts_of_search;
}


lbool Solver::iterate_until_solved() {
    lbool status = l_Undef;
    size_t iteration_num = 0;

    while (status == l_Undef
        && !must_interrupt_asap()
        && cpuTime() < conf.maxTime
        && sumConflicts < conf.max_confl
    ) {
        iteration_num++;
        if (conf.verbosity >= 2) print_clause_size_distrib();
        dump_memory_stats_to_sql();

        const uint64_t num_confl = calc_num_confl_to_do_this_iter(iteration_num);
        if (num_confl == 0) break;
        if (!find_and_init_all_matrices()) {
            status = l_False;
            goto end;
        }
        status = Searcher::solve(num_confl);

        //Check for effectiveness
        check_recursive_minimization_effectiveness(status);
        check_minimization_effectiveness(status);

        //Update stats
        sumSearchStats += Searcher::get_stats();
        sumPropStats += propStats;
        propStats.clear();
        Searcher::resetStats();
        check_too_many_in_tier0();

        //Solution has been found
        if (status != l_Undef) break;

        //If we are over the limit, exit
        if (sumConflicts >= conf.max_confl
            || cpuTime() > conf.maxTime
            || must_interrupt_asap()
        ) break;

        if (conf.do_simplify_problem) {
            status = simplify_problem(false, conf.simplify_schedule_nonstartup);
        }
    }

    #ifdef STATS_NEEDED
    //To record clauses when we finish up
    if (status != l_Undef) {
        dump_clauses_at_finishup_as_last();
        if (conf.verbosity) {
            cout << "c [sql] dumping all remaining clauses as cl_last_in_solver" << endl;
        }
    }
    #endif

    end:
    return status;
}

void Solver::check_too_many_in_tier0()
{
    //For both of these, it makes no sense:
    // * for STATS_NEEDED, we have many in Tier0 because of locking-in
    // * for FINAL_PREDICT Tier0 works completely differently
    #if defined(STATS_NEEDED) || defined(FINAL_PREDICTOR)
    return;
    #endif

    if (conf.glue_put_lev0_if_below_or_eq == 2
        || sumConflicts < conf.min_num_confl_adjust_glue_cutoff
        || adjusted_glue_cutoff_if_too_many
        || conf.adjust_glue_if_too_many_tier0 >= 1.0
    ) {
        return;
    }

    double perc = float_div(sumSearchStats.red_cl_in_which0, sumConflicts);
    if (perc > conf.adjust_glue_if_too_many_tier0) {
        conf.glue_put_lev0_if_below_or_eq--;
        adjusted_glue_cutoff_if_too_many = true;
        if (conf.verbosity) {
            cout << "c Adjusted glue cutoff to " << conf.glue_put_lev0_if_below_or_eq
            << " due to too many low glues: " << perc*100.0 << " %" << endl;
        }
    }
}

void Solver::handle_found_solution(const lbool status, const bool only_sampling_solution) {
    double mytime = cpuTime();
    if (status == l_True) {
        extend_solution(only_sampling_solution);
        cancelUntil(0);
        assert(prop_at_head());

        DEBUG_ATTACH_MORE_DO(find_all_attached());
        DEBUG_ATTACH_MORE_DO(check_all_clause_attached());
    } else if (status == l_False) {
        cancelUntil(0);
        for(const Lit lit: conflict) {
            if (value(lit) == l_Undef) assert(var_inside_assumptions(lit.var()) != l_Undef);
        }
        if (conf.conf_needed) update_assump_conflict_to_orig_outer(conflict);
    }

    USE_BREAKID_DO( if (breakid) breakid->finished_solving());
    DEBUG_IMPLICIT_STATS_DO(check_implicit_stats());
    if (sqlStats) sqlStats->time_passed_min(this, "solution extend", cpuTime() - mytime);
}

lbool Solver::execute_inprocess_strategy(
    const bool startup
    , const string& strategy
) {
    std::istringstream ss(strategy + ", ");
    std::string token;
    std::string occ_strategy_tokens;

    while(std::getline(ss, token, ',')) {
        if (sumConflicts >= conf.max_confl
            || cpuTime() > conf.maxTime
            || must_interrupt_asap()
            || nVars() == 0
            || !okay()
        ) break;

        assert(watches.get_smudged_list().empty());
        assert(prop_at_head());
        assert(okay());
        #ifdef SLOW_DEBUG
        check_no_zero_ID_bins();
        check_wrong_attach();
        check_all_clause_attached();
        check_stats();
        check_no_duplicate_lits_anywhere();
        check_assumptions_sanity();
        #endif

        token = trim(token);
        std::transform(token.begin(), token.end(), token.begin(), ::tolower);
        if (!occ_strategy_tokens.empty() && token.substr(0,3) != "occ") {
            if (conf.perform_occur_based_simp && bnns.empty() && occsimplifier) {
                occ_strategy_tokens = trim(occ_strategy_tokens);
                verb_print(1, "Executing OCC strategy token(s): '" << occ_strategy_tokens);
                occsimplifier->simplify(startup, occ_strategy_tokens);
            }
            occ_strategy_tokens.clear();
            if (sumConflicts >= conf.max_confl || cpuTime() > conf.maxTime
                || must_interrupt_asap() || nVars() == 0 || !ok) {
                break;
            }
            SLOW_DEBUG_DO(check_stats());
            SLOW_DEBUG_DO(check_assumptions_sanity());
        }
        if (okay()) SLOW_DEBUG_DO(check_wrong_attach());

        if (token.substr(0,3) != "occ" && !token.empty())
            verb_print(1, "--> Executing strategy token: " << token);

        if (token == "scc-vrepl") {
            if (conf.doFindAndReplaceEqLits) {
                varReplacer->replace_if_enough_is_found(
                    std::floor((double)get_num_free_vars()*0.001));
            }
        } else if (token == "oracle-vivif-sparsify") {
            bool finished = false;
            if (nVars() > 10 && oracle_vivif(finished)) {
                if (finished) oracle_sparsify();
            }
        } else if (token == "oracle-vivif") {
            bool finished = false;
            if (nVars() > 10) oracle_vivif(finished);
        } else if (token == "oracle-sparsify") {
            bool finished = false;
            backbone_simpl(30LL*1000LL, finished);
            if (nVars() > 10) { if (finished) oracle_sparsify();
            }
        } else if (token == "backbone") {
            bool finished = false;
            backbone_simpl(30LL*1000LL, finished);
        } else if (token == "must-scc-vrepl") {
            if (conf.doFindAndReplaceEqLits) {
                varReplacer->replace_if_enough_is_found();
            }
        } else if (token == "full-probe") {
            if (!full_probe(false)) return l_False;
        } else if (token == "card-find") {
            if (conf.doFindCard) {
                card_finder->find_cards();
                exit(0);
            }
        } else if (token == "sub-impl") {
            //subsume BIN with BIN
            if (conf.doStrSubImplicit) {
                subsumeImplicit->subsume_implicit();
            }
        } else if (token == "sls") {
            assert(false && "unsupported");
        } else if (token == "lucky") {
            assert(false && "unsupported");
//             Lucky lucky(solver);
//             lucky.doit();
        } else if (token == "intree-probe") {
            if (!bnns.empty()) conf.do_hyperbin_and_transred = false;
            if (conf.doIntreeProbe && conf.doFindAndReplaceEqLits) intree->intree_probe();
        } else if (token == "sub-str-cls-with-bin") {
            //Subsumes and strengthens long clauses with binary clauses
            if (conf.do_distill_clauses) {
                dist_long_with_impl->distill_long_with_implicit(true);
            }
        } else if (token == "sub-cls-with-bin") {
            //Subsumes and strengthens long clauses with binary clauses
            if (conf.do_distill_clauses) {
                dist_long_with_impl->distill_long_with_implicit(false);
            }
        } else if (token == "distill-bins") {
            if (conf.do_distill_bin_clauses) {
                distill_bin_cls->distill();
            }
        } else if (token == "distill-litrem") {
            if (conf.do_distill_clauses) {
                distill_lit_rem->distill_lit_rem();
            }
        } else if (token == "distill-cls") {
            //Enqueues literals in long + tri clauses two-by-two and propagates
            if (conf.do_distill_clauses) {
                distill_long_cls->distill(false, false);
            }
        } else if (token == "clean-cls") {
            clauseCleaner->remove_and_clean_all();
        } else if (token == "distill-cls-onlyrem") {
            //Enqueues literals in long + tri clauses two-by-two and propagates
            if (conf.do_distill_clauses) {
                distill_long_cls->distill(false, true);
            }
        } else if (token == "must-distill-cls") {
            //Enqueues literals in long + tri clauses two-by-two and propagates
            if (conf.do_distill_clauses) {
                for(const auto& offs: longIrredCls) {
                    Clause* cl = cl_alloc.ptr(offs);
                    cl->distilled = 0;
                    cl->tried_to_remove = 0;
                }
                distill_long_cls->distill(false, false);
            }
        } else if (token == "must-distill-cls-onlyrem") {
            //Enqueues literals in long + tri clauses two-by-two and propagates
            if (conf.do_distill_clauses) {
                for(const auto& offs: longIrredCls) {
                    Clause* cl = cl_alloc.ptr(offs);
                    cl->tried_to_remove = 0;
                }
                distill_long_cls->distill(false, true);
            }
        } else if (token == "str-impl") {
            if (conf.doStrSubImplicit) {
                dist_impl_with_impl->str_impl_w_impl();
            }
        } else if (token == "cl-consolidate") {
            cl_alloc.consolidate(this, conf.must_always_conslidate, true);
        } else if (token == "louvain-comms") {
            #ifdef STATS_NEEDED
            CommunityFinder comm_finder(this);
            comm_finder.compute();
            #endif
        } else if (token == "renumber" || token == "must-renumber") {
            if (conf.doRenumberVars && !frat->enabled()) {
                if (!renumber_variables(token == "must-renumber" || conf.must_renumber)) {
                    return l_False;
                }
            }
        } else if (token == "breakid") {
            if (conf.doBreakid
                && frat->enabled()
                && (solveStats.num_simplify == 0 ||
                   (solveStats.num_simplify % conf.breakid_every_n == (conf.breakid_every_n-1)))
            ) {
                #ifdef USE_BREAKID
                if (!breakid->doit()) return l_False;
                #else
                verb_print(1,"[breakid] BreakID not compiled in, skipping");
                #endif
            }
        } else if (token == "bosphorus") {
            if (conf.do_bosphorus
                && (solveStats.num_simplify == 0 ||
                   (solveStats.num_simplify % conf.bosphorus_every_n == (conf.bosphorus_every_n-1)))
            ) {
                #ifdef USE_BOSPHORUS
                CMSBosphorus bosph(this);
                bosph.doit();
                #else
                if (conf.verbosity) {
                    cout << "c [bosphorus] Bosphorus not compiled in, skipping" << endl;
                }
                #endif
            }
        } else if (token == "") {
            //Nothing, just an empty comma, ignore
        } else if (token.substr(0,3) == "occ") {
            occ_strategy_tokens += token + ", ";
            //cout << "occ_strategy_tokens now: " << occ_strategy_tokens  << endl;
        } else {
            cout << "ERROR: strategy '" << token << "' not recognised!" << endl;
            exit(-1);
        }

        SLOW_DEBUG_DO(check_stats());
        if (!okay()) return l_False;
        DEBUG_ATTACH_MORE_DO(check_wrong_attach());
        DEBUG_ATTACH_MORE_DO(check_all_clause_attached());
    }
    DEBUG_ATTACH_MORE_DO(find_all_attached());

    return okay() ? l_Undef : l_False;
}

/**
@brief The function that brings together almost all CNF-simplifications
*/
lbool Solver::simplify_problem(const bool startup, const string& strategy) {
    assert(okay());
    verb_print(6,  __func__ << " called");
    DEBUG_IMPLICIT_STATS_DO(check_stats());
    DEBUG_ATTACH_MORE_DO(find_all_attached());
    DEBUG_ATTACH_MORE_DO(check_all_clause_attached());
    DEBUG_ATTACH_MORE_DO(check_implicit_propagated());
    SLOW_DEBUG_DO(assert(check_order_heap_sanity()));
    DEBUG_MARKED_CLAUSE_DO(assert(no_marked_clauses()));
    SLOW_DEBUG_DO(check_assumptions_sanity());

    if (solveStats.num_simplify_this_solve_call >= conf.max_num_simplify_per_solve_call) {
        return l_Undef;
    }

    lbool ret = l_Undef;
    clear_order_heap();
    if (!clear_gauss_matrices(false)) return l_False;

    if (ret == l_Undef) ret = execute_inprocess_strategy(startup, strategy);
    assert(ret != l_True);

    //Free unused watch memory
    free_unused_watches();

    conf.global_timeout_multiplier *= conf.global_timeout_multiplier_multiplier;
    conf.global_timeout_multiplier =
        std::min<double>(
            conf.global_timeout_multiplier,
            conf.orig_global_timeout_multiplier*conf.global_multiplier_multiplier_max
        );
    verb_print(1, "global_timeout_multiplier: " << std::setprecision(4) <<  conf.global_timeout_multiplier);

    solveStats.num_simplify++;
    solveStats.num_simplify_this_solve_call++;
    verb_print(6, __func__ << " finished");

    assert(!(ok == false && ret != l_False));
    if (ret == l_False) return l_False;

    assert(ret == l_Undef);
    DEBUG_IMPLICIT_STATS_DO(check_stats());
    DEBUG_ATTACH_MORE_DO(check_implicit_propagated());
    DEBUG_ATTACH_MORE_DO(check_all_clause_attached());
    DEBUG_ATTACH_MORE_DO(check_wrong_attach());

    //NOTE:
    // we have to rebuild HERE, or we'd rebuild every time solve()
    // is called, which is called form the outside, sometimes 1000x
    // in one second
    rebuildOrderHeap();

    return ret;
}

void CMSat::Solver::print_stats(
    const double cpu_time,
    const double cpu_time_total,
    const double wallclock_time_started) const
{
    if (conf.verbStats >= 1) {
        cout << "c ------- FINAL TOTAL SEARCH STATS ---------" << endl;
    }

    if (conf.do_print_times) {
        print_stats_line("c UIP search time"
            , sumSearchStats.cpu_time
            , stats_line_percent(sumSearchStats.cpu_time, cpu_time)
            , "% time"
        );
    }

    if (conf.verbStats > 1) {
        print_full_stats(cpu_time, cpu_time_total, wallclock_time_started);
    }
    print_norm_stats(cpu_time, cpu_time_total, wallclock_time_started);
}

void Solver::print_stats_time(
    const double cpu_time,
    const double cpu_time_total,
    const double wallclock_time_started) const
{
    if (conf.do_print_times) {
        print_stats_line("c Total time (this thread)", cpu_time);
        if (cpu_time != cpu_time_total) {
            print_stats_line("c Total time (all threads)", cpu_time_total);
            if (wallclock_time_started != 0.0) {
                print_stats_line("c Wall clock time: ", (real_time_sec() - wallclock_time_started));
            }
        }
    }
}

void Solver::print_norm_stats(
    const double cpu_time,
    const double cpu_time_total,
    const double wallclock_time_started) const
{
    sumSearchStats.print_short(sumPropStats.propagations, conf.do_print_times);
    print_stats_line("c props/decision"
        , float_div(propStats.propagations, sumSearchStats.decisions)
    );
    print_stats_line("c props/conflict"
        , float_div(propStats.propagations, sumConflicts)
    );

    print_stats_line("c 0-depth assigns", trail.size()
        , stats_line_percent(trail.size(), nVars())
        , "% vars"
    );
    print_stats_line("c 0-depth assigns by CNF"
        , zeroLevAssignsByCNF
        , stats_line_percent(zeroLevAssignsByCNF, nVars())
        , "% vars"
    );

    print_stats_line("c reduceDB time"
        , reduceDB->get_total_time()
        , stats_line_percent(reduceDB->get_total_time(), cpu_time)
        , "% time"
    );

    //OccSimplifier stats
    if (conf.perform_occur_based_simp) {
        if (conf.do_print_times)
            print_stats_line("c OccSimplifier time"
                , occsimplifier->get_stats().total_time(occsimplifier)
                , stats_line_percent(occsimplifier->get_stats().total_time(occsimplifier) ,cpu_time)
                , "% time"
            );
        occsimplifier->get_stats().print_extra_times();
        occsimplifier->get_sub_str()->get_stats().print_short(this);
    }
    print_stats_line("c SCC time"
        , varReplacer->get_scc_finder()->get_stats().cpu_time
        , stats_line_percent(varReplacer->get_scc_finder()->get_stats().cpu_time, cpu_time)
        , "% time"
    );
    varReplacer->get_scc_finder()->get_stats().print_short(nullptr);
    varReplacer->print_some_stats(cpu_time);

    //varReplacer->get_stats().print_short(nVars());
    print_stats_line("c distill long time"
                    , distill_long_cls->get_stats().time_used
                    , stats_line_percent(distill_long_cls->get_stats().time_used, cpu_time)
                    , "% time"
    );
    print_stats_line("c distill bin time"
                    , distill_bin_cls->get_stats().time_used
                    , stats_line_percent(distill_bin_cls->get_stats().time_used, cpu_time)
                    , "% time"
    );

    print_stats_line("c strength cache-irred time"
                    , dist_long_with_impl->get_stats().irredWatchBased.cpu_time
                    , stats_line_percent(dist_long_with_impl->get_stats().irredWatchBased.cpu_time, cpu_time)
                    , "% time"
    );
    print_stats_line("c strength cache-red time"
                    , dist_long_with_impl->get_stats().redWatchBased.cpu_time
                    , stats_line_percent(dist_long_with_impl->get_stats().redWatchBased.cpu_time, cpu_time)
                    , "% time"
    );

    if (sumConflicts > 0) {
        for(uint32_t i = 0; i < longRedCls.size(); i ++) {
            std::stringstream ss;
            ss << "c avg cls in red " << i;
            print_stats_line(ss.str()
                , (double)longRedClsSizes[i]/(double)sumConflicts
            );
        }
        #if defined(STATS_NEEDED) || defined (FINAL_PREDICTOR) || defined(NORMAL_CL_USE_STATS)
        for(uint32_t i = 0; i < longRedCls.size(); i++) {
            reduceDB->cl_stats[i].print(i);
        }
        #endif
    }

    #ifdef STATS_NEEDED
    print_stats_line(
        "c DB locked ratio",
        stats_line_percent(reduceDB->locked_for_data_gen_total, reduceDB->locked_for_data_gen_cls)
    );
    #endif

    if (conf.do_print_times) {
        print_stats_line("c Conflicts in UIP"
            , sumConflicts
            , float_div(sumConflicts, cpu_time)
            , "confl/time_this_thread"
        );
    } else {
        print_stats_line("c Conflicts in UIP", sumConflicts);
    }
    double vm_usage;
    std::string max_mem_usage;
    double max_rss_mem_mb = (double)memUsedTotal(vm_usage, &max_mem_usage)/(1024UL*1024UL);
    if (max_mem_usage.empty()) {
        print_stats_line("c Mem used"
            , max_rss_mem_mb
            , "MB"
        );
    } else {
        print_stats_line("c Max Memory (rss) used"
            , max_mem_usage
        );
//      print_stats_line("c Virt mem used at exit"
//         , vm_usage/(1024UL*1024UL)
//         , "MB"
//     );
    }
    print_stats_time(cpu_time, cpu_time_total, wallclock_time_started);
}

void Solver::print_full_stats(
    const double cpu_time,
    const double /*cpu_time_total*/,
    const double /*wallclock_time_started*/) const
{
    cout << "c All times are for this thread only except if explicitly specified" << endl;
    sumSearchStats.print(sumPropStats.propagations, conf.do_print_times);
    sumPropStats.print(sumSearchStats.cpu_time);
    //reduceDB->get_total_time().print(cpu_time);

    //OccSimplifier stats
    if (conf.perform_occur_based_simp) {
        occsimplifier->get_stats().print(nVarsOuter(), occsimplifier);
        occsimplifier->get_sub_str()->get_stats().print();
    }

    //TODO after TRI to LONG conversion
    /*if (occsimplifier && conf.doGateFind) {
        occsimplifier->print_gatefinder_stats();
    }*/

    varReplacer->get_scc_finder()->get_stats().print();
    varReplacer->get_stats().print(nVarsOuter());
    varReplacer->print_some_stats(cpu_time);
    distill_bin_cls->get_stats().print(nVarsOuter());
    dist_long_with_impl->get_stats().print();

    if (conf.doStrSubImplicit) {
        subsumeImplicit->get_stats().print("");
    }
    print_mem_stats();
}

uint64_t Solver::print_watch_mem_used(const uint64_t rss_mem_used) const
{
    size_t alloc = watches.mem_used_alloc();
    print_stats_line("c Mem for watch alloc"
        , alloc/(1024UL*1024UL)
        , "MB"
        , stats_line_percent(alloc, rss_mem_used)
        , "%"
    );

    size_t array = watches.mem_used_array();
    print_stats_line("c Mem for watch array"
        , array/(1024UL*1024UL)
        , "MB"
        , stats_line_percent(array, rss_mem_used)
        , "%"
    );

    return alloc + array;
}

size_t Solver::mem_used() const
{
    size_t mem = 0;
    mem += Searcher::mem_used();
    mem += assumptions.capacity()*sizeof(Lit);

    return mem;
}

uint64_t Solver::mem_used_vardata() const
{
    uint64_t mem = 0;
    mem += assigns.capacity()*sizeof(lbool);
    mem += varData.capacity()*sizeof(VarData);

    return mem;
}

void Solver::print_mem_stats() const
{
    double vm_mem_used = 0;
    const uint64_t rss_mem_used = memUsedTotal(vm_mem_used);
    print_stats_line("c Mem used"
        , rss_mem_used/(1024UL*1024UL)
        , "MB"
    );
    uint64_t account = 0;

    account += print_mem_used_longclauses(rss_mem_used);
    account += print_watch_mem_used(rss_mem_used);

    size_t mem = 0;
    mem += mem_used_vardata();
    print_stats_line("c Mem for assings&vardata"
        , mem/(1024UL*1024UL)
        , "MB"
        , stats_line_percent(mem, rss_mem_used)
        , "%"
    );
    account += mem;

    mem = mem_used();
    print_stats_line("c Mem for search&solve"
        , mem/(1024UL*1024UL)
        , "MB"
        , stats_line_percent(mem, rss_mem_used)
        , "%"
    );
    account += mem;

    mem = CNF::mem_used_renumberer();
    print_stats_line("c Mem for renumberer"
        , mem/(1024UL*1024UL)
        , "MB"
        , stats_line_percent(mem, rss_mem_used)
        , "%"
    );
    account += mem;

    if (occsimplifier) {
        mem = occsimplifier->mem_used();
        print_stats_line("c Mem for occsimplifier"
            , mem/(1024UL*1024UL)
            , "MB"
            , stats_line_percent(mem, rss_mem_used)
            , "%"
        );
        account += mem;
    }

    mem = varReplacer->mem_used();
    print_stats_line("c Mem for varReplacer&SCC"
        , mem/(1024UL*1024UL)
        , "MB"
        , stats_line_percent(mem, rss_mem_used)
        , "%"
    );
    account += mem;

    if (subsumeImplicit) {
        mem = subsumeImplicit->mem_used();
        print_stats_line("c Mem for impl subsume"
            , mem/(1024UL*1024UL)
            , "MB"
            , stats_line_percent(mem, rss_mem_used)
            , "%"
        );
        account += mem;
    }


    mem = distill_long_cls->mem_used();
    mem += dist_long_with_impl->mem_used();
    mem += dist_impl_with_impl->mem_used();
    print_stats_line("c Mem for 3 distills"
        , mem/(1024UL*1024UL)
        , "MB"
        , stats_line_percent(mem, rss_mem_used)
        , "%"
    );
    account += mem;

    print_stats_line("c Accounted for mem (rss)"
        , stats_line_percent(account, rss_mem_used)
        , "%"
    );
    print_stats_line("c Accounted for mem (vm)"
        , stats_line_percent(account, vm_mem_used)
        , "%"
    );
}

void Solver::print_clause_size_distrib()
{
    size_t size3 = 0;
    size_t size4 = 0;
    size_t size5 = 0;
    size_t sizeLarge = 0;
    for(const auto longIrredCl: longIrredCls) {
        Clause* cl = cl_alloc.ptr(longIrredCl);
        switch(cl->size()) {
            case 0:
            case 1:
            case 2:
                assert(false);
                break;
            case 3:
                size3++;
                break;
            case 4:
                size4++;
                break;
            case 5:
                size5++;
                break;
            default:
                sizeLarge++;
                break;
        }
    }

    cout
    << "c clause size stats."
    << " size3: " << size3
    << " size4: " << size4
    << " size5: " << size5
    << " larger: " << sizeLarge << endl;
}


vector<Lit> Solver::get_zero_assigned_lits(const bool backnumber,
                                           const bool only_nvars) const
{
    vector<Lit> lits;
    assert(decisionLevel() == 0);
    size_t until;
    if (only_nvars) {
        until = nVars();
    } else {
        until = assigns.size();
    }
    for(size_t i = 0; i < until; i++) {
        if (assigns[i] != l_Undef) {
            Lit lit(i, assigns[i] == l_False);

            //Update to higher-up
            lit = varReplacer->get_lit_replaced_with(lit);
            if (varData[lit.var()].is_bva == false) {
                if (backnumber) lits.push_back(map_inter_to_outer(lit));
                else lits.push_back(lit);

            }

            //Everything it repaces has also been set
            const vector<uint32_t> vars = varReplacer->get_vars_replacing(lit.var());
            for(const uint32_t var: vars) {
                if (varData[var].is_bva) continue;

                Lit tmp_lit = Lit(var, false);
                assert(varReplacer->get_lit_replaced_with(tmp_lit).var() == lit.var());
                if (lit != varReplacer->get_lit_replaced_with(tmp_lit)) {
                    tmp_lit ^= true;
                }
                assert(lit == varReplacer->get_lit_replaced_with(tmp_lit));

                if (backnumber) lits.push_back(map_inter_to_outer(tmp_lit));
                else lits.push_back(tmp_lit);
            }
        }
    }

    //Remove duplicates. Because of above replacing-mimicing algo
    //multipe occurrences of literals can be inside
    std::sort(lits.begin(), lits.end());
    vector<Lit>::iterator it = std::unique (lits.begin(), lits.end());
    lits.resize( std::distance(lits.begin(),it) );

    return lits;
}

bool Solver::verify_model_implicit_clauses() const
{
    uint32_t wsLit = 0;
    for (watch_array::const_iterator
        it = watches.begin(), end = watches.end()
        ; it != end
        ; ++it, wsLit++
    ) {
        Lit lit = Lit::toLit(wsLit);
        watch_subarray_const ws = *it;

        for (Watched w: ws) {
            if (w.isBin()
                && model_value(lit) != l_True
                && model_value(w.lit2()) != l_True
            ) {
                cout
                << "bin clause: "
                << lit << " , " << w.lit2()
                << " not satisfied!"
                << endl;

                cout
                << "value of unsat bin clause: "
                << value(lit) << " , " << value(w.lit2())
                << endl;

                return false;
            }
        }
    }

    return true;
}

bool Solver::verify_model_long_clauses(const vector<ClOffset>& cs) const
{
    #ifdef VERBOSE_DEBUG
    cout << "Checking clauses whether they have been properly satisfied." << endl;
    #endif

    bool verificationOK = true;

    for (const auto& off : cs) {
        Clause& cl = *cl_alloc.ptr(off);
        for (const auto& l : cl) if (model_value(l) == l_True) goto next;
        cout << "unsatisfied clause: " << cl << endl;
        verificationOK = false;
        next:
        ;
    }

    return verificationOK;
}

bool Solver::verify_model() const
{
    bool verificationOK = true;
    verificationOK &= verify_model_long_clauses(longIrredCls);
    for(auto& lredcls: longRedCls) {
        verificationOK &= verify_model_long_clauses(lredcls);
    }
    verificationOK &= verify_model_implicit_clauses();

    if (conf.verbosity && verificationOK) {
        cout
        << "c Verified "
        << longIrredCls.size() + longRedCls.size()
            + binTri.irredBins + binTri.redBins
        << " clause(s)."
        << endl;
    }

    return verificationOK;
}

size_t Solver::get_num_nonfree_vars() const
{
    size_t nonfree = 0;
    if (decisionLevel() == 0) {
        nonfree += trail.size();
    } else {
        nonfree += trail_lim[0];
    }

    if (occsimplifier) {
        if (conf.perform_occur_based_simp) {
            nonfree += occsimplifier->get_num_elimed_vars();
        }
    }
    nonfree += varReplacer->get_num_replaced_vars();

    return nonfree;
}

size_t Solver::get_num_free_vars() const
{
    return nVarsOuter() - get_num_nonfree_vars();
}

void Solver::print_clause_stats() const
{
    //Irredundant
    cout << " " << print_value_kilo_mega(longIrredCls.size());
    cout << " " << print_value_kilo_mega(binTri.irredBins);
    cout
    << " " << std::setw(7) << std::fixed << std::setprecision(2)
    << ratio_for_stat(litStats.irredLits, longIrredCls.size())
    << " " << std::setw(7) << std::fixed << std::setprecision(2)
    << ratio_for_stat(litStats.irredLits + binTri.irredBins*2
    , longIrredCls.size() + binTri.irredBins)
    ;

    //Redundant
    size_t tot = 0;
    for(auto& lredcls: longRedCls) {
        cout << " " << print_value_kilo_mega(lredcls.size());
        tot += lredcls.size();
    }

    cout << " " << print_value_kilo_mega(binTri.redBins);
    cout
    << " " << std::setw(7) << std::fixed << std::setprecision(2)
    << ratio_for_stat(litStats.redLits, tot)
    << " " << std::setw(7) << std::fixed << std::setprecision(2)
    << ratio_for_stat(litStats.redLits + binTri.redBins*2
    , tot + binTri.redBins)
    ;
}

const char* Solver::get_version_sha1()
{
    return CMSat::get_version_sha1();
}

const char* Solver::get_version_tag()
{
    return CMSat::get_version_tag();
}

const char* Solver::get_compilation_env()
{
    return CMSat::get_compilation_env();
}

void Solver::print_watch_list(watch_subarray_const ws, const Lit lit) const
{
    cout << "Watch[" << lit << "]: "<< endl;
    for (const auto& w : ws) {
        if (w.isClause()) {
            Clause* cl = cl_alloc.ptr(w.get_offset());
            cout << "-> Clause: " << *cl << " red: " << cl->red();
        }
        if (w.isBin()) {
            cout << "-> BIN: " << lit << ", " << w.lit2() << " red: " << w.red();
        }
        cout << endl;
    }
    cout << "FIN" << endl;
}


void Solver::check_clause_propagated(const Xor& x) const {
    uint32_t num_undef = 0;
    uint32_t num_false = 0;
    for(const auto& v: x) {
        if (value(v) == l_True) return;
        if (value(v) == l_Undef) num_undef++;
        if (value(v) == l_False) num_false++;
        if (num_undef > 1) return;
    }

    assert(num_undef == 1);
    assert(num_false == x.size()-1);
    cout << "ERROR: xor clause " << x << " should have propagated already!" << endl;
    assert(false);
    exit(-1);
}

void Solver::check_clause_propagated(const ClOffset& offs) const {
    Clause& c = *cl_alloc.ptr(offs);
    uint32_t num_undef = 0;
    uint32_t num_false = 0;
    for(const auto& l: c) {
        if (value(l) == l_True) return;
        if (value(l) == l_Undef) num_undef++;
        if (value(l) == l_False) num_false++;
        if (num_undef > 1) return;
    }

    assert(num_undef == 1);
    assert(num_false == c.size()-1);
    cout << "ERROR: clause " << c << " should have propagated already!" << endl;
    assert(false);
    exit(-1);
}

void Solver::check_all_clause_propagated() const {
    check_implicit_propagated();
    for(const auto& c: longIrredCls) check_clause_propagated(c);
    for(const auto& cs: longRedCls) for(const auto& c: cs) check_clause_propagated(c);
    for(const auto& x: xorclauses) check_clause_propagated(x);
}

void Solver::check_implicit_propagated() const
{
    const double my_time = cpuTime();
    size_t wsLit = 0;
    for(watch_array::const_iterator
        it = watches.begin(), end = watches.end()
        ; it != end
        ; ++it, wsLit++
    ) {
        const Lit lit = Lit::toLit(wsLit);
        watch_subarray_const ws = *it;
        for(const auto& w : ws) {
            //Satisfied, or not implicit, skip
            if (value(lit) == l_True || w.isClause()) continue;

            const lbool val1 = value(lit);
            const lbool val2 = value(w.lit2());

            //Handle binary
            if (w.isBin()) {
                if (val1 == l_False) {
                    if (val2 != l_True) {
                        cout << "not prop BIN: "
                        << lit << ", " << w.lit2()
                        << " (red: " << w.red()
                        << endl;
                    }
                    assert(val2 == l_True);
                }

                if (val2 == l_False)
                    assert(val1 == l_True);
            }
        }
    }
    const double time_used = cpuTime() - my_time;
    if (sqlStats) {
        sqlStats->time_passed_min(
            this
            , "check implicit propagated"
            , time_used
        );
    }
}

size_t Solver::get_num_vars_elimed() const {
    if (conf.perform_occur_based_simp) return occsimplifier->get_num_elimed_vars();
    else return 0;
}

void Solver::free_unused_watches()
{
    size_t wsLit = 0;
    for (watch_array::iterator
        it = watches.begin(), end = watches.end()
        ; it != end
        ; ++it, wsLit++
    ) {
        Lit lit = Lit::toLit(wsLit);
        if (varData[lit.var()].removed == Removed::elimed
            || varData[lit.var()].removed == Removed::replaced
        ) {
            watch_subarray ws = *it;
            assert(ws.empty());
            ws.clear();
        }
    }

    if ((sumConflicts - last_full_watch_consolidate) > conf.full_watch_consolidate_every_n_confl) {
        last_full_watch_consolidate = sumConflicts;
        consolidate_watches(true);
    } else {
        consolidate_watches(false);
    }
}

bool Solver::fully_enqueue_these(const vector<Lit>& toEnqueue)
{
    assert(ok);
    assert(decisionLevel() == 0);
    for(const auto& lit: toEnqueue) {
        if (!fully_enqueue_this(lit)) {
            return false;
        }
    }

    return true;
}

bool Solver::fully_enqueue_this(const Lit lit)
{
    assert(decisionLevel() == 0);
    assert(ok);

    const lbool val = value(lit);
    if (val == l_Undef) {
        assert(varData[lit.var()].removed == Removed::none);
        enqueue<false>(lit);
        ok = propagate<true>().isnullptr();

        if (!ok) {
            return false;
        }
    } else if (val == l_False) {
        *frat << add << ++clauseID << fin;
        ok = false;
        return false;
    }
    return true;
}

void Solver::new_external_var()
{
    new_var(false);
}

void Solver::new_external_vars(size_t n)
{
    new_vars(n);
}

void Solver::add_in_partial_solving_stats()
{
    Searcher::add_in_partial_solving_stats();
    sumSearchStats += Searcher::get_stats();
    sumPropStats += propStats;
}

bool Solver::add_clause_outside(const vector<Lit>& lits, bool red, bool restore)
{
    if (!ok) {
      if (frat->incremental())
	*frat << "new outside\n" << origcl << lits << fin;
      return false;
    };

    if (restore && frat->incremental() && !lits.empty())
      *frat << restorecl << lits << fin;
    SLOW_DEBUG_DO(check_too_large_variable_number(lits)); //we check for this during back-numbering
    vector<Lit> tmp(lits);
    return add_clause_outer(tmp, lits, red, restore);
}

bool Solver::add_xor_clause_outside(const vector<Lit>& lits_out, bool rhs) {
    frat_func_start();
    if (!okay()) return false;
    if (rhs == false && lits_out.empty()) return okay();

    vector<Lit> lits = lits_out;
    const int32_t XID = ++clauseXID;
    *frat << origclx << XID << lits << fin;
    SLOW_DEBUG_DO(check_too_large_variable_number(lits));

    add_clause_helper(lits);
    add_xor_clause_inter(lits, rhs, true, XID);

    frat_func_end();
    return okay();
}

bool Solver::add_xor_clause_outside(const vector<uint32_t>& vars, const bool rhs) {
    frat_func_start();
    if (!okay()) return false;
    if (rhs == false && vars.empty()) return okay();

    vector<Lit> lits = vars_to_lits(vars);
    if (!vars.empty()) lits[0] ^= !rhs;
    const int32_t XID = ++clauseXID;
    *frat << origclx << XID << lits << fin;
    if (!vars.empty()) lits[0] ^= !rhs;
    SLOW_DEBUG_DO(check_too_large_variable_number(lits));

    add_clause_helper(lits);
    add_xor_clause_inter(lits, rhs, true, XID);

    frat_func_end();
    return okay();
}

bool Solver::add_bnn_clause_outside( const vector<Lit>& lits, const int32_t cutoff, Lit out) {
    if (!ok) return false;
    SLOW_DEBUG_DO(check_too_large_variable_number(lits));

    vector<Lit> lits2(lits);
    add_clause_helper(lits2);
    out = map_outer_to_inter(out);
    out = varReplacer->get_lit_replaced_with(out);
    add_bnn_clause_inter(lits2, cutoff, out);

    return ok;
}

void Solver::check_too_large_variable_number(const vector<Lit>& lits) const
{
    for (const Lit lit: lits) {
        if (lit.var() >= nVarsOuter()) {
            std::cerr
            << "ERROR: Variable " << lit.var() + 1 << " inserted, but max var is " << nVarsOuter() << endl;
            assert(false);
            std::exit(-1);
        }
        release_assert(lit.var() < nVarsOuter()
        && "Clause inserted, but variable inside has not been declared with PropEngine::new_var() !");

        if (lit.var() >= var_Undef) {
            std::cerr << "ERROR: Variable number " << lit.var()
            << "too large. PropBy is limiting us, sorry" << endl;
            assert(false);
            std::exit(-1);
        }
    }
}

// [replaced, replaced_with]
vector<pair<Lit, Lit> > Solver::get_all_binary_xors() const {
    vector<pair<Lit, Lit> > bin_xors = varReplacer->get_all_binary_xors_outer();
    vector<pair<Lit, Lit> > ret;
    for(std::pair<Lit, Lit> p: bin_xors) {
        if (!varData[p.first.var()].is_bva && !varData[p.second.var()].is_bva) ret.push_back(p);
    }

    return ret;
}

//TODO later, this can be removed, get_num_free_vars() is MUCH cheaper to
//compute but may have some bugs here-and-there
uint32_t Solver::num_active_vars() const
{
    uint32_t numActive = 0;
    uint32_t removed_replaced = 0;
    uint32_t removed_set = 0;
    uint32_t removed_elimed = 0;
    uint32_t removed_non_decision = 0;
    for(uint32_t var = 0; var < nVarsOuter(); var++) {
        if (value(var) != l_Undef) {
            if (varData[var].removed != Removed::none)
            {
                cout << "ERROR: var " << var + 1 << " has removed: "
                << removed_type_to_string(varData[var].removed)
                << " but is set to " << value(var) << endl;
                assert(varData[var].removed == Removed::none);
                exit(-1);
            }
            removed_set++;
            continue;
        }
        switch(varData[var].removed) {
            case Removed::elimed :
                removed_elimed++;
                continue;
            case Removed::replaced:
                removed_replaced++;
                continue;
            case Removed::none:
                break;
        }
        if (varData[var].removed != Removed::none) {
            removed_non_decision++;
        }
        numActive++;
    }
    assert(removed_non_decision == 0);
    if (occsimplifier) {
        assert(removed_elimed == occsimplifier->get_num_elimed_vars());
    } else {
        assert(removed_elimed == 0);
    }

    assert(removed_set == ((decisionLevel() == 0) ? trail.size() : trail_lim[0]));

    assert(removed_replaced == varReplacer->get_num_replaced_vars());
    assert(numActive == get_num_free_vars());

    return numActive;
}

#ifdef STATS_NEEDED
SatZillaFeatures Solver::calculate_satzilla_features()
{
    latest_satzilla_feature_calc++;
    SatZillaFeaturesCalc extract(this);
    SatZillaFeatures satzilla_feat = extract.extract();
    satzilla_feat.avg_confl_size = hist.conflSizeHistLT.avg();
    satzilla_feat.avg_confl_glue = hist.glueHistLT.avg();
    satzilla_feat.avg_num_resolutions = hist.numResolutionsHistLT.avg();
    satzilla_feat.avg_trail_depth_delta = hist.trailDepthDeltaHist.avg();
    satzilla_feat.avg_branch_depth = hist.branchDepthHist.avg();
    satzilla_feat.avg_branch_depth_delta = hist.branchDepthDeltaHist.avg();

    satzilla_feat.confl_size_min = hist.conflSizeHistLT.getMin();
    satzilla_feat.confl_size_max = hist.conflSizeHistLT.getMax();
    satzilla_feat.confl_glue_min = hist.glueHistLT.getMin();
    satzilla_feat.confl_glue_max = hist.glueHistLT.getMax();
    satzilla_feat.branch_depth_min = hist.branchDepthHist.getMin();
    satzilla_feat.branch_depth_max = hist.branchDepthHist.getMax();
    satzilla_feat.trail_depth_delta_min = hist.trailDepthDeltaHist.getMin();
    satzilla_feat.trail_depth_delta_max = hist.trailDepthDeltaHist.getMax();
    satzilla_feat.num_resolutions_min = hist.numResolutionsHistLT.getMin();
    satzilla_feat.num_resolutions_max = hist.numResolutionsHistLT.getMax();

    if (sumPropStats.propagations != 0
        && sumConflicts != 0
        && sumSearchStats.numRestarts != 0
    ) {
        satzilla_feat.props_per_confl = (double)sumConflicts / (double)sumPropStats.propagations;
        satzilla_feat.confl_per_restart = (double)sumConflicts / (double)sumSearchStats.numRestarts;
        satzilla_feat.decisions_per_conflict = (double)sumSearchStats.decisions / (double)sumConflicts;
        satzilla_feat.learnt_bins_per_confl = (double)sumSearchStats.learntBins / (double)sumConflicts;
    }

    satzilla_feat.num_gates_found_last = sumSearchStats.num_gates_found_last;
    satzilla_feat.num_xors_found_last = sumSearchStats.num_xors_found_last;

    if (conf.verbosity > 2) {
        satzilla_feat.print_stats();
    }

    if (sqlStats) {
        sqlStats->satzilla_features(this, this, satzilla_feat);
    }

    return satzilla_feat;
}
#endif

void Solver::check_implicit_stats(const bool onlypairs) const
{
    //Don't check if in crazy mode
    #ifdef NDEBUG
    return;
    #endif
    const double my_time = cpuTime();

    //Check number of red & irred binary clauses
    uint64_t thisNumRedBins = 0;
    uint64_t thisNumIrredBins = 0;

    size_t wsLit = 0;
    for(watch_array::const_iterator
        it = watches.begin(), end = watches.end()
        ; it != end
        ; ++it, wsLit++
    ) {
        watch_subarray_const ws = *it;
        for(const auto& w: ws) {
            if (w.isBin()) {
                #ifdef DEBUG_IMPLICIT_PAIRS_TRIPLETS
                Lit lits[2];
                lits[0] = Lit::toLit(wsLit);
                lits[1] = w.lit2();
                std::sort(lits, lits + 2);
                findWatchedOfBin(watches, lits[0], lits[1], w.red(), w.get_ID());
                findWatchedOfBin(watches, lits[1], lits[0], w.red(), w.get_ID());
                #endif

                if (w.red()) thisNumRedBins++;
                else thisNumIrredBins++;

                continue;
            }
        }
    }

    if (onlypairs) {
        goto end;
    }

    if (thisNumIrredBins/2 != binTri.irredBins) {
        std::cerr
        << "ERROR:"
        << " thisNumIrredBins/2: " << thisNumIrredBins/2
        << " thisNumIrredBins: " << thisNumIrredBins
        << " binTri.irredBins: " << binTri.irredBins
        << endl;
    }
    assert(thisNumIrredBins % 2 == 0);
    assert(thisNumIrredBins/2 == binTri.irredBins);

    if (thisNumRedBins/2 != binTri.redBins) {
        std::cerr
        << "ERROR:"
        << " thisNumRedBins/2: " << thisNumRedBins/2
        << " thisNumRedBins: " << thisNumRedBins
        << " binTri.redBins: " << binTri.redBins
        << endl;
    }
    assert(thisNumRedBins % 2 == 0);
    assert(thisNumRedBins/2 == binTri.redBins);

    end:

    const double time_used = cpuTime() - my_time;
    if (sqlStats) {
        sqlStats->time_passed_min(
            this
            , "check implicit stats"
            , time_used
        );
    }
}

void Solver::check_stats(const bool allowFreed) const
{
    //If in crazy mode, don't check
    #ifdef NDEBUG
    return;
    #endif

    check_implicit_stats();

    const double my_time = cpuTime();
    uint64_t numLitsIrred = count_lits(longIrredCls, false, allowFreed);
    if (numLitsIrred != litStats.irredLits) {
        std::cerr << "ERROR: " << endl
        << "->numLitsIrred: " << numLitsIrred << endl
        << "->litStats.irredLits: " << litStats.irredLits << endl;
    }

    uint64_t numLitsRed = 0;
    for(auto& lredcls: longRedCls) {
        numLitsRed += count_lits(lredcls, true, allowFreed);
    }
    if (numLitsRed != litStats.redLits) {
        std::cerr << "ERROR: " << endl
        << "->numLitsRed: " << numLitsRed << endl
        << "->litStats.redLits: " << litStats.redLits << endl;
    }
    assert(numLitsRed == litStats.redLits);
    assert(numLitsIrred == litStats.irredLits);

    const double time_used = cpuTime() - my_time;
    if (sqlStats) {
        sqlStats->time_passed_min(
            this
            , "check literal stats"
            , time_used
        );
    }
}

void Solver::add_sql_tag(const string& name, const string& val)
{
    if (sqlStats) {
        sqlStats->add_tag(std::make_pair(name, val));
    }
}

vector<Lit> Solver::get_toplevel_units_internal(bool outer_numbering) const
{
    assert(!outer_numbering);
    vector<Lit> units;
    for(size_t i = 0; i < nVars(); i++) {
        if (value(i) != l_Undef) {
            Lit l = Lit(i, value(i) == l_False);
            units.push_back(l);
        }
    }

    return units;
}

// ONLY used externally
vector<Xor> Solver::get_recovered_xors() {
    vector<Xor> xors_ret;
    if (!okay()) return xors_ret;

    lbool ret = execute_inprocess_strategy(false, "occ-xor");
    if (ret == l_False) return xors_ret;

    auto xors = xorclauses;
    renumber_xors_to_outside(xors, xors_ret);
    return xors_ret;
}

void Solver::renumber_xors_to_outside(const vector<Xor>& xors, vector<Xor>& xors_ret)
{
    for(auto& x: xors) {
        verb_print(5, "XOR before outer numbering: " << x);
        bool OK = true;
        for(const auto v: x.get_vars()) {
            if (varData[v].is_bva) {
                OK = false;
                break;
            }
        }
        if (!OK) continue;

        vector<uint32_t> t = xor_outer_numbered(x.get_vars());
        xors_ret.push_back(Xor(t, x.rhs));
    }
}

// This detaches all xor clauses, and re-attaches them
// with ONLY the ones attached that are not in a matrix
// and the matrices are created and initialized
bool Solver::find_and_init_all_matrices() {
    frat_func_start();
    if (!xorclauses_updated) {
        if (conf.verbosity >= 2) {
            cout << "c [find&init matx] XORs not updated-> not performing matrix init. Matrices: "
                << gmatrices.size() << endl;
        }
        return true;
    }
    if (!clear_gauss_matrices(false)) return false; //attaches XORs actually
    detach_clauses_in_xors();

    verb_print(1, "[find&init matx] performing matrix init");
    MatrixFinder mfinder(solver);
    bool matrix_created;
    ok = mfinder.find_matrices(matrix_created);
    if (!ok) return false;
    if (!init_all_matrices()) return false;

    verb_print(2, "[gauss] matrix_created: " << matrix_created);

    #ifdef SLOW_DEBUG
    for(size_t i = 0; i< gmatrices.size(); i++) {
        if (gmatrices[i]) {
            gmatrices[i]->check_watchlist_sanity();
            assert(gmatrices[i]->get_matrix_no() == i);
        }
    }
    #endif

    xorclauses_updated = false;
    frat_func_end();
    return true;
}

// Runs init on all matrices. Note that the XORs inside the matrices
// are at this point not attached.
bool Solver::init_all_matrices() {
    assert(okay());
    assert(decisionLevel() == 0);

    assert(gmatrices.size() == gqueuedata.size());
    for (uint32_t i = 0; i < gmatrices.size(); i++) {
        auto& g = gmatrices[i];
        bool created = false;
        if (!g->full_init(created)) return false;
        assert(okay());

        if (!created) {
            gqueuedata[i].disabled = true;
            delete g;
            if (conf.verbosity > 5) {
                cout << "DELETED matrix" << endl;
            }
            g = nullptr;
        }
    }

    uint32_t j = 0;
    bool modified = false;
    for (uint32_t i = 0; i < gqueuedata.size(); i++) {
        if (gmatrices[i] != nullptr) {
            gmatrices[j] = gmatrices[i];
            gmatrices[j]->update_matrix_no(j);
            gqueuedata[j] = gqueuedata[i];

            if (modified) {
                for (size_t var = 0; var < nVars(); var++) {
                    for(auto& k: gwatches[var]) {
                        if (k.matrix_num == i) {
                            k.matrix_num = j;
                        }
                    }
                }
            }
            j++;
        } else {
            modified = true;
        }
    }
    gqueuedata.resize(j);
    gmatrices.resize(j);
    return okay();
}

void Solver::start_getting_constraints(bool red, bool simplified,
        uint32_t max_len, uint32_t max_glue) {
    assert(get_clause_query == nullptr);
    get_clause_query = new GetClauseQuery(this);
    get_clause_query->start_getting_constraints(red, simplified, max_len, max_glue);
}

bool Solver::get_next_constraint(std::vector<Lit>& ret, bool& is_xor, bool& rhs) {
    assert(get_clause_query);
    return get_clause_query->get_next_constraint(ret, is_xor, rhs);
}

void Solver::end_getting_constraints()
{
    assert(get_clause_query);
    get_clause_query->end_getting_constraints();
    delete get_clause_query;
    get_clause_query = nullptr;
}

vector<uint32_t> Solver::translate_sampl_set(const vector<uint32_t>& sampl_set) {
    assert(get_clause_query);
    return get_clause_query->translate_sampl_set(sampl_set);
}

void Solver::check_assigns_for_assumptions() const {
    for (Lit p: assumptions) {
        p = solver->varReplacer->get_lit_replaced_with_outer(p);
        p = solver->map_outer_to_inter(p);
        if (value(p) != l_True) {
            cout << "ERROR: Internal assumption " << p
            << " is not set to l_True, it's set to: " << value(p) << endl;
            assert(lit_inside_assumptions(p) == l_True);
        }
        assert(value(p) == l_True);
    }
}

bool Solver::check_assumptions_contradict_foced_assignment() const {
    for (Lit p: assumptions) {
        p = solver->varReplacer->get_lit_replaced_with_outer(p);
        p = solver->map_outer_to_inter(p);
        if (value(p) == l_False) return true;
    }
    return false;
}

void Solver::set_lit_weight([[maybe_unused]] const Lit lit, [[maybe_unused]] const double weight) {
    assert(lit.var() < nVars());
    #ifdef WEIGHTED
    if (!lit.sign()) varData[lit.var()].pos_weight = weight;
    else varData[lit.var()].neg_weight = weight;

    if (!varData[lit.var()].weight_set) {
        varData[lit.var()].weight_set = true;
        if (!lit.sign()) varData[lit.var()].neg_weight = 1.0-weight;
        else varData[lit.var()].neg_weight = 1.0-weight;
    }
    #else
    cout << "ERROR: set_lit_weight only supported if you compile with -DWEIGHTED=ON" << endl;
    exit(-1);
    #endif
}

vector<double> Solver::get_vsids_scores() const
{
    auto scores(var_act_vsids);

    //Map to outer
    vector<double> scores_outer(nVarsOuter(), 0);
    for(uint32_t i = 0; i < scores.size(); i ++) {
        uint32_t outer = map_inter_to_outer(i);
        scores_outer[outer] = scores[i];
    }

    return scores_outer;
}

bool Solver::implied_by(const std::vector<Lit>& lits,
                                  std::vector<Lit>& out_implied)
{

    out_implied.clear();
    if (!okay()) return false;

    implied_by_tmp_lits = lits;
    if (!add_clause_helper(implied_by_tmp_lits)) return false;

    assert(decisionLevel() == 0);
    for(Lit p: implied_by_tmp_lits) {
        if (value(p) == l_Undef) {
            new_decision_level();
            enqueue<false>(p);
        }
        if (value(p) == l_False) {
            cancelUntil<false, true>(0);
            return false;
        }
    }

    if (decisionLevel() == 0) return true;

    PropBy x = propagate<true>();
    if (!x.isnullptr()) {
        //UNSAT due to prop
        cancelUntil<false, true>(0);
        return false;
    }
    //DO NOT add the "optimization" to return when nothing got propagated
    //replaced variables CAN be added!!!

    out_implied.reserve(trail.size()-trail_lim[0]);
    for(uint32_t i = trail_lim[0]; i < trail.size(); i++) {
        if (trail[i].lit.var() < nVars()) {
            out_implied.push_back(trail[i].lit);
        }
    }
    cancelUntil<false, true>(0);

    //Map to outer
    for(auto& l: out_implied) l = map_inter_to_outer(l);
    varReplacer->extend_pop_queue(out_implied);
    return true;
}

void Solver::reset_vsids()
{
    for(auto& x: var_act_vsids) x = 0;
}

#ifdef STATS_NEEDED
void Solver::stats_del_cl(Clause* cl)
{
    if (cl->stats.is_tracked && sqlStats) {
        const ClauseStatsExtra& stats_extra = solver->red_stats_extra[cl->stats.extra_pos];
        assert(stats_extra.orig_ID != 0);
        assert(stats_extra.orig_ID <= cl->stats.ID);
        sqlStats->cl_last_in_solver(this, stats_extra.orig_ID);
    }
}

void Solver::stats_del_cl(ClOffset offs)
{
    Clause* cl = cl_alloc.ptr(offs);
    stats_del_cl(cl);
}
#endif

vector<OrGate> Solver::get_recovered_or_gates()
{
    assert(get_num_bva_vars() == 0 && "not implemented for BVA");
    if (!okay()) {
        return vector<OrGate>();
    }

    vector<OrGate> or_gates = occsimplifier->recover_or_gates();

    for(auto& g: or_gates) {
        g.rhs = map_inter_to_outer(g.rhs);
        for(auto& l: g.lits) l = map_inter_to_outer(l);
    }

    return or_gates;
}

vector<ITEGate> Solver::get_recovered_ite_gates()
{
    assert(get_num_bva_vars() == 0 && "not implemented for BVA");
    if (!okay()) {
        return vector<ITEGate>();
    }

    vector<ITEGate> or_gates = occsimplifier->recover_ite_gates();

    for(auto& g: or_gates) {
        g.rhs = map_inter_to_outer(g.rhs);
        for(auto& l: g.lhs) {
            l = map_inter_to_outer(l);
        }
    }

    return or_gates;
}

vector<uint32_t> Solver::remove_definable_by_irreg_gate(const vector<uint32_t>& vars)
{
    if (!okay()) return vector<uint32_t>{};
    return occsimplifier->remove_definable_by_irreg_gate(vars);
}

void Solver::get_empties(vector<uint32_t>& sampl_vars, vector<uint32_t>& empty_vars)
{
    if (!okay()) return;
    assert(get_num_bva_vars() == 0);

    occsimplifier->get_empties(sampl_vars, empty_vars);
}

bool Solver::remove_and_clean_all() {
    return clauseCleaner->remove_and_clean_all();
}

bool Solver::remove_and_clean_detached_xors(vector<Xor>& xors) {
    return clauseCleaner->clean_xor_clauses(xors, false);
}

void Solver::set_max_confl(uint64_t max_confl)
{
    if (get_stats().conflicts + max_confl < max_confl) {
          conf.max_confl = numeric_limits<uint64_t>::max();
      } else {
          conf.max_confl = get_stats().conflicts + max_confl;
      }
}

lbool Solver::bnn_eval(BNN& bnn)
{
    assert(decisionLevel() == 0);

    for(const auto& p: bnn) assert(value(p) == l_Undef);
    if (bnn.set) assert(bnn.out == lit_Undef);
    else assert(value(bnn.out) == l_Undef);

    // we are at the cutoff no matter what undef is
    if (bnn.cutoff <= 0) {
        if (bnn.set) return l_True;
        enqueue<false>(bnn.out, decisionLevel());
        return l_True;
    }

    // we are under the cutoff no matter what undef is
    if ((int)bnn.size() < bnn.cutoff) {
        if (bnn.set) {
            return l_False;
        }

        enqueue<false>(~bnn.out, decisionLevel());
        return l_True;
    }

    //it's set and cutoff can ONLY be met by ALL TRUE
    if (bnn.set && (int)bnn.size() == bnn.cutoff) {
        for(const auto& l: bnn) {
            enqueue<false>(l, decisionLevel());
        }
        return l_True;
    }

    if (bnn.size() == 0) {
        if (bnn.cutoff <= 0) {
            assert(bnn.set);
        } else {
            assert(false);
        }
        //remove
        return l_True;
    }

    return l_Undef;
}


PicoSAT* Solver::build_picosat()
{
    PicoSAT* picosat = picosat_init();
    for(uint32_t i = 0; i < nVars(); i++) picosat_inc_max_var(picosat);

    for(auto const& off: longIrredCls) {
        Clause* cl = cl_alloc.ptr(off);
        for(auto const& l1: *cl) {
            picosat_add(picosat, PICOLIT(l1));
        }
        picosat_add(picosat, 0);
    }
    for(uint32_t i = 0; i < nVars()*2; i++) {
        Lit l1 = Lit::toLit(i);
        for(auto const& w: watches[l1]) {
            if (!w.isBin() || w.red()) continue;
            const Lit l2 = w.lit2();
            if (l1 > l2) continue;

            picosat_add(picosat, PICOLIT(l1));
            picosat_add(picosat, PICOLIT(l2));
            picosat_add(picosat, 0);
        }
    }
    return picosat;
}

#ifdef ARJUN_SERIALIZE
string Solver::serialize_solution_reconstruction_data() const
{
    assert(!detached_xor_clauses && "Otherwise we need to extend to detached XORs too");

    std::ostringstream archive_stream;
    boost::archive::text_oarchive ar(archive_stream);
    ar << ok;
    if (ok) {
        uint32_t nvars = nVars();
        ar << nvars;
        ar << assigns;
        ar << inter_to_outerMain;
        ar << outer_to_interMain;
        ar << varData;
        ar << minNumVars;
        CNF::serialize(ar);
        occsimplifier->serialize_elimed_cls(ar);
        varReplacer->serialize_tables(ar);
    }
    return archive_stream.str();
}

void Solver::create_from_solution_reconstruction_data(const string& data)
{
    std::istringstream ss(data);
    boost::archive::text_iarchive ar(ss);
    ar >> ok;
    if (ok) {
        uint32_t nvars;
        ar >> nvars;
        new_vars(nvars);
        ar >> assigns;
        ar >> inter_to_outerMain;
        ar >> outer_to_interMain;
        ar >> varData;
        ar >> minNumVars;
        CNF::unserialize(ar);
        occsimplifier->unserialize_elimed_cls(ar);
        varReplacer->unserialize_tables(ar);
    }
}
#endif

pair<lbool, vector<lbool>> Solver::extend_minimized_model(const vector<lbool>& m)
{
    if (!ok) return make_pair(l_False, vector<lbool>());

    verb_print(3, "Size of m: " << m.size());
    verb_print(2, "Size of nVars(): " << nVars());

    assert (get_num_bva_vars() == 0 && "Otherwise we'd need to map outer to outside. Not impossible, but can't be bothered right now");
    assert(m.size() == nVars());

    for (uint32_t i = 0; i < nVars(); i++) {
        if (m[i] == l_Undef) {
            cout << "ERROR: the solution given does NOT contain a value for variable: " << i+1
                << " which was part of the minimized set of variables."
                << " This var corresponds to external: " << map_inter_to_outer(Lit(i, false))
                << endl;
            exit(-1);
        } else {
            verb_print(2, "OK, var " << i+1 \
                    << " set, which was part of the internal set of variables." \
                    << " This var corresponds to external: " << map_outer_to_inter(Lit(i, false)));
        }
    }

    // set values from model given
    for(size_t i = 0; i < m.size(); i++) {
        assigns[i] = m[i];
        assert(varData[i].removed == Removed::none);
    }

    // checking
    for(size_t i = 0; i < assigns.size(); i++) {
        if (varData[i].removed == Removed::none) {
            assert(assigns[i] != l_Undef);
        } else {
            assert(assigns[i] == l_Undef);
        }
    }
    model = assigns;
    updateArrayRev(model, inter_to_outerMain);

    SolutionExtender extender(this, occsimplifier);
    extender.extend();
    return make_pair(l_True, model);
}

// returns whether it can be removed
bool Solver::minimize_clause(vector<Lit>& cl) {
    assert(get_num_bva_vars() == 0);

    add_clause_helper(cl);
    new_decision_level();
    uint32_t i = 0;
    uint32_t j = 0;
    PropBy confl;

    for (uint32_t sz = cl.size(); i < sz; i++) {
        const Lit lit = cl[i];
        lbool val = value(lit);
        if (val == l_Undef) {
            enqueue<true>(~lit);
            cl[j++] = cl[i];
            confl = solver->propagate<true, true, true>();
            if (!confl.isnullptr()) break;
        } else if (val == l_False) {
        } else {
            assert(val == l_True);
            cl[j++] = cl[i];
            break;
        }
    }
    assert(solver->ok);
    cl.resize(j);
    cancelUntil<false, true>(0);
    map_inter_to_outer(cl);

    bool can_be_removed = !confl.isnullptr();
    return can_be_removed;
}

void Solver::copy_to_simp(SATSolver* s2)
{
    s2->new_vars(nVars());
    s2->set_verbosity(0);
    bool ret = true;
    start_getting_constraints(false, true);
    vector<Lit> lits;
    bool is_xor;
    bool rhs;
    while (ret) {
        ret = get_next_constraint(lits, is_xor, rhs);
        if (!ret) break;
        if (!is_xor) s2->add_clause(lits);
        else s2->add_xor_clause(lits, rhs);
    }
    end_getting_constraints();
}

bool Solver::check_clause_represented_by_xor(const Clause& cl) {
    for(const auto& l: cl) if (!seen[l.var()]) return false;

    bool rhs = true;
    for(const auto& l: cl) {seen2[l.var()] = 1; rhs ^= l.sign();}

    Lit minlit = *std::min_element(cl.begin(), cl.end());
    bool found = false;
    for(const auto& w: watches[minlit.unsign()]) {
        if (!w.isIdx()) continue;
        assert(w.isIdx());
        const Xor& x = xorclauses[w.get_idx()];
        if (x.size() != cl.size()) continue;
        if (x.rhs != rhs) continue;

        bool ret = true;
        for(const auto& v: x) if (!seen2[v]) {ret = false; break;}
        if (!ret) continue;
        if (ret) { found = true; break; }
    }

    for(const auto& l: cl) seen2[l.var()] = 0;
    return found;
}

// Detaches clauses that are the XORs
void Solver::detach_clauses_in_xors() {
    double my_time = cpuTime();
    SLOW_DEBUG_DO(check_no_idx_in_watchlist());

    // Setup
    uint32_t maxsize_xor = 0;
    std::set<uint32_t> xor_hashes;
    for(uint32_t i = 0; i < xorclauses.size(); i++) {
        const auto& x = xorclauses[i];
        maxsize_xor = std::max<uint32_t>(maxsize_xor, x.size());
        for(const uint32_t v: x) seen[v] = 1;
        xor_hashes.insert(hash_xcl(x));

        auto v = *std::min_element(x.begin(), x.end());
        watches[Lit(v, false)].push(Watched(i, WatchType::watch_idx_t));
        watches.smudge(Lit(v, false));
    }

    // Go through watchlist
    uint32_t deleted = 0;
    vector<ClOffset> delayed_clause_free;
    for(auto offs: longIrredCls) {
        Clause* cl = cl_alloc.ptr(offs);
        cl->stats.marked_clause = false;
        assert(!cl->freed());
        assert(!cl->get_removed());
        if (cl->size() <= maxsize_xor &&
                xor_hashes.count(hash_xcl(cl)) &&
                check_clause_represented_by_xor(*cl)) {
            detachClause(*cl);
            cl->stats.marked_clause = true;
            deleted++;
        }
    }

    if (deleted > 0) {
        uint32_t j = 0;
        for(uint32_t i = 0; i < longIrredCls.size(); i++) {
            ClOffset offs = longIrredCls[i];
            Clause* cl = cl_alloc.ptr(offs);
            if (!cl->stats.marked_clause) longIrredCls[j++] = offs;
        }
        longIrredCls.resize(j);

        for(ClOffset offset: delayed_clause_free) free_cl(offset);
        delayed_clause_free.clear();
    }
    assert(delayed_clause_free.empty());

    // Cleanup
    for(const auto& x: xorclauses) for(const uint32_t v: x) seen[v] = 0;
    solver->clean_occur_from_idx_types_only_smudged();
    verb_print(1, "[gauss] clauses deleted that are represented by XORs: " << deleted
        << " xorclauses: " << xorclauses.size()
        << " GJ matrices: " << gmatrices.size()
        << conf.print_times(cpuTime() - my_time));
}

bool Solver::removed_var_ext(uint32_t var) const {
    var = map_outer_to_inter(var);
    if (value(var) != l_Undef) return true;
    if (varData[var].removed != Removed::none) return true;
    return false;
}

void Solver::conclude_idrup (lbool result)
{
    if (!conf.idrup) return;
    if (result == l_True) {
      *frat << satisfiable;
      *frat << modelF;
      for (size_t cmVar = 0; cmVar < nVars(); ++cmVar) {
	lbool value = model_value(cmVar);
	*frat << Lit(cmVar, value != l_True);
      }
      *frat << fin;
    }
    else if (result == l_False) {
      *frat << unsatisfiable;
      *frat << unsatcore;
      for (auto x: get_final_conflict()) {
	*frat << ~x;
      }
      *frat << fin;
    }
    else
      *frat << unknown;
}

/* // This needs to be an AIG actually, with an order of what to calculate first. */
/* void Solver::get_var_map(vector<Lit>& var_map, map<uint32_t, bool>& var_set) const { */

#ifdef WEIGHTED
void Solver::get_weights(map<Lit,double>& weights,
        const vector<uint32_t>& sampl_vars,
        const vector<uint32_t>& orig_sampl_vars) const {
    assert(get_weighted());

    auto orig_sampl_vars_int = orig_sampl_vars;
    map_outer_to_inter(orig_sampl_vars_int);
    set<uint32_t> orig_sampl_set_int(orig_sampl_vars_int.begin(), orig_sampl_vars_int.end());

    auto sampl_vars_int = sampl_vars;
    map_outer_to_inter(sampl_vars_int);

    for(const uint32_t& var: sampl_vars_int) {
        Lit l = Lit(var, false);
        assert(var < nVars());
        mpz_class pos_weight = varData[var].pos_weight;
        mpz_class neg_weight = varData[var].neg_weight;

        // get all variables var is replacing
        auto vars = varReplacer->get_vars_replacing(var);
        for(auto v: vars) {
            if (orig_sampl_set_int.count(v) == 0) continue;
            assert(var == varReplacer->get_lit_replaced_with(Lit(v, false)).var());
            if (varReplacer->get_lit_replaced_with(Lit(v, false)) == Lit(var, false)) {
                pos_weight += varData[v].pos_weight;
                neg_weight += varData[v].neg_weight;
            } else {
                pos_weight += varData[v].neg_weight;
                neg_weight += varData[v].pos_weight;
            }
        }
        weights[map_inter_to_outer(l)] = pos_weight;
        weights[map_inter_to_outer(~l)] = neg_weight;
    }
}
#endif

#ifdef STATS_NEEDED
void Solver::dump_clauses_at_finishup_as_last()
{
    if (!sqlStats)
        return;

    for(auto& red_cls: longRedCls) {
        for(auto& offs: red_cls) {
            Clause* cl = cl_alloc.ptr(offs);
            if (cl->stats.is_tracked) {
                ClauseStatsExtra& stats_extra = solver->red_stats_extra[cl->stats.extra_pos];
                sqlStats->cl_last_in_solver(solver, stats_extra.orig_ID);
            }
        }
    }
}
#endif
