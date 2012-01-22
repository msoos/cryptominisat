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

#include "ImplCache.h"

#ifndef __TRANSCACHE_H__
#define __TRANSCACHE_H__

#include "ThreadControl.h"
#include "CommandControl.h"
#include "VarReplacer.h"
#include "time_mem.h"
using std::cout;
using std::endl;

void ImplCache::clean(ThreadControl* control)
{
    assert(control->ok);
    assert(control->decisionLevel() == 0);

    double myTime = cpuTime();
    uint64_t numUpdated = 0;
    uint64_t numCleaned = 0;
    uint64_t numFreed = 0;

    vector<uint16_t>& inside = control->seen;
    vector<uint16_t>& nonLearnt = control->seen2;

    //Free memory if possible
    for (Var var = 0; var < control->nVars(); var++) {
        if (control->value(var) != l_Undef
            || (control->varData[var].elimed != ELIMED_NONE && control->varData[var].elimed != ELIMED_QUEUED_VARREPLACER)
        ) {
            vector<LitExtra> tmp1;
            numFreed += implCache[Lit(var, false).toInt()].lits.capacity();
            implCache[Lit(var, false).toInt()].lits.swap(tmp1);

            vector<LitExtra> tmp2;
            numFreed += implCache[Lit(var, true).toInt()].lits.capacity();
            implCache[Lit(var, true).toInt()].lits.swap(tmp2);
        }
    }

    uint32_t wsLit = 0;
    const vector<Lit>& replaceTable = control->varReplacer->getReplaceTable();
    for(vector<TransCache>::iterator trans = implCache.begin(), transEnd = implCache.end(); trans != transEnd; trans++, wsLit++) {
        //Stats
        size_t origSize = trans->lits.size();
        size_t newSize = 0;

        //Update to replaced vars, remove vars already set or eliminated
        Lit vertLit = ~Lit::toLit(wsLit);
        vector<LitExtra>::iterator it = trans->lits.begin();
        vector<LitExtra>::iterator it2 = it;
        for (vector<LitExtra>::iterator end = trans->lits.end(); it != end; it++) {
            Lit lit = it->getLit();
            assert(lit.var() != vertLit.var());

            //Remove if value is set
            if (control->value(lit.var()) != l_Undef)
                continue;

            //Update to its replaced version
            if (control->varData[lit.var()].elimed == ELIMED_VARREPLACER
                || control->varData[lit.var()].elimed == ELIMED_QUEUED_VARREPLACER
            ) {
                lit = replaceTable[lit.var()] ^ lit.sign();

                //This would be tautological (and incorrect), so skip
                if (lit == vertLit)
                    continue;
                numUpdated++;
            }

            //If updated version is eliminated, skip
            if (control->varData[lit.var()].elimed != ELIMED_NONE)
                continue;

            //If we have already visited this var, just skip over, but update nonLearnt
            if (inside[lit.toInt()]) {
                nonLearnt[lit.toInt()] |= (char)it2->getOnlyNLBin();
                continue;
            }

            inside[lit.toInt()] = true;
            nonLearnt[lit.toInt()] |= (char)it->getOnlyNLBin();
            *it2++ = LitExtra(lit, it->getOnlyNLBin());
            newSize++;
        }
        trans->lits.resize(newSize);

        //Now that we have gone through the list, go through once more to:
        //1) set nonLearnt right (above we might have it set later)
        //2) clear 'inside'
        //3) clear 'nonLearnt'
        for (vector<LitExtra>::iterator it = trans->lits.begin(), end = trans->lits.end(); it != end; it++) {
            Lit lit = it->getLit();

            //Clear 'inside'
            inside[lit.toInt()] = false;

            //Clear 'nonLearnt'
            const bool nLearnt = nonLearnt[lit.toInt()];
            nonLearnt[lit.toInt()] = false;

            //Set non-leartness correctly
            LitExtra(lit, nLearnt);
        }
        numCleaned += origSize-trans->lits.size();
    }

    if (control->conf.verbosity >= 1) {
        cout << "c Cache cleaned."
        << " Updated: " << std::setw(7) << numUpdated/1000 << " K"
        << " Cleaned: " << std::setw(7) << numCleaned/1000 << " K"
        << " Freed: " << std::setw(7) << numFreed/1000 << " K"
        << " T: " << std::setprecision(2) << std::fixed  << (cpuTime()-myTime) << endl;
    }
}

