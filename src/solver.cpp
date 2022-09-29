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
#include "xorfinder.h"
#include "cardfinder.h"
#include "sls.h"
#include "matrixfinder.h"
#include "lucky.h"
#include "get_clause_query.h"
#include "community_finder.h"
#include "oracle/oracle.hpp"
extern "C" {
#include "picosat/picosat.h"
}

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

//#define FRAT_DEBUG

//#define DEBUG_RENUMBER

//#define DEBUG_IMPLICIT_PAIRS_TRIPLETS

Solver::Solver(const SolverConf *_conf, std::atomic<bool>* _must_interrupt_inter) :
    Searcher(_conf, this, _must_interrupt_inter)
{
    sqlStats = NULL;
    intree = new InTree(this);

#ifdef USE_BREAKID
    if (conf.doBreakid) {
        breakid = new BreakID(this);
    }
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
    datasync = new DataSync(this, NULL);
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

void Solver::set_shared_data(SharedData* shared_data)
{
    datasync->set_shared_data(shared_data);
}

bool Solver::add_xor_clause_inter(
    const vector<Lit>& lits
    , bool rhs
    , const bool attach
    , bool addDrat
    , bool red
) {
    VERBOSE_PRINT("add_xor_clause_inter: " << lits << " rhs: " << rhs);
    assert(ok);
    assert(!attach || qhead == trail.size());
    assert(decisionLevel() == 0);

    vector<Lit> ps(lits);
    for(Lit& lit: ps) {
        if (lit.sign()) {
            rhs ^= true;
            lit ^= true;
        }
    }
    clean_xor_no_prop(ps, rhs);

    if (ps.size() >= (0x01UL << 28)) throw CMSat::TooLongClauseError();

    if (ps.empty()) {
        if (rhs) {
            *frat << add << ++clauseID << fin;
            ok = false;
        }
        return ok;
    }

    ps[0] ^= rhs;

    //cout << "without rhs is: " << ps << endl;
    add_every_combination_xor(ps, attach, addDrat, red);
    if (ps.size() > 2) {
        xor_clauses_updated = true;
        xorclauses.push_back(Xor(ps, rhs, tmp_xor_clash_vars));
        xorclauses_orig.push_back(Xor(ps, rhs, tmp_xor_clash_vars));
        TBUDDY_DO(if (frat->enabled()) xorclauses.back().create_bdd_xor());
        TBUDDY_DO(if (frat->enabled()) xorclauses_orig.back().create_bdd_xor());
    }

    return ok;
}

void Solver::add_every_combination_xor(
    const vector<Lit>& lits
    , const bool attach
    , const bool addDrat
    , const bool red
) {
    VERBOSE_PRINT("add_every_combination: " << lits);

    size_t at = 0;
    size_t num = 0;
    vector<Lit> xorlits;
    tmp_xor_clash_vars.clear();
    Lit lastlit_added = lit_Undef;
    while(at != lits.size()) {
        xorlits.clear();
        size_t last_at = at;
        for(; at < last_at+conf.xor_var_per_cut && at < lits.size(); at++) {
            xorlits.push_back(lits[at]);
        }

        //Connect to old cut
        if (lastlit_added != lit_Undef) {
            xorlits.push_back(lastlit_added);
        } else if (at < lits.size()) {
            xorlits.push_back(lits[at]);
            at++;
        }

        if (at + 1 == lits.size()) {
            xorlits.push_back(lits[at]);
            at++;
        }

        //New lit to connect to next cut
        if (at != lits.size()) {
            new_var(true);
            const uint32_t newvar = nVars()-1;
            tmp_xor_clash_vars.push_back(newvar);
            const Lit toadd = Lit(newvar, false);
            xorlits.push_back(toadd);
            lastlit_added = toadd;
        }

        add_xor_clause_inter_cleaned_cut(xorlits, attach, addDrat, red);
        if (!ok)
            break;

        num++;
    }
}

void Solver::add_xor_clause_inter_cleaned_cut(
    const vector<Lit>& lits
    , const bool attach
    , const bool addDrat
    , const bool red
) {
    VERBOSE_PRINT("add_xor_clause_inter_cleaned_cut: " << lits);
    vector<Lit> new_lits;
    for(size_t i = 0; i < (1ULL<<lits.size()); i++) {
        unsigned bits_set = num_bits_set(i, lits.size());
        if (bits_set % 2 == 0) {
            continue;
        }

        new_lits.clear();
        for(size_t at = 0; at < lits.size(); at++) {
            bool xorwith = (i >> at)&1;
            new_lits.push_back(lits[at] ^ xorwith);
        }
        //cout << "Added. " << new_lits << endl;
        Clause* cl = add_clause_int(
            new_lits, //lits
            red, //redundant?
            NULL, //clause stats
            attach, //attach it?
            NULL,  //get back the final set of literals
            addDrat //add to FRAT?
        );

        if (cl) {
            cl->set_used_in_xor(true);
            cl->set_used_in_xor_full(true);
            if (red) {
                longRedCls[2].push_back(cl_alloc.get_offset(cl));
            } else {
                longIrredCls.push_back(cl_alloc.get_offset(cl));
            }
        }

        if (!ok)
            return;
    }
}

unsigned Solver::num_bits_set(const size_t x, const unsigned max_size) const
{
    unsigned bits_set = 0;
    for(size_t i = 0; i < max_size; i++) {
        if ((x>>i)&1) {
            bits_set++;
        }
    }

    return bits_set;
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

            if (!fresh_solver && varData[p.var()].removed != Removed::none) {
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
    , bool addDrat
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
        if (finalLits) {
            finalLits->clear();
        }
        if (remove_frat) {
            *frat << del << cl_stats->ID << lits << fin;
        }
        return NULL;
    }
    VERBOSE_PRINT("add_clause_int final clause: " << ps);

    //If caller required final set of lits, return it.
    if (finalLits) *finalLits = ps;

    int32_t ID;
    if (remove_frat) {
        assert(cl_stats);
        assert(frat_first == lit_Undef);
        assert(addDrat);
        ID = cl_stats->ID;
        if (ps != lits) {
            ID = ++clauseID;
            *frat << add << ID << ps << fin;
            *frat << del << cl_stats->ID << lits << fin;
        }
    } else {
        ID = ++clauseID;
        if (addDrat) {
            size_t i = 0;
            if (frat_first != lit_Undef) {
                assert(ps.size() > 0);
                if (frat_first != lit_Undef) {
                    for(i = 0; i < ps.size(); i++) {
                        if (ps[i] == frat_first) {
                            break;
                        }
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
            assert(unsat_cl_ID == 0);
            unsat_cl_ID = clauseID;
            ok = false;
            if (conf.verbosity >= 6) {
                cout
                << "c solver received clause through addClause(): "
                << lits
                << " that became an empty clause at toplevel --> UNSAT"
                << endl;
            }
            return NULL;
        case 1:
            assert(decisionLevel() == 0);
            enqueue<false>(ps[0]);
            *frat << del << ID << ps[0] << fin; // double unit delete
            if (attach_long) {
                ok = (propagate<true>().isNULL());
            }

            return NULL;
        case 2:
            attach_bin_clause(ps[0], ps[1], red, ID);
            return NULL;

        default:
            Clause* c = cl_alloc.Clause_new(ps, sumConflicts, ID);
            c->isRed = red;
            if (cl_stats) {
                c->stats = *cl_stats;
                STATS_DO(if (ID != c->stats.ID && sqlStats && c->stats.is_tracked) sqlStats->update_id(c->stats.ID, ID));
                c->stats.ID = ID;
            }
            if (red && cl_stats == NULL) {
                assert(false && "does this happen at all? should it happen??");
                #if defined(STATS_NEEDED) || defined(FINAL_PREDICTOR)
                //TODO red_stats_extra setup: glue, size, introduced_at_conflict
                #endif
            }

            //In class 'OccSimplifier' we don't need to attach normall
            if (attach_long) {
                attachClause(*c);
            } else {
                if (red) {
                    litStats.redLits += ps.size();
                } else {
                    litStats.irredLits += ps.size();
                }
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

            if (!fresh_solver && varData[p.var()].removed != Removed::none) {
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
        if (cl != NULL) {
            longIrredCls.push_back(cl_alloc.get_offset(cl));
        }
        return true;
    }

    if (!bnn.set && bnn.cutoff == 1) {
        lits.clear();
        lits.insert(lits.end(), bnn.begin(), bnn.end());
        lits.push_back(~bnn.out);
        Clause* cl = add_clause_int(lits);
        if (cl != NULL) {
            longIrredCls.push_back(cl_alloc.get_offset(cl));
        }
        for(Lit l: bnn) {
            lits.clear();
            lits.push_back(~l);
            lits.push_back(bnn.out);
            Clause* cl2 = add_clause_int(lits);
            assert(cl2 == NULL);
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
        if (cl != NULL) {
            longIrredCls.push_back(cl_alloc.get_offset(cl));
        }
        for(const Lit& l: bnn) {
            lits.clear();
            lits.push_back(l);
            lits.push_back(~bnn.out);
            Clause* cl2 = add_clause_int(lits);
            assert(cl2 == NULL);
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
                if (cl2 != NULL)
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
        bnn = NULL;
    }

    if (bnn != NULL) {
        assert(check_bnn_sane(*bnn));
        if (bnn_to_cnf(*bnn)) {
            free(bnn);
            bnn = NULL;
        } else {
            bnns.push_back(bnn);
            attach_bnn(bnns.size()-1);
        }
    }
    ok = propagate<true>().isNULL();
}

void Solver::attachClause(
    const Clause& cl
    , const bool checkAttach
) {
    #if defined(FRAT_DEBUG)
    if (frat) {
        *frat << add << cl << fin;
    }
    #endif

    //Update stats
    if (cl.red()) {
        litStats.redLits += cl.size();
    } else {
        litStats.irredLits += cl.size();
    }

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
    if (red) {
        binTri.redBins++;
    } else {
        binTri.irredBins++;
    }

    //Call Solver's function for heavy-lifting
    PropEngine::attach_bin_clause(lit1, lit2, red, ID, checkUnassignedFirst);
}

void Solver::detachClause(const Clause& cl, const bool removeDrat)
{
    if (removeDrat) {
        *frat << del << cl << fin;
    }

    assert(cl.size() > 2);
    detach_modified_clause(cl[0], cl[1], cl.size(), &cl);
}

void Solver::detachClause(const ClOffset offset, const bool removeDrat)
{
    Clause* cl = cl_alloc.ptr(offset);
    detachClause(*cl, removeDrat);
}

void Solver::detach_modified_clause(
    const Lit lit1
    , const Lit lit2
    , const uint32_t origSize
    , const Clause* address
) {
    //Update stats
    if (address->red())
        litStats.redLits -= origSize;
    else
        litStats.irredLits -= origSize;

    //Call heavy-lifter
    PropEngine::detach_modified_clause(lit1, lit2, address);
}

//Takes OUTSIDE variables and makes them INTERNAL, replaces them, etc.
bool Solver::addClauseHelper(vector<Lit>& ps)
{
    //If already UNSAT, just return
    if (!ok)
        return false;

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
            std::cerr
            << "ERROR: Variable " << lit.var() + 1
            << " inserted, but max var is "
            << nVarsOuter()
            << endl;
            std::exit(-1);
        }

        //Undo var replacement
        if (!fresh_solver) {
            const Lit updated_lit = varReplacer->get_lit_replaced_with_outer(lit);
            if (conf.verbosity >= 12
                && lit != updated_lit
            ) {
                cout
                << "EqLit updating outer lit " << lit
                << " to outer lit " << updated_lit
                << endl;
            }
            lit = updated_lit;

            //Map outer to inter, and add re-variable if need be
            if (map_outer_to_inter(lit).var() >= nVars()) {
                new_var(false, lit.var(), false);
            }
        }
    }

    if (!fresh_solver)
        renumber_outer_to_inter_lits(ps);

    #ifdef SLOW_DEBUG
    //Check renumberer
    for (const Lit lit: ps) {
        const Lit updated_lit = varReplacer->get_lit_replaced_with(lit);
        assert(lit == updated_lit);
    }
    #endif

    //Uneliminate vars
    if (!fresh_solver
        && (get_num_vars_elimed() > 0 || detached_xor_clauses)
    ) {
        for (const Lit lit: ps) {
            if (detached_xor_clauses
                && varData[lit.var()].removed == Removed::clashed
            ) {
                if (!fully_undo_xor_detach()) return false;
                assert(varData[lit.var()].removed == Removed::none);
            }

            if (conf.perform_occur_based_simp
                && varData[lit.var()].removed == Removed::elimed
            ) {
                if (!occsimplifier->uneliminate(lit.var())) return false;
            }
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

bool Solver::add_clause_outer_copylits(const vector<Lit>& lits)
{
    vector<Lit> ps = lits;
    return Solver::add_clause_outer(ps);
}

// Takes OUTER (NOT *outside*) variables
// Input is ORIGINAL clause.
bool Solver::add_clause_outer(vector<Lit>& ps)
{
    if (conf.perform_occur_based_simp && occsimplifier->getAnythingHasBeenBlocked()) {
        std::cerr
        << "ERROR: Cannot add new clauses to the system if blocking was"
        << " enabled. Turn it off from conf.doBlockClauses"
        << endl;
        std::exit(-1);
    }

    ClauseStats stats;
    stats.ID = ++clauseID;
    *frat << origcl << stats.ID << ps << fin;

    #ifdef VERBOSE_DEBUG
    cout << "Adding clause " << ps << endl;
    #endif //VERBOSE_DEBUG
    const size_t origTrailSize = trail.size();

    if (!addClauseHelper(ps)) {
        *frat << del << stats.ID << ps << fin;
        return false;
    }

    std::sort(ps.begin(), ps.end());
    Clause *cl = add_clause_int(
        ps
        , false //redundant?
        , &stats
        , true //yes, attach
        , NULL
        , true //add frat?
        , lit_Undef
        , true //sorted
        , true //remove old clause from proof if we changed it
    );

    if (cl != NULL) {
        ClOffset offset = cl_alloc.get_offset(cl);
        longIrredCls.push_back(offset);
    }

    zeroLevAssignsByCNF += trail.size() - origTrailSize;

    return ok;
}

void Solver::test_renumbering() const
{
    //Check if we renumbered the variables in the order such as to make
    //the unknown ones first and the known/eliminated ones second
    bool uninteresting = false;
    bool problem = false;
    for(size_t i = 0; i < nVars(); i++) {
        //cout << "val[" << i << "]: " << value(i);

        if (value(i)  != l_Undef)
            uninteresting = true;

        if (varData[i].removed == Removed::elimed
            || varData[i].removed == Removed::replaced
        ) {
            uninteresting = true;
            //cout << " removed" << endl;
        } else {
            //cout << " non-removed" << endl;
        }

        if (value(i) == l_Undef
            && varData[i].removed != Removed::elimed
            && varData[i].removed != Removed::replaced
            && uninteresting
        ) {
            problem = true;
        }
    }
    assert(!problem && "We renumbered the variables in the wrong order!");
}

void Solver::renumber_clauses(const vector<uint32_t>& outerToInter)
{
    //Clauses' abstractions have to be re-calculated
    for(ClOffset offs: longIrredCls) {
        Clause* cl = cl_alloc.ptr(offs);
        updateLitsMap(*cl, outerToInter);
        cl->setStrenghtened();
    }

    for(auto& lredcls: longRedCls) {
        for(ClOffset off: lredcls) {
            Clause* cl = cl_alloc.ptr(off);
            updateLitsMap(*cl, outerToInter);
            cl->setStrenghtened();
        }
    }

    //Clauses' abstractions have to be re-calculated
    xor_clauses_updated = true;
    for(Xor& x: xorclauses) {
        updateVarsMap(x.vars, outerToInter);
        updateVarsMap(x.clash_vars, outerToInter);
    }

    for(Xor& x: xorclauses_unused) {
        updateVarsMap(x.vars, outerToInter);
        updateVarsMap(x.clash_vars, outerToInter);
    }

    for(Xor& x: xorclauses_orig) {
        updateVarsMap(x.vars, outerToInter);
        updateVarsMap(x.clash_vars, outerToInter);
    }

    for(auto& v: removed_xorclauses_clash_vars) {
        v = getUpdatedVar(v, outerToInter);
    }

    for(auto& bnn: bnns) {
        if (bnn == NULL) {
            continue;
        }
        assert(!bnn->isRemoved);
        updateLitsMap(*bnn, outerToInter);
        if (!bnn->set) {
            bnn->out = getUpdatedLit(bnn->out, outerToInter);
        }
    }
}

size_t Solver::calculate_interToOuter_and_outerToInter(
    vector<uint32_t>& outerToInter
    , vector<uint32_t>& interToOuter
) {
    size_t at = 0;
    vector<uint32_t> useless;
    size_t numEffectiveVars = 0;
    for(size_t i = 0; i < nVars(); i++) {
        if (value(i) != l_Undef
            || varData[i].removed == Removed::elimed
            || varData[i].removed == Removed::replaced
        ) {
            useless.push_back(i);
            continue;
        }

        outerToInter[i] = at;
        interToOuter[at] = i;
        at++;
        numEffectiveVars++;
    }

    //Fill the rest with variables that have been removed/eliminated/set
    for(vector<uint32_t>::const_iterator
        it = useless.begin(), end = useless.end()
        ; it != end
        ; ++it
    ) {
        outerToInter[*it] = at;
        interToOuter[at] = *it;
        at++;
    }
    assert(at == nVars());

    //Extend to nVarsOuter() --> these are just the identity transformation
    for(size_t i = nVars(); i < nVarsOuter(); i++) {
        outerToInter[i] = i;
        interToOuter[i] = i;
    }

    return numEffectiveVars;
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
    #ifdef SLOWDEBUG
    for(const auto& x: xorclauses) for(const auto& v: x) assert(v < nVars());
    for(const auto& x: xorclauses_unused) for(const auto& v: x) assert(v < nVars());
    for(const auto& x: xorclauses_orig) for(const auto& v: x) assert(v < nVars());
    #endif

    if (nVars() == 0) return okay();
    if (!must_renumber && calc_renumber_saving() < 0.2) return okay();
    if (!clear_gauss_matrices()) return false;

    double myTime = cpuTime();
    if (!clauseCleaner->remove_and_clean_all()) return false;

    //outerToInter[10] = 0 ---> what was 10 is now 0.
    vector<uint32_t> outerToInter(nVarsOuter());
    vector<uint32_t> interToOuter(nVarsOuter());

    size_t numEffectiveVars =
        calculate_interToOuter_and_outerToInter(outerToInter, interToOuter);

    //Create temporary outerToInter2
    vector<uint32_t> interToOuter2(nVarsOuter()*2);
    for(size_t i = 0; i < nVarsOuter(); i++) {
        interToOuter2[i*2] = interToOuter[i]*2;
        interToOuter2[i*2+1] = interToOuter[i]*2+1;
    }

    renumber_clauses(outerToInter);
    CNF::updateVars(outerToInter, interToOuter, interToOuter2);
    PropEngine::updateVars(outerToInter, interToOuter);
    Searcher::updateVars(outerToInter, interToOuter);
#ifdef USE_BREAKID
    if (breakid) {
        breakid->updateVars(outerToInter, interToOuter);
    }
#endif

    //Update sub-elements' vars
    varReplacer->updateVars(outerToInter, interToOuter);
    datasync->updateVars(outerToInter, interToOuter);

    //Tests
    test_renumbering();
    test_reflectivity_of_renumbering();

    //Print results
    const double time_used = cpuTime() - myTime;
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

    if (conf.doSaveMem) {
        save_on_var_memory(numEffectiveVars);
    }

    #ifdef SLOWDEBUG
    for(const auto& x: xorclauses) {
        for(const auto& v: x.vars) {
            assert(v < nVars());
        }
    }

    for(const auto& x: xorclauses_unused) {
        for(const auto& v: x.vars) {
            assert(v < nVars());
        }
    }
    #endif

    //NOTE order heap is now wrong, but that's OK, it will be restored from
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

    const double myTime = cpuTime();
    minNumVars = newNumVars;
    Searcher::save_on_var_memory();

    varReplacer->save_on_var_memory();
    if (occsimplifier) {
        occsimplifier->save_on_var_memory();
    }
    datasync->save_on_var_memory();

    const double time_used = cpuTime() - myTime;
    if (sqlStats) {
        sqlStats->time_passed_min(
            this
            , "save var mem"
            , time_used
        );
    }
    //print_mem_stats();
}

void Solver::set_assumptions()
{
    assert(assumptions.empty());
    #ifdef SLOW_DEBUG
    for(auto x: varData) {
        assert(x.assumption == l_Undef);
    }
    #endif

    conflict.clear();
    if (get_num_bva_vars() > 0) {
        back_number_from_outside_to_outer(outside_assumptions);
        inter_assumptions_tmp = back_number_from_outside_to_outer_tmp;
    } else {
        inter_assumptions_tmp = outside_assumptions;
    }
    addClauseHelper(inter_assumptions_tmp);
    assert(inter_assumptions_tmp.size() == outside_assumptions.size());

    assumptions.resize(inter_assumptions_tmp.size());
    for(size_t i = 0; i < inter_assumptions_tmp.size(); i++) {
        Lit outside_lit = lit_Undef;
        const Lit inter_lit = inter_assumptions_tmp[i];
        if (i < outside_assumptions.size()) {
            outside_lit = outside_assumptions[i];
        }

        const Lit outer_lit = map_inter_to_outer(inter_lit);
        assumptions[i] = AssumptionPair(outer_lit, outside_lit);
    }

    fill_assumptions_set();
}

void Solver::add_assumption(const Lit assump)
{
    assert(varData[assump.var()].assumption == l_Undef);
    assert(varData[assump.var()].removed == Removed::none);
    assert(value(assump) == l_Undef);

    Lit outer_lit = map_inter_to_outer(assump);
    assumptions.push_back(AssumptionPair(outer_lit, lit_Undef));
    varData[assump.var()].assumption = assump.sign() ? l_False : l_True;
}

void Solver::check_model_for_assumptions() const
{
    for(const AssumptionPair& lit_pair: assumptions) {
        const Lit outside_lit = lit_pair.lit_orig_outside;
        if (outside_lit.var() == var_Undef) {
            //This is an assumption that is a BVA variable
            //Currently, this can only be BreakID
            continue;
        }
        assert(outside_lit.var() < model.size());

        if (model_value(outside_lit) == l_Undef) {
            std::cerr
            << "ERROR, lit " << outside_lit
            << " was in the assumptions, but it wasn't set at all!"
            << endl;
        }
        assert(model_value(outside_lit) != l_Undef);

        if (model_value(outside_lit) != l_True) {
            std::cerr
            << "ERROR, lit " << outside_lit
            << " was in the assumptions, but it was set to: "
            << model_value(outside_lit)
            << endl;
        }
        assert(model_value(outside_lit) == l_True);
    }
}

void Solver::check_recursive_minimization_effectiveness(const lbool status)
{
    const SearchStats& srch_stats = Searcher::get_stats();
    if (status == l_Undef
        && conf.doRecursiveMinim
        && srch_stats.recMinLitRem + srch_stats.litsRedNonMin > 100000
    ) {
        double remPercent =
            float_div(srch_stats.recMinLitRem, srch_stats.litsRedNonMin)*100.0;

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

void Solver::extend_solution(const bool only_sampling_solution)
{
    #ifdef DEBUG_IMPLICIT_STATS
    check_stats();
    #endif

    #ifdef SLOW_DEBUG
    //Check that sampling vars are all assigned
    if (conf.sampling_vars) {
        for(uint32_t outside_var: *conf.sampling_vars) {
            uint32_t outer_var = map_to_with_bva(outside_var);
            outer_var = varReplacer->get_var_replaced_with_outer(outer_var);
            uint32_t int_var = map_outer_to_inter(outer_var);

            assert(varData[int_var].removed == Removed::none);

            if (int_var < nVars() && varData[int_var].removed == Removed::none) {
                assert(model[int_var] != l_Undef);
            }
        }
    }
    #endif

    if (detached_xor_clauses && !only_sampling_solution) {
        extend_model_to_detached_xors();
    }

    const double myTime = cpuTime();
    updateArrayRev(model, interToOuterMain);

    if (!only_sampling_solution) {
        SolutionExtender extender(this, occsimplifier);
        extender.extend();
    } else {
        varReplacer->extend_model_already_set();
    }

    //map back without BVA
    if (get_num_bva_vars() != 0) {
        model = map_back_vars_to_without_bva(model);
    }

    if (only_sampling_solution && conf.sampling_vars) {
        for(uint32_t var: *conf.sampling_vars) {
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
    if (sqlStats) {
        sqlStats->time_passed_min(
            this
            , "extend solution"
            , cpuTime()-myTime
        );
    }
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
    if (conf.xor_var_per_cut < 1) {
        std::cerr << "ERROR: Too low cutting number: " << conf.xor_var_per_cut << ". Needs to be at least 1." << endl;
        exit(-1);
    }

    if (MAX_XOR_RECOVER_SIZE < 4) {
        std::cerr << "ERROR: MAX_XOR_RECOVER_SIZE  must be at least 4. It's currently: " << MAX_XOR_RECOVER_SIZE << endl;
        exit(-1);
    }

    if (conf.xor_var_per_cut+2 > MAX_XOR_RECOVER_SIZE) {
        std::cerr << "ERROR: Too high cutting number, we will not be able to recover cut XORs due to MAX_XOR_RECOVER_SIZE only being " << MAX_XOR_RECOVER_SIZE << endl;
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

    if ((frat->enabled() || conf.simulate_frat))  {
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

    #ifdef SLOW_DEBUG
    if (conf.sampling_vars)
    {
        for(uint32_t v: *conf.sampling_vars) {
            assert(v < nVarsOutside());
        }
    }
    #endif

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
        find_all_attach();
        test_all_clause_attached();
    }
    #endif

    conf.global_timeout_multiplier = conf.orig_global_timeout_multiplier;
    solveStats.num_simplify_this_solve_call = 0;
    set_assumptions();

    lbool status = l_Undef;
    if (!ok) {
        status = l_False;
        goto end;
    }
    check_and_upd_config_parameters();
    datasync->rebuild_bva_map();
    #ifdef USE_BREAKID
    if (breakid) {
        breakid->start_new_solving();
    }
    #endif

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

void Solver::reset_for_solving()
{
    longest_trail_ever_best = 0;
    longest_trail_ever_inv = 0;
    fresh_solver = false;
    set_assumptions();
    #ifdef SLOW_DEBUG
    if (ok) {
        assert(check_order_heap_sanity());
        check_implicit_stats();
        find_all_attach();
        check_no_duplicate_lits_anywhere();
    }
    #endif

    solveStats.num_solve_calls++;
    check_and_upd_config_parameters();

    //Reset parameters
    luby_loop_num = 0;
    conf.global_timeout_multiplier = conf.orig_global_timeout_multiplier;
    solveStats.num_simplify_this_solve_call = 0;
    if (conf.verbosity >= 6) {
        cout << "c " << __func__ << " called" << endl;
    }
    datasync->rebuild_bva_map();
}

void my_bddinthandler(int e)
{
    switch(e) {
        case -1:  cout << "ERROR reported by tbuddy: BDD_MEMORY (-1)   /* Out of memory */" << endl; break;
        case -2:  cout << "ERROR reported by tbuddy: VAR (-2)      /* Unknown variable */" << endl; break;
        case -3:  cout << "ERROR reported by tbuddy: RANGE (-3)    /* Variable value out of range (not in domain) */" << endl; break;
        case -4:  cout << "ERROR reported by tbuddy: DEREF (-4)    /* Removing external reference to unknown node */" << endl; break;
        case -5:  cout << "ERROR reported by tbuddy: RUNNING (-5)  /* Called bdd_init() twice whithout bdd_done() */" << endl; break;
        case -6:  cout << "ERROR reported by tbuddy: FILE (-6)     /* Some file operation failed */" << endl; break;
        case -7:  cout << "ERROR reported by tbuddy: FORMAT (-7)   /* Incorrect file format */" << endl; break;
        case -8:  cout << "ERROR reported by tbuddy: ORDER (-8)    /* Vars. not in order for vector based functions */" << endl; break;
        case -9:  cout << "ERROR reported by tbuddy: BREAK (-9)    /* User called break */" << endl; break;
        case -10: cout << "ERROR reported by tbuddy: VARNUM (-10)  /* Different number of vars. for vector pair */" << endl; break;
        case -11: cout << "ERROR reported by tbuddy: NODES (-11)   /* Tried to set max. number of nodes to be fewer than there already has been allocated */" << endl; break;
        case -12: cout << "ERROR reported by tbuddy: BDD_OP (-12)      /* Unknown operator */" << endl; break;
        case -13: cout << "ERROR reported by tbuddy: BDD_VARSET (-13)  /* Illegal variable set */" << endl; break;
        case -14: cout << "ERROR reported by tbuddy: BDD_VARBLK (-14)  /* Bad variable block operation */" << endl; break;
        case -15: cout << "ERROR reported by tbuddy: BDD_DECVNUM (-15) /* Trying to decrease the number of variables */" << endl; break;
        case -16: cout << "ERROR reported by tbuddy: BDD_REPLACE (-16) /* Replacing to already existing variables */" << endl; break;
        case -17: cout << "ERROR reported by tbuddy: BDD_NODENUM (-17) /* Number of nodes reached user defined maximum */" << endl; break;
        case -18: cout << "ERROR reported by tbuddy: BDD_ILLBDD (-18)  /* Illegal bdd argument */" << endl; break;
        case -19: cout << "ERROR reported by tbuddy: BDD_SIZE (-19)    /* Illegal size argument */" << endl; break;

        case -20: cout << "ERROR reported by tbuddy: BVEC_SIZE (-20)    /* Mismatch in bitvector size */" << endl; break;
        case -21: cout << "ERROR reported by tbuddy: BVEC_SHIFT (-21)   /* Illegal shift-left/right parameter */" << endl; break;
        case -22: cout << "ERROR reported by tbuddy: BVEC_DIVZERO (-22) /* Division by zero */" << endl; break;


        case -23: cout << "ERROR reported by tbuddy: ILIST_ALLOC (-23)  /* Invalid allocation for ilist */" << endl; break;
        case -24: cout << "ERROR reported by tbuddy: TBDD_PROOF (-24)   /* Couldn't complete proof of justification */" << endl; break;
        case -26: cout << "ERROR reported by tbuddy: BDD_ERRNUM 26 /* ?? */" << endl; break;
    }

    assert(false);
}

lbool Solver::solve_with_assumptions(
    const vector<Lit>* _assumptions,
    const bool only_sampling_solution
) {
    if (frat->enabled()) {
        frat->set_sqlstats_ptr(sqlStats);
        int32_t* v = new int;
        *v = nVars()+1;
        #ifdef USE_TBUDDY
        if (frat->enabled()) {
            frat->flush();
            tbdd_init_frat(frat->getFile(), v, &clauseID);
            tbdd_set_verbose(0);
            bdd_error_hook(my_bddinthandler);
        }
        #endif
    }
    move_to_outside_assumps(_assumptions);
    reset_for_solving();

    //Check if adding the clauses caused UNSAT
    lbool status = l_Undef;
    if (!ok) {
        assert(conflict.empty());
        status = l_False;
        if (conf.verbosity >= 6) {
            cout << "c Solver status " << status << " on startup of solve()" << endl;
        }
        goto end;
    }
    assert(prop_at_head());
    assert(okay());
    #ifdef USE_BREAKID
    if (breakid) breakid->start_new_solving();
    #endif

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
    if (_assumptions == NULL || _assumptions->empty()) {
        #ifdef USE_BREAKID
        if (assumptions.empty()) {
            verb_print(1, "[breakid] Under BreakID it's UNSAT. Assumed lit: " << breakid->get_assumed_lit());
        } else
        #endif
        {
            if (status == l_False) {
                assert(!okay());
            }
        }
    }

    write_final_frat_clauses();

    return status;
}

void Solver::write_final_frat_clauses()
{
    if (!frat->enabled()) return;
    assert(decisionLevel() == 0);
    *frat << "write final start\n";

    *frat << "vrepl finalize begin\n";
    if (varReplacer) varReplacer->delete_frat_cls();

    *frat << "gmatrix finalize frat begin\n";
    TBUDDY_DO(for(auto& g: gmatrices) g->finalize_frat());

    *frat << "free bdds begin\n";
    TBUDDY_DO(solver->free_bdds(solver->xorclauses_orig));
    TBUDDY_DO(solver->free_bdds(solver->xorclauses));
    TBUDDY_DO(solver->free_bdds(solver->xorclauses_unused));


    *frat << "tbdd_done() next\n";
    frat->flush();
    TBUDDY_DO(tbdd_done());

    // -1 indicates tbuddy already added the empty clause
    *frat << "empty clause next (if we found it)\n";
    if (!okay() && unsat_cl_ID != -1) {
        assert(unsat_cl_ID != 0);
        *frat << finalcl << unsat_cl_ID << fin;
    }

    *frat << "finalization of unit clauses next\n";
    for(uint32_t i = 0; i < nVars(); i ++) {
        if (unit_cl_IDs[i] != 0) {
            assert(value(i) != l_Undef);
            Lit l = Lit(i, value(i) == l_False);
            *frat << finalcl << unit_cl_IDs[i] << l << fin;
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
        for(const auto offs: cls) {
            Clause* cl = cl_alloc.ptr(offs);
            *frat << finalcl << *cl << fin;
        }
    }
    *frat << "finalization of irredundant clauses next\n";
    for(const auto& offs: longIrredCls) {
        Clause* cl = cl_alloc.ptr(offs);
        *frat << finalcl << *cl << fin;
    }
    frat->flush();
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

        sqlStats->mem_used(
            this
            , "xor"
            , my_time
            , occsimplifier->mem_used_xor()/(1024*1024)
        );

        sqlStats->mem_used(
            this
            , "bva"
            , my_time
            , occsimplifier->mem_used_bva()/(1024*1024)
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


lbool Solver::iterate_until_solved()
{
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
        if (status != l_Undef) {
            break;
        }

        //If we are over the limit, exit
        if (sumConflicts >= conf.max_confl
            || cpuTime() > conf.maxTime
            || must_interrupt_asap()
        ) {
            break;
        }

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

void Solver::handle_found_solution(const lbool status, const bool only_sampling_solution)
{
    double mytime = cpuTime();
    if (status == l_True) {
        extend_solution(only_sampling_solution);
        cancelUntil(0);
        assert(prop_at_head());

        #ifdef DEBUG_ATTACH_MORE
        find_all_attach();
        test_all_clause_attached();
        #endif
    } else if (status == l_False) {
        cancelUntil(0);

        for(const Lit lit: conflict) {
            if (value(lit) == l_Undef) {
                assert(var_inside_assumptions(lit.var()) != l_Undef);
            }
        }
        if (conf.conf_needed) {
            update_assump_conflict_to_orig_outside(conflict);
        }
    }

    #ifdef USE_BREAKID
    if (breakid) {
        breakid->finished_solving();
    }
    #endif

    //Too slow when running lots of small queries
    #ifdef DEBUG_IMPLICIT_STATS
    check_implicit_stats();
    #endif

    if (sqlStats) {
        sqlStats->time_passed_min(this, "solution extend", cpuTime() - mytime);
    }
}

lbool Solver::execute_inprocess_strategy(
    const bool startup
    , const string& strategy
) {
    //std::string input = "abc,def,ghi";
    std::istringstream ss(strategy + ", ");
    std::string token;
    std::string occ_strategy_tokens;

    while(std::getline(ss, token, ',')) {
        if (sumConflicts >= conf.max_confl
            || cpuTime() > conf.maxTime
            || must_interrupt_asap()
            || nVars() == 0
            || !okay()
        ) {
            break;
        }

        assert(watches.get_smudged_list().empty());
        assert(prop_at_head());
        assert(okay());
        #ifdef SLOW_DEBUG
        check_no_zero_ID_bins();
        check_wrong_attach();
        check_stats();
        check_no_duplicate_lits_anywhere();
        check_assumptions_sanity();
        #endif

        token = trim(token);
        std::transform(token.begin(), token.end(), token.begin(), ::tolower);
        if (!occ_strategy_tokens.empty() && token.substr(0,3) != "occ") {
            if (conf.perform_occur_based_simp
                && bnns.empty()
                && occsimplifier
            ) {
                occ_strategy_tokens = trim(occ_strategy_tokens);
                if (conf.verbosity) {
                    cout << "c --> Executing OCC strategy token(s): '"
                    << occ_strategy_tokens << "'\n";
                }
                occsimplifier->simplify(startup, occ_strategy_tokens);
            }
            occ_strategy_tokens.clear();
            if (sumConflicts >= conf.max_confl
                || cpuTime() > conf.maxTime
                || must_interrupt_asap()
                || nVars() == 0
                || !ok
            ) {
                break;
            }
            #ifdef SLOW_DEBUG
            check_stats();
            check_assumptions_sanity();
            #endif
        }

        if (conf.verbosity && token.substr(0,3) != "occ" && token != "") {
            cout << "c --> Executing strategy token: " << token << '\n';
        }

        if (token == "scc-vrepl") {
            if (conf.doFindAndReplaceEqLits) {
                varReplacer->replace_if_enough_is_found(
                    std::floor((double)get_num_free_vars()*0.001));
            }
        } else if (token == "eqlit-find") {
            find_equivs();
        } else if (token == "sparsify") {
            bool finished = true;
            if (nVars() > 10 &&  oracle_vivif(finished)) {
                if (finished) sparsify();
            }
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
                && !(frat->enabled() || conf.simulate_frat)
                && (solveStats.num_simplify == 0 ||
                   (solveStats.num_simplify % conf.breakid_every_n == (conf.breakid_every_n-1)))
            ) {
                #ifdef USE_BREAKID
                if (!breakid->doit()) {
                    return l_False;
                }
                #else
                if (conf.verbosity) {
                    cout << "c [breakid] BreakID not compiled in, skipping" << endl;
                }
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
        SLOW_DEBUG_DO(check_wrong_attach());
    }

    return okay() ? l_Undef : l_False;
}

/**
@brief The function that brings together almost all CNF-simplifications
*/
lbool Solver::simplify_problem(const bool startup, const string& strategy)
{
    assert(okay());
    #ifdef DEBUG_IMPLICIT_STATS
    check_stats();
    #endif
    #ifdef DEBUG_ATTACH_MORE
    test_all_clause_attached();
    find_all_attach();
    assert(check_order_heap_sanity());
    #endif
    #ifdef DEBUG_MARKED_CLAUSE
    assert(no_marked_clauses());
    #endif

    if (solveStats.num_simplify_this_solve_call >= conf.max_num_simplify_per_solve_call) {
        return l_Undef;
    }

    lbool ret = l_Undef;
    clear_order_heap();
    set_clash_decision_vars();
    if (!clear_gauss_matrices()) return l_False;

    if (conf.verbosity >= 6) {
        cout
        << "c " <<  __func__ << " called"
        << endl;
    }

    if (ret == l_Undef) {
        ret = execute_inprocess_strategy(startup, strategy);
    }
    assert(ret != l_True);

    //Free unused watch memory
    free_unused_watches();

    if (conf.verbosity >= 6) {
        cout << "c " << __func__ << " finished" << endl;
    }
    conf.global_timeout_multiplier *= conf.global_timeout_multiplier_multiplier;
    conf.global_timeout_multiplier =
        std::min<double>(
            conf.global_timeout_multiplier,
            conf.orig_global_timeout_multiplier*conf.global_multiplier_multiplier_max
        );
    if (conf.verbosity)
        cout << "c global_timeout_multiplier: " << std::setprecision(4) <<  conf.global_timeout_multiplier << endl;

    solveStats.num_simplify++;
    solveStats.num_simplify_this_solve_call++;

    assert(!(ok == false && ret != l_False));
    if (ret == l_False) {
        return l_False;
    }

    assert(ret == l_Undef);
    check_stats();
    check_implicit_propagated();
    //NOTE:
    // we have to rebuild HERE, or we'd rebuild every time solve()
    // is called, which is called form the outside, sometimes 1000x
    // in one second
    rebuildOrderHeap();
    #ifdef DEBUG_ATTACH_MORE
    find_all_attach();
    test_all_clause_attached();
    #endif
    check_wrong_attach();

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
    varReplacer->get_scc_finder()->get_stats().print_short(NULL);
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
    mem += outside_assumptions.capacity()*sizeof(Lit);

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

        mem = occsimplifier->mem_used_xor();
        print_stats_line("c Mem for xor-finder"
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
    for(vector<ClOffset>::const_iterator
        it = longIrredCls.begin(), end = longIrredCls.end()
        ; it != end
        ; ++it
    ) {
        Clause* cl = cl_alloc.ptr(*it);
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
                if (backnumber) {
                    lits.push_back(map_inter_to_outer(lit));
                } else {
                    lits.push_back(lit);
                }

            }

            //Everything it repaces has also been set
            const vector<uint32_t> vars = varReplacer->get_vars_replacing(lit.var());
            for(const uint32_t var: vars) {
                if (varData[var].is_bva)
                    continue;

                Lit tmp_lit = Lit(var, false);
                assert(varReplacer->get_lit_replaced_with(tmp_lit).var() == lit.var());
                if (lit != varReplacer->get_lit_replaced_with(tmp_lit)) {
                    tmp_lit ^= true;
                }
                assert(lit == varReplacer->get_lit_replaced_with(tmp_lit));

                if (backnumber) {
                    lits.push_back(map_inter_to_outer(tmp_lit));
                } else {
                    lits.push_back(tmp_lit);
                }
            }
        }
    }

    //Remove duplicates. Because of above replacing-mimicing algo
    //multipe occurrences of literals can be inside
    std::sort(lits.begin(), lits.end());
    vector<Lit>::iterator it = std::unique (lits.begin(), lits.end());
    lits.resize( std::distance(lits.begin(),it) );

    //Update to outer without BVA
    if (backnumber) {
        vector<uint32_t> my_map = build_outer_to_without_bva_map();
        updateLitsMap(lits, my_map);
        for(const Lit lit: lits) {
            assert(lit.var() < nVarsOutside());
        }
    }

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

    for (vector<ClOffset>::const_iterator
        it = cs.begin(), end = cs.end()
        ; it != end
        ; ++it
    ) {
        Clause& cl = *cl_alloc.ptr(*it);
        for (uint32_t j = 0; j < cl.size(); j++)
            if (model_value(cl[j]) == l_True)
                goto next;

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
    for (const Watched *it = ws.begin(), *end = ws.end()
        ; it != end
        ; ++it
    ) {
        if (it->isClause()) {
            Clause* cl = cl_alloc.ptr(it->get_offset());
            cout
            << "-> Clause: " << *cl
            << " red: " << cl->red()
            << " xor: " << cl->used_in_xor()
            << " full-xor: " << cl->used_in_xor_full()
            << " xor-detached: " << cl->_xor_is_detached;
        }
        if (it->isBin()) {
            cout
            << "-> BIN: " << lit << ", " << it->lit2()
            << " red: " << it->red();
        }
        cout << endl;
    }
    cout << "FIN" << endl;
}

void Solver::check_implicit_propagated() const
{
    const double myTime = cpuTime();
    size_t wsLit = 0;
    for(watch_array::const_iterator
        it = watches.begin(), end = watches.end()
        ; it != end
        ; ++it, wsLit++
    ) {
        const Lit lit = Lit::toLit(wsLit);
        watch_subarray_const ws = *it;
        for(const Watched *it2 = ws.begin(), *end2 = ws.end()
            ; it2 != end2
            ; it2++
        ) {
            //Satisfied, or not implicit, skip
            if (value(lit) == l_True
                || it2->isClause()
            ) {
                continue;
            }

            const lbool val1 = value(lit);
            const lbool val2 = value(it2->lit2());

            //Handle binary
            if (it2->isBin()) {
                if (val1 == l_False) {
                    if (val2 != l_True) {
                        cout << "not prop BIN: "
                        << lit << ", " << it2->lit2()
                        << " (red: " << it2->red()
                        << endl;
                    }
                    assert(val2 == l_True);
                }

                if (val2 == l_False)
                    assert(val1 == l_True);
            }
        }
    }
    const double time_used = cpuTime() - myTime;
    if (sqlStats) {
        sqlStats->time_passed_min(
            this
            , "check implicit propagated"
            , time_used
        );
    }
}

size_t Solver::get_num_vars_elimed() const
{
    if (conf.perform_occur_based_simp) {
        return occsimplifier->get_num_elimed_vars();
    } else {
        return 0;
    }
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
        ok = propagate<true>().isNULL();

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

bool Solver::add_clause_outside(const vector<Lit>& lits)
{
    if (!ok) return false;

    SLOW_DEBUG_DO(check_too_large_variable_number(lits)); //we check for this during back-numbering
    back_number_from_outside_to_outer(lits);
    return add_clause_outer(back_number_from_outside_to_outer_tmp);
}

bool Solver::full_probe(const bool bin_only)
{
    assert(okay());
    assert(decisionLevel() == 0);

    const size_t orig_num_free_vars = solver->get_num_free_vars();
    double myTime = cpuTime();
    int64_t start_bogoprops = solver->propStats.bogoProps;
    int64_t bogoprops_to_use =
        solver->conf.full_probe_time_limitM*1000ULL*1000ULL
        *solver->conf.global_timeout_multiplier;
    uint64_t probed = 0;
    const auto orig_repl = varReplacer->get_num_replaced_vars();
    *solver->frat << __PRETTY_FUNCTION__ << " start\n";

    vector<uint32_t> vars;
    for(uint32_t i = 0; i < nVars(); i++) {
        Lit l(i, false);
        if (value(l) == l_Undef && varData[i].removed == Removed::none)
            vars.push_back(i);
    }
    std::mt19937 g(mtrand.randInt());
    std::shuffle(vars.begin(), vars.end(), g);

    for(auto const& v: vars) {
        if ((int64_t)solver->propStats.bogoProps > start_bogoprops + bogoprops_to_use)
            break;

        uint32_t min_props;
        Lit l(v, false);

        //we have seen it in every combination, nothing will be learnt
        if (seen2[l.var()] == 3) continue;

        if (value(l) == l_Undef &&
            varData[v].removed == Removed::none)
        {
            probed++;
            lbool ret;
            if (bin_only) ret = probe_inter<true>(l, min_props);
            else ret = probe_inter<false>(l, min_props);
            if (ret == l_False) goto cleanup;

            if (conf.verbosity >= 5) {
                const double time_remain = 1.0-float_div(
                (int64_t)solver->propStats.bogoProps-start_bogoprops, bogoprops_to_use);
                cout << "c probe time remain: " << time_remain << " probed: " << probed
                << " set: "  << (orig_num_free_vars - solver->get_num_free_vars())
                << " T: " << (cpuTime() - myTime)
                << endl;
            }
        }
    }

    cleanup:
    std::fill(seen2.begin(), seen2.end(), 0);

    const double time_used = cpuTime() - myTime;
    const double time_remain = 1.0-float_div(
        (int64_t)solver->propStats.bogoProps-start_bogoprops, bogoprops_to_use);
    const bool time_out = ((int64_t)solver->propStats.bogoProps > start_bogoprops + bogoprops_to_use);

    verb_print(1,
        "[full-probe] "
        << " bin_only: " << bin_only
        << " set: "
        << (orig_num_free_vars - solver->get_num_free_vars())
        << " repl: " << (varReplacer->get_num_replaced_vars() - orig_repl)
        << solver->conf.print_times(time_used,  time_out, time_remain));


    if (solver->sqlStats) {
        solver->sqlStats->time_passed(
            solver
            , "full-probe"
            , time_used
            , time_out
            , time_remain
        );
    }
    *solver->frat << __PRETTY_FUNCTION__ << " end\n";

    return okay();
}

template<bool bin_only>
lbool Solver::probe_inter(const Lit l, uint32_t& min_props)
{
    propStats.bogoProps+=2;

    //Probe l
    uint32_t old_trail_size = trail.size();
    new_decision_level();
    enqueue_light(l);
    PropBy p = propagate_light<bin_only>();
    min_props = trail.size() - old_trail_size;
    for(uint32_t i = old_trail_size+1; i < trail.size(); i++) {
        toClear.push_back(trail[i].lit);
        //seen[x] == 0 -> not propagated
        //seen[x] == 1 -> propagated as POS
        //seen[x] == 2 -> propagated as NEG
        const auto var = trail[i].lit.var();
        seen[var] = 1+(int)trail[i].lit.sign();
        seen2[var] |= 1+(int)trail[i].lit.sign();
    }
    cancelUntil_light();

    //Check result
    if (!p.isNULL()) {
        enqueue<true>(~l);
        p = propagate<true>();
        if (!p.isNULL()) {
            ok = false;
        }
        goto end;
    }

    //Probe ~l
    old_trail_size = trail.size();
    new_decision_level();
    enqueue_light(~l);
    p = propagate_light<bin_only>();
    min_props = std::min<uint32_t>(min_props, trail.size() - old_trail_size);
    probe_inter_tmp.clear();
    for(uint32_t i = old_trail_size+1; i < trail.size(); i++) {
        Lit lit = trail[i].lit;
        uint32_t var = trail[i].lit.var();
        seen2[var] |= 1+(int)trail[i].lit.sign();
        if (seen[var] == 0) continue;

        if (lit.sign() == seen[var]-1) {
            //Same sign both times (set value of literal)
            probe_inter_tmp.push_back(lit);
        } else {
            //Inverse sign in the 2 cases (literal equivalence)
            probe_inter_tmp.push_back(lit_Undef);
            probe_inter_tmp.push_back(~lit);
        }
    }
    cancelUntil_light();

    //Check result
    if (!p.isNULL()) {
        enqueue<true>(l);
        p = propagate<true>();
        if (!p.isNULL()) {
            ok = false;
        }
        goto end;
    }

    //Deal with bothprop
    for(uint32_t i = 0; i < probe_inter_tmp.size(); i++) {
        Lit bp_lit = probe_inter_tmp[i];
        if (bp_lit != lit_Undef) {
            //I am not going to deal with the messy version of it already being set
            if (value(bp_lit) == l_Undef) {
                *solver->frat << add << ++clauseID << ~l << bp_lit << fin;
                const int32_t c1 = clauseID;
                *solver->frat << add << ++clauseID << l << bp_lit << fin;
                const int32_t c2 = clauseID;
                enqueue<true>(bp_lit);
                *solver->frat << del << c1 << ~l << bp_lit << fin;
                *solver->frat << del << c2 << l << bp_lit << fin;
            }
        } else {
            //First we must propagate all the enqueued facts
            p = propagate<true>();
            if (!p.isNULL()) {
                ok = false;
                goto end;
            }

            //Add binary XOR
            i++;
            bp_lit = probe_inter_tmp[i];
            vector<Lit> lits(2);
            lits[0] = l;
            lits[1] = bp_lit;
            ok = add_xor_clause_inter(lits, false, true, true, true);
            if (!ok) {
                goto end;
            }
        }
    }

    //Propagate all enqueued facts due to bprop
    p = propagate<true>();
    if (!p.isNULL()) {
        ok = false;
        goto end;
    }

    end:
    for(auto clear_l: toClear) {
        seen[clear_l.var()] = 0;
    }
    toClear.clear();
    return ok ? l_Undef : l_False;
}

lbool Solver::probe_outside(Lit l, uint32_t& min_props)
{
    assert(decisionLevel() == 0);
    assert(l.var() < nVarsOutside());

    if (!ok) {
        return l_False;
    }

    l = map_to_with_bva(l);
    l = varReplacer->get_lit_replaced_with_outer(l);
    l = map_outer_to_inter(l);
    if (varData[l.var()].removed != Removed::none) {
        //TODO
        return l_Undef;
    }
    if (value(l) != l_Undef) {
        return l_Undef;
    }

    return probe_inter<false>(l, min_props);
}

bool Solver::add_xor_clause_outside(const vector<uint32_t>& vars, bool rhs)
{
    if (!ok) {
        return false;
    }

    vector<Lit> lits(vars.size());
    for(size_t i = 0; i < vars.size(); i++) {
        lits[i] = Lit(vars[i], false);
    }
    #ifdef SLOW_DEBUG //we check for this during back-numbering
    check_too_large_variable_number(lits);
    #endif

    back_number_from_outside_to_outer(lits);
    addClauseHelper(back_number_from_outside_to_outer_tmp);
    add_xor_clause_inter(back_number_from_outside_to_outer_tmp, rhs, true, false);

    return ok;
}

bool Solver::add_bnn_clause_outside(
    const vector<Lit>& lits,
    const int32_t cutoff,
    Lit out)
{
    if (!ok) {
        return false;
    }

    #ifdef SLOW_DEBUG //we check for this during back-numbering
    check_too_large_variable_number(lits);
    #endif

    vector<Lit> lits2(lits);
    if (out != lit_Undef) {
        lits2.push_back(out);
    }
    back_number_from_outside_to_outer(lits2);
    addClauseHelper(back_number_from_outside_to_outer_tmp);
    if (out != lit_Undef) {
        out = back_number_from_outside_to_outer_tmp.back();
        back_number_from_outside_to_outer_tmp.pop_back();
    }

    add_bnn_clause_inter(
        back_number_from_outside_to_outer_tmp, cutoff, out);

    return ok;
}

void Solver::check_too_large_variable_number(const vector<Lit>& lits) const
{
    for (const Lit lit: lits) {
        if (lit.var() >= nVarsOutside()) {
            std::cerr
            << "ERROR: Variable " << lit.var() + 1
            << " inserted, but max var is "
            << nVarsOutside()
            << endl;
            assert(false);
            std::exit(-1);
        }
        release_assert(lit.var() < nVarsOutside()
        && "Clause inserted, but variable inside has not been declared with PropEngine::new_var() !");

        if (lit.var() >= var_Undef) {
            std::cerr << "ERROR: Variable number " << lit.var()
            << "too large. PropBy is limiting us, sorry" << endl;
            assert(false);
            std::exit(-1);
        }
    }
}

void Solver::bva_changed()
{
    datasync->rebuild_bva_map();
}

vector<pair<Lit, Lit> > Solver::get_all_binary_xors() const
{
    vector<pair<Lit, Lit> > bin_xors = varReplacer->get_all_binary_xors_outer();

    //Update to outer without BVA
    vector<pair<Lit, Lit> > ret;
    const vector<uint32_t> my_map = build_outer_to_without_bva_map();
    for(std::pair<Lit, Lit> p: bin_xors) {
        if (p.first.var() < my_map.size()
            && p.second.var() < my_map.size()
        ) {
            ret.push_back(std::make_pair(
                getUpdatedLit(p.first, my_map)
                , getUpdatedLit(p.second, my_map)
            ));
        }
    }

    for(const auto& val: ret) {
        assert(val.first.var() < nVarsOutside());
        assert(val.second.var() < nVarsOutside());
    }

    return ret;
}

void Solver::update_assumptions_after_varreplace()
{
    #ifdef SLOW_DEBUG
    for(AssumptionPair& lit_pair: assumptions) {
        const Lit inter_lit = map_outer_to_inter(lit_pair.lit_outer);
        assert(inter_lit.var() < varData.size());
        if (varData[inter_lit.var()].assumption == l_Undef) {
            cout << "Assump " << inter_lit << " has .assumption : "
            << varData[inter_lit.var()].assumption << endl;
        }
        assert(varData[inter_lit.var()].assumption != l_Undef);
    }
    #endif

    //Update assumptions
    for(AssumptionPair& lit_pair: assumptions) {
        const Lit orig = lit_pair.lit_outer;
        lit_pair.lit_outer = varReplacer->get_lit_replaced_with_outer(orig);

        //remove old from set + add new to set
        if (orig != lit_pair.lit_outer) {
            const Lit old_inter_lit = map_outer_to_inter(orig);
            const Lit new_inter_lit = map_outer_to_inter(lit_pair.lit_outer);
            varData[old_inter_lit.var()].assumption = l_Undef;
            varData[new_inter_lit.var()].assumption =
                new_inter_lit.sign() ? l_False: l_True;
        }
    }

    #ifdef SLOW_DEBUG
    check_assumptions_sanity();
    #endif
}

//TODO later, this can be removed, get_num_free_vars() is MUCH cheaper to
//compute but may have some bugs here-and-there
uint32_t Solver::num_active_vars() const
{
    uint32_t numActive = 0;
    uint32_t removed_replaced = 0;
    uint32_t removed_set = 0;
    uint32_t removed_elimed = 0;
    uint32_t removed_clashed = 0;
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
            case Removed::clashed:
                removed_clashed++;
                continue;
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
    const double myTime = cpuTime();

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
        for(const Watched* it2 = ws.begin(), *end2 = ws.end()
            ; it2 != end2
            ; it2++
        ) {
            if (it2->isBin()) {
                #ifdef DEBUG_IMPLICIT_PAIRS_TRIPLETS
                Lit lits[2];
                lits[0] = Lit::toLit(wsLit);
                lits[1] = it2->lit2();
                std::sort(lits, lits + 2);
                findWatchedOfBin(watches, lits[0], lits[1], it2->red(), it2->get_ID());
                findWatchedOfBin(watches, lits[1], lits[0], it2->red(), it2->get_ID());
                #endif

                if (it2->red())
                    thisNumRedBins++;
                else
                    thisNumIrredBins++;

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

    const double time_used = cpuTime() - myTime;
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

    const double myTime = cpuTime();
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

    const double time_used = cpuTime() - myTime;
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

vector<Xor> Solver::get_recovered_xors(const bool xor_together_xors)
{
    vector<Xor> xors_ret;
    if (!okay()) return xors_ret;

    lbool ret = execute_inprocess_strategy(false, "occ-xor");
    if (ret == l_False) return xors_ret;

    auto xors = xorclauses;
    xors.insert(xors.end(), xorclauses_unused.begin(), xorclauses_unused.end());
    if (xor_together_xors) {
        XorFinder finder(NULL, this);
        finder.xor_together_xors(xors);
        renumber_xors_to_outside(xors, xors_ret);
        return xors_ret;
    } else {
        renumber_xors_to_outside(xors, xors_ret);
        return xors_ret;
    }
}

void Solver::renumber_xors_to_outside(const vector<Xor>& xors, vector<Xor>& xors_ret)
{
    const vector<uint32_t> outer_to_without_bva_map = build_outer_to_without_bva_map();

    if (conf.verbosity >= 5) {
        cout << "XORs before outside numbering:" << endl;
        for(auto& x: xors) {
            cout << x << endl;
        }
    }

    for(auto& x: xors) {
        bool OK = true;
        for(const auto v: x.get_vars()) {
            if (varData[v].is_bva) {
                OK = false;
                break;
            }
        }
        if (!OK) {
            continue;
        }

        vector<uint32_t> t = xor_outer_numbered(x.get_vars());
        for(auto& v: t) {
            v = outer_to_without_bva_map[v];
        }
        xors_ret.push_back(Xor(t, x.rhs, vector<uint32_t>()));
    }
}

bool Solver::find_and_init_all_matrices()
{
    *solver->frat << __PRETTY_FUNCTION__ << " start\n";
    if (!xor_clauses_updated && (!detached_xor_clauses || !assump_contains_xor_clash())) {
        if (conf.verbosity >= 2) {
            cout << "c [find&init matx] XORs not updated, and either (XORs are not detached OR assumps does not contain clash variable) -> or not performing matrix init. Matrices: " << gmatrices.size() << endl;
        }
        return true;
    }
    if (conf.verbosity >= 1) cout << "c [find&init matx] performing matrix init" << endl;

    bool can_detach;
    if (!clear_gauss_matrices()) return false;

    /*Reattach needed in case we are coming in again, after adding new XORs
    we might turn off a previously turned on matrix
    which means we wouldn't re-attach those XORs
    -> but we fixed this in matrixfinder, where we FORCE a Gauss to be ON
       in case it contains a previously detached XOR
    */
    //fully_undo_xor_detach();

    MatrixFinder mfinder(solver);
    ok = mfinder.find_matrices(can_detach);
    if (!ok) return false;
    if (!init_all_matrices()) return false;

    if (conf.verbosity >= 2) {
        cout << "c calculating no_irred_contains_clash..." << endl;
        bool no_irred_contains_clash = no_irred_nonxor_contains_clash_vars();

        cout
        << "c [gauss]"
        << " xorclauses_unused: " << xorclauses_unused.size()
        << " can detach: " << can_detach
        << " no irred with clash: " << no_irred_contains_clash
        << endl;

        cout << "c unused xors follow." << endl;
        for(const auto& x: xorclauses_unused) {
            cout << "c " << x << endl;
        }
        cout << "c FIN" << endl;

        cout << "c used xors follow." << endl;
        for(const auto& x: xorclauses) {
            cout << "c " << x << endl;
        }
        cout << "c FIN" << endl;
    }

    bool ret_no_irred_nonxor_contains_clash_vars;
    if (can_detach &&
        conf.xor_detach_reattach &&
        !conf.gaussconf.autodisable &&
        (ret_no_irred_nonxor_contains_clash_vars=no_irred_nonxor_contains_clash_vars())
    ) {
        detach_xor_clauses(mfinder.clash_vars_unused);
        unset_clash_decision_vars(xorclauses);
        rebuildOrderHeap();
        if (conf.xor_detach_verb) print_watchlist_stats();

    } else {
        if (conf.xor_detach_reattach &&
            (conf.verbosity >= 1 || conf.xor_detach_verb) && conf.doFindXors)
        {
            cout
            << "c WHAAAAT Detach issue. All below must be 1 to work ---" << endl
            << "c -- can_detach: " << (bool)can_detach << endl
            << "c -- mfinder.no_irred_nonxor_contains_clash_vars(): "
            << ret_no_irred_nonxor_contains_clash_vars << endl
            << "c -- !conf.gaussconf.autodisable: " << (bool)(!conf.gaussconf.autodisable) << endl
            << "c -- conf.xor_detach_reattach: " << (bool)conf.xor_detach_reattach << endl;
            print_watchlist_stats();
        }
    }

    #ifdef SLOW_DEBUG
    for(size_t i = 0; i< gmatrices.size(); i++) {
        if (gmatrices[i]) {
            gmatrices[i]->check_watchlist_sanity();
            assert(gmatrices[i]->get_matrix_no() == i);
        }
    }
    #endif

    xor_clauses_updated = false;
    *solver->frat << __PRETTY_FUNCTION__ << " end\n";
    return true;
}

bool Solver::init_all_matrices()
{
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
            g = NULL;
        }
    }

    uint32_t j = 0;
    bool modified = false;
    for (uint32_t i = 0; i < gqueuedata.size(); i++) {
        if (gmatrices[i] != NULL) {
            gmatrices[j] = gmatrices[i];
            gmatrices[j]->update_matrix_no(j);
            gqueuedata[j] = gqueuedata[i];

            if (modified) {
                for (size_t var = 0; var < nVars(); var++) {
                    for(GaussWatched* k = gwatches[var].begin();
                        k != gwatches[var].end();
                        k++)
                    {
                        if (k->matrix_num == i) {
                            k->matrix_num = j;
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

void Solver::start_getting_small_clauses(
    const uint32_t max_len, const uint32_t max_glue, bool red, bool bva_vars,
    bool simplified)
{
    assert(get_clause_query == NULL);
    get_clause_query = new GetClauseQuery(this);
    get_clause_query->start_getting_small_clauses(max_len, max_glue, red, bva_vars, simplified);

}

void Solver::get_all_irred_clauses(vector<Lit>& out)
{
    assert(get_clause_query == NULL);
    get_clause_query = new GetClauseQuery(this);
    get_clause_query->get_all_irred_clauses(out);
    delete get_clause_query;
    get_clause_query = NULL;
}

bool Solver::get_next_small_clause(vector<Lit>& out, bool all_in_one)
{
    assert(get_clause_query);
    return get_clause_query->get_next_small_clause(out, all_in_one);
}

void Solver::end_getting_small_clauses()
{
    assert(get_clause_query);
    get_clause_query->end_getting_small_clauses();
    delete get_clause_query;
    get_clause_query = NULL;
}

vector<uint32_t> Solver::translate_sampl_set(const vector<uint32_t>& sampl_set)
{
    assert(get_clause_query);
    return get_clause_query->translate_sampl_set(sampl_set);
}

void Solver::add_empty_cl_to_frat()
{
    assert(false);
//     *frat << add
//     #ifdef STATS_NEEDED
//     << 0
//     << sumConflicts
//     #endif
//     << fin;
//     frat->flush();
}

void Solver::check_assigns_for_assumptions() const
{
    for (const auto& ass: assumptions) {
        const Lit inter = map_outer_to_inter(ass.lit_outer);
        if (value(inter) != l_True) {
            cout << "ERROR: Internal assumption " << inter
            << " is not set to l_True, it's set to: " << value(inter)
            << endl;
            assert(lit_inside_assumptions(inter) == l_True);
        }
        assert(value(inter) == l_True);
    }
}

bool Solver::check_assumptions_contradict_foced_assignment() const
{
    for (auto& ass: assumptions) {
        const Lit inter = map_outer_to_inter(ass.lit_outer);
        if (value(inter) == l_False) {
            return true;
        }
    }
    return false;
}

void Solver::set_var_weight(
#ifdef WEIGHTED_SAMPLING
const Lit lit, const double weight
#else
const Lit, const double
#endif
) {
    #ifdef WEIGHTED_SAMPLING
    //cout << "set weight called lit: " << lit << " w: " << weight << endl;
    assert(lit.var() < nVars());
    if (weights_given.size() < nVars()) {
        weights_given.resize(nVars(), GivenW());
    }

    if ((weights_given[lit.var()].pos && !lit.sign())
        || (weights_given[lit.var()].neg && lit.sign())
    ) {
        cout << "ERROR: Giving weights twice for literal: " << lit << endl;
        exit(-1);
        return;
    }

    if (!weights_given[lit.var()].neg && !lit.sign()) {
        weights_given[lit.var()].pos = true;
        varData[lit.var()].weight = weight;
        return;
    }

    if (!weights_given[lit.var()].pos && lit.sign()) {
        weights_given[lit.var()].neg = true;
        varData[lit.var()].weight = weight;
        return;
    }

    if (!lit.sign()) {
        //this is the pos
        weights_given[lit.var()].pos = true;
        double neg = varData[lit.var()].weight;
        double pos = weight;
        varData[lit.var()].weight = pos/(pos + neg);
    } else {
        //this is the neg
        weights_given[lit.var()].neg = true;
        double neg = weight;
        double pos = varData[lit.var()].weight;
        varData[lit.var()].weight = pos/(pos + neg);
    }
    #else
    cout << "ERROR: set_var_weight() only supported if you compile with -DWEIGHTED_SAMPLING=ON" << endl;
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

    //Map to outside
    if (get_num_bva_vars() != 0) {
        scores_outer = map_back_vars_to_without_bva(scores_outer);
    }
    return scores_outer;
}

bool Solver::implied_by(const std::vector<Lit>& lits,
                                  std::vector<Lit>& out_implied)
{
    if (get_num_bva_vars() != 0) {
        cout << "ERROR: get_num_bva_vars(): " << get_num_bva_vars() << endl;
        assert(false && "ERROR: BVA is currently not allowed at implied_by(), please turn it off");
        //out_implied = map_back_vars_to_without_bva(out_implied);
        exit(-1);
    }
//     if (solver->occsimplifier->get_num_elimed_vars() > 0) {
//         assert(false && "ERROR, you must not have any eliminated variables when calling implied_by -- otherwise, we cannot guarantee all implied variables are found");
//         exit(-1);
//     }

    out_implied.clear();
    if (!okay()) {
        return false;
    }

    implied_by_tmp_lits = lits;
    if (!addClauseHelper(implied_by_tmp_lits)) {
        return false;
    }

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

    if (decisionLevel() == 0) {
        return true;
    }

    PropBy x = propagate<true>();
    if (!x.isNULL()) {
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
    for(auto& l: out_implied) {
        l = map_inter_to_outer(l);
    }
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
    if (cl->stats.is_tracked != 0 && sqlStats) {
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

void Solver::detach_xor_clauses(
    const set<uint32_t>& clash_vars_unused)
{
    detached_xor_clauses = true;
    double myTime = cpuTime();

    ///////////////
    //Set up seen
    //  '1' means it's part of an UNUSED xor
    //  '2' means it's a CLASH of a USED xor
    ///////////////
    for(const auto& x: xorclauses_unused) {
        for(const uint32_t v: x) {
            seen[v] = 1;
        }
    }
    for(const auto& v: clash_vars_unused) {
        seen[v] = 1;
    }
    for(uint32_t v: removed_xorclauses_clash_vars) {
        seen[v] = 1;
    }

    //Clash on USED xor
    for(auto& x: xorclauses) {
        x.detached = true;
        for(const uint32_t v: x.clash_vars) {
            assert(seen[v] == 0);
            seen[v] = 2;
        }
    }

    ///////////////
    //Go through watchlist
    ///////////////
    uint32_t detached = 0;
    uint32_t deleted = 0;
    vector<ClOffset> delayed_clause_free;
    for(uint32_t x = 0; x < nVars()*2; x++) {
        Lit l = Lit::toLit(x);
        uint32_t j = 0;
        for(uint32_t i = 0; i < watches[l].size(); i++) {
            const Watched& w = watches[l][i];
            assert(!w.isIdx());
            if (w.isBin()) {
                if (!w.red()) {
                    watches[l][j++] = w;
                    continue;
                } else {
                    //Redundant. let's check if it deals with clash vars of
                    //           XORs that will be removed.
                    if (seen[l.var()] == 2 || seen[w.lit2().var()] == 2) {
                        if (l < w.lit2()) {
                            //Only once, hence the check
                            binTri.redBins--;
                            deleted++;
                        }
                        continue;
                    } else {
                        watches[l][j++] = w;
                        continue;
                    }
                }
            }

            if (w.isBNN()) {
                watches[l][j++] = w;
                continue;
            }

            assert(w.isClause());
            ClOffset offs = w.get_offset();
            Clause* cl = cl_alloc.ptr(offs);
            assert(!cl->freed());
            if (cl->getRemoved() || cl->_xor_is_detached) {
                //We have already went through this clause, and set it to be removed/detached
                continue;
            }

            bool torem = false;
            bool todel = false;
            if (cl->used_in_xor() && cl->used_in_xor_full()) {
                torem = true;

                //Except if if it's part of an unused XOR. Then skip
                //TODO: we should use the same check as in no_irred
                for(const Lit lit: *cl) {
                    if (seen[lit.var()] == 1) torem = false;
                }
            } else {
                //It has a USED XOR's clash var, delete
                //obviously, must be redundant
                for(const Lit lit: *cl) {
                    if (seen[lit.var()] == 2) todel = true;
                }
                if (todel) {
                    //cout << "cl: " << *cl << endl;
                    assert(cl->red());
                }
            }

            //CL can be detached
            if (torem) {
                assert(!cl->_xor_is_detached);
                detached++;
                detached_xor_repr_cls.push_back(offs);
                cl->_xor_is_detached = true;
                //cout << "XOR-detaching cl: " << *cl << endl;
                continue;
            }

            //This is a rendundant clause that has to be removed
            //for things to be correct (it clashes with an XOR)
            if (todel) {
                assert(cl->red());
                cl->setRemoved();
                litStats.redLits -= cl->size();
                delayed_clause_free.push_back(offs);
                deleted++;
                continue;
            }

            watches[l][j++] = w;
        }
        watches[l].resize(j);
    }

    if (deleted > 0) {
        uint32_t j = 0;
        for(uint32_t i = 0; i < longIrredCls.size(); i++) {
            ClOffset offs = longIrredCls[i];
            Clause* cl = cl_alloc.ptr(offs);
            if (!cl->getRemoved()) {
                longIrredCls[j++] = offs;
            }
        }
        longIrredCls.resize(j);

        for(auto& cls: longRedCls) {
            j = 0;
            for(uint32_t i = 0; i < cls.size(); i++) {
                ClOffset offs = cls[i];
                Clause* cl = cl_alloc.ptr(offs);
                if (!cl->getRemoved()) {
                    cls[j++] = offs;
                }
            }
            cls.resize(j);
        }

        for(ClOffset offset: delayed_clause_free) {
            free_cl(offset);
        }
        delayed_clause_free.clear();
    }
    assert(delayed_clause_free.empty());

    ///////////////
    //Reset seen
    ///////////////
    for(const auto& x: xorclauses_unused) {
        for(const uint32_t v: x) {
            seen[v] = 0;
        }
    }

    for(const auto& v: clash_vars_unused) {
        seen[v] = 0;
    }

    for(uint32_t v: removed_xorclauses_clash_vars) {
        seen[v] = 0;
    }

    for(const auto& x: xorclauses) {
        for(const uint32_t v: x.clash_vars) {
            seen[v] = 0;

            if (!watches[Lit(v, false)].empty()) {
                print_watch_list(watches[Lit(v, false)], Lit(v, false));
            }
            if (!watches[Lit(v, true)].empty()) {
                print_watch_list(watches[Lit(v, true)], Lit(v, true));
            }
            assert(watches[Lit(v, false)].empty());
            assert(watches[Lit(v, true)].empty());
        }
    }

    if (conf.verbosity >= 1 || conf.xor_detach_verb) {
        cout
        << "c [gauss] XOR-encoding clauses"
        << " detached: " << detached
        << " deleted: " << deleted
        << conf.print_times(cpuTime() - myTime)
        << endl;
    }
}

bool Solver::fully_undo_xor_detach()
{
    assert(okay());
    assert(decisionLevel() == 0);

    if (!detached_xor_clauses) {
        assert(detached_xor_repr_cls.empty());
        if (conf.verbosity >= 1 || conf.xor_detach_verb) {
            cout
            << "c [gauss] XOR-encoding clauses are not detached, so no need to reattach them."
            << endl;
        }
        return okay();
    }
    set_clash_decision_vars();
    rebuildOrderHeap();

    double myTime = cpuTime();
    uint32_t reattached = 0;
    uint32_t removed = 0;
    for(const auto& offs: detached_xor_repr_cls) {
        Clause* cl = cl_alloc.ptr(offs);
        assert(cl->_xor_is_detached);
        assert(cl->used_in_xor() && cl->used_in_xor_full());
        assert(!cl->red());

        cl->_xor_is_detached = false;
        const uint32_t origSize = cl->size();

        reattached++;
        const bool rem_or_unsat = clauseCleaner->full_clean(*cl);
        if (rem_or_unsat) {
            removed++;
            litStats.irredLits -= origSize;
            cl->setRemoved();
            if (!okay()) break;
            continue;
        } else {
            litStats.irredLits -= origSize - cl->size();
            assert(cl->size() > 2);
            PropEngine::attachClause(*cl, true);
        }
    }
    detached_xor_repr_cls.clear();

    if (removed > 0) {
        uint32_t j = 0;
        for(uint32_t i = 0; i < longIrredCls.size(); i ++) {
            ClOffset offs = longIrredCls[i];
            Clause* cl = cl_alloc.ptr(offs);
            if (cl->getRemoved()) {
                cl_alloc.clauseFree(offs);
            } else {
                longIrredCls[j++] = offs;
            }
        }
        longIrredCls.resize(j);
    }

    for(auto& x: xorclauses) x.detached = false;

    detached_xor_clauses = false;
    if (okay()) ok = propagate<false>().isNULL();

    if (conf.verbosity >= 1 || conf.xor_detach_verb) {
        cout
        << "c [gauss] XOR-encoding clauses reattached: " << reattached
        << conf.print_times(cpuTime() - myTime)
        << endl;
    }

    return okay();
}

void Solver::unset_clash_decision_vars(const vector<Xor>& xors)
{
    vector<uint32_t> clash_vars;
    for(const auto& x: xors) {
        for(const auto& v: x.clash_vars) {
            if (!seen[v]) {
                clash_vars.push_back(v);
                seen[v] = 1;
            }
        }
    }

    for(const auto& v: clash_vars) {
        seen[v] = 0;
        varData[v].removed = Removed::clashed;
    }
}

void Solver::set_clash_decision_vars()
{
    for(auto& v: varData) {
        if (v.removed == Removed::clashed) {
            v.removed = Removed::none;
        }
    }
}

//TODO: this is horrifically SLOW!!!
void Solver::extend_model_to_detached_xors()
{
    double myTime = cpuTime();

    uint32_t set_var = 0;
    uint32_t more_vars_unset = 1;
    uint32_t iter = 0;
    while (more_vars_unset > 0) {
        more_vars_unset = 0;
        iter++;
        for(const auto& offs: detached_xor_repr_cls) {
            const Clause* cl = cl_alloc.ptr(offs);
            assert(cl->_xor_is_detached);
            uint32_t unset_vars = 0;
            Lit unset = lit_Undef;
            for(Lit l: *cl) {
                if (model_value(l) == l_True) {
                    unset_vars = 0;
                    break;
                }
                if (model_value(l) == l_Undef) {
                    unset = l;
                    unset_vars++;
                }
            }
            if (unset_vars == 1) {
                model[unset.var()] = unset.sign() ? l_False : l_True;
                set_var++;
            }
            if (unset_vars > 1) {
                more_vars_unset++;
            }
        }
    }

    #ifdef SLOW_DEBUG
    for(const auto& offs: detached_xor_repr_cls) {
        const Clause* cl = cl_alloc.ptr(offs);
        bool val = false;
        uint32_t undef_present = false;
        for(const auto l: *cl) {
            if (model_value(l) == l_Undef) {
                undef_present++;
            }
            if (model_value(l) == l_True) {
                val = true;
                break;
            }
        }
        if (!val) {
            cout << "ERROR: XOR-Detached clause not satisfied: " << *cl << " -- undef present: " << undef_present << endl;
            assert(false);
        }
    }

    for(const auto& x: xorclauses) {
        bool val = !x.rhs;
        for(uint32_t v: x) {
            if (model_value(v) == l_Undef) {
                cout << "ERROR: variable " << v+1 << " in XOR: " << x << " is UNDEF!" << endl;
                assert(false);
            }
            assert(model_value(v) != l_Undef);
            val ^= model_value(v) == l_True;
        }
        if (!val) {
            cout << "ERROR:Value of following XOR is not TRUE: " << x << endl;
            assert(false);
        }
    }
    #endif

    //Set the rest randomly
    uint32_t random_set = 0;
    for(const auto& offs: detached_xor_repr_cls) {
        const Clause* cl = cl_alloc.ptr(offs);
        assert(cl->_xor_is_detached);
        for(Lit l: *cl) {
            if (model_value(l) == l_Undef) {
                model[l.var()] = l_False;
                random_set++;
            }
        }
    }

    if (conf.verbosity >= 1) {
        cout
        << "c [gauss] extended XOR clash vars."
        << " set: " << set_var
        << " double-undef: " << more_vars_unset
        << " iters: " << iter
        << " random_set: " << random_set
        << conf.print_times(cpuTime() - myTime)
        << endl;
    }
}

bool Solver::no_irred_nonxor_contains_clash_vars()
{
    bool ret = true;

    //seen 1: it's a variable that's a clash variable
    //seen 2: it's a variable in an XOR

    //Set variables that are part of an XOR
    for(const auto& x: xorclauses) {
        //REAL vars
        for(uint32_t v: x) {
            seen[v] = 2;
        }
    }

    //Set variables that are clashing
    for(const auto& x: xorclauses) {
        //CLASH vars
        for(uint32_t v: x.clash_vars) {
            //assert(seen[v] != 2); -- actually, it could be (weird, but possible)
            //in these cases, we should treat it as a clash var (more safe)
            seen[v] = 1;
//                 cout << "c clash var: " << v + 1 << endl;
        }
    }

    for(const auto& v: removed_xorclauses_clash_vars) {
        seen[v] = 1;
    }

    for(const auto& l: assumptions) {
        const Lit p = map_outer_to_inter(l.lit_outer);
        if (seen[p.var()] == 1) {
            //We cannot have a clash variable that's an assumption
            //it would enqueue the assumption variable but it's a clash
            //var and that's not supposed to be in the trail at all.
            ret = false;
            break;
        }
    }

    for(uint32_t i = 0; i < longIrredCls.size() && ret; i++) {
        const ClOffset offs = longIrredCls[i];
        const Clause* cl = cl_alloc.ptr(offs);

        //contains NO clash var, never an issue
        //contains at least 1 clash var AND no real vars, MUST be an issue
        //contains at least 1 clash var AND some not all real vars--> MUST be an issue
        //contains at least 1 clash var AND only real vars--> must be a FULL XOR to be OK

        if (cl->red()) {
            continue;
        }

        uint32_t num_real_vars = 0;
        uint32_t num_clash_vars = 0;
        for(const Lit l: *cl) {
            if (seen[l.var()] == 1) {
                num_clash_vars++;
//                 if (!(cl->used_in_xor() && cl->used_in_xor_full())) {
//                     cout << "clash : " << l << endl;
//                 }
            }
            else if (seen[l.var()] == 2) {
                num_real_vars++;
//                 if (!(cl->used_in_xor() && cl->used_in_xor_full())) {
//                     cout << "real : " << l << endl;
//                 }
            }
            else if (value(l) != l_Undef) {
                num_real_vars++;
            }
        }
        if (num_clash_vars == 0) {
            continue;
        }

        if (cl->used_in_xor() && cl->used_in_xor_full() && num_clash_vars+num_real_vars == cl->size()) {
            continue;
        }

        //non-full XORs or other non-XOR clause
        if (conf.verbosity >= 3 || conf.xor_detach_verb) {
            cout << "c CL with clash: " << *cl
            << " red: " << cl->red()
            << " xor: " << cl->used_in_xor()
            << " full-xor: " << cl->used_in_xor_full()
            << " num_clash_vars: " << num_clash_vars
            << " num_real_vars: " << num_real_vars
            << " size: " << cl->size()
            << " missing: " << (cl->size()-num_clash_vars-num_real_vars)
            << endl;
            for(const Lit l: *cl) {
                if (seen[l.var()] == 1) {
                    cout << "c clash lit: " << l
                    << " value: " << value(l) << endl;
                }
            }
            for(const Lit l: *cl) {
                if (seen[l.var()] == 0) {
                    cout << "c neither clash nor real: " << l
                    << " value: " << value(l) << endl;
                }
            }

        }
        ret = false;
    }

    for(uint32_t i = 0; i < nVars()*2 && ret; i++) {
        Lit l = Lit::toLit(i);
        const auto& ws = watches[l];
        for(const auto& w: ws) {
            if (w.isBin() && !w.red()) {
                if (seen[l.var()]==1 || seen[w.lit2().var()]==1) {
                    if (conf.verbosity >= 3 || conf.xor_detach_verb) {
                        cout << "c BIN with clash: " << l << " " << w.lit2() << endl;
                    }
                    ret = false;
                    break;
                }
            }
        }
    }

    for(const auto& x: xorclauses) {
        //REAL vars
        for(uint32_t v: x) {
            seen[v] = 0;
        }

        //CLASH vars
        for(uint32_t v: x.clash_vars) {
            seen[v] = 0;
//                 cout << "c clash var: " << v + 1 << endl;
        }
    }

    for(const auto& v: removed_xorclauses_clash_vars) {
        seen[v] = 0;
    }

    return ret;
}

bool Solver::assump_contains_xor_clash()
{
    assert(detached_xor_clauses);
    //Set variables that are clashing
    for(const auto& x: xorclauses) {
        for(uint32_t v: x.clash_vars) {
            seen[v] = 1;
        }
    }

    for(const auto& v: removed_xorclauses_clash_vars) {
        seen[v] = 1;
    }

    bool ret = false;
    for(const auto& l: assumptions) {
        const Lit p = map_outer_to_inter(l.lit_outer);
        if (seen[p.var()] == 1) {
            //We cannot have a clash variable that's an assumption
            //it would enqueue the assumption variable but it's a clash
            //var and that's not supposed to be in the trail at all.
            ret = true;
            break;
        }
    }

    for(const auto& x: xorclauses) {
        for(uint32_t v: x.clash_vars) {
            seen[v] = 0;
        }
    }

    for(const auto& v: removed_xorclauses_clash_vars) {
        seen[v] = 0;
    }

    return ret;
}

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

void Solver::find_equiv_subformula(
    vector<uint32_t>& sampl_vars, vector<uint32_t>& empty_vars, const bool mirror_empty)
{
    if (!okay()) return;
    return occsimplifier->find_equiv_subformula(sampl_vars, empty_vars, mirror_empty);
}

bool Solver::remove_and_clean_all() {
    return clauseCleaner->remove_and_clean_all();
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

#define PICOLIT(x) ((x).var() * ((x).sign() ? -1:1))

bool Solver::find_equivs()
{
    double myTime = cpuTime();
    PicoSAT* picosat = picosat_init();
    for(uint32_t i = 0; i < nVars(); i++) picosat_inc_max_var(picosat);

    vector<vector<char>> tocheck(nVars());
    for(uint32_t v = 0; v < nVars(); v++) tocheck[v].resize(nVars(), 0);
    for(auto const& off: longIrredCls) {
        Clause* cl = cl_alloc.ptr(off);
        for(auto const& l1: *cl) {
            picosat_add(picosat, PICOLIT(l1));
            for(auto const& l2: *cl) {
                if (l1.var() < l2.var()) tocheck[l1.var()][l2.var()] = 1;
            }
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
            if (l1.var() < l2.var()) tocheck[l1.var()][l2.var()] = 1;
        }
    }
    double build_time =  (cpuTime()-myTime);

    uint32_t checked = 0;
    uint32_t added = 0;
    for(uint32_t i = 0; i < nVars(); i++) {
        for(uint32_t i2 = 0; i2 < nVars(); i2++) {
            if (i >= i2) continue;
            if (i == i2) continue;
            if (!tocheck[i][i2]) continue;

            Lit lit1 = Lit(i, false);
            Lit lit2 = Lit(i2, false);
            if (value(lit1) != l_Undef || value(lit2) != l_Undef ||
                varData[i].removed != Removed::none || varData[i2].removed != Removed::none)
                continue;

            checked++;
            assert(decisionLevel() == 0);
            assert(prop_at_head());

            int ret;
            picosat_assume(picosat, PICOLIT(lit1));
            picosat_assume(picosat, PICOLIT(lit2));
            ret = picosat_sat(picosat, 30);
            if (ret != PICOSAT_UNSATISFIABLE) goto next;

            picosat_assume(picosat, PICOLIT(~lit1));
            picosat_assume(picosat, PICOLIT(~lit2));
            ret = picosat_sat(picosat, 30);
            if (ret != PICOSAT_UNSATISFIABLE) goto next;
            added++;
            if (!add_xor_clause_inter(vector<Lit>{lit1, lit2}, true, true)) goto fin;
            continue;

            next:;
            picosat_assume(picosat, PICOLIT(lit1));
            picosat_assume(picosat, PICOLIT(~lit2));
            ret = picosat_sat(picosat, 30);
            if (ret != PICOSAT_UNSATISFIABLE) continue;

            picosat_assume(picosat, PICOLIT(~lit1));
            picosat_assume(picosat, PICOLIT(lit2));
            ret = picosat_sat(picosat, 30);
            if (ret != PICOSAT_UNSATISFIABLE) continue;
            added++;
            if (!add_xor_clause_inter(vector<Lit>{lit1, lit2}, false, true)) goto fin;
        }
    }
    fin:
    picosat_reset(picosat);

    verb_print(1, "[eqlit-find] checked: " << checked << " added: " << added << " T: " << (cpuTime()-myTime) << " buildT: " << build_time);
    return solver->okay();
}

inline int orclit(const Lit x) {
    return ((x).sign() ? (((x).var()+1)*2+1) : ((x).var()+1)*2);
}

inline int Neg(int x) {
	return x^1;
}

inline Lit orc_to_lit(int x) {
    uint32_t var = x/2-1;
    bool neg = x&1;
    return Lit(var, neg);
}


inline vector<int> Negate(vector<int> vec) {
	for (int& lit : vec) {
		lit = Neg(lit);
	}
	return vec;
}

template<typename T>
void SwapDel(vector<T>& vec, size_t i) {
	assert(i < vec.size());
	std::swap(vec[i], vec.back());
	vec.pop_back();
}

bool Solver::oracle_vivif(bool& finished)
{
    assert(!frat->enabled());
    assert(solver->okay());
    double myTime = cpuTime();

    vector<vector<int>> clauses;
    vector<int> tmp;
    for (const auto& off: longIrredCls) {
        Clause& cl = *cl_alloc.ptr(off);
        tmp.clear();
        for (auto const& l: cl) {
            tmp.push_back(orclit(l));
        }
//         std::sort(tmp.begin(), tmp.end());
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

    for(auto& ws: watches) {
        uint32_t j = 0;
        for(uint32_t i = 0; i < ws.size(); i++) {
            if (ws[i].isBin()) {
                if (ws[i].red()) ws[j++] = ws[i];
                continue;
            }
            assert(!ws[i].isBNN());
            assert(ws[i].isClause());
            Clause* cl = cl_alloc.ptr(ws[i].get_offset());
            if (cl->red()) ws[j++] = ws[i];
            continue;
        }
        ws.resize(j);
    }
    binTri.irredBins = 0;
    for(auto& c: longIrredCls) free_cl(c);
    longIrredCls.clear();
    litStats.irredLits = 0;
    cl_alloc.consolidate(this, true);

    sspp::oracle::Oracle oracle(nVars(), clauses, {});
    bool sat = false;
    for (int i = 0; i < (int)clauses.size(); i++) {
        for (int j = 0; j < (int)clauses[i].size(); j++) {
            if (oracle.getStats().mems > 800LL*1000LL*1000LL) {
                finished = false;
                goto end;
            }
            auto assump = Negate(clauses[i]);
            SwapDel(assump, j);
            auto ret = oracle.Solve(assump, true, 500LL*1000LL*1000LL);
            if (ret.isUnknown()) {
                finished = false;
                goto end;
            }
            if (ret.isFalse()) {
                    sort(assump.begin(), assump.end());
                    auto clause = Negate(assump);
                    oracle.AddClauseIfNeeded(clause, true);
                    clauses[i] = clause;
                    j = -1;
                    if (clause.empty()) {
                            ok = false;
                            cout<<"c o UNSAT"<<endl;
                            return false;
                    }
            } else if(!sat) {
                    sat = true;
                    cout<<"c o SAT"<<endl;
            }
        }
    }

    end:
    vector<Lit> tmp2;
    for(const auto& cl: clauses) {
        tmp2.clear();
        for(const auto& l: cl) tmp2.push_back(orc_to_lit(l));
        Clause* cl2 = solver->add_clause_int(tmp2);
        if (cl2) longIrredCls.push_back(cl_alloc.get_offset(cl2));
        if (!okay()) return false;
    }

//     for (const auto& cl: oracle.LearnedClauses()) {
//         tmp2.clear();
//         for(const auto& l: cl) tmp2.push_back(orc_to_lit(l));
//         Clause* cl2 = solver->add_clause_int(tmp2, true);
//         if (cl2) longIrredCls.push_back(cl_alloc.get_offset(cl2));
//         if (!okay()) return false;
//     }

    verb_print(1, "[oracle-vivif] finished: " << finished << " T: " << (cpuTime()-myTime));
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
        if (c.which == 0) {
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

bool Solver::sparsify()
{
    assert(!frat->enabled());
    execute_inprocess_strategy(false, "occ-backw-sub, sub-impl, must-renumber");
    if (!solver->okay()) return solver->okay();
    if (nVars() < 10) return solver->okay();

    double myTime = cpuTime();
    uint32_t removed = 0;
    uint32_t removed_bin = 0;

    vector<vector<int>> edgew(nVars());
    for (uint32_t i = 0; i < nVars(); i++) edgew[i].resize(nVars());

    for (const auto& off: longIrredCls) {
        Clause& cl = *cl_alloc.ptr(off);
        for (auto const& l1 : cl) {
            for (auto const& l2 : cl) {
                uint32_t v1 = l1.var();
                uint32_t v2 = l2.var();
                if (v1 < v2) {
                    edgew[v1][v2]++;
                    //assert(edgew[v1][v2] <= (int)clauses.size());
                }
            }
        }
    }
    for (uint32_t i = 0; i < nVars()*2; i++) {
        Lit l = Lit::toLit(i);
        for(auto const& ws: watches[l]) {
            if (!ws.isBin() || ws.red())  continue;
            uint32_t v1 = l.var();
            uint32_t v2 = ws.lit2().var();
            if (v1 < v2) {
                edgew[v1][v2]++;
                //assert(edgew[v1][v2] <= (int)clauses.size());
            }
        }
    }

    //[vector<int>, clause] pairs. vector<INT> =
    vector<OracleDat> cs;
    for (const auto& off: longIrredCls) {
        Clause& cl = *cl_alloc.ptr(off);
        assert(!cl.red());
        vector<int> ww;
        for (auto const& l1 : cl) {
            for (auto const& l2: cl) {
                uint32_t v1 = l1.var();
                uint32_t v2 = l2.var();
                if (v1 < v2) {
                    assert(edgew[v1][v2] >= 1);
                    if ((int)ww.size() < edgew[v1][v2]) {
                        ww.resize(edgew[v1][v2]);
                    }
                    ww[edgew[v1][v2]-1]--;
                }
            }
        }
//         cout << "CL is: " << cl << endl;
        cs.push_back(OracleDat(ww, off));
    }

    for (uint32_t i = 0; i < nVars()*2; i++) {
        Lit l = Lit::toLit(i);
        vector<int> ww;
        for(auto const& ws: watches[l]) {
            if (!ws.isBin() || ws.red())  continue;
            uint32_t v1 = l.var();
            uint32_t v2 = ws.lit2().var();
            if (v1 < v2) {
                assert(edgew[v1][v2] >= 1);
                if ((int)ww.size() < edgew[v1][v2]) {
                    ww.resize(edgew[v1][v2]);
                }
                ww[edgew[v1][v2]-1]--;
                cs.push_back(OracleDat(ww, OracleBin(l, ws.lit2(), ws.get_ID())));
            }
        }
    }


    std::sort(cs.begin(), cs.end());
    const uint32_t tot_cls = longIrredCls.size() + binTri.irredBins;
    assert(cs.size() == tot_cls);
    //dump_cls_oracle("debug.xt");

    sspp::oracle::Oracle oracle(nVars()+tot_cls, {});
    vector<sspp::Lit> tmp;
    for(uint32_t i = 0; i < cs.size(); i++) {
        const auto& c = cs[i];
        tmp.clear();
        if (c.which == 0) {
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
        tmp.push_back(orclit(Lit(nVars()+i, false)));
        oracle.AddClause(tmp, false);
    }
    const double build_time = cpuTime() - myTime;

    for (uint32_t i = 0; i < tot_cls; i++) {
        oracle.SetAssumpLit(orclit(Lit(nVars()+i, true)), false);
    }

    uint32_t last_printed = 0;
    for (uint32_t i = 0; i < tot_cls; i++) {
        if ((10*i)/(tot_cls) != last_printed) {
            verb_print(1, "[sparsify] done with " << ((10*i)/(tot_cls))*10 << " %"
                << " oracle props: "
                << print_value_kilo_mega(oracle.getStats().mems)
                << " T: " << (cpuTime()-myTime));
            last_printed = (10*i)/(tot_cls);
        }

        oracle.SetAssumpLit(orclit(Lit(nVars()+i, false)), false);
        tmp.clear();
        const auto& c = cs[i];
        if (c.which == 0) {
            Clause& cl = *cl_alloc.ptr(c.off);
            for(auto const& l: cl) tmp.push_back(orclit(~l));
        } else {
            tmp.push_back(orclit(~(c.bin.l1)));
            tmp.push_back(orclit(~(c.bin.l2)));
        }

        auto ret = oracle.Solve(tmp, false, 600LL*1000LL*1000LL);
        if (ret.isUnknown()) {
            goto fin;
        }

        if (ret.isTrue()) {
            oracle.SetAssumpLit(orclit(Lit(nVars()+i, true)), true);
        } else {
            assert(ret.isFalse());
            oracle.SetAssumpLit(orclit(Lit(nVars()+i, false)), true);
            removed++;
            if (c.which == 0) {
                Clause& cl = *cl_alloc.ptr(c.off);
                assert(!cl.stats.marked_clause);
                cl.stats.marked_clause = 1;
            } else {
                removed_bin++;
                Lit lit1 = c.bin.l1;
                Lit lit2 = c.bin.l2;
                findWatchedOfBin(watches, lit1, lit2, false, c.bin.ID).mark_bin_cl();
                findWatchedOfBin(watches, lit2, lit1, false, c.bin.ID).mark_bin_cl();
                binTri.irredBins--;
            }
        }

        if (oracle.getStats().mems > 700LL*1000LL*1000LL) {
            verb_print(1, "[sparsify] too many props in oracle, aborting");
            goto fin;
        }
    }

    fin:
    for(auto& ws: watches) {
        uint32_t j = 0;
        for(uint32_t i = 0; i < ws.size(); i++) {
            if (ws[i].isBNN()) {
                ws[j++] = ws[i];
                continue;
            } else if (ws[i].isBin()) {
                if (!ws[i].bin_cl_marked()) ws[j++] = ws[i];
                continue;
            } else if (ws[i].isClause()) {
                Clause* cl = cl_alloc.ptr(ws[i].get_offset());
                if (!cl->stats.marked_clause) ws[j++] = ws[i];
                continue;
            }
        }
        ws.shrink(ws.size()-j);
    }

    uint32_t j = 0;
    for(uint32_t i = 0; i < longIrredCls.size(); i++) {
        ClOffset off = longIrredCls[i];
        Clause* cl = cl_alloc.ptr(off);
        if (!cl->stats.marked_clause) {
            longIrredCls[j++] = longIrredCls[i];
        } else {
            litStats.irredLits -= cl->size();
            cl_alloc.clauseFree(off);
        }
    }
    longIrredCls.resize(j);

    //cout << "New cls size: " << clauses.size() << endl;
    //Subsume();

    verb_print(1, "[sparsify] removed: " << removed
        << " of which bin: " << removed_bin
        << " tot considered: " << tot_cls
        << " T: " << (cpuTime()-myTime) << " buildT: " << build_time);

    return solver->okay();
}

#ifdef STATS_NEEDED
void Solver::dump_clauses_at_finishup_as_last()
{
    if (!sqlStats)
        return;

    for(auto& red_cls: longRedCls) {
        for(auto& offs: red_cls) {
            Clause* cl = cl_alloc.ptr(offs);
            if (cl->stats.ID != 0) {
                sqlStats->cl_last_in_solver(solver, cl->stats.ID);
            }
        }
    }
}
#endif
