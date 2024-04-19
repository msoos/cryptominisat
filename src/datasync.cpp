/******************************************
Copyright (C) 2009-2020 Authors of CryptoMiniSat, see AUTHORS file

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

#include <iostream>
#include <iomanip>

//#define VERBOSE_DEBUG_MPI_SENDRCV

using namespace CMSat;

DataSync::DataSync(Solver* _solver, SharedData* _sharedData) :
    solver(_solver)
    , sharedData(_sharedData)
    , seen(solver->seen)
    , toClear(solver->toClear)
{
}

void DataSync::finish_up_mpi()
{
    #ifdef USE_MPI
    if (mpiSendData) {
        //mpiSendData is only non-nullptr when MPI is on and it's the 0 thread
        assert(solver->conf.is_mpi);
        assert(solver->conf.thread_num == 0);

        /*MPI_Status status;
        int op_completed = false;
        int err;

        err = MPI_Test(&sendReq, &op_completed, &status);
        assert(err == MPI_SUCCESS);
        if (!op_completed) {
            err = MPI_Cancel(&sendReq);
            assert(err == MPI_SUCCESS);
        }*/
        delete[] mpiSendData;
        mpiSendData = nullptr;
    }
    #endif
}

void DataSync::set_shared_data(SharedData* _sharedData)
{
    sharedData = _sharedData;
    thread_id = _sharedData->cur_thread_id++;
    #ifdef USE_MPI
    set_up_for_mpi();
    #endif
}

void DataSync::new_var(const bool bva)
{
    if (!enabled())
        return;

    if (!bva) {
        syncFinish.push_back(0);
        syncFinish.push_back(0);
    }
    assert(solver->nVarsOuter()*2 == syncFinish.size());
}

void DataSync::new_vars(size_t n)
{
    if (!enabled())
        return;

    syncFinish.insert(syncFinish.end(), 2*n, 0);
    assert(solver->nVarsOuter()*2 == syncFinish.size());
}

void DataSync::save_on_var_memory()
{
}

void DataSync::updateVars(
    [[maybe_unused]] const vector<uint32_t>&  outer_to_inter
    , [[maybe_unused]] const vector<uint32_t>& inter_to_outer
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

    assert(sharedData != nullptr);
    assert(solver->decisionLevel() == 0);

    //SEND data
    bool ok;
    sharedData->unit_mutex.lock();
    ok = shareUnitData();
    sharedData->unit_mutex.unlock();
    if (!ok) {
        return false;
    }
    solver->ok = solver->propagate<false>().isnullptr();
    if (!solver->ok) {
        return false;
    }

    //RECEIVE data
    sharedData->bin_mutex.lock();
    extend_bins_if_needed();
    clear_set_binary_values();
    ok = shareBinData();
    sharedData->bin_mutex.unlock();
    if (!ok) {
        return false;
    }

    #ifdef USE_MPI
    if (solver->conf.is_mpi
        && solver->conf.thread_num == 0)
    {
        if (syncMPIFinish.size() < solver->nVarsOutside()*2) {
            syncMPIFinish.resize(solver->nVarsOutside()*2, 0);
        }

        if (!mpi_get_interrupt()) {
            sharedData->unit_mutex.lock();
            sharedData->bin_mutex.lock();
            ok = mpi_recv_from_others();
            assert(solver->conf.every_n_mpi_sync > 0);
            if (ok &&
                numCalls % solver->conf.every_n_mpi_sync == solver->conf.every_n_mpi_sync-1
            ) {
                mpi_send_to_others();
            }
            sharedData->unit_mutex.unlock();
            sharedData->bin_mutex.unlock();
            if (!ok) {
                return false;
            }
        }
    }
    #endif

    lastSyncConf = solver->sumConflicts;

    return true;
}

