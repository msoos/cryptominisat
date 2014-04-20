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

#include "datasync.h"
#include "varreplacer.h"
#include "solver.h"
#include "shareddata.h"
#include <iomanip>

#define SYNC_EVERY_CONFL 6000

using namespace CMSat;

DataSync::DataSync(Solver* _solver, SharedData* _sharedData) :
    solver(_solver)
    , sharedData(_sharedData)
    , seen(solver->seen)
    , toClear(solver->toClear)
{}

void DataSync::new_var()
{
    syncFinish.push_back(0);
    syncFinish.push_back(0);
}

void DataSync::saveVarMem()
{
}

void DataSync::updateVars(
    const vector<uint32_t>& outerToInter
    , const vector<uint32_t>& interToOuter
) {
    assert(solver->decisionLevel() == 0);
    for(std::pair<Lit, Lit>& p : newBinClauses) {
        p.first = getUpdatedLit(p.first, outerToInter);
        p.second = getUpdatedLit(p.second, outerToInter);
    }
}

bool DataSync::syncData()
{
    if (sharedData == NULL
        || lastSyncConf + SYNC_EVERY_CONFL >= solver->sumConflicts()
    ) {
        //cout << "sharedData:" << sharedData << endl;
        //cout << "conf: " << solver->sumConflicts() << endl;
        //cout << "todo: " << lastSyncConf + SYNC_EVERY_CONFL << endl;
        return true;
    }

    assert(sharedData != NULL);
    assert(solver->decisionLevel() == 0);

    bool ok;
    sharedData->unit_mutex.lock();
    ok = shareUnitData();
    sharedData->unit_mutex.unlock();
    if (!ok) return false;

    sharedData->bin_mutex.lock();
    ok = shareBinData();
    sharedData->bin_mutex.unlock();
    if (!ok) return false;

    lastSyncConf = solver->sumConflicts();

    return true;
}

bool DataSync::shareBinData()
{
    uint32_t oldRecvBinData = stats.recvBinData;
    uint32_t oldSentBinData = stats.sentBinData;

    assert(solver->conf.do_bva == 0);
    assert(solver->get_num_bva_vars() == 0);
    if (sharedData->bins.size() < (solver->nVarsOutside())*2) {
        sharedData->bins.resize(solver->nVarsOutside()*2);
    }

    for (uint32_t wsLit = 0; wsLit < sharedData->bins.size(); wsLit++) {
        Lit lit1 = Lit::toLit(wsLit);
        lit1 = solver->varReplacer->getLitReplacedWithOuter(lit1);
        lit1 = solver->map_outer_to_inter(lit1);
        if (solver->varData[lit1.var()].removed != Removed::none
            || solver->value(lit1.var()) != l_Undef
        ) {
            continue;
        }

        vector<Lit>& bins = sharedData->bins[wsLit];
        watch_subarray ws = solver->watches[lit1.toInt()];

        assert(syncFinish.size() > wsLit);
        if (bins.size() > syncFinish[wsLit]
            && !syncBinFromOthers(lit1, bins, syncFinish[wsLit], ws)
        ) {
            return false;
        }
    }

    syncBinToOthers();

    if (solver->conf.verbosity >= 2) {
        cout
        << "c [sync] got bins " << std::setw(10) << (stats.recvBinData - oldRecvBinData)
        << std::setw(10) << " sent bins " << (stats.sentBinData - oldSentBinData)
        << endl;
    }

    return true;
}

bool DataSync::syncBinFromOthers(
    const Lit lit
    , const vector<Lit>& bins
    , uint32_t& finished
    , watch_subarray ws
) {
    assert(solver->varReplacer->getLitReplacedWith(lit) == lit);
    assert(solver->varData[lit.var()].removed == Removed::none);

    assert(toClear.empty());
    for (const Watched& w: ws) {
        if (w.isBinary()) {
            toClear.push_back(w.lit2());
            assert(seen.size() > w.lit2().toInt());
            seen[w.lit2().toInt()] = true;
        }
    }

    vector<Lit> lits(2);
    for (uint32_t i = finished; i < bins.size(); i++) {
        Lit otherLit = bins[i];
        otherLit = solver->varReplacer->getLitReplacedWithOuter(otherLit);
        otherLit = solver->map_outer_to_inter(otherLit);
        if (solver->varData[otherLit.var()].removed != Removed::none
            || solver->value(otherLit) != l_Undef
        ) {
            continue;
        }
        assert(seen.size() > otherLit.toInt());
        if (!seen[otherLit.toInt()]) {
            stats.recvBinData++;
            lits[0] = lit;
            lits[1] = otherLit;

            //Don't add DRUP: it would add to the thread data, too
            solver->addClauseInt(lits, true, ClauseStats(), true, NULL, false);
            if (!solver->ok) {
                goto end;
            }
        }
    }
    finished = bins.size();

    end:
    for (const Lit lit: toClear) {
        seen[lit.toInt()] = false;
    }
    toClear.clear();

    return solver->ok;
}

void DataSync::syncBinToOthers()
{
    for(const std::pair<Lit, Lit>& bin: newBinClauses) {
        const Lit lit1 = solver->map_inter_to_outer(bin.first);
        const Lit lit2 = solver->map_inter_to_outer(bin.second);
        addOneBinToOthers(lit1, lit2);
    }

    newBinClauses.clear();
}

void DataSync::addOneBinToOthers(Lit lit1, Lit lit2)
{
    if (lit1 > lit2) {
        std::swap(lit1, lit2);
    }

    vector<Lit>& bins = sharedData->bins[lit1.toInt()];
    for (const Lit lit : bins) {
        if (lit == lit2)
            return;
    }

    bins.push_back(lit2);
    stats.sentBinData++;
}

bool DataSync::shareUnitData()
{
    uint32_t thisGotUnitData = 0;
    uint32_t thisSentUnitData = 0;

    SharedData& shared = *sharedData;
    if (shared.value.size() < solver->nVarsOutside()) {
        shared.value.resize(solver->nVarsOutside(), l_Undef);
    }
    for (uint32_t var = 0; var < solver->nVarsOutside(); var++) {
        Lit thisLit = Lit(var, false);
        thisLit = solver->varReplacer->getLitReplacedWithOuter(thisLit);
        thisLit = solver->map_outer_to_inter(thisLit);
        const lbool thisVal = solver->value(thisLit);
        const lbool otherVal = shared.value[var];

        if (thisVal == l_Undef && otherVal == l_Undef)
            continue;

        if (thisVal != l_Undef && otherVal != l_Undef) {
            if (thisVal != otherVal) {
                solver->ok = false;
                return false;
            } else {
                continue;
            }
        }

        if (otherVal != l_Undef) {
            assert(thisVal == l_Undef);
            Lit litToEnqueue = thisLit ^ (otherVal == l_False);
            if (solver->varData[litToEnqueue.var()].removed != Removed::none) {
                continue;
            }

            solver->enqueue(litToEnqueue);
            solver->ok = solver->propagate().isNULL();
            if (!solver->ok)
                return false;

            thisGotUnitData++;
            continue;
        }

        if (thisVal != l_Undef) {
            assert(otherVal == l_Undef);
            shared.value[var] = thisVal;
            thisSentUnitData++;
            continue;
        }
    }

    if (solver->conf.verbosity >= 2
        //&& (thisGotUnitData > 0 || thisSentUnitData > 0)
    ) {
        cout
        << "c [sync] got units " << thisGotUnitData
        << " sent units " << thisSentUnitData
        << endl;
    }

    stats.recvUnitData += thisGotUnitData;
    stats.sentUnitData += thisSentUnitData;

    return true;
}
