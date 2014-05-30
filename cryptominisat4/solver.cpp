/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2013, Mate Soos. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
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

#include "varreplacer.h"
#include "time_mem.h"
#include "searcher.h"
#include "sccfinder.h"
#include "simplifier.h"
#include "prober.h"
#include "vivifier.h"
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

using namespace CMSat;
using std::cout;
using std::endl;

#ifdef USE_MYSQL
#include "mysqlstats.h"
#endif

//#define DRUP_DEBUG

//#define DEBUG_RENUMBER

//#define DEBUG_TRI_SORTED_SANITY

Solver::Solver(const SolverConf _conf, bool* _needToInterrupt) :
    Searcher(_conf, this, _needToInterrupt)
    , mtrand(_conf.origSeed)
    , clauseCleaner(NULL)
    , varReplacer(NULL)
    , subsumeImplicit(NULL)
    , sCCFinder(NULL)
    , prober(NULL)
    , simplifier(NULL)
    , vivifier(NULL)
    , strengthener(NULL)
    , compHandler(NULL)
{
    if (conf.doSQL) {
        #ifdef USE_MYSQL
        sqlStats = new MySQLStats();
        #else
        if (conf.doSQL == 2) {
            cout
            << "ERROR: "
            << "Cannot use SQL: no SQL library was found during compilation."
            << endl;
            std::exit(-1);
        } else {
            sqlStats = NULL;
            conf.doSQL = 0;
            if (conf.verbosity >= 2) {
                cout
                << "c SQL not found during compilation, not dumping to DB"
                << endl;
            }
        }
        #endif
    } else {
        sqlStats = NULL;
    }

    if (conf.doProbe) {
        prober = new Prober(this);
    }
    if (conf.perform_occur_based_simp) {
        simplifier = new Simplifier(this);
    }
    sCCFinder = new SCCFinder(this);
    vivifier = new Vivifier(this);
    strengthener = new Strengthener(this);
    clauseCleaner = new ClauseCleaner(this);
    varReplacer = new VarReplacer(this);
    if (conf.doCompHandler) {
        compHandler = new CompHandler(this);
    }
    if (conf.doStrSubImplicit) {
        subsumeImplicit = new SubsumeImplicit(this);
    }
    datasync = new DataSync(this, NULL, 0);
    Searcher::solver = this;
    reduceDB = new ReduceDB(this);
}

Solver::~Solver()
{
    delete compHandler;
    delete sqlStats;
    delete prober;
    delete simplifier;
    delete sCCFinder;
    delete vivifier;
    delete strengthener;
    delete clauseCleaner;
    delete varReplacer;
    delete subsumeImplicit;
    delete datasync;
}

