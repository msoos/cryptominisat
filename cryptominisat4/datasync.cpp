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

using namespace CMSat;

DataSync::DataSync(Solver* _solver, SharedData* _sharedData, uint32_t _thread_num) :
    solver(_solver)
    , sharedData(_sharedData)
    , thread_num(_thread_num)
    , seen(solver->seen)
    , toClear(solver->toClear)
{}

void DataSync::new_var(const bool bva)
{
    if (!enabled())
        return;

    if (!bva) {
        syncFinish.push_back(0);
        syncFinish.push_back(0);
    }
    assert(solver->nVarsOutside()*2 == syncFinish.size());
}

void DataSync::new_vars(size_t n)
{
    if (!enabled())
        return;

    syncFinish.resize(syncFinish.size() + 2*n, 0);
    assert(solver->nVarsOutside()*2 == syncFinish.size());
}

void DataSync::saveVarMem()
{
}

void DataSync::rebuild_bva_map()
{
    if (!enabled())
        return;

    outer_to_without_bva_map = solver->build_outer_to_without_bva_map();
}

void DataSync::updateVars(
    const vector<uint32_t>& outerToInter
    , const vector<uint32_t>& interToOuter
) {
}

bool DataSync::syncData()
{
    if (!enabled()
        || lastSyncConf + solver->conf.sync_every_confl >= solver->sumConflicts()
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
    extend_bins_if_needed();
    clear_set_binary_values();
    ok = shareBinData();
    sharedData->bin_mutex.unlock();
    if (!ok) return false;

    lastSyncConf = solver->sumConflicts();

    return true;
}

void DataSync::clear_set_binary_values()
{
    for(size_t i = 0; i < solver->nVarsOutside()*2; i++) {
        Lit lit1 = Lit::toLit(i);
        lit1 = solver->map_to_with_bva(lit1);
        lit1 = solver->varReplacer->getLitReplacedWithOuter(lit1);
        lit1 = solver->map_outer_to_inter(lit1);
        if (solver->value(lit1) != l_Undef) {
            sharedData->bins[i].clear();
        }
    }
}

void DataSync::extend_bins_if_needed()
{
    assert(sharedData->bins.size() <= (solver->nVarsOutside())*2);
    if (sharedData->bins.size() == (solver->nVarsOutside())*2)
        return;

    sharedData->bins.resize(solver->nVarsOutside()*2);
}

bool DataSync::shareBinData()
{
    uint32_t oldRecvBinData = stats.recvBinData;
    uint32_t oldSentBinData = stats.sentBinData;

    syncBinFromOthers();
    syncBinToOthers();
    size_t mem = sharedData->calc_memory_use_bins();

    if (solver->conf.verbosity >= 2) {
        cout
        << "c [sync] got bins " << (stats.recvBinData - oldRecvBinData)
        << " sent bins " << (stats.sentBinData - oldSentBinData)
        << " mem use: " << mem/(1024*1024) << " M"
        << endl;
    }

    return true;
}

bool DataSync::syncBinFromOthers()
{
    for (uint32_t wsLit = 0; wsLit < sharedData->bins.size(); wsLit++) {
        if (sharedData->bins[wsLit].data == NULL) {
            continue;
        }

        Lit lit1 = Lit::toLit(wsLit);
        lit1 = solver->map_to_with_bva(lit1);
        lit1 = solver->varReplacer->getLitReplacedWithOuter(lit1);
        lit1 = solver->map_outer_to_inter(lit1);
        if (solver->varData[lit1.var()].removed != Removed::none
            || solver->value(lit1.var()) != l_Undef
        ) {
            continue;
        }

        vector<Lit>& bins = *sharedData->bins[wsLit].data;
        watch_subarray ws = solver->watches[lit1.toInt()];

        assert(syncFinish.size() > wsLit);
        if (bins.size() > syncFinish[wsLit]
            && !syncBinFromOthers(lit1, bins, syncFinish[wsLit], ws)
        ) {
            return false;
        }
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
        otherLit = solver->map_to_with_bva(otherLit);
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
        addOneBinToOthers(bin.first, bin.second);
    }

    newBinClauses.clear();
}

void DataSync::addOneBinToOthers(Lit lit1, Lit lit2)
{
    assert(lit1 < lit2);
    if (sharedData->bins[lit1.toInt()].data == NULL) {
        return;
    }

    vector<Lit>& bins = *sharedData->bins[lit1.toInt()].data;
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
        thisLit = solver->map_to_with_bva(thisLit);
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

void DataSync::signalNewBinClause(Lit lit1, Lit lit2)
{
    if (!enabled()) {
        return;
    }

    if (solver->varData[lit1.var()].is_bva)
        return;
    if (solver->varData[lit2.var()].is_bva)
        return;

    lit1 = solver->map_inter_to_outer(lit1);
    lit1 = map_outside_without_bva(lit1);
    lit2 = solver->map_inter_to_outer(lit2);
    lit2 = map_outside_without_bva(lit2);

    if (lit1.toInt() > lit2.toInt()) {
        std::swap(lit1, lit2);
    }
    newBinClauses.push_back(std::make_pair(lit1, lit2));
}
