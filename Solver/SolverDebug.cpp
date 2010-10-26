/***************************************************************************
CryptoMiniSat -- Copyright (c) 2009 Mate Soos

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
****************************************************************************/

#include "Solver.h"

#include "VarReplacer.h"

#ifdef DEBUG_ATTACH
void Solver::testAllClauseAttach() const
{
    for (Clause *const*it = clauses.getData(), *const*end = clauses.getDataEnd(); it != end; it++) {
        const Clause& c = **it;
        assert(normClauseIsAttached(c));
    }

    for (XorClause *const*it = xorclauses.getData(), *const*end = xorclauses.getDataEnd(); it != end; it++) {
        const XorClause& c = **it;
        assert(xorClauseIsAttached(c));
    }
}

const bool Solver::normClauseIsAttached(const Clause& c) const
{
    bool attached = true;
    assert(c.size() > 2);

    ClauseOffset offset = clauseAllocator.getOffset(&c);
    if (c.size() == 3) {
        //The clause might have been longer, and has only recently
        //became 3-long. Check, and detach accordingly
        if (findWCl(watches[(~c[0]).toInt()], offset)) goto fullClause;

        Lit lit1 = c[0];
        Lit lit2 = c[1];
        Lit lit3 = c[2];
        attached &= findWTri(watches[(~lit1).toInt()], lit2, lit3);
        attached &= findWTri(watches[(~lit2).toInt()], lit1, lit3);
        attached &= findWTri(watches[(~lit3).toInt()], lit1, lit2);
    } else {
        fullClause:
        attached &= findWCl(watches[(~c[0]).toInt()], offset);
        attached &= findWCl(watches[(~c[1]).toInt()], offset);
    }

    return attached;
}

const bool Solver::xorClauseIsAttached(const XorClause& c) const
{
    ClauseOffset offset = clauseAllocator.getOffset(&c);
    bool attached = true;
    attached &= findWXCl(watches[(c[0]).toInt()], offset);
    attached &= findWXCl(watches[(~c[0]).toInt()], offset);
    attached &= findWXCl(watches[(c[1]).toInt()], offset);
    attached &= findWXCl(watches[(~c[1]).toInt()], offset);

    return attached;
}

void Solver::findAllAttach() const
{
    for (uint32_t i = 0; i < watches.size(); i++) {
        for (uint32_t i2 = 0; i2 < watches[i].size(); i2++) {
            const Watched& w = watches[i][i2];
            if (w.isClause()) findClause(clauseAllocator.getPointer(w.getOffset()));
            if (w.isXorClause()) findClause(clauseAllocator.getPointer(w.getOffset()));
        }
    }
}

const bool Solver::findClause(XorClause* c) const
{
    for (uint32_t i = 0; i < xorclauses.size(); i++) {
        if (xorclauses[i] == c) return true;
    }
    return false;
}

const bool Solver::findClause(Clause* c) const
{
    for (uint32_t i = 0; i < clauses.size(); i++) {
        if (clauses[i] == c) return true;
    }
    for (uint32_t i = 0; i < learnts.size(); i++) {
        if (learnts[i] == c) return true;
    }

    return false;
}
#endif //DEBUG_ATTACH

void Solver::checkSolution()
{
    model.growTo(nVars());
    for (Var var = 0; var != nVars(); var++) model[var] = value(var);
    release_assert(verifyModel());
    model.clear();
}

const bool Solver::verifyXorClauses() const
{
    #ifdef VERBOSE_DEBUG
    cout << "Checking xor-clauses whether they have been properly satisfied." << endl;;
    #endif

    bool verificationOK = true;

    for (uint32_t i = 0; i != xorclauses.size(); i++) {
        XorClause& c = *xorclauses[i];
        bool final = c.xorEqualFalse();

        #ifdef VERBOSE_DEBUG
        std::cout << "verifying xor clause: " << c << std::endl;
        #endif

        for (uint32_t j = 0; j < c.size(); j++) {
            assert(modelValue(c[j].unsign()) != l_Undef);
            final ^= (modelValue(c[j].unsign()) == l_True);
        }
        if (!final) {
            printf("unsatisfied clause: ");
            xorclauses[i]->plainPrint();
            verificationOK = false;
        }
    }

    return verificationOK;
}

const bool Solver::verifyBinClauses() const
{
    uint32_t wsLit = 0;
    for (const vec<Watched> *it = watches.getData(), *end = watches.getDataEnd(); it != end; it++, wsLit++) {
        Lit lit = ~Lit::toLit(wsLit);
        const vec<Watched>& ws = *it;

        uint32_t i;
        for (i = 0; i < ws.size(); i++) {
            if (ws[i].isBinary()
                && value(lit) != l_True
                && value(ws[i].getOtherLit()) != l_True
            ) {
                std::cout << "bin clause: " << lit << " , " << ws[i].getOtherLit() << " not satisfied!" << std::endl;
                return false;
            }
        }
    }

    return true;
}

const bool Solver::verifyClauses(const vec<Clause*>& cs) const
{
    #ifdef VERBOSE_DEBUG
    cout << "Checking clauses whether they have been properly satisfied." << endl;;
    #endif

    bool verificationOK = true;

    for (uint32_t i = 0; i != cs.size(); i++) {
        Clause& c = *cs[i];
        for (uint32_t j = 0; j < c.size(); j++)
            if (modelValue(c[j]) == l_True)
                goto next;

        printf("unsatisfied clause: ");
        cs[i]->plainPrint();
        verificationOK = false;
        next:
        ;
    }

    return verificationOK;
}

const bool Solver::verifyModel() const
{
    bool verificationOK = true;
    verificationOK &= verifyClauses(clauses);
    verificationOK &= verifyClauses(learnts);
    verificationOK &= verifyBinClauses();
    verificationOK &= verifyXorClauses();

    if (conf.verbosity >=1 && verificationOK)
        printf("c Verified %d clauses.\n", clauses.size() + xorclauses.size());

    return verificationOK;
}


void Solver::checkLiteralCount()
{
    // Check that sizes are calculated correctly:
    int cnt = 0;
    for (uint32_t i = 0; i != clauses.size(); i++)
        cnt += clauses[i]->size();

    for (uint32_t i = 0; i != xorclauses.size(); i++)
        cnt += xorclauses[i]->size();

    if ((int)clauses_literals != cnt) {
        fprintf(stderr, "literal count: %d, real value = %d\n", (int)clauses_literals, cnt);
        assert((int)clauses_literals == cnt);
    }
}
