/******************************************
Copyright (c) 2016, Mate Soos

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

#include "datasync.h"
#include "varreplacer.h"
#include "solver.h"
#include "shareddata.h"
#include <iomanip>

using namespace CMSat;

DataSync::DataSync(Solver* _solver, SharedData* _sharedData, bool _is_mpi) :
    solver(_solver)
    , sharedData(_sharedData)
    , is_mpi(_is_mpi)
    , seen(solver->seen)
    , toClear(solver->toClear)
{
#ifdef USE_MPI
    if (is_mpi) {
        int err;
        err = MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
        assert(err == MPI_SUCCESS);

        err = MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
        assert(err == MPI_SUCCESS);
        release_assert(!(mpiSize > 1 && mpiRank == 0));
        assert(!(mpiSize > 1 && sharedData == NULL));
    }
#endif
}

void DataSync::set_shared_data(SharedData* _sharedData)
{
    sharedData = _sharedData;
}

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

    syncFinish.insert(syncFinish.end(), 2*n, 0);
    assert(solver->nVarsOutside()*2 == syncFinish.size());
}

void DataSync::save_on_var_memory()
{
}

void DataSync::rebuild_bva_map()
{
    must_rebuild_bva_map = true;
}

void DataSync::updateVars(
    const vector<uint32_t>& /*outerToInter*/
    , const vector<uint32_t>& /*interToOuter*/
) {
}

bool DataSync::syncData()
{
    if (!enabled()
        || lastSyncConf + solver->conf.sync_every_confl >= solver->sumConflicts
    ) {
        return true;
    }
    numCalls++;

    assert(sharedData != NULL);
    assert(solver->decisionLevel() == 0);

    if (must_rebuild_bva_map) {
        outer_to_without_bva_map = solver->build_outer_to_without_bva_map();
        must_rebuild_bva_map = false;
    }

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

    #ifdef USE_MPI
    if (is_mpi && mpiSize > 1 && solver->conf.thread_num == 0) {
        sharedData->unit_mutex.lock();
        sharedData->bin_mutex.lock();
        ok = syncFromMPI();
        if (ok && numCalls % 2 == 1) {
            syncToMPI();
        }
        if (!ok) return false;
    }

    if (is_mpi) {
        getNeedToInterruptFromMPI();
    }
    #endif

    lastSyncConf = solver->sumConflicts;

    return true;
}