void Solver::set_shared_data(SharedData* shared_data, uint32_t thread_num)
{
    delete datasync;
    datasync = new DataSync(this, shared_data, thread_num);
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
            const Var newvar = solver->nVars()-1;
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
        Clause* cl = addClauseInt(new_lits, false, ClauseStats(), attach, NULL, addDrup);
        if (cl) {
            solver->longIrredCls.push_back(clAllocator.getOffset(cl));
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


/**
@brief Adds a clause to the problem. Should ONLY be called internally

This code is very specific in that it must NOT be called with varibles in
"ps" that have been replaced, eliminated, etc. Also, it must not be called
when the wer are in an UNSAT (!ok) state, for example. Use it carefully,
and only internally
*/
Clause* Solver::addClauseInt(
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
    cout << "addClauseInt clause " << lits << endl;
    #endif //VERBOSE_DEBUG

    //Make stats sane
    stats.introduced_at_conflict = std::min<uint64_t>(Searcher::sumConflicts(), stats.introduced_at_conflict);

    vector<Lit> ps = lits;

    std::sort(ps.begin(), ps.end());
    Lit p = lit_Undef;
    uint32_t i, j;
    for (i = j = 0; i != ps.size(); i++) {
        if (value(ps[i]) == l_True || ps[i] == ~p)
            return NULL;
        else if (value(ps[i]) != l_False && ps[i] != p) {
            ps[j++] = p = ps[i];

            if (varData[p.var()].removed != Removed::none
                && varData[p.var()].removed != Removed::queued_replacer
            ) {
                cout << "ERROR: clause " << lits << " contains literal "
                << p << " whose variable has been removed (removal type: "
                << removed_type_to_string(varData[p.var()].removed)
                << " var-updated lit: "
                << varReplacer->getVarReplacedWith(p)
                << ")"
                << endl;
            }

            //Variables that have been eliminated cannot be added internally
            //as part of a clause. That's a bug
            assert(varData[p.var()].removed == Removed::none
                    || varData[p.var()].removed == Removed::queued_replacer);
        }
    }
    ps.resize(ps.size() - (i - j));

    #ifdef VERBOSE_DEBUG
    cout << "addClauseInt final clause " << ps << endl;
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
                ok = (solver->propagate().isNULL());
            }

            return NULL;
        case 2:
            attachBinClause(ps[0], ps[1], red);
            return NULL;

        case 3:
            /*cout << "Attached tri clause: "
            << ps[0] << ", "
            << ps[1] << ", "
            << ps[2]
            << endl;*/

            attachTriClause(ps[0], ps[1], ps[2], red);
            return NULL;

        default:
            Clause* c = clAllocator.Clause_new(ps, sumStats.conflStats.numConflicts);
            if (red)
                c->makeRed(stats.glue);
            c->stats = stats;

            //In class 'Simplifier' we don't need to attach normall
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

void Solver::attachTriClause(
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
    PropEngine::attachTriClause(lit1, lit2, lit3, red);
}

void Solver::attachBinClause(
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
    PropEngine::attachBinClause(lit1, lit2, red, checkUnassignedFirst);
}

void Solver::detachTriClause(
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

    PropEngine::detachTriClause(lit1, lit2, lit3, red, allow_empty_watch);
}

void Solver::detachBinClause(
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

    PropEngine::detachBinClause(lit1, lit2, red, allow_empty_watch);
}

void Solver::detachClause(const Clause& cl, const bool removeDrup)
{
    if (removeDrup) {
        *drup << del << cl << fin;
    }

    assert(cl.size() > 3);
    detachModifiedClause(cl[0], cl[1], cl.size(), &cl);
}

void Solver::detachClause(const ClOffset offset, const bool removeDrup)
{
    Clause* cl = clAllocator.getPointer(offset);
    detachClause(*cl, removeDrup);
}

void Solver::detachModifiedClause(
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
    PropEngine::detachModifiedClause(lit1, lit2, origSize, address);
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
    for (const Lit lit: ps) {
        if (lit.var() >= nVarsOuter()) {
            cout
            << "ERROR: Variable " << lit.var() + 1
            << " inserted, but max var is "
            << nVarsOuter()
            << endl;
            assert(false);
            std::exit(-1);
        }
        assert(lit.var() < nVarsOuter()
        && "Clause inserted, but variable inside has not been declared with PropEngine::new_var() !");
    }

    //Undo var replacement
    for (Lit& lit: ps) {
        const Lit updated_lit = varReplacer->getLitReplacedWithOuter(lit);
        if (conf.verbosity >= 12
            && lit != updated_lit
        ) {
            cout
            << "EqLit updating outer lit " << lit
            << " to outer lit " << updated_lit
            << endl;
        }
        lit = updated_lit;
    }

    for(Lit& lit: ps) {
        if (map_outer_to_inter(lit).var() >= nVars()) {
            new_var(false, lit.var());
        }
    }

    //External var number -> Internal var number
    for (Lit& lit: ps) {
        const Lit origLit = lit;

        //Update variable numbering
        assert(lit.var() < nVarsOuter());
        lit = map_outer_to_inter(lit);

        if (conf.verbosity >= 12) {
            cout
            << "var-renumber updating lit "
            << origLit
            << " to lit "
            << lit
            << endl;
        }
    }

    //Check renumberer
    for (const Lit lit: ps) {
        const Lit updated_lit = varReplacer->getLitReplacedWith(lit);
        assert(lit == updated_lit);
    }

    //Uneliminate vars
    for (const Lit lit: ps) {
        if (conf.perform_occur_based_simp
            && varData[lit.var()].removed == Removed::elimed
        ) {
            #ifdef VERBOSE_DEBUG_RECONSTRUCT
            cout << "Uneliminating var " << lit.var() + 1 << endl;
            #endif
            if (!simplifier->unEliminate(lit.var()))
                return false;
        }
    }

    //Check
    for (Lit& lit: ps) {
        const Lit updated_lit = varReplacer->getLitReplacedWith(lit);
        assert(lit == updated_lit);
    }

    //Undo comp handler
    if (conf.doCompHandler) {
        for (const Lit lit: ps) {
            if (varData[lit.var()].removed == Removed::decomposed) {
                compHandler->readdRemovedClauses();
            }
        }
    }

    return true;
}

bool Solver::addClause(const vector<Lit>& lits)
{
    if (conf.perform_occur_based_simp && simplifier->getAnythingHasBeenBlocked()) {
        cout
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
    Clause* cl = addClauseInt(
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
        if (!solver->okay()) {
            *drup << fin;
        }
        *drup << del << ps << fin;
    }

    if (cl != NULL) {
        ClOffset offset = clAllocator.getOffset(cl);
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

    Clause* cl = addClauseInt(ps, true, stats);
    if (cl != NULL) {
        ClOffset offset = clAllocator.getOffset(cl);
        longRedCls.push_back(offset);
    }

    return ok;
}*/

void Solver::reArrangeClause(ClOffset offset)
{
    Clause& cl = *clAllocator.getPointer(offset);
    assert(cl.size() > 3);
    if (cl.size() == 3) return;

    //Change anything, but find the first two and assign them
    //accordingly at the ClauseData
    const Lit lit1 = cl[0];
    const Lit lit2 = cl[1];
    assert(lit1 != lit2);

    std::sort(cl.begin(), cl.end(), PolaritySorter(varData));

    uint8_t foundDatas = 0;
    for (uint32_t i = 0; i < cl.size(); i++) {
        if (cl[i] == lit1) {
            std::swap(cl[i], cl[0]);
            foundDatas++;
        }
    }

    for (uint32_t i = 0; i < cl.size(); i++) {
        if (cl[i] == lit2) {
            std::swap(cl[i], cl[1]);
            foundDatas++;
        }
    }
    assert(foundDatas == 2);
}

void Solver::reArrangeClauses()
{
    assert(decisionLevel() == 0);
    assert(ok);
    assert(qhead == trail.size());

    double myTime = cpuTime();
    for (uint32_t i = 0; i < longIrredCls.size(); i++) {
        reArrangeClause(longIrredCls[i]);
    }
    for (uint32_t i = 0; i < longRedCls.size(); i++) {
        reArrangeClause(longRedCls[i]);
    }

    const double time_used = cpuTime() - myTime;
    if (conf.verbosity >= 3) {
        cout
        << "c Rearrange lits in clauses "
        << std::setprecision(2) << time_used  << " s"
        << endl;
    }
    if (solver->conf.doSQL) {
        solver->sqlStats->time_passed_min(
            solver
            , "rearrange claues"
            , time_used
        );
    }
}

#ifdef DEBUG_RENUMBER
static void printArray(const vector<Var>& array, const std::string& str)
{
    cout << str << " : " << endl;
    for(size_t i = 0; i < array.size(); i++) {
        cout << str << "[" << i << "] : " << array[i] << endl;
    }
    cout << endl;
}
#endif

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
        Clause* cl = clAllocator.getPointer(longIrredCls[i]);
        updateLitsMap(*cl, outerToInter);
        cl->reCalcAbstraction();
    }

    for(size_t i = 0; i < longRedCls.size(); i++) {
        Clause* cl = clAllocator.getPointer(longRedCls[i]);
        updateLitsMap(*cl, outerToInter);
        cl->reCalcAbstraction();
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
        ; it++
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
void Solver::renumberVariables()
{
    double myTime = cpuTime();
    clauseCleaner->removeAndCleanAll();

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
    if (conf.verbosity >= 3) {
        cout
        << "c Reordered variables T: "
        << std::fixed << std::setw(5) << std::setprecision(2)
        << time_used
        << endl;
    }
    if (solver->conf.doSQL) {
        solver->sqlStats->time_passed_min(
            solver
            , "renumber"
            , time_used
        );
    }

    if (conf.doSaveMem) {
        saveVarMem(numEffectiveVars);
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
        && nVars() + n > 1ULL*1000ULL*1000ULL
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
    check_switchoff_limits_newvar(n);
    Searcher::new_vars(n);
    if (conf.doCache) {
        litReachable.resize(litReachable.size()+n*2, LitReachData());
    }
    varReplacer->new_vars(n);

    if (conf.perform_occur_based_simp) {
        simplifier->new_vars(n);
    }

    if (conf.doCompHandler) {
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

    if (conf.doCompHandler) {
        compHandler->new_var(orig_outer);
    }
    if (orig_outer == std::numeric_limits<Var>::max()) {
        datasync->new_var(bva);
    }

    //Too expensive
    //test_reflectivity_of_renumbering();
}

void Solver::saveVarMem(const uint32_t newNumVars)
{
    //TODO should we resize assumptionsSet ??

    //printMemStats();

    const double myTime = cpuTime();
    minNumVars = newNumVars;
    Searcher::saveVarMem();

    litReachable.resize(nVars()*2);
    litReachable.shrink_to_fit();
    varReplacer->saveVarMem();
    if (simplifier) {
        simplifier->saveVarMem();
    }
    if (conf.doCompHandler) {
        compHandler->saveVarMem();
    }
    datasync->saveVarMem();

    const double time_used = cpuTime() - myTime;
    if (conf.doSQL) {
        sqlStats->time_passed_min(
            this
            , "save var mem"
            , time_used
        );
    }
    //printMemStats();
}

void Solver::set_assumptions()
{
    assert(solver->okay());
    back_number_from_caller(origAssumptions);
    assumptions = back_number_from_caller_tmp;
    addClauseHelper(assumptions);
    for(const Lit lit: assumptions) {
        if (lit.var() < assumptionsSet.size()) {
            if (assumptionsSet[lit.var()]) {
                /*cout
                << "ERROR, the assumptions have the same variable inside"
                << " more than once!"
                << endl;*/
                //Yes, it can happen... due to variable replacement
            } else {
                assumptionsSet[lit.var()] = true;
            }
        } else {
            if (solver->value(lit) == l_Undef) {
                cout
                << "ERROR: Lit " << lit
                << " varData[lit.var()].removed: " << removed_type_to_string(varData[lit.var()].removed)
                << " value: " << value(lit)
                << " -- value should be l_Undef"
                << endl;
            }
            assert(solver->value(lit) != l_Undef);
        }
    }
}

void Solver::check_model_for_assumptions() const
{
    for(const Lit lit: origAssumptions) {
        assert(model.size() > lit.var());
        if (modelValue(lit) == l_Undef) {
            cout
            << "ERROR, lit " << lit
            << " was in the assumptions, but it wasn't set at all!"
            << endl;
        }
        assert(model[lit.var()] != l_Undef);
        if (modelValue(lit) != l_True) {
            cout
            << "ERROR, lit " << lit
            << " was in the assumptions, but it was set to its opposite value!"
            << endl;
        }
        assert(modelValue(lit) == l_True);
    }
}

void Solver::check_recursive_minimization_effectiveness(const lbool status)
{
    const Searcher::Stats& stats = Searcher::getStats();
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
    const Searcher::Stats& stats = Searcher::getStats();
    if (status == l_Undef
        && conf.doMinimRedMore
        && stats.moreMinimLitsStart > 100000
    ) {
        const Searcher::Stats& stats = Searcher::getStats();
        double remPercent =
            (double)(stats.moreMinimLitsStart-stats.moreMinimLitsEnd)/
                (double)(stats.moreMinimLitsStart)*100.0;

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
    check_stats();
    const double myTime = cpuTime();
    model = solver->back_number_solution(model);

    //Extend solution to stored solution in component handler
    if (conf.doCompHandler) {
        compHandler->addSavedState(model);
    }

    if (conf.perform_occur_based_simp
        || conf.doFindAndReplaceEqLits
    ) {
        SolutionExtender extender(this, simplifier);
        extender.extend();
    }
    model = map_back_to_without_bva(model);
    check_model_for_assumptions();
    if (conf.doSQL) {
        sqlStats->time_passed_min(
            this
            , "extend solution"
            , cpuTime()-myTime
        );
    }
}

void Solver::set_up_sql_writer()
{
    if (!conf.doSQL || solveStats.num_solve_calls > 1) {
        //Either it's already initialized, or it's not needed
        return;
    }

    bool ret = sqlStats->setup(this);
    if (!ret) {
        if (conf.doSQL == 2) {
            cout
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
    if (conf.doLHBR && !conf.propBinFirst) {
        std::cerr << "You must NOT set both LHBR and any-order propagation. LHBR needs binary clauses propagated first." << endl;
        exit(-1);
    }

    if (conf.shortTermHistorySize <= 0) {
        std::cerr << "You MUST give a short term history size (\"--gluehist\")  greater than 0!" << endl;
        exit(-1);
    }
}

lbool Solver::solve()
{
    solveStats.num_solve_calls++;
    conflict.clear();
    check_config_parameters();

    if (conf.verbosity >= 6) {
        cout
        << "c Solver::solve() called"
        << endl;
    }

    set_up_sql_writer();

    //Check if adding the clauses caused UNSAT
    lbool status = l_Undef;
    if (!ok) {
        status = l_False;
        if (conf.verbosity >= 6) {
            cout
            << "c Solver status l_Fase on startup of solve()"
            << endl;
        }
        goto end;
    }


    //Clean up as a startup
    reduceDB->reset_next_clean_limit();
    reduceDB->reduce_db_and_update_reset_stats(false);
    datasync->rebuild_bva_map();

    //Initialise
    reduceDB->increment_next_clean_limit();
    if (!origAssumptions.empty()) {
        //origAssumptions = *_assumptions;
        set_assumptions();
    } else {
        std::fill(assumptionsSet.begin(), assumptionsSet.end(), false);
        //origAssumptions.clear();
        assumptions.clear();
    }

    //If still unknown, simplify
    if (status == l_Undef
        && conf.simplify_at_startup
        && nVars() > 0
        && conf.regularly_simplify_problem
    ) {
        status = simplifyProblem();
    }

    //Iterate until solved
    while (status == l_Undef
        && !must_interrupt_asap()
        && cpuTime() < conf.maxTime
        && sumStats.conflStats.numConflicts < (uint64_t)conf.maxConfl
    ) {
        if (conf.verbosity >= 2)
            printClauseSizeDistrib();

        //This is crucial, since we need to attach() clauses to threads
        clauseCleaner->removeAndCleanAll();

        //Solve using threads
        const size_t origTrailSize = trail.size();
        vector<lbool> statuses;
        long numConfls = reduceDB->get_nextCleanLimit() - sumStats.conflStats.numConflicts;
        assert(conf.increaseClean >= 1 && "Clean increment factor between cleaning must be >=1");
        for (size_t i = 0; i < conf.numCleanBetweenSimplify; i++) {
            numConfls += (double)reduceDB->get_nextCleanLimitInc() * std::pow(conf.increaseClean, (int)i);
        }

        //Abide by maxConfl limit
        numConfls = std::min<long>((long)numConfls, conf.maxConfl - (long)sumStats.conflStats.numConflicts);
        if (numConfls <= 0) break;
        status = Searcher::solve(numConfls);

        //Check for effectiveness
        check_recursive_minimization_effectiveness(status);
        check_minimization_effectiveness(status);

        //Update stats
        sumStats += Searcher::getStats();
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
            status = l_Undef;
            break;
        }

        reduceDB->reduce_db_and_update_reset_stats();
        zeroLevAssignsByThreads += trail.size() - origTrailSize;

        //Simplify
        if (conf.regularly_simplify_problem) {
            status = simplifyProblem();
        }
    }
    handle_found_solution(status);

    end:
    if (solver->conf.doSQL) {
        sqlStats->finishup(status);
    }

    return status;
}

void Solver::handle_found_solution(const lbool status)
{
    if (status == l_True) {
        extend_solution();
        cancelUntil(0);
    } else if (status == l_False) {
        //TODO
        //update_conflict_to_orig_assumptions();

        //Back-number the conflict
        solver->map_inter_to_outer(conflict);
    }
    checkDecisionVarCorrectness();
    check_implicit_stats();
}

void Solver::checkDecisionVarCorrectness() const
{
    //Check for var deicisonness
    for(size_t var = 0; var < nVarsOuter(); var++) {
        if (varData[var].removed != Removed::none
            && varData[var].removed != Removed::queued_replacer
        ) {
            assert(!varData[var].is_decision);
        }
    }
}

/**
@brief The function that brings together almost all CNF-simplifications

It burst-searches for given number of conflicts, then it tries all sorts of
things like variable elimination, subsumption, failed literal probing, etc.
to try to simplifcy the problem at hand.
*/
lbool Solver::simplifyProblem()
{
    assert(ok);
    testAllClauseAttach();
    #ifdef DEBUG_IMPLICIT_STATS
    check_stats();
    #endif
    reArrangeClauses();

    if (conf.verbosity >= 6) {
        cout
        << "c Solver::simplifyProblem() called"
        << endl;
    }

    if (conf.doFindComps
        && getNumFreeVars() < conf.compVarLimit
        && !solver->must_interrupt_asap()
    ) {
        CompFinder findParts(this);
        if (!findParts.findComps()) {
            goto end;
        }
    }

    if (conf.doCompHandler
        && getNumFreeVars() < conf.compVarLimit
        && solveStats.numSimplify >= conf.handlerFromSimpNum
        //Only every 2nd, since it can be costly to find parts
        && solveStats.numSimplify % 2 == 0
        && !solver->must_interrupt_asap()
    ) {
        if (!compHandler->handle())
            goto end;
    }

    //SCC&VAR-REPL
    if (solveStats.numSimplify > 0
        && conf.doFindAndReplaceEqLits
        && !solver->must_interrupt_asap()
    ) {
        if (!sCCFinder->performSCC())
            goto end;

        if (varReplacer->getNewToReplaceVars() > ((double)getNumFreeVars()*0.001)) {
            if (!varReplacer->performReplace())
                goto end;
        }
    }

    //Cache clean before probing (for speed)
    if (conf.doCache
        && !solver->must_interrupt_asap()
    ) {
        if (!implCache.clean(this))
            goto end;

        if (!implCache.tryBoth(this))
            goto end;
    }

    //Treat implicits
    if (conf.doStrSubImplicit
        && !solver->must_interrupt_asap()
    ) {
        subsumeImplicit->subsume_implicit();
    }
    if (sumStats.conflStats.numConflicts >= (uint64_t)conf.maxConfl
        || cpuTime() > conf.maxTime
        || must_interrupt_asap()
    ) {
        return l_Undef;
    }

    //PROBE
    updateDominators();
    if (conf.doProbe && !prober->probe()) {
        goto end;
    }
    if (sumStats.conflStats.numConflicts >= (uint64_t)conf.maxConfl
        || cpuTime() > conf.maxTime
        || must_interrupt_asap()
    ) {
        return l_Undef;
    }

    //Don't replace first -- the stamps won't work so well
    if (conf.doClausVivif && !strengthener->strengthen(true)) {
        goto end;
    }
    if (conf.doClausVivif && !vivifier->vivify(true)) {
        goto end;
    }
    if (sumStats.conflStats.numConflicts >= (uint64_t)conf.maxConfl
        || cpuTime() > conf.maxTime
        || must_interrupt_asap()
    ) {
        return l_Undef;
    }

    //SCC&VAR-REPL
    if (conf.doFindAndReplaceEqLits) {
        if (!sCCFinder->performSCC())
            goto end;

        if (!varReplacer->performReplace())
            goto end;
    }
    if (sumStats.conflStats.numConflicts >= (uint64_t)conf.maxConfl
        || cpuTime() > conf.maxTime
        || must_interrupt_asap()
    ) {
        return l_Undef;
    }

    //Treat implicits
    if (conf.doStrSubImplicit) {
        subsumeImplicit->subsume_implicit();
    }

    //Var-elim, gates, subsumption, strengthening
    if (conf.perform_occur_based_simp && !simplifier->simplify())
        goto end;

    //Treat implicits
    if (conf.doStrSubImplicit) {
        if (!strengthener->strengthenImplicit()) {
            goto end;
        }

        subsumeImplicit->subsume_implicit();
    }
    if (sumStats.conflStats.numConflicts >= (uint64_t)conf.maxConfl
        || cpuTime() > conf.maxTime
        || must_interrupt_asap()
    ) {
        return l_Undef;
    }

    //Clean cache before vivif
    if (conf.doCache && !implCache.clean(this))
        goto end;

    //Vivify clauses
    if (conf.doClausVivif && !strengthener->strengthen(true)) {
        goto end;
    }
    if (conf.doClausVivif && !vivifier->vivify(true)) {
        goto end;
    }
    if (sumStats.conflStats.numConflicts >= (uint64_t)conf.maxConfl
        || cpuTime() > conf.maxTime
        || must_interrupt_asap()
    ) {
        return l_Undef;
    }

    //Search & replace 2-long XORs
    if (conf.doFindAndReplaceEqLits) {
        if (!sCCFinder->performSCC())
            goto end;

        if (varReplacer->getNewToReplaceVars() > ((double)getNumFreeVars()*0.001)) {
            if (!varReplacer->performReplace())
                goto end;
        }
    }

    if (conf.doSortWatched)
        sortWatched();

    //Delete and disable cache if too large
    if (conf.doCache) {
        const size_t memUsedMB = implCache.memUsed()/(1024UL*1024UL);
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
    if (sumStats.conflStats.numConflicts >= (uint64_t)conf.maxConfl
        || cpuTime() > conf.maxTime
        || must_interrupt_asap()
    ) {
        return l_Undef;
    }

    if (conf.doRenumberVars) {
        //Clean cache before renumber -- very important, otherwise
        //we will be left with lits inside the cache that are out-of-bounds
        if (conf.doCache) {
            bool setSomething = true;
            while(setSomething) {
                if (!implCache.clean(this, &setSomething))
                    goto end;
            }
        }

        renumberVariables();
    }

    //Re-calculate reachability after re-numbering and new cache data
    if (conf.doCache) {
        calculate_reachability();
    }

    //Free unused watch memory
    freeUnusedWatches();

    reArrangeClauses();

    //addSymmBreakClauses();

end:
    if (conf.verbosity >= 3)
        cout << "c Searcher::simplifyProblem() finished" << endl;

    testAllClauseAttach();
    checkNoWrongAttach();

    //TODO not so good....
    //The algorithms above probably have changed the propagation&usage data
    //so let's clear it
    if (conf.doClearStatEveryClauseCleaning) {
        clear_clauses_stats();
    }

    solveStats.numSimplify++;

    if (!ok) {
        return l_False;
    } else {
        check_stats();
        checkImplicitPropagated();
        return l_Undef;
    }
}

void Solver::clear_clauses_stats()
{
    for(ClOffset offs: longIrredCls) {
        Clause* cl = clAllocator.getPointer(offs);
        cl->stats.clear(0);
    }

    for(ClOffset offs: longRedCls) {
        Clause* cl = clAllocator.getPointer(offs);
        cl->stats.clear(conf.multiplier_perf_values_after_cl_clean);
    }
}

void Solver::printPropConflStats(
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

void Solver::consolidateMem()
{
    const double myTime = cpuTime();
    clAllocator.consolidate(this, true);
    const double time_used = cpuTime() - myTime;
    if (conf.doSQL) {
        sqlStats->time_passed_min(
            this
            , "consolidate mem"
            , time_used
        );
    }
}

void Solver::printStats() const
{
    const double cpu_time = cpuTime();
    cout << "c ------- FINAL TOTAL SEARCH STATS ---------" << endl;
    printStatsLine("c UIP search time"
        , sumStats.cpu_time
        , stats_line_percent(sumStats.cpu_time, cpu_time)
        , "% time"
    );

    if (conf.verbStats >= 1) {
        printFullStats();
    } else {
        printMinStats();
    }
}

void Solver::printMinStats() const
{
    const double cpu_time = cpuTime();
    sumStats.printShort();
    printStatsLine("c props/decision"
        , (double)propStats.propagations/(double)sumStats.decisions
    );
    printStatsLine("c props/conflict"
        , (double)propStats.propagations/(double)sumStats.conflStats.numConflicts
    );

    printStatsLine("c 0-depth assigns", trail.size()
        , stats_line_percent(trail.size(), nVars())
        , "% vars"
    );
    printStatsLine("c 0-depth assigns by thrds"
        , zeroLevAssignsByThreads
        , stats_line_percent(zeroLevAssignsByThreads, nVars())
        , "% vars"
    );
    printStatsLine("c 0-depth assigns by CNF"
        , zeroLevAssignsByCNF
        , stats_line_percent(zeroLevAssignsByCNF, nVars())
        , "% vars"
    );

    //Failed lit stats
    if (conf.doProbe) {
        printStatsLine("c probing time"
            , prober->getStats().cpu_time
            , stats_line_percent(prober->getStats().cpu_time, cpu_time)
            , "% time"
        );

        prober->getStats().printShort();
    }
    //Simplifier stats
    if (conf.perform_occur_based_simp) {
        printStatsLine("c Simplifier time"
            , simplifier->getStats().totalTime()
            , stats_line_percent(simplifier->getStats().totalTime() ,cpu_time)
            , "% time"
        );
        simplifier->getStats().printShort(nVars());
    }
    printStatsLine("c SCC time"
        , sCCFinder->getStats().cpu_time
        , stats_line_percent(sCCFinder->getStats().cpu_time, cpu_time)
        , "% time"
    );
    sCCFinder->getStats().printShort(NULL);
    varReplacer->print_some_stats(cpu_time);

    //varReplacer->getStats().printShort(nVars());
    printStatsLine("c asymm time"
                    , vivifier->getStats().time_used
                    , stats_line_percent(vivifier->getStats().time_used, cpu_time)
                    , "% time"
    );
    printStatsLine("c strength cache-irred time"
                    , strengthener->getStats().irredCacheBased.cpu_time
                    , stats_line_percent(strengthener->getStats().irredCacheBased.cpu_time, cpu_time)
                    , "% time"
    );
    printStatsLine("c strength cache-red time"
                    , strengthener->getStats().redCacheBased.cpu_time
                    , stats_line_percent(strengthener->getStats().redCacheBased.cpu_time, cpu_time)
                    , "% time"
    );
    printStatsLine("c Conflicts in UIP"
        , sumStats.conflStats.numConflicts
        , (double)sumStats.conflStats.numConflicts/cpu_time
        , "confl/TOTAL_TIME_SEC"
    );
    printStatsLine("c Total time", cpu_time);
    printStatsLine("c Mem used"
        , memUsed()/(1024UL*1024UL)
        , "MB"
    );
    if (conf.doCache) {
        implCache.printStatsSort(this);
    }
}

void Solver::printFullStats() const
{
    const double cpu_time = cpuTime();

    sumStats.print();
    sumPropStats.print(sumStats.cpu_time);
    printStatsLine("c props/decision"
        , (double)propStats.propagations/(double)sumStats.decisions
    );
    printStatsLine("c props/conflict"
        , (double)propStats.propagations/(double)sumStats.conflStats.numConflicts
    );
    cout << "c ------- FINAL TOTAL SOLVING STATS END ---------" << endl;

    printStatsLine("c clause clean time"
        , reduceDB->cleaningStats.cpu_time
        , stats_line_percent(reduceDB->cleaningStats.cpu_time, cpu_time)
        , "% time"
    );
    //reduceDB->cleaningStats.print(reduceDB->nbReduceDB);

    printStatsLine("c reachability time"
        , reachStats.cpu_time
        , stats_line_percent(reachStats.cpu_time, cpu_time)
        , "% time"
    );
    reachStats.print();

    printStatsLine("c 0-depth assigns", trail.size()
        , stats_line_percent(trail.size(), nVars())
        , "% vars"
    );
    printStatsLine("c 0-depth assigns by thrds"
        , zeroLevAssignsByThreads
        , stats_line_percent(zeroLevAssignsByThreads, nVars())
        , "% vars"
    );
    printStatsLine("c 0-depth assigns by CNF"
        , zeroLevAssignsByCNF
        , stats_line_percent(zeroLevAssignsByCNF, nVars())
        , "% vars"
    );

    //Failed lit stats
    if (conf.doProbe) {
        printStatsLine("c probing time"
            , prober->getStats().cpu_time
            , stats_line_percent(prober->getStats().cpu_time, cpu_time)
            , "% time"
        );

        prober->getStats().print(nVars());
    }

    //Simplifier stats
    if (conf.perform_occur_based_simp) {
        printStatsLine("c Simplifier time"
            , simplifier->getStats().totalTime()
            , stats_line_percent(simplifier->getStats().totalTime(), cpu_time)
            , "% time"
        );

        simplifier->getStats().print(nVars());
    }

    if (simplifier && conf.doGateFind) {
        simplifier->printGateFinderStats();
    }

    //GateFinder stats


    /*
    //XOR stats
    printStatsLine("c XOR time"
        , subsumer->getXorFinder()->getStats().totalTime()
        , subsumer->getXorFinder()->getStats().totalTime()/cpu_time*100.0
        , "% time"
    );
    subsumer->getXorFinder()->getStats().print(
        subsumer->getXorFinder()->getNumCalls()
    );*/

    //VarReplacer stats
    printStatsLine("c SCC time"
        , sCCFinder->getStats().cpu_time
        , stats_line_percent(sCCFinder->getStats().cpu_time, cpu_time)
        , "% time"
    );
    sCCFinder->getStats().print();

    varReplacer->getStats().print(nVars());
    varReplacer->print_some_stats(cpu_time);

    //Vivifier-ASYMM stats
    printStatsLine("c vivif time"
                    , vivifier->getStats().time_used
                    , stats_line_percent(vivifier->getStats().time_used, cpu_time)
                    , "% time");
    printStatsLine("c strength cache-irred time"
                    , strengthener->getStats().irredCacheBased.cpu_time
                    , stats_line_percent(strengthener->getStats().irredCacheBased.cpu_time, cpu_time)
                    , "% time");
    printStatsLine("c strength cache-red time"
                    , strengthener->getStats().redCacheBased.cpu_time
                    , stats_line_percent(strengthener->getStats().redCacheBased.cpu_time, cpu_time)
                    , "% time");
    vivifier->getStats().print(nVars());
    strengthener->getStats().print();

    if (conf.doStrSubImplicit) {
        subsumeImplicit->getStats().print();
    }

    if (conf.doCache) {
        implCache.printStats(this);
    }

    //Other stats
    printStatsLine("c Conflicts in UIP"
        , sumStats.conflStats.numConflicts
        , (double)sumStats.conflStats.numConflicts/cpu_time
        , "confl/TOTAL_TIME_SEC"
    );
    printStatsLine("c Total time", cpu_time);
    printMemStats();
}

uint64_t Solver::printWatchMemUsed(const uint64_t totalMem) const
{
    size_t alloc = watches.mem_used_alloc();
    printStatsLine("c Mem for watch alloc"
        , alloc/(1024UL*1024UL)
        , "MB"
        , stats_line_percent(alloc, totalMem)
        , "%"
    );

    size_t array = watches.mem_used_array();
    printStatsLine("c Mem for watch array"
        , array/(1024UL*1024UL)
        , "MB"
        , stats_line_percent(array, totalMem)
        , "%"
    );

    return alloc + array;
}

void Solver::printMemStats() const
{
    const uint64_t totalMem = memUsedTotal();
    printStatsLine("c Mem used"
        , totalMem/(1024UL*1024UL)
        , "MB"
    );
    uint64_t account = 0;

    account += print_mem_used_longclauses(totalMem);
    account += printWatchMemUsed(totalMem);

    size_t mem = 0;
    mem += assigns.capacity()*sizeof(lbool);
    mem += varData.capacity()*sizeof(VarData);
    #ifdef STATS_NEEDED_EXTRA
    mem += varDataLT.capacity()*sizeof(VarData::Stats);
    #endif
    mem += assumptions.capacity()*sizeof(Lit);
    printStatsLine("c Mem for vars"
        , mem/(1024UL*1024UL)
        , "MB"
        , stats_line_percent(mem, totalMem)
        , "%"

    );
    account += mem;

    account += print_stamp_mem(totalMem);

    mem = implCache.memUsed();
    mem += litReachable.capacity()*sizeof(LitReachData);
    printStatsLine("c Mem for impl cache"
        , mem/(1024UL*1024UL)
        , "MB"
        , stats_line_percent(mem, totalMem)
        , "%"
    );
    account += mem;

    mem = hist.memUsed();
    printStatsLine("c Mem for history stats"
        , mem/(1024UL*1024UL)
        , "MB"
        , stats_line_percent(mem, totalMem)
        , "%"
    );
    account += mem;

    mem = memUsed();
    printStatsLine("c Mem for search&solve"
        , mem/(1024UL*1024UL)
        , "MB"
        , stats_line_percent(mem, totalMem)
        , "%"
    );
    account += mem;

    mem = 0;
    mem += seen.capacity()*sizeof(uint16_t);
    mem += seen2.capacity()*sizeof(uint16_t);
    mem += toClear.capacity()*sizeof(Lit);
    mem += analyze_stack.capacity()*sizeof(Lit);
    printStatsLine("c Mem for temporaries"
        , mem/(1024UL*1024UL)
        , "MB"
        , stats_line_percent(mem, totalMem)
        , "%"
    );
    account += mem;

    mem = CNF::get_renumber_mem();
    printStatsLine("c Mem for renumberer"
        , mem/(1024UL*1024UL)
        , "MB"
        , stats_line_percent(mem, totalMem)
        , "%"
    );
    account += mem;

    if (conf.perform_occur_based_simp) {
        mem = simplifier->memUsed();
        printStatsLine("c Mem for simplifier"
            , mem/(1024UL*1024UL)
            , "MB"
            , stats_line_percent(mem, totalMem)
            , "%"
        );
        account += mem;

        mem = simplifier->memUsedXor();
        printStatsLine("c Mem for xor-finder"
            , mem/(1024UL*1024UL)
            , "MB"
            , stats_line_percent(mem, totalMem)
            , "%"
        );
        account += mem;
    }

    mem = varReplacer->memUsed();
    printStatsLine("c Mem for varReplacer"
        , mem/(1024UL*1024UL)
        , "MB"
        , stats_line_percent(mem, totalMem)
        , "%"
    );
    account += mem;

    mem = sCCFinder->memUsed();
    printStatsLine("c Mem for SCC"
        , mem/(1024UL*1024UL)
        , "MB"
        , stats_line_percent(mem, totalMem)
        , "%"
    );
    account += mem;

    if (conf.doProbe) {
        mem = prober->memUsed();
        printStatsLine("c Mem for prober"
            , mem/(1024UL*1024UL)
            , "MB"
            , stats_line_percent(mem, totalMem)
            , "%"
        );
        account += mem;
    }

    printStatsLine("c Accounted for mem"
        , stats_line_percent(account, totalMem)
        , "%"
    );
}

void Solver::dumpBinClauses(
    const bool dumpRed
    , const bool dumpIrred
    , std::ostream* outfile
) const {
    if (conf.do_bva) {
        cout << "ERROR: cannot make meaningful dump with BVA turned on." << endl;
        exit(-1);
    }

    size_t wsLit = 0;
    for (watch_array::const_iterator
        it = watches.begin(), end = watches.end()
        ; it != end
        ; ++it, wsLit++
    ) {
        Lit lit = Lit::toLit(wsLit);
        watch_subarray_const ws = *it;

        //Each element in the watchlist
        for (watch_subarray_const::const_iterator
            it2 = ws.begin(), end2 = ws.end()
            ; it2 != end2
            ; it2++
        ) {
            //Only dump binaries
            if (it2->isBinary() && lit < it2->lit2()) {
                bool toDump = false;
                if (it2->red() && dumpRed) toDump = true;
                if (!it2->red() && dumpIrred) toDump = true;

                if (toDump) {
                    tmpCl.clear();
                    tmpCl.push_back(map_inter_to_outer(it2->lit2()));
                    tmpCl.push_back(map_inter_to_outer(lit));
                    std::sort(tmpCl.begin(), tmpCl.end());

                    *outfile
                    << tmpCl[0] << " "
                    << tmpCl[1]
                    << " 0\n";
                }
            }
        }
    }
}

void Solver::dumpTriClauses(
    const bool alsoRed
    , const bool alsoIrred
    , std::ostream* outfile
) const {
    if (conf.do_bva) {
        cout << "ERROR: cannot make meaningful dump with BVA turned on." << endl;
        exit(-1);
    }

    uint32_t wsLit = 0;
    for (watch_array::const_iterator
        it = watches.begin(), end = watches.end()
        ; it != end
        ; ++it, wsLit++
    ) {
        Lit lit = Lit::toLit(wsLit);
        watch_subarray_const ws = *it;
        for (watch_subarray_const::const_iterator
            it2 = ws.begin(), end2 = ws.end()
            ; it2 != end2
            ; it2++
        ) {
            //Only one instance of tri clause
            if (it2->isTri() && lit < it2->lit2()) {
                bool toDump = false;
                if (it2->red() && alsoRed) toDump = true;
                if (!it2->red() && alsoIrred) toDump = true;

                if (toDump) {
                    tmpCl.clear();
                    tmpCl.push_back(map_inter_to_outer(it2->lit2()));
                    tmpCl.push_back(map_inter_to_outer(it2->lit3()));
                    tmpCl.push_back(map_inter_to_outer(lit));
                    std::sort(tmpCl.begin(), tmpCl.end());

                    *outfile
                    << tmpCl[0] << " "
                    << tmpCl[1] << " "
                    << tmpCl[2]
                    << " 0\n";
                }
            }
        }
    }
}

void Solver::printClauseSizeDistrib()
{
    size_t size4 = 0;
    size_t size5 = 0;
    size_t sizeLarge = 0;
    for(vector<ClOffset>::const_iterator
        it = longIrredCls.begin(), end = longIrredCls.end()
        ; it != end
        ; it++
    ) {
        Clause* cl = clAllocator.getPointer(*it);
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

void Solver::dumpEquivalentLits(std::ostream* os) const
{
    if (conf.do_bva) {
        cout << "ERROR: cannot make meaningful dump with BVA turned on." << endl;
        exit(-1);
    }

    *os
    << "c " << endl
    << "c ---------------------------------------" << endl
    << "c equivalent literals" << endl
    << "c ---------------------------------------" << endl;

    varReplacer->print_equivalent_literals(os);
}

vector<Lit> Solver::get_zero_assigned_lits() const
{
    vector<Lit> lits;
    assert(decisionLevel() == 0);
    for(size_t i = 0; i < assigns.size(); i++) {
        if (assigns[i] != l_Undef) {
            Lit lit(i, assigns[i] == l_False);

            //Update to higher-up
            lit = varReplacer->getLitReplacedWith(lit);
            if (varData[lit.var()].is_bva == false) {
                lits.push_back(map_inter_to_outer(lit));
            }

            //Everything it repaces has also been set
            const vector<Var> vars = varReplacer->get_vars_replacing(lit.var());
            for(const Var var: vars) {
                if (varData[var].is_bva)
                    continue;

                Lit tmp_lit = Lit(var, false);
                assert(varReplacer->getLitReplacedWith(tmp_lit).var() == lit.var());
                if (lit != varReplacer->getLitReplacedWith(tmp_lit)) {
                    tmp_lit ^= true;
                }
                assert(lit == varReplacer->getLitReplacedWith(tmp_lit));

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

void Solver::dumpUnitaryClauses(std::ostream* os) const
{
    if (conf.do_bva) {
        cout << "ERROR: cannot make meaningful dump with BVA turned on." << endl;
        exit(-1);
    }

    *os
    << "c " << endl
    << "c ---------" << endl
    << "c unitaries" << endl
    << "c ---------" << endl;

    //'trail' cannot be trusted between 0....size()
    assert(decisionLevel() == 0);
    vector<Lit> lits = get_zero_assigned_lits();
    for(const Lit lit: lits) {
        *os << lit << " 0\n";
    }
}

void Solver::dumpRedClauses(
    std::ostream* os
    , const uint32_t maxSize
) const {
    if (conf.do_bva) {
        cout << "ERROR: cannot make meaningful dump with BVA turned on." << endl;
        exit(-1);
    }

    dumpUnitaryClauses(os);

    *os
    << "c " << endl
    << "c ---------------------------------" << endl
    << "c redundant binary clauses (extracted from watchlists)" << endl
    << "c ---------------------------------" << endl;
    if (maxSize >= 2) {
        dumpBinClauses(true, false, os);
    }

    *os
    << "c " << endl
    << "c ---------------------------------" << endl
    << "c redundant tertiary clauses (extracted from watchlists)" << endl
    << "c ---------------------------------" << endl;
    if (maxSize >= 3) {
        dumpTriClauses(true, false, os);
    }

    if (maxSize >= 2) {
        dumpEquivalentLits(os);
    }

    *os
    << "c " << endl
    << "c --------------------" << endl
    << "c redundant long clauses" << endl
    << "c --------------------" << endl;
    dump_clauses(longRedCls, os, maxSize);
}

uint64_t Solver::count_irred_clauses_for_dump() const
{
    uint64_t numClauses = 0;

    //unitary clauses
    for (size_t
        i = 0, end = (trail_lim.size() > 0) ? trail_lim[0] : trail.size()
        ; i < end; i++
    ) {
        numClauses++;
    }

    //binary XOR clauses
    if (varReplacer) {
        varReplacer->get_num_bin_clauses();
    }

    //Normal clauses
    numClauses += binTri.irredBins;
    numClauses += binTri.irredTris;
    numClauses += longIrredCls.size();
    if (conf.doCompHandler) {
        compHandler->getRemovedClauses().sizes.size();
    }

    //previously eliminated clauses
    if (conf.perform_occur_based_simp) {
        const vector<BlockedClause>& blockedClauses = simplifier->getBlockedClauses();
        numClauses += blockedClauses.size();
    }

    return numClauses;
}

void Solver::dump_clauses(
    const vector<ClOffset>& cls
    , std::ostream* os
    , size_t max_size
) const {
    if (conf.do_bva) {
        cout << "ERROR: cannot make meaningful dump with BVA turned on." << endl;
        exit(-1);
    }

    for(vector<ClOffset>::const_iterator
        it = cls.begin(), end = cls.end()
        ; it != end
        ; it++
    ) {
        Clause* cl = clAllocator.getPointer(*it);
        if (cl->size() <= max_size)
            *os << sortLits(clauseBackNumbered(*cl)) << " 0\n";
    }
}

void Solver::dump_blocked_clauses(std::ostream* os) const
{
    if (conf.do_bva) {
        cout << "ERROR: cannot make meaningful dump with BVA turned on." << endl;
        exit(-1);
    }

    if (conf.perform_occur_based_simp) {
        const vector<BlockedClause>& blockedClauses
            = simplifier->getBlockedClauses();

        for (vector<BlockedClause>::const_iterator
            it = blockedClauses.begin(); it != blockedClauses.end()
            ; it++
        ) {
            if (it->dummy)
                continue;

            //Print info about clause
            *os
            << "c next clause is eliminated/blocked on lit "
            << it->blockedOn
            << endl;

            //Print clause
            *os
            << sortLits(it->lits)
            << " 0"
            << endl;
        }
    }
}

void Solver::dump_component_clauses(std::ostream* os) const
{
    if (conf.do_bva) {
        cout << "ERROR: cannot make meaningful dump with BVA turned on." << endl;
        exit(-1);
    }

    if (conf.doCompHandler) {
        const CompHandler::RemovedClauses& removedClauses = compHandler->getRemovedClauses();

        vector<Lit> tmp;
        size_t at = 0;
        for (uint32_t size :removedClauses.sizes) {
            tmp.clear();
            for(size_t i = at; i < at + size; i++) {
                tmp.push_back(removedClauses.lits[i]);
            }
            std::sort(tmp.begin(), tmp.end());
            *os << tmp << " 0" << endl;

            //Move 'at' along
            at += size;
        }
    }
}

void Solver::dumpIrredClauses(std::ostream* os) const
{
    if (conf.do_bva) {
        cout << "ERROR: cannot make meaningful dump with BVA turned on." << endl;
        exit(-1);
    }

    *os
    << "p cnf "
    << nVarsOuter()
    << " " << count_irred_clauses_for_dump()
    << endl;

    dumpUnitaryClauses(os);
    dumpEquivalentLits(os);

    *os
    << "c " << endl
    << "c ---------------" << endl
    << "c binary clauses" << endl
    << "c ---------------" << endl;
    dumpBinClauses(false, true, os);

    *os
    << "c " << endl
    << "c ---------------" << endl
    << "c tertiary clauses" << endl
    << "c ---------------" << endl;
    dumpTriClauses(false, true, os);

    *os
    << "c " << endl
    << "c ---------------" << endl
    << "c normal clauses" << endl
    << "c ---------------" << endl;
    dump_clauses(longIrredCls, os);

    *os
    << "c " << endl
    << "c -------------------------------" << endl
    << "c previously eliminated variables" << endl
    << "c -------------------------------" << endl;
    dump_blocked_clauses(os);

    *os
    << "c " << endl
    << "c ---------------" << endl
    << "c clauses in components" << endl
    << "c ---------------" << endl;
    dump_component_clauses(os);

    write_irred_stats_to_cnf(os);
}

void Solver::write_irred_stats_to_cnf(std::ostream* os) const
{
    *os << "c units: " << ((trail_lim.size() > 0) ? trail_lim[0] : trail.size()) << endl;
    if (varReplacer) {
        *os << "c binaries related to binary XORs: " << varReplacer->get_num_bin_clauses() << endl;
    }
    *os << "c normal binary cls: " << binTri.irredBins << endl;
    *os << "c normal tertiary cls: " << binTri.irredTris << endl;
    *os << "c normal long cls: " << longIrredCls.size() << endl;
    if (conf.doCompHandler) {
        *os << "c disconnected component cls: "
        << compHandler->getRemovedClauses().sizes.size()
        << endl;
    }
    if (conf.perform_occur_based_simp) {
        const vector<BlockedClause>& blockedClauses = simplifier->getBlockedClauses();
        *os << "c blocked cls: " << blockedClauses.size() << endl;
    }
}

void Solver::printAllClauses() const
{
    for(vector<ClOffset>::const_iterator
        it = longIrredCls.begin(), end = longIrredCls.end()
        ; it != end
        ; it++
    ) {
        Clause* cl = clAllocator.getPointer(*it);
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
                cout << "Normal clause offs " << it2->getOffset() << endl;
            } else if (it2->isTri()) {
                cout << "Tri clause:"
                << lit << " , "
                << it2->lit2() << " , "
                << it2->lit3() << endl;
            }
        }
    }
}

bool Solver::verifyImplicitClauses() const
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
                && modelValue(lit) != l_True
                && modelValue(w.lit2()) != l_True
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
                && modelValue(lit) != l_True
                && modelValue(w.lit2()) != l_True
                && modelValue(w.lit3()) != l_True
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

bool Solver::verifyClauses(const vector<ClOffset>& cs) const
{
    #ifdef VERBOSE_DEBUG
    cout << "Checking clauses whether they have been properly satisfied." << endl;
    #endif

    bool verificationOK = true;

    for (vector<ClOffset>::const_iterator
        it = cs.begin(), end = cs.end()
        ; it != end
        ; it++
    ) {
        Clause& cl = *clAllocator.getPointer(*it);
        for (uint32_t j = 0; j < cl.size(); j++)
            if (modelValue(cl[j]) == l_True)
                goto next;

        cout << "unsatisfied clause: " << cl << endl;
        verificationOK = false;
        next:
        ;
    }

    return verificationOK;
}

bool Solver::verifyModel() const
{
    bool verificationOK = true;
    verificationOK &= verifyClauses(longIrredCls);
    verificationOK &= verifyClauses(longRedCls);
    verificationOK &= verifyImplicitClauses();

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

lbool Solver::modelValue (const Lit p) const
{
    return model[p.var()] ^ p.sign();
}

void Solver::testAllClauseAttach() const
{
#ifndef DEBUG_ATTACH_MORE
    return;
#endif

    for (vector<ClOffset>::const_iterator
        it = longIrredCls.begin(), end = longIrredCls.end()
        ; it != end
        ; it++
    ) {
        assert(normClauseIsAttached(*it));
    }
}

bool Solver::normClauseIsAttached(const ClOffset offset) const
{
    bool attached = true;
    const Clause& cl = *clAllocator.getPointer(offset);
    assert(cl.size() > 3);

    attached &= findWCl(watches[cl[0].toInt()], offset);
    attached &= findWCl(watches[cl[1].toInt()], offset);

    return attached;
}

void Solver::findAllAttach() const
{

    #ifndef MORE_DEBUG
    return;
    #endif

    for (size_t i = 0; i < watches.size(); i++) {
        const Lit lit = Lit::toLit(i);
        for (uint32_t i2 = 0; i2 < watches[i].size(); i2++) {
            const Watched& w = watches[i][i2];
            if (!w.isClause())
                continue;

            //Get clause
            Clause* cl = clAllocator.getPointer(w.getOffset());
            assert(!cl->getFreed());

            //Assert watch correctness
            if ((*cl)[0] != lit
                && (*cl)[1] != lit
            ) {
                cout
                << "ERROR! Clause " << (*cl)
                << " not attached?"
                << endl;

                assert(false);
                std::exit(-1);
            }

            //Clause in one of the lists
            if (!findClause(w.getOffset())) {
                cout
                << "ERROR! did not find clause " << *cl
                << endl;

                assert(false);
                std::exit(1);
            }
        }
    }

    findAllAttach(longIrredCls);
    findAllAttach(longRedCls);
}

void Solver::findAllAttach(const vector<ClOffset>& cs) const
{
    for(vector<ClOffset>::const_iterator
        it = cs.begin(), end = cs.end()
        ; it != end
        ; it++
    ) {
        Clause& cl = *clAllocator.getPointer(*it);
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


bool Solver::findClause(const ClOffset offset) const
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

void Solver::checkNoWrongAttach() const
{
    #ifndef MORE_DEBUG
    return;
    #endif

    for (vector<ClOffset>::const_iterator
        it = longRedCls.begin(), end = longRedCls.end()
        ; it != end
        ; it++
    ) {
        const Clause& cl = *clAllocator.getPointer(*it);
        for (uint32_t i = 0; i < cl.size(); i++) {
            if (i > 0)
                assert(cl[i-1].var() != cl[i].var());
        }
    }
}

size_t Solver::getNumFreeVars() const
{
    uint32_t freeVars = nVarsOuter();
    if (decisionLevel() == 0) {
        freeVars -= trail.size();
    } else {
        freeVars -= trail_lim[0];
    }

    if (conf.perform_occur_based_simp) {
        freeVars -= simplifier->getStats().numVarsElimed;
    }
    freeVars -= varReplacer->getNumReplacedVars();

    return freeVars;
}

void Solver::printClauseStats() const
{
    //Irredundant
    print_value_kilo_mega(longIrredCls.size());
    print_value_kilo_mega(binTri.irredTris);
    print_value_kilo_mega(binTri.irredBins);
    cout
    << " " << std::setw(5) << std::fixed << std::setprecision(1)
    << (double)litStats.irredLits/(double)(longIrredCls.size());

    //Redundant
    print_value_kilo_mega(longRedCls.size());
    print_value_kilo_mega(binTri.redTris);
    print_value_kilo_mega(binTri.redBins);
    cout
    << " " << std::setw(5) << std::fixed << std::setprecision(1)
    << (double)litStats.redLits/(double)(longRedCls.size())
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
        cout
        << "ERROR:"
        << " thisNumIrredBins/2: " << thisNumIrredBins/2
        << " binTri.irredBins: " << binTri.irredBins
        << "thisNumIrredBins: " << thisNumIrredBins
        << "thisNumRedBins: " << thisNumRedBins << endl;
    }
    assert(thisNumIrredBins % 2 == 0);
    assert(thisNumIrredBins/2 == binTri.irredBins);

    if (thisNumRedBins/2 != binTri.redBins) {
        cout
        << "ERROR:"
        << " thisNumRedBins/2: " << thisNumRedBins/2
        << " binTri.redBins: " << binTri.redBins
        << endl;
    }
    assert(thisNumRedBins % 2 == 0);
    assert(thisNumRedBins/2 == binTri.redBins);

    if (thisNumIrredTris/3 != binTri.irredTris) {
        cout
        << "ERROR:"
        << " thisNumIrredTris/3: " << thisNumIrredTris/3
        << " binTri.irredTris: " << binTri.irredTris
        << endl;
    }
    assert(thisNumIrredTris % 3 == 0);
    assert(thisNumIrredTris/3 == binTri.irredTris);

    if (thisNumRedTris/3 != binTri.redTris) {
        cout
        << "ERROR:"
        << " thisNumRedTris/3: " << thisNumRedTris/3
        << " binTri.redTris: " << binTri.redTris
        << endl;
    }
    assert(thisNumRedTris % 3 == 0);
    assert(thisNumRedTris/3 == binTri.redTris);

    const double time_used = cpuTime() - myTime;
    if (conf.doSQL) {
        sqlStats->time_passed_min(
            this
            , "check implicit stats"
            , time_used
        );
    }
}

uint64_t Solver::countLits(
    const vector<ClOffset>& clause_array
    , bool allowFreed
) const {
    uint64_t lits = 0;
    for(vector<ClOffset>::const_iterator
        it = clause_array.begin(), end = clause_array.end()
        ; it != end
        ; it++
    ) {
        const Clause& cl = *clAllocator.getPointer(*it);
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
    uint64_t numLitsIrred = countLits(longIrredCls, allowFreed);
    if (numLitsIrred != litStats.irredLits) {
        cout << "ERROR: " << endl;
        cout << "->numLitsIrred: " << numLitsIrred << endl;
        cout << "->litStats.irredLits: " << litStats.irredLits << endl;
    }
    assert(numLitsIrred == litStats.irredLits);

    uint64_t numLitsRed = countLits(longRedCls, allowFreed);
    if (numLitsRed != litStats.redLits) {
        cout << "ERROR: " << endl;
        cout << "->numLitsRed: " << numLitsRed << endl;
        cout << "->litStats.redLits: " << litStats.redLits << endl;
    }
    assert(numLitsRed == litStats.redLits);

    const double time_used = cpuTime() - myTime;
    if (conf.doSQL) {
        sqlStats->time_passed_min(
            this
            , "check literal stats"
            , time_used
        );
    }
}

size_t Solver::getNewToReplaceVars() const
{
    return varReplacer->getNewToReplaceVars();
}

const char* Solver::getVersion()
{
    #ifdef _MSC_VER
    return "MSVC-compiled, without GIT";
    #else
    return get_git_version();
    #endif //_MSC_VER
}


void Solver::printWatchlist(watch_subarray_const ws, const Lit lit) const
{
    for (watch_subarray::const_iterator
        it = ws.begin(), end = ws.end()
        ; it != end
        ; it++
    ) {
        if (it->isClause()) {
            cout
            << "Clause: " << *clAllocator.getPointer(it->getOffset());
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

void Solver::checkImplicitPropagated() const
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
    if (conf.doSQL) {
        sqlStats->time_passed_min(
            this
            , "check implicit propagated"
            , time_used
        );
    }
}

size_t Solver::getNumVarsElimed() const
{
    if (conf.perform_occur_based_simp) {
        return simplifier->getStats().numVarsElimed;
    } else {
        return 0;
    }
}

size_t Solver::getNumVarsReplaced() const
{
    return varReplacer->getNumReplacedVars();
}

void Solver::open_file_and_dump_red_clauses(const string redDumpFname) const
{
    if (conf.doStrSubImplicit && okay()) {
        subsumeImplicit->subsume_implicit();
    }

    std::ofstream outfile;
    open_dump_file(outfile, redDumpFname);
    try {
        if (!okay()) {
            outfile
            << "p cnf 0 1\n"
            << "0\n";
        } else {
            dumpRedClauses(&outfile, conf.maxDumpRedsSize);
        }
    } catch (std::ifstream::failure e) {
        cout
        << "Error writing clause dump to file: " << e.what()
        << endl;
        std::exit(-1);
    }
}

void Solver::open_file_and_dump_irred_clauses(const string irredDumpFname) const
{
    std::ofstream outfile;
    open_dump_file(outfile, irredDumpFname);

    try {
        if (!okay()) {
            outfile
            << "p cnf 0 1\n"
            << "0\n";
        } else {
            dumpIrredClauses(&outfile);
        }
    } catch (std::ifstream::failure e) {
        cout
        << "Error writing clause dump to file: " << e.what()
        << endl;
        std::exit(-1);
    }
}

void Solver::open_dump_file(std::ofstream& outfile, std::string filename) const
{
    outfile.open(filename.c_str());
    if (!outfile) {
        cout
        << "Cannot open file '"
        << filename
        << "' for writing. exiting"
        << endl;
        std::exit(-1);
    }
    outfile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
}

Lit Solver::updateLitForDomin(Lit lit) const
{
    //Nothing to update
    if (lit == lit_Undef)
        return lit;

    //Update to parent
    lit = varReplacer->getLitReplacedWith(lit);

    //If parent is removed, then this dominator cannot be updated
    if (varData[lit.var()].removed != Removed::none)
        return lit_Undef;

    return lit;
}

void Solver::updateDominators()
{
    for(Timestamp& tstamp: stamp.tstamp) {
        for(size_t i = 0; i < 2; i++) {
            Lit newLit = updateLitForDomin(tstamp.dominator[i]);
            tstamp.dominator[i] = newLit;
            if (newLit == lit_Undef)
                tstamp.numDom[i] = 0;
        }
    }
}

void Solver::calculate_reachability()
{
    double myTime = cpuTime();

    //Clear out
    for (size_t i = 0; i < nVars()*2; i++) {
        litReachable[i] = LitReachData();
    }

    //Go through each that is decision variable
    for (size_t i = 0, end = solver->nVars()*2; i < end; i++) {
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
        for (vector<LitExtra>::const_iterator
            it = cache.begin(), end = cache.end()
            ; it != end
            ; it++
        ) {
            assert(it->getLit() != lit);
            assert(it->getLit() != ~lit);

            if (litReachable[it->getLit().toInt()].lit == lit_Undef
                || litReachable[it->getLit().toInt()].numInCache < cacheSize
            ) {
                litReachable[it->getLit().toInt()].lit = ~lit;
                litReachable[it->getLit().toInt()].numInCache = cacheSize;
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
    if (conf.doSQL) {
        sqlStats->time_passed_min(
            this
            , "calc reachability"
            , time_used
        );
    }
}

void Solver::freeUnusedWatches()
{
    size_t wsLit = 0;
    for (watch_array::iterator
        it = solver->watches.begin(), end = solver->watches.end()
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
    solver->watches.consolidate();
}

bool Solver::enqueueThese(const vector<Lit>& toEnqueue)
{
    assert(ok);
    assert(decisionLevel() == 0);
    for(const Lit lit: toEnqueue) {

        const lbool val = value(lit);
        if (val == l_Undef) {
            assert(varData[lit.var()].removed == Removed::none
                || varData[lit.var()].removed == Removed::queued_replacer
            );
            enqueue(lit);
            ok = propagate().isNULL();

            if (!ok) {
                return false;
            }
        } else if (val == l_False) {
            ok = false;
            return false;
        }
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
    sumStats += Searcher::getStats();
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
    check_too_large_variable_number(lits);
    back_number_from_caller(lits);
    return addClause(back_number_from_caller_tmp);
}

bool Solver::add_xor_clause_outer(const vector<Var>& vars, bool rhs)
{
    vector<Lit> lits(vars.size());
    for(size_t i = 0; i < vars.size(); i++) {
        lits[i] = Lit(vars[i], false);
    }
    check_too_large_variable_number(lits);

    back_number_from_caller(lits);
    addClauseHelper(back_number_from_caller_tmp);
    add_xor_clause_inter(back_number_from_caller_tmp, rhs, true, false);

    return ok;
}

void Solver::check_too_large_variable_number(const vector<Lit>& lits) const
{
    for (const Lit lit: lits) {
        if (lit.var() >= nVarsOutside()) {
            cout
            << "ERROR: Variable " << lit.var() + 1
            << " inserted, but max var is "
            << nVarsOutside()
            << endl;
            assert(false);
            std::exit(-1);
        }
        release_assert(lit.var() < nVarsOutside()
        && "Clause inserted, but variable inside has not been declared with PropEngine::new_var() !");
    }
}

void Solver::bva_changed()
{
    datasync->rebuild_bva_map();
}

uint64_t Solver::getNextCleanLimit() const
{
    return reduceDB->get_nextCleanLimit();
}
