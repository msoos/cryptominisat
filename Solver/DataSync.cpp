/*****************************************************************************
CryptoMiniSat -- Copyright (c) 2010 Mate Soos

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
******************************************************************************/

#include "DataSync.h"
#include "Subsumer.h"
#include "VarReplacer.h"
#include "XorSubsumer.h"
#include "PartHandler.h"
#include <iomanip>
#include "omp.h"

DataSync::DataSync(Solver& _solver, SharedData* _sharedData) :
    sentUnitData(0)
    , recvUnitData(0)
    , sentBinData(0)
    , recvBinData(0)
    , sentTriData(0)
    , recvTriData(0)
    , lastSyncConf(0)
    , numCalls(0)
    , sharedData(_sharedData)
    , solver(_solver)
{
}

void DataSync::newVar()
{
    syncFinish.push(0);
    syncFinish.push(0);
    syncFinishTri.push(0);
    syncFinishTri.push(0);
    seen.push(false);
    seen.push(false);
}

const bool DataSync::syncData()
{
    numCalls++;
    if (solver.numThreads == 1) return true;
    if (sharedData == NULL
        || lastSyncConf + SYNC_EVERY_CONFL >= solver.conflicts) return true;

    assert(sharedData != NULL);
    assert(solver.decisionLevel() == 0);

    bool ok;
    #pragma omp critical (ERSync)
    {
        if (!sharedData->EREnded
            && sharedData->threadAddingVars == solver.threadNum
            && solver.subsumer->getFinishedAddingVars())
            syncERVarsFromHere();

        if (sharedData->EREnded
            && sharedData->threadAddingVars != solver.threadNum
            && sharedData->othersSyncedER.find(solver.threadNum) == sharedData->othersSyncedER.end())
            syncERVarsToHere();

        if (sharedData->othersSyncedER.size() == (uint32_t)(solver.numThreads-1)) {
            sharedData->threadAddingVars = (sharedData->threadAddingVars+1) % solver.numThreads;
            sharedData->EREnded = false;
            sharedData->othersSyncedER.clear();
        }

        #pragma omp critical (unitData)
        ok = shareUnitData();
        if (!ok) goto end;

        #pragma omp critical (binData)
        ok = shareBinData();
        if (!ok) goto end;

        #pragma omp critical (triData)
        ok = shareTriData();
        if (!ok) goto end;

        end:;
    }

    lastSyncConf = solver.conflicts;

    return solver.ok;
}

void DataSync::syncERVarsToHere()
{
    for (uint32_t i = 0; i < sharedData->numNewERVars; i++) {
        Var var = solver.newVar();
        solver.subsumer->setVarNonEliminable(var);
    }
    solver.subsumer->incNumERVars(sharedData->numNewERVars);
    sharedData->othersSyncedER.insert(solver.threadNum);
}

void DataSync::syncERVarsFromHere()
{
    sharedData->EREnded = true;
    sharedData->numNewERVars = solver.subsumer->getNumERVars() - sharedData->lastNewERVars;
    sharedData->lastNewERVars = solver.subsumer->getNumERVars();
    solver.subsumer->setFinishedAddingVars(false);
}

const bool DataSync::shareBinData()
{
    uint32_t oldRecvBinData = recvBinData;
    uint32_t oldSentBinData = sentBinData;

    SharedData& shared = *sharedData;
    if (shared.bins.size() != solver.nVars()*2)
        shared.bins.resize(solver.nVars()*2);

    for (uint32_t wsLit = 0; wsLit < solver.nVars()*2; wsLit++) {
        Lit lit1 = ~Lit::toLit(wsLit);
        lit1 = solver.varReplacer->getReplaceTable()[lit1.var()] ^ lit1.sign();
        if (solver.subsumer->getVarElimed()[lit1.var()]
            || solver.xorSubsumer->getVarElimed()[lit1.var()]
            || solver.value(lit1.var()) != l_Undef
            ) continue;

        vector<BinClause>& bins = shared.bins[wsLit];

        if (bins.size() > syncFinish[wsLit]
            && !syncBinFromOthers(lit1, bins, syncFinish[wsLit])) return false;
    }

    syncBinToOthers();

    if (solver.conf.verbosity >= 3) {
        std::cout << "c got bins " << std::setw(10) << (recvBinData - oldRecvBinData)
        << std::setw(10) << " sent bins " << (sentBinData - oldSentBinData) << std::endl;
    }

    return true;
}