void DataSync::clear_set_binary_values()
{
    for(size_t i = 0; i < solver->nVarsOutside()*2; i++) {
        Lit lit1 = Lit::toLit(i);
        lit1 = solver->map_to_with_bva(lit1);
        lit1 = solver->varReplacer->get_lit_replaced_with_outer(lit1);
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

    if (solver->conf.verbosity >= 3) {
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
        lit1 = solver->varReplacer->get_lit_replaced_with_outer(lit1);
        lit1 = solver->map_outer_to_inter(lit1);
        if (solver->varData[lit1.var()].removed != Removed::none
            || solver->value(lit1.var()) != l_Undef
        ) {
            continue;
        }

        vector<Lit>& bins = *sharedData->bins[wsLit].data;
        watch_subarray ws = solver->watches[lit1];

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
    assert(solver->varReplacer->get_lit_replaced_with(lit) == lit);
    assert(solver->varData[lit.var()].removed == Removed::none);

    assert(toClear.empty());
    for (const Watched& w: ws) {
        if (w.isBin()) {
            toClear.push_back(w.lit2());
            assert(seen.size() > w.lit2().toInt());
            seen[w.lit2().toInt()] = true;
        }
    }

    vector<Lit> lits(2);
    for (uint32_t i = finished; i < bins.size(); i++) {
        Lit otherLit = bins[i];
        otherLit = solver->map_to_with_bva(otherLit);
        otherLit = solver->varReplacer->get_lit_replaced_with_outer(otherLit);
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

            //Don't add DRAT: it would add to the thread data, too
            solver->add_clause_int(lits, true, ClauseStats(), true, NULL, false);
            if (!solver->ok) {
                goto end;
            }
        }
    }
    finished = bins.size();

    end:
    for (const Lit l: toClear) {
        seen[l.toInt()] = false;
    }
    toClear.clear();

    return solver->okay();
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
        thisLit = solver->varReplacer->get_lit_replaced_with_outer(thisLit);
        thisLit = solver->map_outer_to_inter(thisLit);
        const lbool thisVal = solver->value(thisLit);
        const lbool otherVal = shared.value[var];

        if (thisVal == l_Undef && otherVal == l_Undef) {
            continue;
        }

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
            solver->ok = solver->propagate<false>().isNULL();
            if (!solver->ok) {
                return false;
            }

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

    if (solver->conf.verbosity >= 3
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

    if (must_rebuild_bva_map) {
        outer_to_without_bva_map = solver->build_outer_to_without_bva_map();
        must_rebuild_bva_map = false;
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


///////////////////////////////////////
// MPI
///////////////////////////////////////

#ifdef USE_MPI
void DataSync::getNeedToInterruptFromMPI()
{
    int flag;
    MPI_Status status;
    int err = MPI_Iprobe(0, 1, MPI_COMM_WORLD, &flag, &status);
    assert(err == MPI_SUCCESS);
    if (flag == false) {
        return;
    }

    char* buf = NULL;
    err = MPI_Recv((unsigned*)buf, 0, MPI_UNSIGNED, 0, 1, MPI_COMM_WORLD, &status);
    assert(err == MPI_SUCCESS);
    solver->set_must_interrupt_asap();
}

bool DataSync::syncFromMPI()
{
    int err;
    MPI_Status status;
    int flag;
    int count;
    uint32_t tmp = 0;

    uint32_t thisMpiRecvUnitData = 0;
    uint32_t thisMpiRecvBinData = 0;

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

    //Unit clauses
    int at = 0;
    assert(solver->nVars() == buf[at]);
    at++;
    for (uint32_t var = 0; var < solver->nVars(); var++, at++) {
        const lbool otherVal = toLbool(buf[at]);
        if (!sync_mpi_unit(otherVal, var, NULL, thisMpiRecvUnitData, tmp)) {
            #ifdef VERBOSE_DEBUG_MPI_SENDRCV
            std::cout << "-->> MPI " << mpiRank << " solver FALSE" << std::endl;
            #endif
            goto end;
        }
    }
    solver->ok = solver->propagate<true>().isNULL();
    if (!solver->ok) goto end;
    mpiRecvUnitData += thisMpiRecvUnitData;

    //Binary clauses
    assert(buf[at] == solver->nVars()*2);
    at++;
    for (uint32_t wsLit = 0; wsLit < solver->nVars()*2; wsLit++) {
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

    end:
    #ifdef VERBOSE_DEBUG_MPI_SENDRCV
    std::cout << "-->> MPI " << mpiRank << " Received " << thisMpiRecvUnitData << " units" << std::endl;
    std::cout << "-->> MPI " << mpiRank << " Received " << thisMpiRecvBinData << " bins" << std::endl;
    std::cout << "-->> MPI " << mpiRank << " Received " << thisMpiRecvTriData << " tris" << std::endl;
    #endif

    delete[] buf;
    return solver->ok;
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
    data.push_back((uint32_t)solver->nVars());
    for (uint32_t var = 0; var < solver->nVars(); var++) {
        data.push_back(toInt(solver->value(var)));
    }

    //Binary
    uint32_t thisMpiSentBinData = 0;
    data.push_back((uint32_t)solver->nVars()*2);
    uint32_t wsLit = 0;
    for(auto it = sharedData->bins.begin()
        , end = sharedData->bins.end(); it != end; it++, wsLit++
    ) {
        //Lit lit1 = ~Lit::toLit(wsLit);
        assert(it->data->size() >= syncMPIFinish[wsLit]);
        uint32_t sizeToSend = it->data->size() - syncMPIFinish[wsLit];
        data.push_back(sizeToSend);
        for (uint32_t i = syncMPIFinish[wsLit]; i < it->data->size(); i++) {
            data.push_back(it->data->at(i).toInt());
            thisMpiSentBinData++;
        }
        syncMPIFinish[wsLit] = it->data->size();
    }
    assert(wsLit == solver->nVars()*2);
    mpiSentBinData += thisMpiSentBinData;

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

bool DataSync::sync_mpi_unit(
    const lbool otherVal,
    const uint32_t var,
    SharedData* shared,
    uint32_t& thisGotUnitData,
    uint32_t& thisSentUnitData
) {
    Lit l = Lit(var, false);
    Lit lit1 = solver->map_to_with_bva(l);
    lit1 = solver->varReplacer->get_lit_replaced_with_outer(lit1);
    lit1 = solver->map_outer_to_inter(lit1);
    const lbool thisVal = solver->value(lit1);

    if (thisVal == l_Undef && otherVal == l_Undef) {
        return true;
    }
    if (thisVal != l_Undef && otherVal != l_Undef) {
        if (thisVal != otherVal) {
            solver->ok = false;
            return false;
        } else {
            return true;
        }
    }

    if (otherVal != l_Undef) {
        assert(thisVal == l_Undef);
        Lit litToEnqueue = lit1 ^ (otherVal == l_False);
        if (solver->varData[litToEnqueue.var()].removed != Removed::none) {
            return true;
        }

        solver->enqueue(litToEnqueue);
        solver->ok = solver->propagate<false>().isNULL();
        if (!solver->ok) {
            return false;
        }

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

#endif
