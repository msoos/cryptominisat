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
#include "mpi.h"

#ifdef VERBOSE_DEBUG
#define VERBOSE_DEBUG_MPI_SENDRCV
#endif

//#define VERBOSE_DEBUG_MPI_SENDRCV

DataSync::DataSync(Solver& _solver, SharedData* _sharedData) :
    lastSyncConf(0)
    , mpiSendData(NULL)
    , sentUnitData(0)
    , recvUnitData(0)
    , sentBinData(0)
    , recvBinData(0)
    , mpiRecvBinData(0)
    , mpiSentBinData(0)
    , sharedData(_sharedData)
    , solver(_solver)
    , numCalls(0)
{
    int err;
    err = MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    assert(err == MPI_SUCCESS);

    err = MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
    assert(err == MPI_SUCCESS);
    release_assert(!(mpiSize > 1 && mpiRank == 0));

    assert(sizeof(unsigned) == sizeof(uint32_t));
}

void DataSync::newVar()
{
    syncMPIFinish.push(0);
    syncMPIFinish.push(0);
    syncFinish.push(0);
    syncFinish.push(0);
    seen.push(false);
    seen.push(false);
}

const bool DataSync::syncData()
{
    numCalls++;
    if (mpiSize == 1 && solver.numThreads == 1) return true;
    if (sharedData == NULL
        || lastSyncConf + solver.conf.syncEveryConf >= solver.conflicts) return true;

    assert(sharedData != NULL);
    assert(solver.decisionLevel() == 0);

    bool ok;
    #pragma omp critical (unitData)
    ok = shareUnitData();
    if (!ok) return false;

    #pragma omp critical (binData)
    ok = shareBinData();
    if (!ok) return false;

    if (mpiSize > 1 && solver.threadNum == 0) {
        #pragma omp critical (unitData)
        {
            #pragma omp critical (binData)
            {
            ok = syncFromMPI();
            if (ok && numCalls % 2 == 2) syncToMPI();
            }
        }
        if (!ok) return false;
    }

    lastSyncConf = solver.conflicts;

    return true;
}

const bool DataSync::syncFromMPI()
{
    int err;
    MPI_Status status;
    int flag;
    int count;
    uint32_t thisMpiRecvBinData = 0;
    uint32_t thisGotUnitData = 0;
    uint32_t tmp = 0;

    err = MPI_Iprobe(0, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);
    assert(err == MPI_SUCCESS);
    if (flag == false) return true;

    err = MPI_Get_count(&status, MPI_UNSIGNED, &count);
    assert(err == MPI_SUCCESS);
    #ifdef VERBOSE_DEBUG_MPI_SENDRCV
    std::cout << "-->> MPI " << mpiRank << " Received " << count << " uint32_t-s" << std::endl;
    #endif

    uint32_t* buf = new uint32_t[count];
    err = MPI_Recv((unsigned*)buf, count, MPI_UNSIGNED, 0, 0, MPI_COMM_WORLD, &status);
    assert(err == MPI_SUCCESS);

    uint32_t at = 0;
    assert(solver.nVars() == buf[at]);
    at++;
    for (Var var = 0; var < solver.nVars(); var++, at++) {
        const lbool otherVal = toLbool(buf[at]);
        if (!syncUnit(otherVal, var, NULL, thisGotUnitData, tmp)) {
            #ifdef VERBOSE_DEBUG_MPI_SENDRCV
            std::cout << "-->> MPI " << mpiRank << " solver FALSE" << std::endl;
            #endif
            goto end;
        }
    }
    solver.ok = solver.propagate().isNULL();
    if (!solver.ok) goto end;

    assert(buf[at] == solver.nVars()*2);
    at++;
    for (uint32_t wsLit = 0; wsLit < solver.nVars()*2; wsLit++) {
        Lit lit = ~Lit::toLit(wsLit);
        uint32_t num = buf[at];
        at++;
        for (uint32_t i = 0; i < num; i++, at++) {
            Lit otherLit = Lit::toLit(buf[at]);
            addOneBinToOthers(lit, otherLit);
            thisMpiRecvBinData++;
        }
    }
    mpiRecvBinData += thisMpiRecvBinData;

    #ifdef VERBOSE_DEBUG_MPI_SENDRCV
    std::cout << "-->> MPI " << mpiRank << " Received " << thisMpiRecvBinData << " bins" << std::endl;
    #endif

    end:
    delete buf;
    return solver.ok;
}

