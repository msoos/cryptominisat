/******************************************
Copyright (c) 2010, Mate Soos

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
***********************************************/

#include "DataSync.h"
#include "Subsumer.h"
#include "VarReplacer.h"
#include "XorSubsumer.h"
#include "PartHandler.h"
#include <iomanip>
#include "omp.h"

#ifdef VERBOSE_DEBUG
#define VERBOSE_DEBUG_MPI_SENDRCV
#endif

//#define VERBOSE_DEBUG_MPI_SENDRCV

DataSync::DataSync(Solver& _solver, SharedData* _sharedData) :
    sentUnitData(0)
    , recvUnitData(0)
    , sentBinData(0)
    , recvBinData(0)
    , sentTriData(0)
    , recvTriData(0)
    #ifdef USE_MPI
    , mpiRecvUnitData(0)
    , mpiRecvBinData(0)
    , mpiSentBinData(0)
    , mpiRecvTriData(0)
    , mpiSentTriData(0)
    #endif
    , sharedData(_sharedData)
    , solver(_solver)
    , numCalls(0)
    , lastSyncConf(0)
{
    #ifdef USE_MPI
    int err;
    err = MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    assert(err == MPI_SUCCESS);

    err = MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
    assert(err == MPI_SUCCESS);
    release_assert(!(mpiSize > 1 && mpiRank == 0));
    assert(!(mpiSize > 1 && sharedData == NULL));

    assert(sizeof(unsigned) == sizeof(uint32_t));
    #endif
}

void DataSync::newVar()
{
    #ifdef USE_MPI
    syncMPIFinish.push(0);
    syncMPIFinish.push(0);
    syncMPIFinishTri.push(0);
    syncMPIFinishTri.push(0);
    #endif

    syncFinish.push(0);
    syncFinish.push(0);
    syncFinishTri.push(0);
    syncFinishTri.push(0);
    seen.push(false);
    seen.push(false);
}