bool DataSync::shareUnitData()
{
    assert(solver->okay());
    assert(!solver->frat->enabled());

    uint32_t thisGotUnitData = 0;
    uint32_t thisSentUnitData = 0;

    SharedData& shared = *sharedData;
    if (shared.value.size() < solver->nVarsOuter()) {
        shared.value.insert(
            shared.value.end(),
            solver->nVarsOuter()-shared.value.size(), l_Undef);
    }
    for (uint32_t var = 0; var < solver->nVarsOuter(); var++) {
        Lit thisLit = Lit(var, false);
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

            solver->enqueue<false>(litToEnqueue);

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
    stats.recvUnitData += thisGotUnitData;
    stats.sentUnitData += thisSentUnitData;

    if (solver->conf.verbosity >= 1) {
        cout
        << "c [sync " << thread_id << "  ]"
        << " got units " << thisGotUnitData
        << " (total: " << stats.recvUnitData << ")"
        << " sent units " << thisSentUnitData
        << " (total: " << stats.sentUnitData << ")"
        << endl;
    }

    return true;
}

void CMSat::DataSync::signal_new_long_clause(const vector<Lit>& cl)
{
    if (!enabled()) return;
    assert(thread_id != -1);
    if (cl.size() == 2) signal_new_bin_clause(cl[0], cl[1]);
}

bool DataSync::syncBinFromOthers()
{
    for (uint32_t wsLit = 0; wsLit < sharedData->bins.size(); wsLit++) {
        if (sharedData->bins[wsLit].data == nullptr) {
            continue;
        }

        Lit lit1 = Lit::toLit(wsLit);
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

            //Don't add FRAT: it would add to the thread data, too
            solver->add_clause_int(lits, true, nullptr, true, nullptr, false);
            if (!solver->okay()) {
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
        add_bin_to_threads(bin.first, bin.second);
    }

    newBinClauses.clear();
}

bool DataSync::add_bin_to_threads(Lit lit1, Lit lit2)
{
    assert(lit1 < lit2);
    if (sharedData->bins[lit1.toInt()].data == nullptr) {
        return false;
    }

    vector<Lit>& bins = *sharedData->bins[lit1.toInt()].data;
    for (const Lit lit : bins) {
        if (lit == lit2)
            return false;
    }

    bins.push_back(lit2);
    stats.sentBinData++;
    return true;
}

void DataSync::clear_set_binary_values()
{
    for(size_t i = 0; i < solver->nVarsOuter()*2; i++) {
        Lit lit1 = Lit::toLit(i);
        lit1 = solver->varReplacer->get_lit_replaced_with_outer(lit1);
        lit1 = solver->map_outer_to_inter(lit1);
        if (solver->value(lit1) != l_Undef) {
            sharedData->bins[i].clear();
        }
    }
}

void DataSync::extend_bins_if_needed()
{
    assert(sharedData->bins.size() <= (solver->nVarsOuter())*2);
    if (sharedData->bins.size() == (solver->nVarsOuter())*2)
        return;

    sharedData->bins.resize(solver->nVarsOuter()*2);
}

bool DataSync::shareBinData()
{
    assert(solver->okay());
    uint32_t oldRecvBinData = stats.recvBinData;
    uint32_t oldSentBinData = stats.sentBinData;

    bool ok = syncBinFromOthers();
    syncBinToOthers();
    size_t mem = sharedData->calc_memory_use_bins();

    if (solver->conf.verbosity >= 1) {
        cout
        << "c [sync " << thread_id << "  ]"
        << " got bins " << (stats.recvBinData - oldRecvBinData)
        << " (total: " << stats.recvBinData << ")"
        << " sent bins " << (stats.sentBinData - oldSentBinData)
        << " (total: " << stats.sentBinData << ")"
        << " mem use: " << mem/(1024*1024) << " M"
        << endl;
    }

    return ok;
}

void DataSync::signal_new_bin_clause(Lit lit1, Lit lit2)
{
    if (!enabled()) return;
    if (solver->varData[lit1.var()].is_bva) return;
    if (solver->varData[lit2.var()].is_bva) return;

    lit1 = solver->map_inter_to_outer(lit1);
    lit2 = solver->map_inter_to_outer(lit2);

    if (lit1.toInt() > lit2.toInt()) std::swap(lit1, lit2);
    newBinClauses.push_back(std::make_pair(lit1, lit2));
}

#ifdef USE_MPI
void DataSync::set_up_for_mpi()
{
    if (solver->conf.is_mpi) {
        int err;
        err = MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
        assert(err == MPI_SUCCESS);

        err = MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
        assert(err == MPI_SUCCESS);
        release_assert(mpiRank != 0);
        assert(sharedData != nullptr);
    }
}

//Interrupts are tag 1, coming from master (i.e. rank 0)
bool DataSync::mpi_get_interrupt()
{
    #ifdef VERBOSE_DEBUG_MPI_SENDRCV
    std::cout << "-->> MPI " << mpiRank << " thread " << thread_id <<
    " trying to get interrupt" << endl;
    #endif

    int flag;
    MPI_Status status;
    int err = MPI_Iprobe(
        0, //master source
        1, //tag "1" for finish interrupt
        MPI_COMM_WORLD, &flag, &status);
    assert(err == MPI_SUCCESS);
    if (flag == false) {
        return false;
    }


    //Receive the tag 1 message
    unsigned buf;
    err = MPI_Recv(
        &buf,
        0, //data is empty
        MPI_UNSIGNED,
        0, //source is master
        1, //tag is "1"
        MPI_COMM_WORLD, &status);
    assert(err == MPI_SUCCESS);

    //TODO should we cancel the potentially running SEND request?

    solver->set_must_interrupt_asap();

    return true;
}

bool DataSync::mpi_recv_from_others()
{
    int err;
    MPI_Status status;
    int flag;
    int count;
    uint32_t thisMpiRecvUnitData = 0;
    uint32_t thisMpiRecvBinData = 0;
    #ifdef VERBOSE_DEBUG_MPI_SENDRCV
    std::cout << "-->> MPI " << mpiRank << " thread " << thread_id <<
    " syncing from MPI..." << std::endl;
    #endif


    //Check if there is data, tagged 0, from source 0 (root)
    err = MPI_Iprobe(0, 0, MPI_COMM_WORLD, &flag, &status);
    assert(err == MPI_SUCCESS);
    if (flag == false) {
        #ifdef VERBOSE_DEBUG_MPI_SENDRCV
        std::cout << "-->> MPI " << mpiRank << " thread " << thread_id <<
        " No data to receive." << std::endl;
        #endif

        //no data
        return true;
    }

    //Get data size
    err = MPI_Get_count(&status, MPI_UNSIGNED, &count);
    assert(err == MPI_SUCCESS);
    #ifdef VERBOSE_DEBUG_MPI_SENDRCV
    std::cout << "-->> MPI " << mpiRank << " thread " << thread_id <<
    " Receiving " << count << " uint32_t ..." << std::endl;
    #endif

    //Receive data
    uint32_t* buf = new uint32_t[count];
    err = MPI_Recv((unsigned*)buf, count, MPI_UNSIGNED, 0, 0, MPI_COMM_WORLD, &status);
    assert(err == MPI_SUCCESS);

    //Unit clauses
    int at = 0;
    assert(solver->nVarsOutside() == buf[at]);
    at++;
    for (uint32_t var = 0; var < solver->nVarsOutside(); var++, at++) {
        const lbool otherVal = toLbool(buf[at]);
        if (!mpi_get_unit(otherVal, var, thisMpiRecvUnitData)) {
            #ifdef VERBOSE_DEBUG_MPI_SENDRCV
            std::cout << "-->> MPI " << mpiRank << " thread " << thread_id <<
            " solver FALSE" << std::endl;
            #endif
            goto end;
        }
    }
    solver->ok = solver->propagate<false>().isnullptr();
    if (!solver->ok) {
        goto end;
    }
    mpiRecvUnitData += thisMpiRecvUnitData;

    //Binary clauses
    assert(buf[at] == solver->nVarsOutside()*2);
    at++;
    for (uint32_t wsLit = 0; wsLit < solver->nVarsOutside()*2; wsLit++) {
        Lit lit = Lit::toLit(wsLit);
        uint32_t num = buf[at];
        at++;
        for (uint32_t i = 0; i < num; i++, at++) {
            Lit otherLit = Lit::toLit(buf[at]);
            thisMpiRecvBinData += add_bin_to_threads(lit, otherLit);
        }
    }
    mpiRecvBinData += thisMpiRecvBinData;

    end:
    #ifdef VERBOSE_DEBUG_MPI_SENDRCV
    std::cout << "-->> MPI " << mpiRank << " thread " << thread_id <<
    " Received " << thisMpiRecvUnitData << " units (total: " << mpiRecvUnitData << ")" << std::endl;
    std::cout << "-->> MPI " << mpiRank << " thread " << thread_id <<
    " Received " << thisMpiRecvBinData << " bins (total: " << mpiRecvBinData << ")" << std::endl;
    #endif

    delete[] buf;
    return solver->okay();
}

void DataSync::mpi_send_to_others()
{
    int err;

    //We still are sending data, let's do that first
    if (mpiSendData != nullptr) {
        #ifdef VERBOSE_DEBUG_MPI_SENDRCV
        std::cout << "-->> MPI " << mpiRank << " thread " << thread_id <<
        " Still sending data, waiting now." << std::endl;
        #endif

        /*MPI_Status status;
        int op_completed;
        err = MPI_Request_get_status(sendReq, &op_completed, &status);
        assert(err == MPI_SUCCESS);
        if (op_completed) {
            err = MPI_Wait(&sendReq, &status);
            assert(err == MPI_SUCCESS);*/
            delete[] mpiSendData;
            mpiSendData = nullptr;
        /*} else {
            return;
        }*/
    }

    #ifdef VERBOSE_DEBUG_MPI_SENDRCV
    std::cout << "-->> MPI " << mpiRank << " thread " << thread_id <<
    " Building data to send via MPI..." << std::endl;
    #endif

    //Set up units
    assert(solver->nVarsOutside() == sharedData->value.size());
    vector<uint32_t> data;
    data.push_back(solver->nVarsOutside());
    for (uint32_t var = 0; var < solver->nVarsOutside(); var++) {
        data.push_back(toInt(sharedData->value[var]));
    }

    //Set up binaries
    assert(sharedData->bins.size() == solver->nVarsOutside()*2);
    uint32_t thisMpiSentBinData = 0;
    data.push_back(solver->nVarsOutside()*2);

    for(uint32_t wsLit = 0; wsLit < solver->nVarsOutside()*2; wsLit++) {
        //Lit lit1 = ~Lit::toLit(wsLit);
        assert(syncMPIFinish.size() > wsLit);
        if (sharedData->bins[wsLit].data == nullptr) {
            data.push_back(0);
            continue;
        }
        uint32_t size = sharedData->bins[wsLit].data->size();
        assert(size >= syncMPIFinish[wsLit]);
        uint32_t sizeToSend = size - syncMPIFinish[wsLit];
        data.push_back(sizeToSend);
        for (uint32_t i = syncMPIFinish[wsLit]; i < size; i++) {
            data.push_back(sharedData->bins[wsLit].data->at(i).toInt());
            thisMpiSentBinData++;
        }
        syncMPIFinish[wsLit] = size;
    }
    mpiSentBinData += thisMpiSentBinData;

    #ifdef VERBOSE_DEBUG_MPI_SENDRCV
    std::cout << "-->> MPI " << mpiRank << " thread " << thread_id <<
    " Sending " << data.size() << " uint32_t -s" << std::endl;
    std::cout << "-->> MPI " << mpiRank << " thread " << thread_id <<
    " and " << thisMpiSentBinData << " bins.." << std::endl;
    #endif

    //Send the data
    mpiSendData = new uint32_t[data.size()];
    std::copy(data.begin(), data.end(), mpiSendData);
    //err = MPI_Isend(mpiSendData, data.size(), MPI_UNSIGNED, 0, 0, MPI_COMM_WORLD, &sendReq);
    err = MPI_Send(mpiSendData, data.size(), MPI_UNSIGNED, 0, 0, MPI_COMM_WORLD);
    assert(err == MPI_SUCCESS);

    #ifdef VERBOSE_DEBUG_MPI_SENDRCV
    std::cout << "-->> MPI " << mpiRank << " thread " << thread_id <<
    " Sent MPI sync data" << std::endl;
    #endif
}

bool DataSync::mpi_get_unit(
    const lbool otherVal,
    const uint32_t var,
    uint32_t& thisGotUnitData
) {
    Lit l = Lit(var, false);
    Lit lit1 = solver->map_to_with_bva(l);
    lit1 = solver->varReplacer->get_lit_replaced_with_outer(lit1);
    lit1 = solver->map_outer_to_inter(lit1);
    const lbool thisVal = solver->value(lit1);

    if (thisVal == otherVal) {
        return true;
    }

    if (otherVal == l_Undef) {
        return true;
    }

    if (thisVal != l_Undef) {
        if (thisVal != otherVal) {
            solver->ok = false;
            return false;
        } else {
            return true;
        }
    }

    assert(otherVal != l_Undef);
    assert(thisVal == l_Undef);
    Lit litToEnqueue = lit1 ^ (otherVal == l_False);
    if (solver->varData[litToEnqueue.var()].removed != Removed::none) {
        return true;
    }

    solver->enqueue<false>(litToEnqueue);
    solver->ok = solver->propagate<false>().isnullptr();
    if (!solver->ok) {
        return false;
    }

    thisGotUnitData++;

    return true;
}
#endif