template <class T>
void DataSync::signalNewBinClause(const T& ps, const bool learnt)
{
    assert(ps.size() == 2);
    signalNewBinClause(ps[0], ps[1], learnt);
}
template void DataSync::signalNewBinClause(const Clause& ps, const bool learnt);
template void DataSync::signalNewBinClause(const XorClause& ps, const bool learnt);
template void DataSync::signalNewBinClause(const vec<Lit>& ps, const bool learnt);

void DataSync::signalNewBinClause(Lit lit1, Lit lit2, const bool learnt)
{
    if (lit1.toInt() > lit2.toInt()) std::swap(lit1, lit2);
    newBinClauses.push_back(BinClause(lit1, lit2, learnt));
}

template<class T>
void DataSync::signalNewTriClause(const T& ps, const bool learnt)
{
    assert(ps.size() == 3);
    signalNewTriClause(ps[0], ps[1], ps[2], learnt);
}
template void DataSync::signalNewTriClause(const Clause& ps, const bool learnt);
template void DataSync::signalNewTriClause(const vec<Lit>& ps, const bool learnt);

void DataSync::signalNewTriClause(const Lit lit1, const Lit lit2, const Lit lit3, const bool learnt)
{
    Lit lits[3];
    lits[0] = lit1;
    lits[1] = lit2;
    lits[2] = lit3;
    std::sort(lits + 0, lits + 3);
    newTriClauses.push_back(TriClause(lits[0], lits[1], lits[2], learnt));
}

const bool DataSync::shareTriData()
{
    uint32_t oldRecvTriData = recvTriData;
    uint32_t oldSentTriData = sentTriData;

    SharedData& shared = *sharedData;
    if (shared.tris.size() != solver.nVars()*2)
        shared.tris.resize(solver.nVars()*2);

    for (uint32_t wsLit = 0; wsLit < solver.nVars()*2; wsLit++) {
        Lit lit1 = ~Lit::toLit(wsLit);
        lit1 = solver.varReplacer->getReplaceTable()[lit1.var()] ^ lit1.sign();
        if (solver.subsumer->getVarElimed()[lit1.var()]
            || solver.xorSubsumer->getVarElimed()[lit1.var()]
            || solver.partHandler->getSavedState()[lit1.var()] != l_Undef
            || solver.value(lit1.var()) != l_Undef
            ) continue;

        vector<TriClause>& tris = shared.tris[wsLit];

        if (tris.size() > syncFinishTri[wsLit]
            && !syncTriFromOthers(lit1, tris, syncFinishTri[wsLit])) return false;
    }

    syncTriToOthers();

    if (solver.conf.verbosity >= 3) {
        std::cout << "c got tris " << std::setw(10) << (recvTriData - oldRecvTriData)
        << std::setw(10) << " sent tris " << (sentTriData - oldSentTriData) << std::endl;
    }

    return true;
}

