/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2015, Mate Soos. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation
 * version 2.0 of the License.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301  USA
*/

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

#include "varreplacer.h"
#include "time_mem.h"
#include "searcher.h"
#include "occsimplifier.h"
#include "prober.h"
#include "distiller.h"
#include "clausecleaner.h"
#include "solutionextender.h"
#include "varupdatehelper.h"
#include "gatefinder.h"
#include "sqlstats.h"
#include "completedetachreattacher.h"
#include "compfinder.h"
#include "comphandler.h"
#include "subsumestrengthen.h"
#include "watchalgos.h"
#include "clauseallocator.h"
#include "subsumeimplicit.h"
#include "strengthener.h"
#include "datasync.h"
#include "reducedb.h"
#include "clausedumper.h"
#include "sccfinder.h"
#include "intree.h"
#include "features_calc.h"
#include "GitSHA1.h"
#include "features_to_reconf.h"
#include "trim.h"

using namespace CMSat;
using std::cout;
using std::endl;

#ifdef USE_MYSQL
#include "mysqlstats.h"
#endif

#ifdef USE_SQLITE3
#include "sqlitestats.h"
#endif

//#define DRUP_DEBUG

//#define DEBUG_RENUMBER

//#define DEBUG_TRI_SORTED_SANITY

Solver::Solver(const SolverConf *_conf, bool* _needToInterrupt) :
    Searcher(_conf, this, _needToInterrupt)
{
    parse_sql_option();

    if (conf.doProbe) {
        prober = new Prober(this);
    }
    intree = new InTree(this);
    if (conf.perform_occur_based_simp) {
        simplifier = new OccSimplifier(this);
    }
    distiller = new Distiller(this);
    strengthener = new Strengthener(this);
    clauseCleaner = new ClauseCleaner(this);
    varReplacer = new VarReplacer(this);
    if (conf.doCompHandler) {
        compHandler = new CompHandler(this);
    }
    if (conf.doStrSubImplicit) {
        subsumeImplicit = new SubsumeImplicit(this);
    }
    datasync = new DataSync(this, NULL);
    Searcher::solver = this;
    reduceDB = new ReduceDB(this);
}

Solver::~Solver()
{
    delete compHandler;
    delete sqlStats;
    delete prober;
    delete intree;
    delete simplifier;
    delete distiller;
    delete strengthener;
    delete clauseCleaner;
    delete varReplacer;
    delete subsumeImplicit;
    delete datasync;
    delete reduceDB;
}

void Solver::parse_sql_option()
{
    if (conf.doSQL > 2) {
        std::cerr << "ERROR: '--sql'  option must be given value 0..2"
        << endl;
        std::exit(-1);
    }
    if (conf.whichSQL > 3) {
        std::cerr << "ERROR: '--wsql'  option must be given value 0..3"
        << endl;
        std::exit(-1);
    }

    sqlStats = NULL;
    if (conf.doSQL) {
        if (conf.whichSQL == 2) {
            #if defined(USE_MYSQL)
            sqlStats = new MySQLStats();
            #else
            if (conf.doSQL >= 2) {
                std::cerr << "MySQL support was not compiled in, cannot use it. Exiting."
                << endl;
                std::exit(-1);
            }
            #endif
        }

        if (conf.whichSQL == 3) {
            #if defined(USE_SQLITE3)
            sqlStats = new SQLiteStats();
            #else
            if (conf.doSQL == 2) {
                std::cerr << "SQLite support was not compiled in, cannot use it. Exiting."
                << endl;
                std::exit(-1);
            }
            #endif
        }

        if (conf.whichSQL == 0) {
            #if defined(USE_MYSQL)
            sqlStats = new MySQLStats();
            #elif defined(USE_SQLITE3)
            sqlStats = new SQLiteStats();
            #else
            if (conf.doSQL == 2) {
                std::cerr << "Neither MySQL nor SQLite support was compiled in"
                << ", cannot use either. Exiting." << endl;
                std::exit(-1);
            }
            #endif
        }

        if (conf.whichSQL == 1) {
            #if defined(USE_SQLITE3)
            sqlStats = new SQLiteStats();
            #elif defined(USE_MYSQL)
            sqlStats = new MySQLStats();
            #else
            if (conf.doSQL == 2) {
                std::cerr << "Neither MySQL nor SQLite support was compiled in"
                << ", cannot use either. Exiting." << endl;
                std::exit(-1);
            }
            #endif
        }
    }
}

void Solver::set_shared_data(SharedData* shared_data)
{
    delete datasync;
    datasync = new DataSync(this, shared_data);
}

bool Solver::add_xor_clause_inter(
    const vector<Lit>& lits
    , bool rhs
    , const bool attach
    , bool addDrup
) {
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
    std::sort(ps.begin(), ps.end());
    Lit p;
    uint32_t i, j;
    for (i = j = 0, p = lit_Undef; i != ps.size(); i++) {
        assert(ps[i].sign() == false);

        if (ps[i].var() == p.var()) {
            //added, but easily removed
            j--;
            p = lit_Undef;

            //Flip rhs if neccessary
            if (value(ps[i]) != l_Undef) {
                rhs ^= value(ps[i]) == l_True;
            }

        } else if (value(ps[i]) == l_Undef) {
            //Add and remember as last one to have been added
            ps[j++] = p = ps[i];

            assert(varData[p.var()].removed != Removed::elimed);
        } else {
            //modify rhs instead of adding
            assert(value(ps[i]) != l_Undef);
            rhs ^= value(ps[i]) == l_True;
        }
    }
    ps.resize(ps.size() - (i - j));

    if (ps.size() > (0x01UL << 18)) {
        cout << "Too long clause!" << endl;
        std::exit(-1);
    }
    //cout << "Cleaned ps is: " << ps << endl;

    if (!ps.empty()) {
        ps[0] ^= rhs;
    } else {
        if (rhs) {
            *drup << fin;
            ok = false;
        }
        return ok;
    }

    //cout << "without rhs is: " << ps << endl;
    add_every_combination_xor(ps, attach, addDrup);

    return ok;
}

void Solver::add_every_combination_xor(
    const vector<Lit>& lits
    , const bool attach
    , const bool addDrup
) {
    //cout << "add_every_combination got: " << lits << endl;

    size_t at = 0;
    size_t num = 0;
    vector<Lit> xorlits;
    Lit lastlit_added = lit_Undef;
    while(at != lits.size()) {
        xorlits.clear();
        size_t last_at = at;
        for(; at < last_at+2 && at < lits.size(); at++) {
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
            const Var newvar = nVars()-1;
            const Lit toadd = Lit(newvar, false);
            xorlits.push_back(toadd);
            lastlit_added = toadd;
        }

        add_xor_clause_inter_cleaned_cut(xorlits, attach, addDrup);
        if (!ok)
            break;

        num++;
    }
}