void DataSync::syncToMPI()
{
    int err;
    if (mpiSendData != NULL) {
        MPI_Status status;
        err = MPI_Wait(&sendReq, &status);
        assert(err == MPI_SUCCESS);
        delete mpiSendData;
        mpiSendData = NULL;
    }

    uint32_t thisMpiSentBinData = 0;
    vector<uint32_t> data;
    data.push_back((uint32_t)solver.nVars());
    for (Var var = 0; var < solver.nVars(); var++) {
        data.push_back((uint32_t)solver.value(var).getchar());
    }

    data.push_back((uint32_t)solver.nVars()*2);
    uint32_t at = 0;
    for(vector<vector<Lit> >::const_iterator it = sharedData->bins.begin(), end = sharedData->bins.end(); it != end; it++, at++) {
        const vector<Lit>& binSet = *it;
        uint32_t sizeToSend = binSet.size() - syncMPIFinish[at];
        data.push_back(sizeToSend);
        for (uint32_t i = syncMPIFinish[at]; i < binSet.size(); i++) {
            data.push_back(binSet[i].toInt());
            thisMpiSentBinData++;
        }
        syncMPIFinish[at] = binSet.size();
    }
    mpiSentBinData += thisMpiSentBinData;

    #ifdef VERBOSE_DEBUG_MPI_SENDRCV
    std::cout << "-->> MPI " << mpiRank << " Sent " << data.size() << " uint32_t -s" << std::endl;
    std::cout << "-->> MPI " << mpiRank << " Sent " << thisMpiSentBinData << " bins " << std::endl;
    #endif

    mpiSendData = new uint32_t[data.size()];
    std::copy(data.begin(), data.end(), mpiSendData);
    err = MPI_Isend(mpiSendData, data.size(), MPI_UNSIGNED, 0, 0, MPI_COMM_WORLD, &sendReq);
    assert(err == MPI_SUCCESS);
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

        vector<Lit>& bins = shared.bins[wsLit];

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
void DataSync::signalNewBinClause(T& ps)
{
    assert(ps.size() == 2);
    signalNewBinClause(ps[0], ps[1]);
}

void DataSync::signalNewBinClause(Lit lit1, Lit lit2)
{
    if (lit1.toInt() > lit2.toInt()) std::swap(lit1, lit2);
    newBinClauses.push_back(std::make_pair(lit1, lit2));
}

template void DataSync::signalNewBinClause(Clause& ps);
template void DataSync::signalNewBinClause(XorClause& ps);
template void DataSync::signalNewBinClause(vec<Lit>& ps);

const bool DataSync::syncBinFromOthers(const Lit lit, const vector<Lit>& bins, uint32_t& finished)
{
    assert(solver.varReplacer->getReplaceTable()[lit.var()].var() == lit.var());
    assert(solver.subsumer->getVarElimed()[lit.var()] == false);
    assert(solver.xorSubsumer->getVarElimed()[lit.var()] == false);

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
        if (!seen[bins[i].toInt()]) {
            Lit otherLit = bins[i];
            otherLit = solver.varReplacer->getReplaceTable()[otherLit.var()] ^ otherLit.sign();
            if (solver.subsumer->getVarElimed()[otherLit.var()]
                || solver.xorSubsumer->getVarElimed()[otherLit.var()]
                || solver.partHandler->getSavedState()[otherLit.var()] != l_Undef
                || solver.value(otherLit.var()) != l_Undef
                ) continue;

            recvBinData++;
            lits[0] = lit;
            lits[1] = otherLit;
            solver.addClauseInt(lits, 0, true, 2, 0, true);
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
    for(vector<std::pair<Lit, Lit> >::const_iterator it = newBinClauses.begin(), end = newBinClauses.end(); it != end; it++) {
        addOneBinToOthers(it->first, it->second);
        sentBinData++;
    }

    for (uint32_t i = 0; i < sharedData->bins.size(); i++) {
        syncFinish[i] = sharedData->bins[i].size();
    }

    newBinClauses.clear();
}

void DataSync::addOneBinToOthers(const Lit lit1, const Lit lit2)
{
    assert(lit1.toInt() < lit2.toInt());

    vector<Lit>& bins = sharedData->bins[(~lit1).toInt()];
    for (vector<Lit>::const_iterator it = bins.begin(), end = bins.end(); it != end; it++) {
        if (*it == lit2) return;
    }

    bins.push_back(lit2);
}

const bool DataSync::shareUnitData()
{
    uint32_t thisGotUnitData = 0;
    uint32_t thisSentUnitData = 0;

    SharedData& shared = *sharedData;
    shared.value.growTo(solver.nVars(), l_Undef);
    for (uint32_t var = 0; var < solver.nVars(); var++) {
        const lbool otherVal = shared.value[var];
        if (!syncUnit(otherVal, var, sharedData, thisGotUnitData, thisSentUnitData)) return false;
    }

    if (solver.conf.verbosity >= 3 && (thisGotUnitData > 0 || thisSentUnitData > 0)) {
        std::cout << "c got units " << std::setw(8) << thisGotUnitData
        << " sent units " << std::setw(8) << thisSentUnitData << std::endl;
    }

    recvUnitData += thisGotUnitData;
    sentUnitData += thisSentUnitData;

    return true;
}

const bool DataSync::syncUnit(const lbool otherVal, const Var var, SharedData* shared, uint32_t& thisGotUnitData, uint32_t& thisSentUnitData)
{
    Lit thisLit = Lit(var, false);
    thisLit = solver.varReplacer->getReplaceTable()[thisLit.var()] ^ thisLit.sign();
    const lbool thisVal = solver.value(thisLit);

    if (thisVal == l_Undef && otherVal == l_Undef) return true;
    if (thisVal != l_Undef && otherVal != l_Undef) {
        if (thisVal != otherVal) {
            solver.ok = false;
            return false;
        } else {
            return true;
        }
    }

    if (otherVal != l_Undef) {
        assert(thisVal == l_Undef);
        Lit litToEnqueue = thisLit ^ (otherVal == l_False);
        if (solver.subsumer->getVarElimed()[litToEnqueue.var()]
            || solver.xorSubsumer->getVarElimed()[litToEnqueue.var()]
            || solver.partHandler->getSavedState()[litToEnqueue.var()] != l_Undef
            ) return true;

        solver.uncheckedEnqueue(litToEnqueue);
        solver.ok = solver.propagate().isNULL();
        if (!solver.ok) return false;
        thisGotUnitData++;
        return true;
    }

    if (shared != NULL && thisVal != l_Undef) {
        assert(otherVal == l_Undef);
        shared->value[var] = thisVal;
        thisSentUnitData++;
        return true;
    }

    return true;
}