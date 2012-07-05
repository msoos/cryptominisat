/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2011, Mate Soos and collaborators. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3.0 of the License, or (at your option) any later version.
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

#ifndef XORSIMPLIFIER_H
#define XORSIMPLIFIER_H

#include "Solver.h"
#include "Vec.h"
#include "CSet.h"

class CommandControl;
class ClauseCleaner;
class SolutionExtender;

struct XorElimedClause
{
    vector<Lit> lits;
    bool xorEqualFalse;
};

inline std::ostream& operator<<(std::ostream& os, const XorElimedClause& cl)
{
    os << "x";
    if (cl.xorEqualFalse) os << "-";
    for (size_t i = 0; i < cl.lits.size(); i++) {
        assert(!cl.lits[i].sign());
        os << Lit(cl.lits[i].var(), false) << " ";
    }

    return os;
}

/**
@brief Handles xor-subsumption and variable elimination at the XOR level

This class achieves three things:

1) it removes variables though XOR-ing of two xors thereby removing their common
variable. If that variable is not used anywere else, the variable is now removed
from the problem

2) It tries to XOR clauses together to get 1-long or 2-long XOR clauses. These
force variables to certain values or replace variables with other variables,
respectively

3) It tries to subsume XOR clauses with other XOR clauses (making 2 XOR clauses
in the process, but one of them is going to be much smaller than it was originally)
*/
class XorSimplifier
{
public:

    XorSimplifier(Solver& S2);
    const bool simplifyBySubsumption();
    void unlinkClause(ClauseSimp cc, Var elim = var_Undef);
    ClauseSimp linkInClause(XorClause& cl);
    void linkInAlreadyClause(ClauseSimp& c);
    void newVar();
    void extendModel(SolutionExtender* solver2);

    const uint32_t getNumElimed() const;
    const vector<char>& getVarElimed() const;
    const bool unEliminate(const Var var, CommandControl* ccsolver);
    const bool checkElimedUnassigned() const;
    const double getTotalTime() const;

    const map<Var, vector<XorElimedClause> >& getElimedOutVar() const;

private:

    friend class ClauseCleaner;
    friend class ClauseAllocator;

    //Main
    vector<XorClause*>        clauses;
    vector<AbstData>          clauseData;
    vector<vector<ClauseSimp> >  occur;          // 'occur[index(lit)]' is a list of constraints containing 'lit'.
    Solver&                solver;         // The Solver

    // Temporaries (to reduce allocation overhead):
    //
    vector<char>              seen_tmp;       // (used in various places)

    //Start-up
    void addFromSolver(vector<XorClause*>& cs);
    void addBackToSolver();

    // Subsumption:
    void findSubsumed(XorClause& ps, uint32_t index, vector<ClauseSimp>& out_subsumed);
    bool isSubsumed(XorClause& ps);
    void subsume0(ClauseSimp ps, XorClause& cl);
    template<class T1, class T2>
    bool subset(const T1& A, const T2& B);
    bool subsetAbst(const CL_ABST_TYPE A, const CL_ABST_TYPE B);
    template<class T>
    void findUnMatched(const T& A, const T& B, vector<Lit>& unmatchedPart);

    //helper
    void testAllClauseAttach() const;

    //dependent removal
    const bool removeDependent();
    void fillCannotEliminate();
    vector<char> cannot_eliminate;
    void addToCannotEliminate(Clause* it);
    void removeWrong(vector<Clause*>& cs);
    void removeWrongBins();
    void removeAssignedVarsFromEliminated();

    //Global stats
    double totalTime;
    map<Var, vector<XorElimedClause> > elimedOutVar;
    vector<char> var_elimed;
    uint32_t numElimed;

    //Heule-process
    template<class T>
    void xorTwoClauses(const T& c1, const T& c2, vector<Lit>& xored);
    const bool localSubstitute();
    uint32_t localSubstituteUseful;

    uint32_t clauses_subsumed;
    uint32_t clauses_cut;
    uint32_t origNClauses;
};

inline bool XorSimplifier::subsetAbst(const CL_ABST_TYPE A, const CL_ABST_TYPE B)
{
    return !(A & ~B);
}

// Assumes 'seen' is cleared (will leave it cleared)
template<class T1, class T2>
bool XorSimplifier::subset(const T1& A, const T2& B)
{
    for (uint32_t i = 0; i != B.size(); i++)
        seen_tmp[B[i].var()] = 1;
    for (uint32_t i = 0; i != A.size(); i++) {
        if (!seen_tmp[A[i].var()]) {
            for (uint32_t i = 0; i != B.size(); i++)
                seen_tmp[B[i].var()] = 0;
            return false;
        }
    }
    for (uint32_t i = 0; i != B.size(); i++)
        seen_tmp[B[i].var()] = 0;
    return true;
}

inline void XorSimplifier::newVar()
{
    occur.resize(occur.size()+1);
    seen_tmp    .push_back(0);
    cannot_eliminate.push_back(0);
    var_elimed.push_back(0);
}

inline const vector<char>& XorSimplifier::getVarElimed() const
{
    return var_elimed;
}

inline const uint32_t XorSimplifier::getNumElimed() const
{
    return numElimed;
}

inline const double XorSimplifier::getTotalTime() const
{
    return totalTime;
}

inline const map<Var, vector<XorElimedClause> >& XorSimplifier::getElimedOutVar() const
{
    return elimedOutVar;
}

#endif //XORSIMPLIFIER_H