const bool DataSync::syncTriFromOthers(const Lit lit1, const vector<TriClause>& tris, uint32_t& finished)
{
    assert(solver.ok);
    assert(solver.varReplacer->getReplaceTable()[lit1.var()].var() == lit1.var());
    assert(solver.subsumer->getVarElimed()[lit1.var()] == false);
    assert(solver.xorSubsumer->getVarElimed()[lit1.var()] == false);
    assert(solver.partHandler->getSavedState()[lit1.var()] == l_Undef);

    vec<Lit> tmp;
    for (uint32_t i = finished; i < tris.size(); i++) {
        const TriClause& cl = tris[i];

        Lit lit2 = solver.varReplacer->getReplaceTable()[cl.lit2.var()] ^ cl.lit2.sign();
        if (solver.subsumer->getVarElimed()[lit2.var()]
            || solver.xorSubsumer->getVarElimed()[lit2.var()]
            || solver.partHandler->getSavedState()[lit2.var()] != l_Undef
            ) continue;

        Lit lit3 = solver.varReplacer->getReplaceTable()[cl.lit3.var()] ^ cl.lit3.sign();
        if (solver.subsumer->getVarElimed()[lit3.var()]
            || solver.xorSubsumer->getVarElimed()[lit3.var()]
            || solver.partHandler->getSavedState()[lit3.var()] != l_Undef
            ) continue;

        bool alreadyInside = false;
        const vec<Watched>& ws = solver.watches[(~lit1).toInt()];
        for (const Watched *it = ws.getData(), *end = ws.getDataEnd(); it != end; it++) {
            if (it->isTriClause()) {
                if (it->getOtherLit() == lit2
                    && it->getOtherLit2() == lit3) alreadyInside = true;

                if (it->getOtherLit2() == lit2
                    && it->getOtherLit() == lit3) alreadyInside = true;
            }
        }
        if (alreadyInside) continue;

        const vec<Watched>& ws2 = solver.watches[(~lit2).toInt()];
        for (const Watched *it = ws2.getData(), *end = ws2.getDataEnd(); it != end; it++) {
            if (it->isTriClause()) {
                if (it->getOtherLit() == lit1
                    && it->getOtherLit2() == lit3) alreadyInside = true;

                if (it->getOtherLit2() == lit3
                    && it->getOtherLit() == lit1) alreadyInside = true;
            }
        }
        if (alreadyInside) continue;

        tmp.clear();
        tmp.growTo(3);
        tmp[0] = lit1;
        tmp[1] = lit2;
        tmp[2] = lit3;
        Clause* c = solver.addClauseInt(tmp, 0, cl.learnt, 3, true);
        if (c != NULL) solver.learnts.push(c);
        if (!solver.ok) return false;
        recvTriData++;
    }
    finished = tris.size();

    return true;
}

void DataSync::syncTriToOthers()
{
    for(vector<TriClause>::const_iterator it = newTriClauses.begin(), end = newTriClauses.end(); it != end; it++) {
        addOneTriToOthers(it->lit1, it->lit2, it->lit3);
        sentTriData++;
    }

    for (uint32_t i = 0; i < sharedData->tris.size(); i++) {
        syncFinishTri[i] = sharedData->tris[i].size();
    }

    newTriClauses.clear();
}

void DataSync::addOneTriToOthers(const Lit lit1, const Lit lit2, const Lit lit3)
{
    assert(lit1.toInt() < lit2.toInt());
    assert(lit2.toInt() < lit3.toInt());

    vector<TriClause>& tris = sharedData->tris[(~lit1).toInt()];
    for (vector<TriClause>::const_iterator it = tris.begin(), end = tris.end(); it != end; it++) {
        if (it->lit2 == lit2
            && it->lit3 == lit3) return;
    }

    tris.push_back(TriClause(lit1, lit2, lit3));
}