bool ImplCache::tryBoth(ThreadControl* control)
{
    assert(control->ok);
    assert(control->decisionLevel() == 0);

    //Setup
    vector<uint16_t>& seen = control->seen;
    vector<uint16_t>& val = control->seen2;
    vector<Lit> tmp;
    uint32_t bProp = 0;
    uint32_t bXProp = 0;

    //Measuring time & usefulness
    double myTime = cpuTime();
    uint32_t backupTrailSize = control->trail.size();

    for (Var var = 0; var < control->nVars(); var++) {
        //If value is set of eliminated, skip
        if (control->value(var) != l_Undef
            || (control->varData[var].elimed != ELIMED_NONE && control->varData[var].elimed != ELIMED_QUEUED_VARREPLACER))
            continue;

        Lit lit = Lit(var, false);

        //To have the least effort, chache1 and cache2 can be interchanged
        vector<LitExtra> const* cache1;
        vector<LitExtra> const* cache2;
        bool startWithTrue;
        if (implCache[lit.toInt()].lits.size() < implCache[(~lit).toInt()].lits.size()) {
            cache1 = &implCache[lit.toInt()].lits;
            cache2 = &implCache[(~lit).toInt()].lits;
            startWithTrue = false;
        } else {
            cache1 = &implCache[(~lit).toInt()].lits;
            cache2 = &implCache[lit.toInt()].lits;
            startWithTrue = true;
        }

        //If the smaller one is empty, there is nothing to do
        if (cache1->empty())
            continue;

        //Fill 'seen' and 'val' with smaller one
        for (vector<LitExtra>::const_iterator it = cache1->begin(), end = cache1->end(); it != end; it++) {
            seen[it->getLit().var()] = true;
            val[it->getLit().var()] = it->getLit().sign();
        }

        for (vector<LitExtra>::const_iterator it = cache2->begin(), end = cache2->end(); it != end; it++) {
            assert(it->getLit().var() != var);
            const Var var2 = it->getLit().var();

            //Only if the other one (cache1) also contained it
            if (!seen[var2])
                continue;

            //If var has been elimed, skip
            if (control->varData[var2].elimed != ELIMED_NONE
                && control->varData[var2].elimed != ELIMED_QUEUED_VARREPLACER
            ) continue;

            if  (val[var2] == it->getLit().sign()) {
                tmp.clear();
                tmp.push_back(it->getLit());
                control->addClauseInt(tmp, true);
                if  (!control->ok) goto end;
                bProp++;
            } else {
                tmp.clear();
                tmp.push_back(Lit(var, false));
                tmp.push_back(Lit(var2, false));
                bool sign = startWithTrue ^ it->getLit().sign();
                control->addXorClauseInt(tmp, sign);
                if  (!control->ok) goto end;
                bXProp++;
            }
        }

        //Clear 'seen' and 'val' from smaller one
        for (vector<LitExtra>::const_iterator it = cache1->begin(), end = cache1->end(); it != end; it++) {
            seen[it->getLit().var()] = false;
            val[it->getLit().var()] = false;
        }
    }

    end:
    if (control->conf.verbosity >= 1) {
        cout << "c Cache " <<
        " BProp: " << bProp <<
        " Set: " << (control->trail.size() - backupTrailSize) <<
        " BXProp: " << bXProp <<
        " T: " << (cpuTime() - myTime) <<
        endl;
    }

    return control->ok;
}

#endif //__TRANSCACHE_H__