const bool DataSync::syncData()
{
    #ifdef USE_MPI
    if (mpiSize == 1 && solver.numThreads == 1) return true;
    #else
    if (solver.numThreads == 1) return true;
    #endif

    if (sharedData == NULL
        || lastSyncConf + solver.conf.syncEveryConf >= solver.conflicts) return true;
    numCalls++;

    assert(sharedData != NULL);
    assert(solver.decisionLevel() == 0);

    bool ok;
    #pragma omp critical (ERSync)
    {
        if (solver.conf.doER) {
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

    #ifdef USE_MPI
    if (mpiSize > 1 && solver.threadNum == 0) {
        #pragma omp critical (unitData)
        {
        #pragma omp critical (binData)
        {
        #pragma omp critical (triData)
        {
            ok = syncFromMPI();
            if (ok && numCalls % 2 == 1) syncToMPI();
        }}}
        if (!ok) return false;
    }

    getNeedToInterruptFromMPI();
    #endif //USE_MPI

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

#ifdef USE_MPI
void DataSync::getNeedToInterruptFromMPI()
{
    int flag;
    MPI_Status status;
    int err = MPI_Iprobe(0, 1, MPI_COMM_WORLD, &flag, &status);
    assert(err == MPI_SUCCESS);
    if (flag == false) return;

    char* buf = NULL;
    err = MPI_Recv((unsigned*)buf, 0, MPI_UNSIGNED, 0, 1, MPI_COMM_WORLD, &status);
    assert(err == MPI_SUCCESS);
    solver.setNeedToInterrupt();
}

const bool DataSync::syncFromMPI()
{
    int err;
    MPI_Status status;
    int flag;
    int count;
    uint32_t tmp = 0;

    uint32_t thisMpiRecvUnitData = 0;
    uint32_t thisMpiRecvBinData = 0;
    uint32_t thisMpiRecvTriData = 0;

    err = MPI_Iprobe(0, 0, MPI_COMM_WORLD, &flag, &status);
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

    int at = 0;
    assert(solver.nVars() == buf[at]);
    at++;
    for (Var var = 0; var < solver.nVars(); var++, at++) {
        const lbool otherVal = toLbool(buf[at]);
        if (!syncUnit(otherVal, var, NULL, thisMpiRecvUnitData, tmp)) {
            #ifdef VERBOSE_DEBUG_MPI_SENDRCV
            std::cout << "-->> MPI " << mpiRank << " solver FALSE" << std::endl;
            #endif
            goto end;
        }
    }
    solver.ok = solver.propagate<true>().isNULL();
    if (!solver.ok) goto end;
    mpiRecvUnitData += thisMpiRecvUnitData;

    assert(buf[at] == solver.nVars()*2);
    at++;
    for (uint32_t wsLit = 0; wsLit < solver.nVars()*2; wsLit++) {
        Lit lit = ~Lit::toLit(wsLit);
        uint32_t num = buf[at];
        at++;
        for (uint32_t i = 0; i < num; i++, at++) {
            Lit otherLit = Lit::toLit(buf[at]);
            addOneBinToOthers(lit, otherLit, true);
            thisMpiRecvBinData++;
        }
    }
    mpiRecvBinData += thisMpiRecvBinData;

    assert(buf[at] == solver.nVars()*2);
    at++;
    for (uint32_t wsLit = 0; wsLit < solver.nVars()*2; wsLit++) {
        Lit lit = ~Lit::toLit(wsLit);
        uint32_t num = buf[at];
        at++;
        for (uint32_t i = 0; i < num; i++, at++) {
            Lit otherLit = Lit::toLit(buf[at]);
            at++;
            Lit otherLit2 = Lit::toLit(buf[at]);
            addOneTriToOthers(lit, otherLit, otherLit2);
            thisMpiRecvTriData++;
        }
    }
    mpiRecvTriData += thisMpiRecvTriData;

    end:
    #ifdef VERBOSE_DEBUG_MPI_SENDRCV
    std::cout << "-->> MPI " << mpiRank << " Received " << thisMpiRecvUnitData << " units" << std::endl;
    std::cout << "-->> MPI " << mpiRank << " Received " << thisMpiRecvBinData << " bins" << std::endl;
    std::cout << "-->> MPI " << mpiRank << " Received " << thisMpiRecvTriData << " tris" << std::endl;
    #endif

    delete[] buf;
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

    vector<uint32_t> data;
    data.push_back((uint32_t)solver.nVars());
    for (Var var = 0; var < solver.nVars(); var++) {
        data.push_back((uint32_t)solver.value(var).getchar());
    }

    uint32_t thisMpiSentBinData = 0;
    data.push_back((uint32_t)solver.nVars()*2);
    uint32_t wsLit = 0;
    for(vector<vector<BinClause> >::const_iterator it = sharedData->bins.begin()
        , end = sharedData->bins.end(); it != end; it++, wsLit++
    ) {
        Lit lit1 = ~Lit::toLit(wsLit);
        const vector<BinClause>& binSet = *it;
        assert(binSet.size() >= syncMPIFinish[wsLit]);
        uint32_t sizeToSend = binSet.size() - syncMPIFinish[wsLit];
        data.push_back(sizeToSend);
        for (uint32_t i = syncMPIFinish[wsLit]; i < binSet.size(); i++) {
            assert(lit1 == binSet[i].lit1);
            assert(lit1 < binSet[i].lit2);
            data.push_back(binSet[i].lit2.toInt());
            thisMpiSentBinData++;
        }
        syncMPIFinish[wsLit] = binSet.size();
    }
    assert(wsLit == solver.nVars()*2);
    mpiSentBinData += thisMpiSentBinData;

    uint32_t thisMpiSentTriData = 0;
    data.push_back((uint32_t)solver.nVars()*2);
    wsLit = 0;
    for(vector<vector<TriClause> >::const_iterator it = sharedData->tris.begin()
        , end = sharedData->tris.end(); it != end; it++, wsLit++
    ) {
        Lit lit1 = ~Lit::toLit(wsLit);
        const vector<TriClause>& triSet = *it;
        assert(triSet.size() >= syncMPIFinishTri[wsLit]);
        uint32_t sizeToSend = triSet.size() - syncMPIFinishTri[wsLit];
        data.push_back(sizeToSend);
        for (uint32_t i = syncMPIFinishTri[wsLit]; i < triSet.size(); i++) {
            assert(triSet[i].lit1 == lit1);
            assert(lit1 < triSet[i].lit2);
            assert(triSet[i].lit2 < triSet[i].lit3);

            data.push_back(triSet[i].lit2.toInt());
            data.push_back(triSet[i].lit3.toInt());
            thisMpiSentTriData++;
        }
        syncMPIFinishTri[wsLit] = triSet.size();
    }
    assert(wsLit == solver.nVars()*2);
    mpiSentTriData += thisMpiSentTriData;

    #ifdef VERBOSE_DEBUG_MPI_SENDRCV
    std::cout << "-->> MPI " << mpiRank << " Sent " << data.size() << " uint32_t -s" << std::endl;
    std::cout << "-->> MPI " << mpiRank << " Sent " << thisMpiSentBinData << " bins " << std::endl;
    std::cout << "-->> MPI " << mpiRank << " Sent " << thisMpiSentTriData << " tris " << std::endl;
    #endif

    mpiSendData = new uint32_t[data.size()];
    std::copy(data.begin(), data.end(), mpiSendData);
    err = MPI_Isend(mpiSendData, data.size(), MPI_UNSIGNED, 0, 0, MPI_COMM_WORLD, &sendReq);
    assert(err == MPI_SUCCESS);
}
#endif //USE_MPI

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
        assert(tris[i].lit1 < tris[i].lit2);
        assert(tris[i].lit2 < tris[i].lit3);

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

                if (it->getOtherLit2() == lit1
                    && it->getOtherLit() == lit3) alreadyInside = true;
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
    assert(lit1 < lit2);
    assert(lit2 < lit3);
    assert(lit1.var() != lit2.var());
    assert(lit2.var() != lit3.var());

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
        solver.ok = solver.propagate<true>().isNULL();
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
