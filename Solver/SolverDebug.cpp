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
//warning This needs to be fixed!
void Solver::testAllClauseAttach() const
{
    for (Clause *const*it = clauses.getData(), *const*end = clauses.getDataEnd(); it != end; it++) {
        const Clause& c = **it;
        if (c.size() > 2) {
            assert(findWatchedCl(watches[(~c[0]).toInt()], &c));
            assert(findWatchedCl(watches[(~c[1]).toInt()], &c));
        } else {
            assert(findWatchedBinCl(binwatches[(~c[0]).toInt()], &c));
            assert(findWatchedBinCl(binwatches[(~c[1]).toInt()], &c));
        }
    }

    for (Clause *const*it = binaryClauses.getData(), *const*end = binaryClauses.getDataEnd(); it != end; it++) {
        const Clause& c = **it;
        assert(c.size() == 2);
        assert(findWatchedBinCl(binwatches[(~c[0]).toInt()], &c));
        assert(findWatchedBinCl(binwatches[(~c[1]).toInt()], &c));
    }

    for (XorClause *const*it = xorclauses.getData(), *const*end = xorclauses.getDataEnd(); it != end; it++) {
        const XorClause& c = **it;
        assert(find(xorwatches[c[0].var()], &c));
        assert(find(xorwatches[c[1].var()], &c));
        if (assigns[c[0].var()]!=l_Undef || assigns[c[1].var()]!=l_Undef) {
            for (uint32_t i = 0; i < c.size();i++) {
                assert(assigns[c[i].var()] != l_Undef);
            }
        }
    }
}

void Solver::findAllAttach() const
{
    for (uint32_t i = 0; i < binwatches.size(); i++) {
        for (uint32_t i2 = 0; i2 < binwatches[i].size(); i2++) {
            assert(findClause(binwatches[i][i2].clause));
        }
    }
    for (uint32_t i = 0; i < watches.size(); i++) {
        for (uint32_t i2 = 0; i2 < watches[i].size(); i2++) {
            assert(findClause(watches[i][i2].clause));
        }
    }

    for (uint32_t i = 0; i < xorwatches.size(); i++) {
        for (uint32_t i2 = 0; i2 < xorwatches[i].size(); i2++) {
            assert(findClause(xorwatches[i][i2]));
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
    for (uint32_t i = 0; i < binaryClauses.size(); i++) {
        if (binaryClauses[i] == c) return true;
    }
    for (uint32_t i = 0; i < clauses.size(); i++) {
        if (clauses[i] == c) return true;
    }
    for (uint32_t i = 0; i < learnts.size(); i++) {
        if (learnts[i] == c) return true;
    }
    vec<Clause*> cs = varReplacer->getClauses();
    for (uint32_t i = 0; i < cs.size(); i++) {
        if (cs[i] == c) return true;
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

const bool Solver::verifyXorClauses(const vec<XorClause*>& cs) const
{
    #ifdef VERBOSE_DEBUG
    cout << "Checking xor-clauses whether they have been properly satisfied." << endl;;
    #endif

    bool verificationOK = true;

    for (uint32_t i = 0; i != xorclauses.size(); i++) {
        XorClause& c = *xorclauses[i];
        bool final = c.xorEqualFalse();

        #ifdef VERBOSE_DEBUG
        printXorClause(*cs[i], c.xorEqualFalse());
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
    verificationOK &= verifyClauses(binaryClauses);
    verificationOK &= verifyXorClauses(xorclauses);

    if (verbosity >=1 && verificationOK)
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