void Solver::add_xor_clause_inter_cleaned_cut(
    const vector<Lit>& lits
    , const bool attach
    , const bool addDrup
) {
    //cout << "xor_inter_cleaned_cut got: " << lits << endl;
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
        Clause* cl = add_clause_int(new_lits, false, ClauseStats(), attach, NULL, addDrup);
        if (cl) {
            longIrredCls.push_back(cl_alloc.get_offset(cl));
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


bool Solver::sort_and_clean_clause(vector<Lit>& ps, const vector<Lit>& origCl)
{
    std::sort(ps.begin(), ps.end());
    Lit p = lit_Undef;
    uint32_t i, j;
    for (i = j = 0; i != ps.size(); i++) {
        if (value(ps[i]) == l_True || ps[i] == ~p)
            return false;
        else if (value(ps[i]) != l_False && ps[i] != p) {
            ps[j++] = p = ps[i];

            if (varData[p.var()].removed != Removed::none) {
                cout << "ERROR: clause " << origCl << " contains literal "
                << p << " whose variable has been removed (removal type: "
                << removed_type_to_string(varData[p.var()].removed)
                << " var-updated lit: "
                << varReplacer->get_var_replaced_with(p)
                << ")"
                << endl;
            }

            //Variables that have been eliminated cannot be added internally
            //as part of a clause. That's a bug
            assert(varData[p.var()].removed == Removed::none);
        }
    }
    ps.resize(ps.size() - (i - j));
    return true;
}

/**
@brief Adds a clause to the problem. Should ONLY be called internally

This code is very specific in that it must NOT be called with varibles in
"ps" that have been replaced, eliminated, etc. Also, it must not be called
when the wer are in an UNSAT (!ok) state, for example. Use it carefully,
and only internally
*/
Clause* Solver::add_clause_int(
    const vector<Lit>& lits
    , const bool red
    , ClauseStats stats
    , const bool attach
    , vector<Lit>* finalLits
    , bool addDrup
    , const Lit drup_first
) {
    assert(ok);
    assert(decisionLevel() == 0);
    assert(!attach || qhead == trail.size());
    #ifdef VERBOSE_DEBUG
    cout << "add_clause_int clause " << lits << endl;
    #endif //VERBOSE_DEBUG

    //Make stats sane
    #ifdef STATS_NEEDED
    stats.introduced_at_conflict = std::min<uint64_t>(Searcher::sumConflicts(), stats.introduced_at_conflict);
    #endif

    vector<Lit> ps = lits;
    if (!sort_and_clean_clause(ps, lits)) {
        return NULL;
    }

    #ifdef VERBOSE_DEBUG
    cout << "add_clause_int final clause " << ps << endl;
    #endif

    //If caller required final set of lits, return it.
    if (finalLits) {
        *finalLits = ps;
    }

    if (addDrup) {
        size_t i = 0;
        if (drup_first != lit_Undef) {
            for(i = 0; i < ps.size(); i++) {
                if (ps[i] == drup_first) {
                    break;
                }
            }
        }
        std::swap(ps[0], ps[i]);
        *drup << ps << fin;
        std::swap(ps[0], ps[i]);

        if (ps.size() == 2) {
            datasync->signalNewBinClause(ps);
        }
    }

    //Handle special cases
    switch (ps.size()) {
        case 0:
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
            enqueue(ps[0]);
            #ifdef STATS_NEEDED
            propStats.propsUnit++;
            #endif
            if (attach) {
                ok = (propagate<true>().isNULL());
            }

            return NULL;
        case 2:
            attach_bin_clause(ps[0], ps[1], red);
            return NULL;

        case 3:
            attach_tri_clause(ps[0], ps[1], ps[2], red);
            return NULL;

        default:
            Clause* c = cl_alloc.Clause_new(ps, sumStats.conflStats.numConflicts);
            if (red)
                c->makeRed(stats.glue);
            c->stats = stats;

            //In class 'OccSimplifier' we don't need to attach normall
            if (attach) {
                attachClause(*c);
            } else {
                if (red)
                    litStats.redLits += ps.size();
                else
                    litStats.irredLits += ps.size();
            }

            return c;
    }
}

void Solver::attachClause(
    const Clause& cl
    , const bool checkAttach
) {
    #if defined(DRUP_DEBUG) && defined(DRUP)
    if (drup) {
        for(size_t i = 0; i < cl.size(); i++) {
            *drup << cl[i];
        }
        *drup << fin;
    }
    #endif

    //Update stats
    if (cl.red())
        litStats.redLits += cl.size();
    else
        litStats.irredLits += cl.size();

    //Call Solver's function for heavy-lifting
    PropEngine::attachClause(cl, checkAttach);
}

void Solver::attach_tri_clause(
    const Lit lit1
    , const Lit lit2
    , const Lit lit3
    , const bool red
) {
    #if defined(DRUP_DEBUG) && defined(DRUP)
    if (drup) {
        *drup << lit1  << lit2  << lit3 << fin;
    }
    #endif

    //Update stats
    if (red) {
        binTri.redTris++;
    } else {
        binTri.irredTris++;
    }

    //Call Solver's function for heavy-lifting
    PropEngine::attach_tri_clause(lit1, lit2, lit3, red);
}

void Solver::attach_bin_clause(
    const Lit lit1
    , const Lit lit2
    , const bool red
    , const bool checkUnassignedFirst
) {
    #if defined(DRUP_DEBUG)
    *drup << lit1 << lit2 << fin;
    #endif

    //Update stats
    if (red) {
        binTri.redBins++;
    } else {
        binTri.irredBins++;
    }
    binTri.numNewBinsSinceSCC++;

    //Call Solver's function for heavy-lifting
    PropEngine::attach_bin_clause(lit1, lit2, red, checkUnassignedFirst);
}

void Solver::detach_tri_clause(
    const Lit lit1
    , const Lit lit2
    , const Lit lit3
    , const bool red
    , const bool allow_empty_watch
) {
    if (red) {
        binTri.redTris--;
    } else {
        binTri.irredTris--;
    }

    PropEngine::detach_tri_clause(lit1, lit2, lit3, red, allow_empty_watch);
}

void Solver::detach_bin_clause(
    const Lit lit1
    , const Lit lit2
    , const bool red
    , const bool allow_empty_watch
) {
    if (red) {
        binTri.redBins--;
    } else {
        binTri.irredBins--;
    }

    PropEngine::detach_bin_clause(lit1, lit2, red, allow_empty_watch);
}

void Solver::detachClause(const Clause& cl, const bool removeDrup)
{
    if (removeDrup) {
        *drup << del << cl << fin;
    }

    assert(cl.size() > 3);
    detach_modified_clause(cl[0], cl[1], cl.size(), &cl);
}

void Solver::detachClause(const ClOffset offset, const bool removeDrup)
{
    Clause* cl = cl_alloc.ptr(offset);
    detachClause(*cl, removeDrup);
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
    PropEngine::detach_modified_clause(lit1, lit2, origSize, address);
}

bool Solver::addClauseHelper(vector<Lit>& ps)
{
    //If already UNSAT, just return
    if (!ok)
        return false;

    //Sanity checks
    assert(decisionLevel() == 0);
    assert(qhead == trail.size());

    //Check for too long clauses
    if (ps.size() > (0x01UL << 18)) {
        cout << "Too long clause!" << endl;
        std::exit(-1);
    }

    //Check for too large variable number
    for (Lit& lit: ps) {
        if (lit.var() >= nVarsOuter()) {
            std::cerr
            << "ERROR: Variable " << lit.var() + 1
            << " inserted, but max var is "
            << nVarsOuter()
            << endl;
            assert(false);
            std::exit(-1);
        }
        assert(lit.var() < nVarsOuter()
        && "Clause inserted, but variable inside has not been declared with new_var() !");

        //Undo var replacement
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
            new_var(false, lit.var());
        }
    }

    renumber_outer_to_inter_lits(ps);

    #ifdef SLOW_DEBUG
    //Check renumberer
    for (const Lit lit: ps) {
        const Lit updated_lit = varReplacer->get_lit_replaced_with(lit);
        assert(lit == updated_lit);
    }
    #endif

     //Undo comp handler
    if (compHandler) {
        bool readd = false;
        for (Lit lit: ps) {
            if (varData[lit.var()].removed == Removed::decomposed) {
                readd = true;
                break;
            }
        }

        if (readd) {
            compHandler->readdRemovedClauses();
        }
    }

    //Uneliminate vars
    for (const Lit lit: ps) {
        if (conf.perform_occur_based_simp
            && varData[lit.var()].removed == Removed::elimed
        ) {
            #ifdef VERBOSE_DEBUG_RECONSTRUCT
            cout << "Uneliminating var " << lit.var() + 1 << endl;
            #endif
            if (!simplifier->uneliminate(lit.var()))
                return false;
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

bool Solver::addClause(const vector<Lit>& lits)
{
    if (conf.perform_occur_based_simp && simplifier->getAnythingHasBeenBlocked()) {
        std::cerr
        << "ERROR: Cannot add new clauses to the system if blocking was"
        << " enabled. Turn it off from conf.doBlockClauses"
        << endl;
        std::exit(-1);
    }

    #ifdef VERBOSE_DEBUG
    cout << "Adding clause " << lits << endl;
    #endif //VERBOSE_DEBUG
    const size_t origTrailSize = trail.size();

    vector<Lit> ps = lits;

    if (!addClauseHelper(ps)) {
        return false;
    }

    finalCl_tmp.clear();
    std::sort(ps.begin(), ps.end());
    Clause* cl = add_clause_int(
        ps
        , false //irred
        , ClauseStats() //default stats
        , true //yes, attach
        , &finalCl_tmp
        , false
    );

    //Drup -- We manipulated the clause, delete
    if (drup->enabled()
        && ps != finalCl_tmp
    ) {
        //Dump only if non-empty (UNSAT handled later)
        if (!finalCl_tmp.empty()) {
            *drup << finalCl_tmp << fin;
        }

        //Empty clause, it's UNSAT
        if (!okay()) {
            *drup << fin;
        }
        *drup << del << ps << fin;
    }

    if (cl != NULL) {
        ClOffset offset = cl_alloc.get_offset(cl);
        longIrredCls.push_back(offset);
    }

    zeroLevAssignsByCNF += trail.size() - origTrailSize;

    return ok;
}

/*bool Solver::addRedClause(
    const vector<Lit>& lits
    , const ClauseStats& stats
) {
    vector<Lit> ps(lits.size());
    std::copy(lits.begin(), lits.end(), ps.begin());

    if (!addClauseHelper(ps))
        return false;

    Clause* cl = add_clause_int(ps, true, stats);
    if (cl != NULL) {
        ClOffset offset = cl_alloc.get_offset(cl);
        longRedCls.push_back(offset);
    }

    return ok;
}*/

void Solver::test_renumbering() const
{
    //Check if we renumbered the varibles in the order such as to make
    //the unknown ones first and the known/eliminated ones second
    bool uninteresting = false;
    bool problem = false;
    for(size_t i = 0; i < nVars(); i++) {
        //cout << "val[" << i << "]: " << value(i);

        if (value(i)  != l_Undef)
            uninteresting = true;

        if (varData[i].removed == Removed::elimed
            || varData[i].removed == Removed::replaced
            || varData[i].removed == Removed::decomposed
        ) {
            uninteresting = true;
            //cout << " removed" << endl;
        } else {
            //cout << " non-removed" << endl;
        }

        if (value(i) == l_Undef
            && varData[i].removed != Removed::elimed
            && varData[i].removed != Removed::replaced
            && varData[i].removed != Removed::decomposed
            && uninteresting
        ) {
            problem = true;
        }
    }
    assert(!problem && "We renumbered the variables in the wrong order!");
}

void Solver::renumber_clauses(const vector<Var>& outerToInter)
{
    //Clauses' abstractions have to be re-calculated
    for(size_t i = 0; i < longIrredCls.size(); i++) {
        Clause* cl = cl_alloc.ptr(longIrredCls[i]);
        updateLitsMap(*cl, outerToInter);
        cl->setStrenghtened();
    }

    for(size_t i = 0; i < longRedCls.size(); i++) {
        Clause* cl = cl_alloc.ptr(longRedCls[i]);
        updateLitsMap(*cl, outerToInter);
        cl->setStrenghtened();
    }
}

size_t Solver::calculate_interToOuter_and_outerToInter(
    vector<Var>& outerToInter
    , vector<Var>& interToOuter
) {
    size_t at = 0;
    vector<Var> useless;
    size_t numEffectiveVars = 0;
    for(size_t i = 0; i < nVars(); i++) {
        if (value(i) != l_Undef
            || varData[i].removed == Removed::elimed
            || varData[i].removed == Removed::replaced
            || varData[i].removed == Removed::decomposed
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
    for(vector<Var>::const_iterator
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

//Beware. Cannot be called while Searcher is running.
void Solver::renumber_variables()
{
    double myTime = cpuTime();
    clauseCleaner->remove_and_clean_all();

    //outerToInter[10] = 0 ---> what was 10 is now 0.
    vector<Var> outerToInter(nVarsOuter());
    vector<Var> interToOuter(nVarsOuter());
    size_t numEffectiveVars =
        calculate_interToOuter_and_outerToInter(outerToInter, interToOuter);

    //Create temporary outerToInter2
    vector<uint32_t> interToOuter2(nVarsOuter()*2);
    for(size_t i = 0; i < nVarsOuter(); i++) {
        interToOuter2[i*2] = interToOuter[i]*2;
        interToOuter2[i*2+1] = interToOuter[i]*2+1;
    }

    CNF::updateVars(outerToInter, interToOuter);
    PropEngine::updateVars(outerToInter, interToOuter, interToOuter2);
    Searcher::updateVars(outerToInter, interToOuter);

    if (conf.doStamp) {
        stamp.updateVars(outerToInter, interToOuter2, seen);
    }
    renumber_clauses(outerToInter);

    //Update sub-elements' vars
    varReplacer->updateVars(outerToInter, interToOuter);
    if (conf.doCache) {
        implCache.updateVars(seen, outerToInter, interToOuter2, numEffectiveVars);
    }
    datasync->updateVars(outerToInter, interToOuter);

    //Tests
    test_renumbering();
    test_reflectivity_of_renumbering();

    //Print results
    const double time_used = cpuTime() - myTime;
    if (conf.verbosity >= 2) {
        cout
        << "c [renumber] T: "
        << std::fixed << std::setw(5) << std::setprecision(2)
        << time_used
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

    //NOTE order heap is now wrong, but that's OK, it will be restored from
    //backed up activities and then rebuilt at the start of Searcher
}

void Solver::check_switchoff_limits_newvar(size_t n)
{
    if (conf.doStamp
        && nVars() + n > 15ULL*1000ULL*1000ULL
    ) {
        conf.doStamp = false;
        stamp.freeMem();
        if (conf.verbosity >= 2) {
            cout
            << "c Switching off stamping due to excessive number of variables"
            << " (it would take too much memory)"
            << endl;
        }
    }

    if (conf.doCache
        && nVars() + n > 8ULL*1000ULL*1000ULL
    ) {
        conf.doCache = false;
        implCache.free();

        if (conf.verbosity >= 2) {
            cout
            << "c Switching off caching due to excessive number of variables"
            << " (it would take too much memory)"
            << endl;
        }
    }

    if (conf.perform_occur_based_simp
        && conf.doFindXors
        && nVars() + n > 3ULL*1000ULL*1000ULL
    ) {
        conf.doFindXors = false;
        simplifier->freeXorMem();

        if (conf.verbosity >= 2) {
            cout
            << "c Switching off XOR finding due to excessive number of variables"
            << " (it would take too much memory)"
            << endl;
        }
    }
}

void Solver::new_vars(size_t n)
{
    if (n == 0) {
        return;
    }

    check_switchoff_limits_newvar(n);
    Searcher::new_vars(n);
    if (conf.doCache) {
        litReachable.resize(litReachable.size()+n*2, LitReachData());
    }
    varReplacer->new_vars(n);

    if (conf.perform_occur_based_simp) {
        simplifier->new_vars(n);
    }

    if (compHandler) {
        compHandler->new_vars(n);
    }
    datasync->new_vars(n);
}

void Solver::new_var(const bool bva, const Var orig_outer)
{
    check_switchoff_limits_newvar();
    Searcher::new_var(bva, orig_outer);
    if (conf.doCache) {
        litReachable.push_back(LitReachData());
        litReachable.push_back(LitReachData());
    }

    varReplacer->new_var(orig_outer);

    if (conf.perform_occur_based_simp) {
        simplifier->new_var(orig_outer);
    }

    if (compHandler) {
        compHandler->new_var(orig_outer);
    }
    if (orig_outer == std::numeric_limits<Var>::max()) {
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

    litReachable.resize(nVars()*2);
    litReachable.shrink_to_fit();
    varReplacer->save_on_var_memory();
    if (simplifier) {
        simplifier->save_on_var_memory();
    }
    if (compHandler) {
        compHandler->save_on_var_memory();
    }
    datasync->save_on_var_memory();
    assumptionsSet.resize(nVars());
    assumptionsSet.shrink_to_fit();

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
    assert(okay());

    conflict.clear();
    assumptions.clear();
    assumptionsSet.clear();
    if (outside_assumptions.empty()) {
        assumptionsSet.shrink_to_fit();
        return;
    }

    back_number_from_outside_to_outer(outside_assumptions);
    vector<Lit> inter_assumptions = back_number_from_outside_to_outer_tmp;
    addClauseHelper(inter_assumptions);
    assumptionsSet.resize(nVars(), false);

    assert(inter_assumptions.size() == outside_assumptions.size());
    for(size_t i = 0; i < inter_assumptions.size(); i++) {
        const Lit inter_lit = inter_assumptions[i];
        const Lit outside_lit = outside_assumptions[i];
        assumptions.push_back(AssumptionPair(inter_lit, outside_lit));
    }

    fill_assumptions_set_from(assumptions);
}

void Solver::check_model_for_assumptions() const
{
    for(const AssumptionPair lit_pair: assumptions) {
        const Lit outside_lit = lit_pair.lit_orig_outside;
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
            << " was in the assumptions, but it was set to its opposite value!"
            << endl;
        }
        assert(model_value(outside_lit) == l_True);
    }
}

void Solver::check_recursive_minimization_effectiveness(const lbool status)
{
    const Searcher::Stats& stats = Searcher::get_stats();
    if (status == l_Undef
        && conf.doRecursiveMinim
        && stats.recMinLitRem + stats.litsRedNonMin > 100000
    ) {
        double remPercent =
            (double)stats.recMinLitRem/(double)stats.litsRedNonMin*100.0;

        double costPerGained = (double)stats.recMinimCost/remPercent;
        if (costPerGained > 200ULL*1000ULL*1000ULL) {
            conf.doRecursiveMinim = false;
            if (conf.verbosity >= 2) {
                cout
                << "c recursive minimization too costly: "
                << std::fixed << std::setprecision(0) << (costPerGained/1000.0)
                << "Kcost/(% lits removed) --> disabling"
                << endl;
            }
        } else {
            if (conf.verbosity >= 2) {
                cout
                << "c recursive minimization cost OK: "
                << std::fixed << std::setprecision(0) << (costPerGained/1000.0)
                << "Kcost/(% lits removed)"
                << endl;
            }
        }
    }
}

void Solver::check_minimization_effectiveness(const lbool status)
{
    const Searcher::Stats& search_stats = Searcher::get_stats();
    if (status == l_Undef
        && conf.doMinimRedMore
        && search_stats.moreMinimLitsStart > 100000
    ) {
        double remPercent =
            (double)(search_stats.moreMinimLitsStart-search_stats.moreMinimLitsEnd)/
                (double)(search_stats.moreMinimLitsStart)*100.0;

        //TODO take into account the limit on the number of first literals, too
        if (remPercent < 1.0) {
            conf.doMinimRedMore = false;
            if (conf.verbosity >= 2) {
                cout
                << "c more minimization effectiveness low: "
                << std::fixed << std::setprecision(2) << remPercent
                << " % lits removed --> disabling"
                << endl;
            }
        } else if (remPercent > 7.0) {
            more_red_minim_limit_binary_actual = 3*conf.more_red_minim_limit_binary;
            more_red_minim_limit_cache_actual  = 3*conf.more_red_minim_limit_cache;
            if (conf.verbosity >= 2) {
                cout
                << "c more minimization effectiveness good: "
                << std::fixed << std::setprecision(2) << remPercent
                << " % --> increasing limit to 3x"
                << endl;
            }
        } else {
            more_red_minim_limit_binary_actual = conf.more_red_minim_limit_binary;
            more_red_minim_limit_cache_actual  = conf.more_red_minim_limit_cache;
            if (conf.verbosity >= 2) {
                cout
                << "c more minimization effectiveness OK: "
                << std::fixed << std::setprecision(2) << remPercent
                << " % --> setting limit to norm"
                << endl;
            }
        }
    }
}

void Solver::extend_solution()
{
    #ifdef DEBUG_IMPLICIT_STATS
    check_stats();
    #endif

    const double myTime = cpuTime();
    model = back_number_solution_from_inter_to_outer(model);

    //Extend solution to stored solution in component handler
    if (compHandler) {
        compHandler->addSavedState(model);
    }

    SolutionExtender extender(this, simplifier);
    extender.extend();

    model = map_back_to_without_bva(model);
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
    if (!sqlStats || solveStats.num_solve_calls > 1) {
        //Either it's already initialized, or it's not needed
        return;
    }

    bool ret = sqlStats->setup(this);
    if (!ret) {
        if (conf.doSQL == 2) {
            std::cerr
            << "c ERROR: SQL was required (with option '--sql 2'), but couldn't connect to SQL server." << endl;
            std::exit(-1);
        }
        delete sqlStats;
        sqlStats = NULL;
        conf.doSQL = false;
    }
}

void Solver::check_config_parameters() const
{
    if (conf.maxConfl < 0) {
        std::cerr << "Maximum number conflicts set must be greater or equal to 0" << endl;
        exit(-1);
    }

    if (conf.shortTermHistorySize <= 0) {
        std::cerr << "You MUST give a short term history size (\"--gluehist\")  greater than 0!" << endl;
        exit(-1);
    }
}

lbool Solver::solve()
{
    #ifdef SLOW_DEBUG
    assert(solver->check_order_heap_sanity());
    check_implicit_stats();
    #endif

    solveStats.num_solve_calls++;
    conflict.clear();
    check_config_parameters();
    if (solveStats.num_solve_calls > 1) {
        conf.do_calc_polarity_every_time = false;
    }

    if (conf.verbosity >= 6) {
        cout << "c Solver::solve() called" << endl;
    }
    conf.global_timeout_multiplier = solver->conf.orig_global_timeout_multiplier;

    set_up_sql_writer();

    //Check if adding the clauses caused UNSAT
    lbool status = l_Undef;
    if (!ok) {
        conflict.clear();
        status = l_False;
        if (conf.verbosity >= 6) {
            cout << "c Solver status l_Fase on startup of solve()" << endl;
        }
        goto end;
    }

    //Clean up as a startup
    datasync->rebuild_bva_map();
    set_assumptions();

    //If still unknown, simplify
    if (status == l_Undef
        && conf.simplify_at_startup
        && (solveStats.numSimplify == 0 || conf.simplify_at_every_startup)
        && conf.do_simplify_problem
        && nVars() > 0
    ) {
        status = simplify_problem(!conf.full_simplify_at_startup);
    }

    if (status == l_Undef) {
        status = iterate_until_solved();
    }
    handle_found_solution(status);

    end:
    if (sqlStats) {
        sqlStats->finishup(status);
    }

    return status;
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
        , "stamp"
        , my_time
        , Searcher::mem_used_stamp()/(1024*1024)
    );

    sqlStats->mem_used(
        this
        , "cache"
        , my_time
        , implCache.mem_used()/(1024*1024)
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


    if (compHandler) {
        sqlStats->mem_used(
            this
            , "component"
            , my_time
            , compHandler->mem_used()/(1024*1024)
        );
    }

    if (simplifier) {
        sqlStats->mem_used(
            this
            , "simplifier"
            , my_time
            , simplifier->mem_used()/(1024*1024)
        );

        sqlStats->mem_used(
            this
            , "xor"
            , my_time
            , simplifier->mem_used_xor()/(1024*1024)
        );
    }

    sqlStats->mem_used(
        this
        , "varreplacer"
        , my_time
        , varReplacer->mem_used()/(1024*1024)
    );

    if (prober) {
        sqlStats->mem_used(
            this
            , "prober"
            , my_time
            , prober->mem_used()/(1024*1024)
        );
    }

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

lbool Solver::iterate_until_solved()
{
    uint64_t backup_burst_len = conf.burst_search_len;
    conf.burst_search_len = 0;
    size_t iteration_num = 0;

    lbool status = l_Undef;
    while (status == l_Undef
        && !must_interrupt_asap()
        && cpuTime() < conf.maxTime
        && sumStats.conflStats.numConflicts < (uint64_t)conf.maxConfl
    ) {
        iteration_num++;
        if (conf.verbosity >= 2 && iteration_num >= 2) {
            print_clause_size_distrib();
        }
        if (iteration_num >= 2) {
            conf.burst_search_len = backup_burst_len;
        }
        dump_memory_stats_to_sql();

        const size_t origTrailSize = trail.size();
        long num_conflicts_of_search = conf.num_conflicts_of_search
            *(std::pow(conf.num_conflicts_of_search_inc, (double)iteration_num));
        if (conf.never_stop_search) {
            num_conflicts_of_search = 500ULL*1000ULL*1000ULL;
        }
        num_conflicts_of_search = std::min<long>(
            num_conflicts_of_search
            , (long)conf.maxConfl - (long)sumStats.conflStats.numConflicts
        );
        if (num_conflicts_of_search <= 0) {
            break;
        }
        status = Searcher::solve(num_conflicts_of_search, iteration_num);

        //Check for effectiveness
        check_recursive_minimization_effectiveness(status);
        check_minimization_effectiveness(status);

        //Update stats
        sumStats += Searcher::get_stats();
        sumPropStats += propStats;
        propStats.clear();
        Searcher::resetStats();

        //Solution has been found
        if (status != l_Undef) {
            break;
        }

        //If we are over the limit, exit
        if (sumStats.conflStats.numConflicts >= (uint64_t)conf.maxConfl
            || cpuTime() > conf.maxTime
            || must_interrupt_asap()
        ) {
            break;
        }

        zero_level_assigns_by_searcher += trail.size() - origTrailSize;

        if (conf.do_simplify_problem) {
            status = simplify_problem(false);
        }
    }

    conf.burst_search_len = backup_burst_len;
    return status;
}

void Solver::handle_found_solution(const lbool status)
{
    if (status == l_True) {
        extend_solution();
        cancelUntil(0);
    } else if (status == l_False) {
        cancelUntil(0);

        for(const Lit lit: conflict) {
            if (value(lit) == l_Undef) {
                assert(var_inside_assumptions(lit.var()));
            }
        }
        update_assump_conflict_to_orig_outside(conflict);
    }
    checkDecisionVarCorrectness();

    //Too slow when running lots of small queries
    #ifdef DEBUG_IMPLICIT_STATS
    check_implicit_stats();
    #endif
}

void Solver::checkDecisionVarCorrectness() const
{
    //Check for var deicisonness
    for(size_t var = 0; var < nVarsOuter(); var++) {
        if (varData[var].removed != Removed::none) {
            assert(!varData[var].is_decision);
        } else {
            assert(varData[var].is_decision);
        }
    }
}

bool Solver::execute_inprocess_strategy(
    const string& strategy
    , const bool startup
) {
    //std::string input = "abc,def,ghi";
    std::istringstream ss(strategy);
    std::string token;

    while(std::getline(ss, token, ',')) {
        if (sumStats.conflStats.numConflicts >= (uint64_t)conf.maxConfl
            || cpuTime() > conf.maxTime
            || must_interrupt_asap()
            || nVars() == 0
            || !ok
        ) {
            return ok;
        }

        trim(token);
        std::transform(token.begin(), token.end(), token.begin(), ::tolower);
        if (conf.verbosity >= 2) {
            cout << "c --> Executing strategy token: " << token << '\n';
        }
        if (token == "find-comps") {
            if (get_num_free_vars() < conf.compVarLimit) {
                CompFinder findParts(this);
                findParts.find_components();
            }
        } else if (token == "handle-comps") {
            if (compHandler
                && conf.doCompHandler
                && get_num_free_vars() < conf.compVarLimit
                && solveStats.numSimplify >= conf.handlerFromSimpNum
                //Only every 2nd, since it can be costly to find parts
                && solveStats.numSimplify % 2 == 0 //TODO
            ) {
                compHandler->handle();
            }
        }  else if (token == "scc-vrepl") {
            if (conf.doFindAndReplaceEqLits) {
                varReplacer->replace_if_enough_is_found(
                    floor((double)get_num_free_vars()*0.001));
            }
        } else if (token == "cache-clean") {
            if (conf.doCache) {
                implCache.clean(this);
            }
        } else if (token == "cache-tryboth") {
            if (conf.doCache) {
                implCache.tryBoth(this);
            }
        } else if (token == "sub-impl") {
            if (conf.doStrSubImplicit) {
                subsumeImplicit->subsume_implicit();
            }
        } else if (token == "intree-probe") {
            if (conf.doIntreeProbe) {
                intree->intree_probe();
            }
        } else if (token == "probe") {
            if (conf.doProbe)
                prober->probe();
        } else if (token == "str-cls") {
            if (conf.do_distill_clauses) {
                strengthener->strengthen(true);
            }
        } else if (token == "distill-cls") {
            if (conf.do_distill_clauses) {
                distiller->distill(true);
            }
        }  else if (token == "simplify") {
            //Var-elim, gates, subsumption, strengthening
            if (conf.perform_occur_based_simp
                && simplifier
            ) {
                simplifier->simplify(startup);
            }
        } else if (token == "str-impl") {
            if (conf.doStrSubImplicit) {
                strengthener->strengthen_implicit();
            }
        } else if (token == "check-cache-size") {
            //Delete and disable cache if too large
            if (conf.doCache) {
                const size_t memUsedMB = implCache.mem_used()/(1024UL*1024UL);
                if (memUsedMB > conf.maxCacheSizeMB) {
                    if (conf.verbosity >= 2) {
                        cout
                        << "c Turning off cache, memory used, "
                        << memUsedMB/(1024UL*1024UL) << " MB"
                        << " is over limit of " << conf.maxCacheSizeMB  << " MB"
                        << endl;
                    }
                    implCache.free();
                    vector<LitReachData> tmp;
                    litReachable.swap(tmp);
                    conf.doCache = false;
                }
            }
        } else if (token == "renumber") {
            if (conf.doRenumberVars) {
                //Clean cache before renumber -- very important, otherwise
                //we will be left with lits inside the cache that are out-of-bounds
                if (conf.doCache) {
                    bool setSomething = true;
                    while(setSomething) {
                        if (!implCache.clean(this, &setSomething))
                            return false;
                    }
                }

                renumber_variables();
            }
        } else if (token == "") {
            //Nothing, just an empty comma, ignore
        } else {
            cout << "ERROR: strategy " << token << " not recognised!" << endl;
            exit(-1);
        }

        if (!ok) {
            return ok;
        }
    }

    return ok;
}

/**
@brief The function that brings together almost all CNF-simplifications
*/
lbool Solver::simplify_problem(const bool startup)
{
    assert(ok);
    test_all_clause_attached();
    #ifdef DEBUG_IMPLICIT_STATS
    check_stats();
    #endif
    #ifdef SLOW_DEBUG
    assert(solver->check_order_heap_sanity());
    #endif

    update_polarity_and_activity = false;

    if (conf.verbosity >= 6) {
        cout
        << "c Solver::simplify_problem() called"
        << endl;
    }

    if (startup) {
        execute_inprocess_strategy(
            conf.simplify_at_startup_sequence
            , startup
        );
    } else {
        execute_inprocess_strategy(
            conf.simplify_nonstartup_sequence
            , startup
        );
    }

    //Free unused watch memory
    free_unused_watches();
    //addSymmBreakClauses();

    //Re-calculate reachability after re-numbering and new cache data
    //This may actually affect correctness/code sanity, so we must do it!
    if (conf.doCache) {
        calculate_reachability();
    }

    update_polarity_and_activity = true;
    if (conf.verbosity >= 3) {
        cout << "c Searcher::simplify_problem() finished" << endl;
    }
    test_all_clause_attached();
    check_wrong_attach();
    conf.global_timeout_multiplier *= solver->conf.global_timeout_multiplier_multiplier;

    //TODO not so good....
    //The algorithms above probably have changed the propagation&usage data
    //so let's clear it
    if (conf.doClearStatEveryClauseCleaning) {
        clear_clauses_stats();
    }

    if (nVars() > 2
        && (longIrredCls.size() > 1 ||
            (binTri.irredBins + binTri.redBins
             + binTri.irredTris + binTri.redTris) > 1
            )
    ) {
        if (solveStats.numSimplify == conf.reconfigure_at) {
            Features feat = calculate_features();
            if (conf.reconfigure_val == 100) {
                conf.reconfigure_val = get_reconf_from_features(feat, conf.verbosity);
            }
            if (conf.reconfigure_val != 0) {
                reconfigure(conf.reconfigure_val);
            }
        }
    }

    solveStats.numSimplify++;

    if (!ok) {
        return l_False;
    } else {
        check_stats();
        check_implicit_propagated();
        restore_order_heap();
        reset_reason_levels_of_vars_to_zero();
        num_red_cls_reducedb = count_num_red_cls_reducedb();

        return l_Undef;
    }
}

void Solver::reset_reason_levels_of_vars_to_zero()
{
    assert(decisionLevel() == 0);
    for(VarData& dat: varData) {
        dat.level = 0;
    }
}

void Solver::clear_clauses_stats()
{
    for(ClOffset offs: longIrredCls) {
        Clause* cl = cl_alloc.ptr(offs);
        cl->stats.clear();
    }

    for(ClOffset offs: longRedCls) {
        Clause* cl = cl_alloc.ptr(offs);
        cl->stats.clear();
    }
}

void Solver::print_prop_confl_stats(
    std::string name
    , const vector<ClauseUsageStats>& stats
) const {
    for(size_t i = 0; i < stats.size(); i++) {
        //Nothing to do here, no stats really
        if (stats[i].num == 0)
            continue;

        cout
        << name << " : " << std::setw(4) << i
        << " Avg. props: " << std::setw(6) << std::fixed << std::setprecision(2)
        << ((double)stats[i].sumProp/(double)stats[i].num);

        cout
        << name << " : " << std::setw(4) << i
        << " Avg. confls: " << std::setw(6) << std::fixed << std::setprecision(2)
        << ((double)stats[i].sumConfl/(double)stats[i].num);

        if (stats[i].sumLookedAt > 0) {
            cout
            << " Props&confls/looked at: " << std::setw(6) << std::fixed << std::setprecision(2)
            << ((double)stats[i].sumPropAndConfl()/(double)stats[i].sumLookedAt);
        }

        cout
        << " Avg. lits visited: " << std::setw(6) << std::fixed << std::setprecision(2)
        << ((double)stats[i].sumLitVisited/(double)stats[i].num);

        if (stats[i].sumLookedAt > 0) {
            cout
            << " Lits visited/looked at: " << std::setw(6) << std::fixed << std::setprecision(2)
            << ((double)stats[i].sumLitVisited/(double)stats[i].sumLookedAt);
        }

        if (stats[i].sumLitVisited > 0) {
            cout
            << " Props&confls/Litsvisited*10: "
            << std::setw(6) << std::fixed << std::setprecision(4)
            << (10.0*(double)stats[i].sumPropAndConfl()/(double)stats[i].sumLitVisited);
        }

        cout << endl;
    }
}

void Solver::consolidate_mem()
{
    const double myTime = cpuTime();
    cl_alloc.consolidate(this, true);
    const double time_used = cpuTime() - myTime;
    if (sqlStats) {
        sqlStats->time_passed_min(
            this
            , "consolidate mem"
            , time_used
        );
    }
}

void Solver::print_stats() const
{
    const double cpu_time = cpuTime();
    cout << "c ------- FINAL TOTAL SEARCH STATS ---------" << endl;
    print_stats_line("c UIP search time"
        , sumStats.cpu_time
        , stats_line_percent(sumStats.cpu_time, cpu_time)
        , "% time"
    );

    if (conf.verbStats >= 1) {
        print_all_stats();
    } else {
        print_min_stats();
    }
}

void Solver::print_min_stats() const
{
    const double cpu_time = cpuTime();
    sumStats.print_short();
    print_stats_line("c props/decision"
        , (double)propStats.propagations/(double)sumStats.decisions
    );
    print_stats_line("c props/conflict"
        , (double)propStats.propagations/(double)sumStats.conflStats.numConflicts
    );

    print_stats_line("c 0-depth assigns", trail.size()
        , stats_line_percent(trail.size(), nVars())
        , "% vars"
    );
    print_stats_line("c 0-depth assigns by thrds"
        , zero_level_assigns_by_searcher
        , stats_line_percent(zero_level_assigns_by_searcher, nVars())
        , "% vars"
    );
    print_stats_line("c 0-depth assigns by CNF"
        , zeroLevAssignsByCNF
        , stats_line_percent(zeroLevAssignsByCNF, nVars())
        , "% vars"
    );

    //Failed lit stats
    if (conf.doProbe) {
        print_stats_line("c probing time"
            , prober->get_stats().cpu_time
            , stats_line_percent(prober->get_stats().cpu_time, cpu_time)
            , "% time"
        );

        prober->get_stats().print_short(this, 0, 0);
    }
    //OccSimplifier stats
    if (conf.perform_occur_based_simp) {
        print_stats_line("c OccSimplifier time"
            , simplifier->get_stats().total_time()
            , stats_line_percent(simplifier->get_stats().total_time() ,cpu_time)
            , "% time"
        );
        simplifier->get_stats().print_short(this, nVars());
    }
    print_stats_line("c SCC time"
        , varReplacer->get_scc_finder()->get_stats().cpu_time
        , stats_line_percent(varReplacer->get_scc_finder()->get_stats().cpu_time, cpu_time)
        , "% time"
    );
    varReplacer->get_scc_finder()->get_stats().print_short(NULL);
    varReplacer->print_some_stats(cpu_time);

    //varReplacer->get_stats().print_short(nVars());
    print_stats_line("c distill time"
                    , distiller->get_stats().time_used
                    , stats_line_percent(distiller->get_stats().time_used, cpu_time)
                    , "% time"
    );
    print_stats_line("c strength cache-irred time"
                    , strengthener->get_stats().irredCacheBased.cpu_time
                    , stats_line_percent(strengthener->get_stats().irredCacheBased.cpu_time, cpu_time)
                    , "% time"
    );
    print_stats_line("c strength cache-red time"
                    , strengthener->get_stats().redCacheBased.cpu_time
                    , stats_line_percent(strengthener->get_stats().redCacheBased.cpu_time, cpu_time)
                    , "% time"
    );
    print_stats_line("c Conflicts in UIP"
        , sumStats.conflStats.numConflicts
        , (double)sumStats.conflStats.numConflicts/cpu_time
        , "confl/TOTAL_TIME_SEC"
    );
    print_stats_line("c Total time", cpu_time);
    print_stats_line("c Mem used"
        , mem_used()/(1024UL*1024UL)
        , "MB"
    );
    if (conf.doCache) {
        implCache.print_statsSort(this);
    }
}

void Solver::print_all_stats() const
{
    const double cpu_time = cpuTime();

    sumStats.print();
    sumPropStats.print(sumStats.cpu_time);
    print_stats_line("c props/decision"
        , (double)propStats.propagations/(double)sumStats.decisions
    );
    print_stats_line("c props/conflict"
        , (double)propStats.propagations/(double)sumStats.conflStats.numConflicts
    );
    cout << "c ------- FINAL TOTAL SOLVING STATS END ---------" << endl;
    reduceDB->get_cleaning_stats().print(cpu_time);

    print_stats_line("c reachability time"
        , reachStats.cpu_time
        , stats_line_percent(reachStats.cpu_time, cpu_time)
        , "% time"
    );
    reachStats.print();

    print_stats_line("c 0-depth assigns", trail.size()
        , stats_line_percent(trail.size(), nVars())
        , "% vars"
    );
    print_stats_line("c 0-depth assigns by searcher"
        , zero_level_assigns_by_searcher
        , stats_line_percent(zero_level_assigns_by_searcher, nVars())
        , "% vars"
    );
    print_stats_line("c 0-depth assigns by CNF"
        , zeroLevAssignsByCNF
        , stats_line_percent(zeroLevAssignsByCNF, nVars())
        , "% vars"
    );

    //Failed lit stats
    if (conf.doProbe) {
        print_stats_line("c probing time"
            , prober->get_stats().cpu_time
            , stats_line_percent(prober->get_stats().cpu_time, cpu_time)
            , "% time"
        );

        prober->get_stats().print(nVars());
    }

    //OccSimplifier stats
    if (conf.perform_occur_based_simp) {
        print_stats_line("c OccSimplifier time"
            , simplifier->get_stats().total_time()
            , stats_line_percent(simplifier->get_stats().total_time(), cpu_time)
            , "% time"
        );

        simplifier->get_stats().print(nVars());
    }

    if (simplifier && conf.doGateFind) {
        simplifier->print_gatefinder_stats();
    }

    //GateFinder stats


    /*
    //XOR stats
    print_stats_line("c XOR time"
        , subsumer->getXorFinder()->get_stats().total_time()
        , subsumer->getXorFinder()->get_stats().total_time()/cpu_time*100.0
        , "% time"
    );
    subsumer->getXorFinder()->get_stats().print(
        subsumer->getXorFinder()->getNumCalls()
    );*/

    //VarReplacer stats
    print_stats_line("c SCC time"
        , varReplacer->get_scc_finder()->get_stats().cpu_time
        , stats_line_percent(varReplacer->get_scc_finder()->get_stats().cpu_time, cpu_time)
        , "% time"
    );
    varReplacer->get_scc_finder()->get_stats().print();

    varReplacer->get_stats().print(nVars());
    varReplacer->print_some_stats(cpu_time);

    //Distiller stats
    print_stats_line("c distill time"
                    , distiller->get_stats().time_used
                    , stats_line_percent(distiller->get_stats().time_used, cpu_time)
                    , "% time");
    distiller->get_stats().print(nVars());

    print_stats_line("c strength cache-irred time"
                    , strengthener->get_stats().irredCacheBased.cpu_time
                    , stats_line_percent(strengthener->get_stats().irredCacheBased.cpu_time, cpu_time)
                    , "% time");
    print_stats_line("c strength cache-red time"
                    , strengthener->get_stats().redCacheBased.cpu_time
                    , stats_line_percent(strengthener->get_stats().redCacheBased.cpu_time, cpu_time)
                    , "% time");
    strengthener->get_stats().print();

    if (conf.doStrSubImplicit) {
        subsumeImplicit->get_stats().print();
    }

    if (conf.doCache) {
        implCache.print_stats(this);
    }

    //Other stats
    print_stats_line("c Conflicts in UIP"
        , sumStats.conflStats.numConflicts
        , (double)sumStats.conflStats.numConflicts/cpu_time
        , "confl/TOTAL_TIME_SEC"
    );
    print_stats_line("c Total time", cpu_time);
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
    mem += litReachable.capacity()*sizeof(Lit);
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

    mem = implCache.mem_used();
    mem += litReachable.capacity()*sizeof(LitReachData);
    print_stats_line("c Mem for impl cache"
        , mem/(1024UL*1024UL)
        , "MB"
        , stats_line_percent(mem, rss_mem_used)
        , "%"
    );
    account += mem;

    account += print_stamp_mem(rss_mem_used);

    mem = hist.mem_used();
    print_stats_line("c Mem for history stats"
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

    if (compHandler) {
        mem = compHandler->mem_used();
        print_stats_line("c Mem for component handler"
            , mem/(1024UL*1024UL)
            , "MB"
            , stats_line_percent(mem, rss_mem_used)
            , "%"
        );
        account += mem;
    }

    if (simplifier) {
        mem = simplifier->mem_used();
        print_stats_line("c Mem for simplifier"
            , mem/(1024UL*1024UL)
            , "MB"
            , stats_line_percent(mem, rss_mem_used)
            , "%"
        );
        account += mem;

        mem = simplifier->mem_used_xor();
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

    if (prober) {
        mem = prober->mem_used();
        print_stats_line("c Mem for prober"
            , mem/(1024UL*1024UL)
            , "MB"
            , stats_line_percent(mem, rss_mem_used)
            , "%"
        );
        account += mem;
    }

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
                assert(false);
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
    << " size4: " << size4
    << " size5: " << size5
    << " larger: " << sizeLarge << endl;
}


vector<Lit> Solver::get_zero_assigned_lits() const
{
    vector<Lit> lits;
    assert(decisionLevel() == 0);
    for(size_t i = 0; i < assigns.size(); i++) {
        if (assigns[i] != l_Undef) {
            Lit lit(i, assigns[i] == l_False);

            //Update to higher-up
            lit = varReplacer->get_lit_replaced_with(lit);
            if (varData[lit.var()].is_bva == false) {
                lits.push_back(map_inter_to_outer(lit));
            }

            //Everything it repaces has also been set
            const vector<Var> vars = varReplacer->get_vars_replacing(lit.var());
            for(const Var var: vars) {
                if (varData[var].is_bva)
                    continue;

                Lit tmp_lit = Lit(var, false);
                assert(varReplacer->get_lit_replaced_with(tmp_lit).var() == lit.var());
                if (lit != varReplacer->get_lit_replaced_with(tmp_lit)) {
                    tmp_lit ^= true;
                }
                assert(lit == varReplacer->get_lit_replaced_with(tmp_lit));

                lits.push_back(map_inter_to_outer(tmp_lit));
            }
        }
    }

    //Remove duplicates. Because of above replacing-mimicing algo
    //multipe occurrences of literals can be inside
    vector<Lit>::iterator it;
    std::sort(lits.begin(), lits.end());
    it = std::unique (lits.begin(), lits.end());
    lits.resize( std::distance(lits.begin(),it) );

    //Update to outer without BVA
    vector<Var> my_map = build_outer_to_without_bva_map();
    updateLitsMap(lits, my_map);
    for(const Lit lit: lits) {
        assert(lit.var() < nVarsOutside());
    }

    return lits;
}

void Solver::print_all_clauses() const
{
    for(vector<ClOffset>::const_iterator
        it = longIrredCls.begin(), end = longIrredCls.end()
        ; it != end
        ; ++it
    ) {
        Clause* cl = cl_alloc.ptr(*it);
        cout
        << "Normal clause offs " << *it
        << " cl: " << *cl
        << endl;
    }


    uint32_t wsLit = 0;
    for (watch_array::const_iterator
        it = watches.begin(), end = watches.end()
        ; it != end
        ; ++it, wsLit++
    ) {
        Lit lit = Lit::toLit(wsLit);
        watch_subarray_const ws = *it;
        cout << "watches[" << lit << "]" << endl;
        for (watch_subarray_const::const_iterator
            it2 = ws.begin(), end2 = ws.end()
            ; it2 != end2
            ; it2++
        ) {
            if (it2->isBinary()) {
                cout << "Binary clause part: " << lit << " , " << it2->lit2() << endl;
            } else if (it2->isClause()) {
                cout << "Normal clause offs " << it2->get_offset() << endl;
            } else if (it2->isTri()) {
                cout << "Tri clause:"
                << lit << " , "
                << it2->lit2() << " , "
                << it2->lit3() << endl;
            }
        }
    }
}

bool Solver::verify_implicit_clauses() const
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
            if (w.isBinary()
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

             if (w.isTri()
                && model_value(lit) != l_True
                && model_value(w.lit2()) != l_True
                && model_value(w.lit3()) != l_True
            ) {
                cout
                << "tri clause: "
                << lit
                << " , " << w.lit2()
                << " , " << w.lit3()
                << " not satisfied!"
                << endl;

                cout
                << "value of unsat tri clause: "
                << value(lit)
                << " , " << value(w.lit2())
                << " , " << value(w.lit3())
                << endl;

                return false;
            }
        }
    }

    return true;
}

bool Solver::verify_long_clauses(const vector<ClOffset>& cs) const
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
    verificationOK &= verify_long_clauses(longIrredCls);
    verificationOK &= verify_long_clauses(longRedCls);
    verificationOK &= verify_implicit_clauses();

    if (conf.verbosity >= 1 && verificationOK) {
        cout
        << "c Verified "
        << longIrredCls.size() + longRedCls.size()
            + binTri.irredBins + binTri.redBins
            + binTri.irredTris + binTri.redTris
        << " clause(s)."
        << endl;
    }

    return verificationOK;
}

lbool Solver::model_value (const Lit p) const
{
    return model[p.var()] ^ p.sign();
}

lbool Solver::model_value (const Var p) const
{
    return model[p];
}

void Solver::test_all_clause_attached() const
{
#ifndef DEBUG_ATTACH_MORE
    return;
#endif

    for (vector<ClOffset>::const_iterator
        it = longIrredCls.begin(), end = longIrredCls.end()
        ; it != end
        ; ++it
    ) {
        assert(normClauseIsAttached(*it));
    }
}

bool Solver::normClauseIsAttached(const ClOffset offset) const
{
    bool attached = true;
    const Clause& cl = *cl_alloc.ptr(offset);
    assert(cl.size() > 3);

    attached &= findWCl(watches[cl[0].toInt()], offset);
    attached &= findWCl(watches[cl[1].toInt()], offset);

    return attached;
}

void Solver::find_all_attach() const
{

    #ifndef SLOW_DEBUG
    return;
    #endif

    for (size_t i = 0; i < watches.size(); i++) {
        const Lit lit = Lit::toLit(i);
        for (uint32_t i2 = 0; i2 < watches[i].size(); i2++) {
            const Watched& w = watches[i][i2];
            if (!w.isClause())
                continue;

            //Get clause
            Clause* cl = cl_alloc.ptr(w.get_offset());
            assert(!cl->freed());

            //Assert watch correctness
            if ((*cl)[0] != lit
                && (*cl)[1] != lit
            ) {
                std::cerr
                << "ERROR! Clause " << (*cl)
                << " not attached?"
                << endl;

                assert(false);
                std::exit(-1);
            }

            //Clause in one of the lists
            if (!find_clause(w.get_offset())) {
                std::cerr
                << "ERROR! did not find clause " << *cl
                << endl;

                assert(false);
                std::exit(1);
            }
        }
    }

    find_all_attach(longIrredCls);
    find_all_attach(longRedCls);
}

void Solver::find_all_attach(const vector<ClOffset>& cs) const
{
    for(vector<ClOffset>::const_iterator
        it = cs.begin(), end = cs.end()
        ; it != end
        ; ++it
    ) {
        Clause& cl = *cl_alloc.ptr(*it);
        bool ret = findWCl(watches[cl[0].toInt()], *it);
        if (!ret) {
            cout
            << "Clause " << cl
            << " (red: " << cl.red() << ")"
            << " doesn't have its 1st watch attached!"
            << endl;

            assert(false);
            std::exit(-1);
        }

        ret = findWCl(watches[cl[1].toInt()], *it);
        if (!ret) {
            cout
            << "Clause " << cl
            << " (red: " << cl.red() << ")"
            << " doesn't have its 2nd watch attached!"
            << endl;

            assert(false);
            std::exit(-1);
        }
    }
}


bool Solver::find_clause(const ClOffset offset) const
{
    for (uint32_t i = 0; i < longIrredCls.size(); i++) {
        if (longIrredCls[i] == offset)
            return true;
    }
    for (uint32_t i = 0; i < longRedCls.size(); i++) {
        if (longRedCls[i] == offset)
            return true;
    }

    return false;
}

void Solver::check_wrong_attach() const
{
    #ifndef SLOW_DEBUG
    return;
    #endif

    for (vector<ClOffset>::const_iterator
        it = longRedCls.begin(), end = longRedCls.end()
        ; it != end
        ; ++it
    ) {
        const Clause& cl = *cl_alloc.ptr(*it);
        for (uint32_t i = 0; i < cl.size(); i++) {
            if (i > 0)
                assert(cl[i-1].var() != cl[i].var());
        }
    }
}

size_t Solver::get_num_nonfree_vars() const
{
    size_t nonfree = 0;
    if (decisionLevel() == 0) {
        nonfree += trail.size();
    } else {
        nonfree += trail_lim[0];
    }

    if (simplifier) {
        if (conf.perform_occur_based_simp) {
            nonfree += simplifier->get_num_elimed_vars();
        }
    }
    nonfree += varReplacer->get_num_replaced_vars();


    if (compHandler) {
        nonfree += compHandler->get_num_vars_removed();
    }
    return nonfree;
}

size_t Solver::get_num_free_vars() const
{
    return nVarsOuter() - get_num_nonfree_vars();
}

void Solver::print_clause_stats() const
{
    //Irredundant
    print_value_kilo_mega(longIrredCls.size());
    print_value_kilo_mega(binTri.irredTris);
    print_value_kilo_mega(binTri.irredBins);
    cout
    << " " << std::setw(5) << std::fixed << std::setprecision(2)
    << ratio_for_stat(litStats.irredLits, longIrredCls.size())
    << " " << std::setw(5) << std::fixed << std::setprecision(2)
    << ratio_for_stat(litStats.irredLits + binTri.irredTris*3 + binTri.irredBins*2
    , longIrredCls.size() + binTri.irredTris + binTri.irredBins)
    ;

    //Redundant
    print_value_kilo_mega(longRedCls.size());
    print_value_kilo_mega(binTri.redTris);
    print_value_kilo_mega(binTri.redBins);
    cout
    << " " << std::setw(5) << std::fixed << std::setprecision(2)
    << ratio_for_stat(litStats.redLits, longRedCls.size())
    << " " << std::setw(5) << std::fixed << std::setprecision(2)
    << ratio_for_stat(litStats.redLits + binTri.redTris*3 + binTri.redBins*2
    , longRedCls.size() + binTri.redTris + binTri.redBins)
    ;
}

void Solver::check_implicit_stats() const
{
    //Don't check if in crazy mode
    #ifdef NDEBUG
    return;
    #endif
    const double myTime = cpuTime();

    //Check number of red & irred binary clauses
    uint64_t thisNumRedBins = 0;
    uint64_t thisNumIrredBins = 0;
    uint64_t thisNumRedTris = 0;
    uint64_t thisNumIrredTris = 0;

    size_t wsLit = 0;
    for(watch_array::const_iterator
        it = watches.begin(), end = watches.end()
        ; it != end
        ; ++it, wsLit++
    ) {
        #ifdef DEBUG_TRI_SORTED_SANITY
        const Lit lit = Lit::toLit(wsLit);
        #endif //DEBUG_TRI_SORTED_SANITY

        watch_subarray_const ws = *it;
        for(watch_subarray_const::const_iterator
            it2 = ws.begin(), end2 = ws.end()
            ; it2 != end2
            ; it2++
        ) {
            if (it2->isBinary()) {
                if (it2->red())
                    thisNumRedBins++;
                else
                    thisNumIrredBins++;

                continue;
            }

            if (it2->isTri()) {
                assert(it2->lit2() < it2->lit3());
                assert(it2->lit2().var() != it2->lit3().var());

                #ifdef DEBUG_TRI_SORTED_SANITY
                Lit lits[3];
                lits[0] = lit;
                lits[1] = it2->lit2();
                lits[2] = it2->lit3();
                std::sort(lits, lits + 3);
                findWatchedOfTri(watches, lits[0], lits[1], lits[2], it2->red());
                findWatchedOfTri(watches, lits[1], lits[0], lits[2], it2->red());
                findWatchedOfTri(watches, lits[2], lits[0], lits[1], it2->red());
                #endif //DEBUG_TRI_SORTED_SANITY

                if (it2->red())
                    thisNumRedTris++;
                else
                    thisNumIrredTris++;

                continue;
            }
        }
    }

    if (thisNumIrredBins/2 != binTri.irredBins) {
        std::cerr
        << "ERROR:"
        << " thisNumIrredBins/2: " << thisNumIrredBins/2
        << " binTri.irredBins: " << binTri.irredBins
        << "thisNumIrredBins: " << thisNumIrredBins
        << "thisNumRedBins: " << thisNumRedBins << endl;
    }
    assert(thisNumIrredBins % 2 == 0);
    assert(thisNumIrredBins/2 == binTri.irredBins);

    if (thisNumRedBins/2 != binTri.redBins) {
        std::cerr
        << "ERROR:"
        << " thisNumRedBins/2: " << thisNumRedBins/2
        << " binTri.redBins: " << binTri.redBins
        << endl;
    }
    assert(thisNumRedBins % 2 == 0);
    assert(thisNumRedBins/2 == binTri.redBins);

    if (thisNumIrredTris/3 != binTri.irredTris) {
        std::cerr
        << "ERROR:"
        << " thisNumIrredTris/3: " << thisNumIrredTris/3
        << " binTri.irredTris: " << binTri.irredTris
        << endl;
    }
    assert(thisNumIrredTris % 3 == 0);
    assert(thisNumIrredTris/3 == binTri.irredTris);

    if (thisNumRedTris/3 != binTri.redTris) {
        std::cerr
        << "ERROR:"
        << " thisNumRedTris/3: " << thisNumRedTris/3
        << " binTri.redTris: " << binTri.redTris
        << endl;
    }
    assert(thisNumRedTris % 3 == 0);
    assert(thisNumRedTris/3 == binTri.redTris);

    const double time_used = cpuTime() - myTime;
    if (sqlStats) {
        sqlStats->time_passed_min(
            this
            , "check implicit stats"
            , time_used
        );
    }
}

uint64_t Solver::count_lits(
    const vector<ClOffset>& clause_array
    , bool allowFreed
) const {
    uint64_t lits = 0;
    for(vector<ClOffset>::const_iterator
        it = clause_array.begin(), end = clause_array.end()
        ; it != end
        ; ++it
    ) {
        const Clause& cl = *cl_alloc.ptr(*it);
        if (cl.freed()) {
            assert(allowFreed);
        } else {
            lits += cl.size();
        }
    }

    return lits;
}

void Solver::check_stats(const bool allowFreed) const
{
    //If in crazy mode, don't check
    #ifdef NDEBUG
    return;
    #endif

    check_implicit_stats();

    const double myTime = cpuTime();
    uint64_t numLitsIrred = count_lits(longIrredCls, allowFreed);
    if (numLitsIrred != litStats.irredLits) {
        std::cerr << "ERROR: " << endl
        << "->numLitsIrred: " << numLitsIrred << endl
        << "->litStats.irredLits: " << litStats.irredLits << endl;
    }
    assert(numLitsIrred == litStats.irredLits);

    uint64_t numLitsRed = count_lits(longRedCls, allowFreed);
    if (numLitsRed != litStats.redLits) {
        std::cerr << "ERROR: " << endl
        << "->numLitsRed: " << numLitsRed << endl
        << "->litStats.redLits: " << litStats.redLits << endl;
    }
    assert(numLitsRed == litStats.redLits);

    const double time_used = cpuTime() - myTime;
    if (sqlStats) {
        sqlStats->time_passed_min(
            this
            , "check literal stats"
            , time_used
        );
    }
}

const char* Solver::get_version_sha1()
{
    return ::get_version_sha1();
}

const char* Solver::get_version_tag()
{
    return ::get_version_tag();
}

const char* Solver::get_compilation_env()
{
    return ::get_compilation_env();
}

void Solver::print_watch_list(watch_subarray_const ws, const Lit lit) const
{
    for (watch_subarray::const_iterator
        it = ws.begin(), end = ws.end()
        ; it != end
        ; ++it
    ) {
        if (it->isClause()) {
            cout
            << "Clause: " << *cl_alloc.ptr(it->get_offset());
        }

        if (it->isBinary()) {
            cout
            << "BIN: " << lit << ", " << it->lit2()
            << " (l: " << it->red() << ")";
        }

        if (it->isTri()) {
            cout
            << "TRI: " << lit << ", " << it->lit2() << ", " << it->lit3()
            << " (l: " << it->red() << ")";
        }

        cout << endl;
    }
    cout << endl;
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
        for(watch_subarray_const::const_iterator
            it2 = ws.begin(), end2 = ws.end()
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
            if (it2->isBinary()) {
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

            //Handle 3-long clause
            if (it2->isTri()) {
                const lbool val3 = value(it2->lit3());

                if (val1 == l_False
                    && val2 == l_False
                ) {
                    assert(val3 == l_True);
                }

                if (val2 == l_False
                    && val3 == l_False
                ) {
                    assert(val1 == l_True);
                }

                if (val1 == l_False
                    && val3 == l_False
                ) {
                    assert(val2 == l_True);
                }
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
        return simplifier->get_num_elimed_vars();
    } else {
        return 0;
    }
}

size_t Solver::get_num_vars_replaced() const
{
    return varReplacer->get_num_replaced_vars();
}

void Solver::calculate_reachability()
{
    double myTime = cpuTime();

    //Clear out
    for (size_t i = 0; i < nVars()*2; i++) {
        litReachable[i] = LitReachData();
    }

    //Go through each that is decision variable
    for (size_t i = 0, end = nVars()*2; i < end; i++) {
        const Lit lit = Lit::toLit(i);

        //Check if it's a good idea to look at the variable as a dominator
        if (value(lit) != l_Undef
            || varData[lit.var()].removed != Removed::none
            || !varData[lit.var()].is_decision
        ) {
            continue;
        }

        const vector<LitExtra>& cache = implCache[lit.toInt()].lits;
        uint32_t cacheSize = cache.size();
        for (const LitExtra litex: cache) {
            assert(litex.getLit() != lit);
            assert(litex.getLit() != ~lit);

            if (litReachable[litex.getLit().toInt()].lit == lit_Undef
                || litReachable[litex.getLit().toInt()].numInCache < cacheSize
            ) {
                litReachable[litex.getLit().toInt()].lit = ~lit;
                litReachable[litex.getLit().toInt()].numInCache = cacheSize;
            }
        }
    }

    const double time_used = cpuTime() - myTime;
    if (conf.verbosity >= 1) {
        cout
        << "c calculated reachability. T: "
        << std::setprecision(3) << time_used
        << endl;
    }
    if (sqlStats) {
        sqlStats->time_passed_min(
            this
            , "calc reachability"
            , time_used
        );
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
            || varData[lit.var()].removed == Removed::decomposed
        ) {
            watch_subarray ws = *it;
            assert(ws.empty());
            ws.clear();
        }
    }
    watches.consolidate();
}

bool Solver::fully_enqueue_these(const vector<Lit>& toEnqueue)
{
    assert(ok);
    assert(decisionLevel() == 0);
    for(const Lit lit: toEnqueue) {
        if (!fully_enqueue_this(lit)) {
            return false;
        }
    }

    return true;
}

bool Solver::fully_enqueue_this(const Lit lit)
{
    const lbool val = value(lit);
    if (val == l_Undef) {
        assert(varData[lit.var()].removed == Removed::none);
        enqueue(lit);
        ok = propagate<true>().isNULL();

        if (!ok) {
            return false;
        }
    } else if (val == l_False) {
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
    sumStats += Searcher::get_stats();
    sumPropStats += propStats;
}

unsigned long Solver::get_sql_id() const
{
    if (!conf.doSQL
        || sqlStats == NULL)
    {
        return 0;
    }

    return sqlStats->get_runID();
}

bool Solver::add_clause_outer(const vector<Lit>& lits)
{
    if (!ok) {
        return false;
    }
    check_too_large_variable_number(lits);
    back_number_from_outside_to_outer(lits);
    return addClause(back_number_from_outside_to_outer_tmp);
}

bool Solver::add_xor_clause_outer(const vector<Var>& vars, bool rhs)
{
    if (!ok) {
        return false;
    }

    vector<Lit> lits(vars.size());
    for(size_t i = 0; i < vars.size(); i++) {
        lits[i] = Lit(vars[i], false);
    }
    check_too_large_variable_number(lits);

    back_number_from_outside_to_outer(lits);
    addClauseHelper(back_number_from_outside_to_outer_tmp);
    add_xor_clause_inter(back_number_from_outside_to_outer_tmp, rhs, true, false);

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

void Solver::open_file_and_dump_irred_clauses(string fname) const
{
    ClauseDumper dumper(this);
    dumper.open_file_and_dump_irred_clauses(fname);
}

void Solver::open_file_and_dump_red_clauses(string fname) const
{
    ClauseDumper dumper(this);
    dumper.open_file_and_dump_red_clauses(fname);
}

vector<pair<Lit, Lit> > Solver::get_all_binary_xors() const
{
    vector<pair<Lit, Lit> > bin_xors = varReplacer->get_all_binary_xors_outer();

    //Update to outer without BVA
    vector<pair<Lit, Lit> > ret;
    const vector<Var> my_map = build_outer_to_without_bva_map();
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

    for(const std::pair<Lit, Lit> val: ret) {
        assert(val.first.var() < nVarsOutside());
        assert(val.second.var() < nVarsOutside());
    }

    return ret;
}

Solver::ReachabilityStats& Solver::ReachabilityStats::operator+=(const ReachabilityStats& other)
{
    cpu_time += other.cpu_time;

    numLits += other.numLits;
    dominators += other.dominators;
    numLitsDependent += other.numLitsDependent;

    return *this;
}

void Solver::ReachabilityStats::print() const
{
    cout << "c ------- REACHABILITY STATS -------" << endl;
    print_stats_line("c time"
        , cpu_time
    );

    print_stats_line("c dominator lits"
        , stats_line_percent(dominators, numLits)
        , "% of unknowns lits"
    );

    print_stats_line("c dependent lits"
        , stats_line_percent(numLitsDependent, numLits)
        , "% of unknown lits"
    );

    print_stats_line("c avg num. dominated lits"
        , ratio_for_stat(numLitsDependent, dominators)
    );

    cout << "c ------- REACHABILITY STATS END -------" << endl;
}

void Solver::ReachabilityStats::print_short(const Solver* solver) const
{
    cout
    << "c [reach]"
    << " dom lits: " << std::fixed << std::setprecision(2)
    << stats_line_percent(dominators, numLits)
    << " %"

    << " dep-lits: " << std::fixed << std::setprecision(2)
    << stats_line_percent(numLitsDependent, numLits)
    << " %"

    << " dep-lits/dom-lits : " << std::fixed << std::setprecision(2)
    << stats_line_percent(numLitsDependent, dominators)

    << solver->conf.print_times(cpu_time)
    << endl;
}

void Solver::update_assumptions_after_varreplace()
{
    //Update assumptions
    for(AssumptionPair& lit_pair: solver->assumptions) {
        if (assumptionsSet.size() > lit_pair.lit_inter.var()) {
            assumptionsSet[lit_pair.lit_inter.var()] = false;
        } else {
            assert(value(lit_pair.lit_inter) != l_Undef
                && "There can be NO other reason -- vars in assumptions cannot be elimed or decomposed");
        }

        lit_pair.lit_inter = varReplacer->get_lit_replaced_with(lit_pair.lit_inter);

        if (assumptionsSet.size() > lit_pair.lit_inter.var()) {
            assumptionsSet[lit_pair.lit_inter.var()] = true;
        }
    }
}

//TODO later, this can be removed, get_num_free_vars() is MUCH cheaper to
//compute but may have some bugs here-and-there
Var Solver::num_active_vars() const
{
    Var numActive = 0;
    uint32_t removed_decomposed = 0;
    uint32_t removed_replaced = 0;
    uint32_t removed_set = 0;
    uint32_t removed_elimed = 0;
    uint32_t removed_non_decision = 0;
    for(Var var = 0; var < solver->nVarsOuter(); var++) {
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
            case Removed::decomposed :
                removed_decomposed++;
                continue;
                break;
            case Removed::elimed :
                removed_elimed++;
                continue;
                break;
            case Removed::replaced:
                removed_replaced++;
                continue;
                break;
            case Removed::none:
                break;
        }
        if (!varData[var].is_decision) {
            removed_non_decision++;
        }
        numActive++;
    }
    assert(removed_non_decision == 0);
    if (simplifier) {
        assert(removed_elimed == simplifier->get_num_elimed_vars());
    } else {
        assert(removed_elimed == 0);
    }

    if (compHandler) {
        assert(removed_decomposed == compHandler->get_num_vars_removed());
    } else {
        assert(removed_decomposed == 0);
    }

    assert(removed_set == ((decisionLevel() == 0) ? trail.size() : trail_lim[0]));

    assert(removed_replaced == varReplacer->get_num_replaced_vars());
    assert(numActive == get_num_free_vars());

    return numActive;
}

Features Solver::calculate_features() const
{
    FeaturesCalc extract(this);
    Features feat = extract.extract();
    feat.avg_confl_size = hist.conflSizeHistLT.avg();
    feat.avg_confl_glue = hist.glueHistLT.avg();
    feat.avg_num_resolutions = hist.numResolutionsHistLT.avg();
    feat.avg_trail_depth_delta = hist.trailDepthDeltaHist.avg();
    feat.avg_branch_depth = hist.branchDepthHist.avg();
    feat.avg_branch_depth_delta = hist.branchDepthDeltaHist.avg();

    feat.confl_size_min = hist.conflSizeHistLT.getMax();
    feat.confl_size_max = hist.conflSizeHistLT.getMax();
    feat.confl_glue_min = hist.glueHistLT.getMin();
    feat.confl_glue_max = hist.glueHistLT.getMax();
    feat.branch_depth_min = hist.branchDepthHist.getMin();
    feat.branch_depth_max = hist.branchDepthHist.getMax();
    feat.trail_depth_delta_min = hist.trailDepthDeltaHist.getMin();
    feat.trail_depth_delta_max = hist.trailDepthDeltaHist.getMax();
    feat.num_resolutions_min = hist.numResolutionsHistLT.getMin();
    feat.num_resolutions_max = hist.numResolutionsHistLT.getMax();

    if (sumPropStats.propagations != 0
        && sumStats.conflStats.numConflicts != 0
        && sumStats.numRestarts != 0
    ) {
        feat.props_per_confl = (double)sumStats.conflStats.numConflicts / (double)sumPropStats.propagations;
        feat.confl_per_restart = (double)sumStats.conflStats.numConflicts / (double)sumStats.numRestarts;
        feat.decisions_per_conflict = (double)sumStats.decisions / (double)sumStats.conflStats.numConflicts;
        feat.learnt_bins_per_confl = (double)sumStats.learntBins / (double)sumStats.conflStats.numConflicts;
        feat.learnt_tris_per_confl = (double)sumStats.learntTris / (double)sumStats.conflStats.numConflicts;
    }

    feat.num_gates_found_last = sumStats.num_gates_found_last;
    feat.num_xors_found_last = sumStats.num_xors_found_last;

    if (conf.verbosity >= 1) {
        feat.print_stats();
    }

    return feat;
}

void Solver::reconfigure(int val)
{
    assert(val > 0);
    switch (val) {
        case 1: {
            conf.max_temporary_learnt_clauses = 30000;
            reset_temp_cl_num();
            break;
        }

        case 2: {
            //Luby
            conf.restart_inc = 1.5;
            conf.restart_first = 100;
            conf.restartType = CMSat::Restart::luby;
            break;
        }

        case 3: {
            //Similar to old CMS except we look at learnt DB size insteead
            //of conflicts to see if we need to clean.
            conf.ratio_keep_clauses[clean_to_int(ClauseClean::size)] = 0;
            conf.ratio_keep_clauses[clean_to_int(ClauseClean::activity)] = 0;
            conf.ratio_keep_clauses[clean_to_int(ClauseClean::glue)] = 0.5;
            conf.glue_must_keep_clause_if_below_or_eq = 0;
            conf.inc_max_temp_red_cls = 1.03;
            reset_temp_cl_num();
            break;
        }

        case 4: {
            //Different glue limit
            conf.glue_must_keep_clause_if_below_or_eq = 4;
            conf.max_num_lits_more_red_min = 3;
            conf.max_glue_more_minim = 4;
            reset_temp_cl_num();
            break;
        }

        case 5: {
            //Lots of simplifying
            conf.global_timeout_multiplier = 2;
            conf.num_conflicts_of_search_inc = 1.25;
            break;
        }

        case 6: {
            //No more simplifying
            conf.never_stop_search = true;
            break;
        }

        case 7: {
            conf.varElimRatioPerIter = 1;
            conf.restartType = Restart::geom;
            conf.polarity_mode = CMSat::PolarityMode::polarmode_neg;

            conf.inc_max_temp_red_cls = 1.02;
            conf.ratio_keep_clauses[clean_to_int(ClauseClean::glue)] = 0;
            conf.ratio_keep_clauses[clean_to_int(ClauseClean::size)] = 0;
            conf.ratio_keep_clauses[clean_to_int(ClauseClean::activity)] = 0.5;
            reset_temp_cl_num();
            break;
        }

        case 8: {
            conf.glue_must_keep_clause_if_below_or_eq = 7;
            conf.var_decay_max = 0.98; //more 'fast' in adjusting activities
            break;
        }

        case 9: {
            conf.propBinFirst= true;
            break;
        }

        case 10: {
            conf.more_red_minim_limit_cache = 1200;
            conf.more_red_minim_limit_binary = 600;
            conf.max_num_lits_more_red_min = 20;
            break;
        }

        case 11: {
            conf.more_otf_shrink_with_cache = 0;
            conf.max_temporary_learnt_clauses = 29633;
            conf.burst_search_len = 1114;
            conf.probe_bogoprops_time_limitM = 309;
            conf.strengthen_implicit_time_limitM = 145;
            conf.blocking_restart_multip = 0.10120348330944741;
            conf.do_blocking_restart = 1;
            conf.shortTermHistorySize = 84;
            conf.max_num_lits_more_red_min = 8;
            conf.varElimCostEstimateStrategy = 0;
            conf.doOTFSubsume = 1;
            conf.do_calc_polarity_first_time = 1;
            conf.doFindXors = 0;
            conf.varElimRatioPerIter = 0.836063764936515;
            conf.update_glues_on_analyze = 0;
            conf.varelim_time_limitM = 134;
            conf.bva_limit_per_call = 410437;
            conf.subsume_implicit_time_limitM = 154;
            conf.bva_time_limitM = 166;
            conf.distill_time_limitM = 1;
            conf.cacheUpdateCutoff = 2669;
            conf.num_conflicts_of_search = 21671;
            conf.inc_max_temp_red_cls = 1.029816784872561;
            conf.do_calc_polarity_every_time = 1;
            conf.distill_long_irred_cls_time_limitM = 1;
            conf.random_var_freq = 0.004446797134521431;
            conf.intree_time_limitM = 1652;
            conf.bva_also_twolit_diff = 0;
            conf.blocking_restart_trail_hist_length = 1;
            conf.update_glues_on_prop = 1;
            conf.dominPickFreq = 503;
            conf.sccFindPercent = 0.0174091218619471;
            conf.do_empty_varelim = 1;
            conf.updateVarElimComplexityOTF = 1;
            conf.more_otf_shrink_with_stamp = 0;
            conf.watch_cache_stamp_based_str_time_limitM = 37;
            conf.var_decay_max = 0.9565818549080972;
            reset_temp_cl_num();
            break;
        }

        case 12: {
            conf.do_bva = false;
            conf.glue_must_keep_clause_if_below_or_eq = 2;
            conf.varElimRatioPerIter = 1;
            conf.inc_max_temp_red_cls = 1.04;
            conf.ratio_keep_clauses[clean_to_int(ClauseClean::glue)] = 0.1;
            conf.ratio_keep_clauses[clean_to_int(ClauseClean::size)] = 0.1;
            conf.ratio_keep_clauses[clean_to_int(ClauseClean::activity)] = 0.3;
            conf.var_decay_max = 0.90; //more 'slow' in adjusting activities
            update_var_decay();
            reset_temp_cl_num();
            break;
        }

        case 13: {
            conf.global_timeout_multiplier = 5;
            conf.num_conflicts_of_search_inc = 1.15;
            conf.more_red_minim_limit_cache = 1200;
            conf.more_red_minim_limit_binary = 600;
            conf.max_num_lits_more_red_min = 20;
            conf.max_temporary_learnt_clauses = 10000;
            conf.var_decay_max = 0.99; //more 'fast' in adjusting activities
            break;
        }

        default: {
            cout << "ERROR: You must give a value for reconfigure that is lower" << endl;
            exit(-1);
        }
    }

    if (conf.verbosity >= 2) {
        cout << "c [features] reconfigured solver to config " << val << endl;
    }

    /*Note to self: change
     * propBinFirst 0->1
     * inc_max_temp_red_cls 1.1 -> 1.3
     * numCleanBetweenSimplify 2->4
     * bva: 1->0
    */
}