const bool DataSync::syncBinFromOthers(const Lit lit, const vector<BinClause>& bins, uint32_t& finished)
{
    assert(solver.varReplacer->getReplaceTable()[lit.var()].var() == lit.var());
    assert(solver.subsumer->getVarElimed()[lit.var()] == false);
    assert(solver.xorSubsumer->getVarElimed()[lit.var()] == false);
    assert(solver.partHandler->getSavedState()[lit.var()] == l_Undef);

    vec<Lit> addedToSeen;
    const vec<Watched>& ws = solver.watches[(~lit).toInt()];
    for (const Watched *it = ws.getData(), *end = ws.getDataEnd(); it != end; it++) {
        if (it->isBinary()) {
            addedToSeen.push(it->getOtherLit());
            seen[it->getOtherLit().toInt()] = true;
        }
    }
    const vector<Lit>& cache = solver.transOTFCache[(~lit).toInt()].lits;
    for (vector<Lit>::const_iterator it = cache.begin(), end = cache.end(); it != end; it++) {
        addedToSeen.push(*it);
        seen[it->toInt()] = true;
    }

    vec<Lit> lits(2);
    for (uint32_t i = finished; i < bins.size(); i++) {
        if (!seen[bins[i].lit2.toInt()]) {
            Lit otherLit = bins[i].lit2;
            otherLit = solver.varReplacer->getReplaceTable()[otherLit.var()] ^ otherLit.sign();
            if (solver.subsumer->getVarElimed()[otherLit.var()]
                || solver.xorSubsumer->getVarElimed()[otherLit.var()]
                || solver.partHandler->getSavedState()[otherLit.var()] != l_Undef
                || solver.value(otherLit.var()) != l_Undef
                ) continue;

            recvBinData++;
            lits[0] = lit;
            lits[1] = otherLit;
            solver.addClauseInt(lits, 0, bins[i].learnt, 2, true);
            lits.clear();
            lits.growTo(2);
            if (!solver.ok) goto end;
        }
    }
    finished = bins.size();

    end:
    for (uint32_t i = 0; i < addedToSeen.size(); i++)
        seen[addedToSeen[i].toInt()] = false;

    return solver.ok;
}

void DataSync::syncBinToOthers()
{
    for(vector<BinClause>::const_iterator it = newBinClauses.begin(), end = newBinClauses.end(); it != end; it++) {
        addOneBinToOthers(it->lit1, it->lit2, it->learnt);
        sentBinData++;
    }

    for (uint32_t i = 0; i < sharedData->bins.size(); i++) {
        syncFinish[i] = sharedData->bins[i].size();
    }

    newBinClauses.clear();
}

void DataSync::addOneBinToOthers(const Lit lit1, const Lit lit2, const bool learnt)
{
    assert(lit1.toInt() < lit2.toInt());

    vector<BinClause>& bins = sharedData->bins[(~lit1).toInt()];
    for (vector<BinClause>::const_iterator it = bins.begin(), end = bins.end(); it != end; it++) {
        if (it->lit2 == lit2 && it->learnt == learnt) return;
    }

    bins.push_back(BinClause(lit1, lit2, learnt));
}

const bool DataSync::shareUnitData()
{
    uint32_t thisGotUnitData = 0;
    uint32_t thisSentUnitData = 0;

    SharedData& shared = *sharedData;
    shared.value.growTo(solver.nVars(), l_Undef);
    for (uint32_t var = 0; var < solver.nVars(); var++) {
        Lit thisLit = Lit(var, false);
        thisLit = solver.varReplacer->getReplaceTable()[thisLit.var()] ^ thisLit.sign();
        const lbool thisVal = solver.value(thisLit);
        const lbool otherVal = shared.value[var];

        if (thisVal == l_Undef && otherVal == l_Undef) continue;
        if (thisVal != l_Undef && otherVal != l_Undef) {
            if (thisVal != otherVal) {
                solver.ok = false;
                return false;
            } else {
                continue;
            }
        }

        if (otherVal != l_Undef) {
            assert(thisVal == l_Undef);
            Lit litToEnqueue = thisLit ^ (otherVal == l_False);
            if (solver.subsumer->getVarElimed()[litToEnqueue.var()]
                || solver.xorSubsumer->getVarElimed()[litToEnqueue.var()]
                || solver.partHandler->getSavedState()[litToEnqueue.var()] != l_Undef
                ) continue;

            solver.uncheckedEnqueue(litToEnqueue);
            solver.ok = solver.propagate().isNULL();
            if (!solver.ok) return false;
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

    if (solver.conf.verbosity >= 3 && (thisGotUnitData > 0 || thisSentUnitData > 0)) {
        std::cout << "c got units " << std::setw(8) << thisGotUnitData
        << " sent units " << std::setw(8) << thisSentUnitData << std::endl;
    }

    recvUnitData += thisGotUnitData;
    sentUnitData += thisSentUnitData;

    return true;
}