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

#include "datasyncserver.h"
#include "solvertypes.h"
#include <cassert>
using std::vector;

//#define VERBOSE_DEBUG_MPI_SENDRCV

using namespace CMSat;

DataSyncServer::DataSyncServer()
{
    int err;
    err = MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
    assert(err == MPI_SUCCESS);

    sendRequests.resize(mpiSize);
    sendRequestsFinished.resize(mpiSize, true);

    alreadySentInterrupt.resize(mpiSize, false);
    interruptRequests.resize(mpiSize);
    numAlreadyInterrupted = 0;

    int mpiRank;
    err = MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    assert(err == MPI_SUCCESS);
    assert(mpiRank == 0);

    std::cout << "c MPI server says -- mpiSize:" << mpiSize << std::endl;
    assert(sizeof(unsigned) == sizeof(uint32_t));
}


//Tag of messsage "0"
void DataSyncServer::syncFromMPI()
{
    int err;
    MPI_Status status;
    int flag;
    int count;
    uint32_t thisRecvBinData = 0;

    //Check for message
    err = MPI_Iprobe(MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &flag, &status);
    assert(err == MPI_SUCCESS);
    if (flag == false) {
        return;
    }

    //Get message size
    err = MPI_Get_count(&status, MPI_UNSIGNED, &count);
    assert(err == MPI_SUCCESS);
    #ifdef VERBOSE_DEBUG_MPI_SENDRCV
    std::cout << "-->> MPI Server Received " << count << " uint32_t-s" << std::endl;
    #endif

    //Get message
    uint32_t* buf = new uint32_t[count];
    err = MPI_Recv((unsigned*)buf, count, MPI_UNSIGNED, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
    assert(err == MPI_SUCCESS);

    #ifdef VERBOSE_DEBUG_MPI_SENDRCV
    std::cout << "-->> MPI Server Received data from " << status.MPI_SOURCE << std::endl;
    #endif

    int at = 0;
    assert(num_vars == buf[at]); //the first uint32_t is the number of bytes, always

    //Sync all units, starts with num_vars
    assert(num_vars == buf[at]);
    at++;
    for (uint32_t var = 0; var < num_vars; var++, at++) {
        lbool val = toLbool(buf[at]);
        if (value[var] == l_Undef) {
            if (val != l_Undef) value[var] = val;
        } else if (val != l_Undef && value[var] != val) {
            assert(val == l_True || val == l_False);
            assert(value[var] == l_True || value[var] == l_False);

            ok = false;
            #ifdef VERBOSE_DEBUG_MPI_SENDRCV
            std::cout << "-->> MPI Server solver FALSE" << std::endl;
            #endif
        }
    }

    //Sync all binaries, starts with num_vars*2
    //for each Lit, there is SIZE of elements that follow, then the elements
    assert(buf[at] == num_vars*2);
    at++;
    for (uint32_t wsLit = 0; wsLit < num_vars*2; wsLit++) {
        Lit lit = ~Lit::toLit(wsLit);
        uint32_t num = buf[at];
        at++;
        for (uint32_t i = 0; i < num; i++, at++) {
            Lit otherLit = Lit::toLit(buf[at]);
            add_bin_to_threads(lit, otherLit);
            thisRecvBinData++;
        }
    }
    recvBinData += thisRecvBinData;

    #ifdef VERBOSE_DEBUG_MPI_SENDRCV
    std::cout << "-->> MPI Server Received " << thisRecvBinData << " bins" << std::endl;
    std::cout << "-->> MPI Server Received " << thisRecvTriData << " tris" << std::endl;
    #endif

    delete[] buf;
    numGotPacket++;
}

void DataSyncServer::add_bin_to_threads(const Lit lit1, const Lit lit2)
{
    assert(lit1 < lit2);

    //check if binary is already known
    vector<Lit>& thisBins = bins[(~lit1).toInt()];
    for (vector<Lit>::const_iterator it = thisBins.begin(), end = thisBins.end(); it != end; ++it) {
        if (*it == lit2) {
            return;
        }
    }

    //binary not already known
    thisBins.push_back(lit2);
}

void DataSyncServer::sendDataToAll()
{
    int err;

    //Something to send, then send it.
    if (sendData != NULL) {
        int numFinished = 0;
        for (int i = 1; i < mpiSize; i++) {
            if (sendRequestsFinished[i]) {
                numFinished++;
                continue;
            }
            MPI_Status status;
            int op_completed;
            err = MPI_Test(&(sendRequests[i]), &op_completed, &status);
            assert(err == MPI_SUCCESS);
            if (op_completed) {
                err = MPI_Request_free(&(sendRequests[i]));
                assert(err == MPI_SUCCESS);
                sendRequestsFinished[i] = true;
                numFinished++;
                #ifdef VERBOSE_DEBUG_MPI_SENDRCV
                std::cout << "-->> MPI Server Sending finished to " << i << std::endl;
                #endif
            }
        }
        if (numFinished != mpiSize-1) {
            return;
        }
        delete sendData;
        sendData = NULL;
    }

    //Set up units. First, the num_vars, then the values
    uint32_t thisSentBinData = 0;
    vector<uint32_t> data;
    data.push_back((uint32_t)num_vars);
    for (uint32_t var = 0; var < num_vars; var++) {
        data.push_back(toInt(value[var]));
    }



    //Binaries. First, the 2*num_vars, then the list sizes, then the data.
    data.push_back((uint32_t)num_vars*2);
    uint32_t at = 0;
    for(vector<vector<Lit> >::const_iterator it = bins.begin(), end = bins.end(); it != end; ++it, at++) {
        const vector<Lit>& binSet = *it;
        assert(binSet.size() >= syncMPIFinish[at]);
        uint32_t sizeToSend = binSet.size() - syncMPIFinish[at];
        data.push_back(sizeToSend);
        for (uint32_t i = syncMPIFinish[at]; i < binSet.size(); i++) {
            data.push_back(binSet[i].toInt());
            thisSentBinData++;
        }
        syncMPIFinish[at] = binSet.size();
    }
    sentBinData += thisSentBinData;

    sendData = new uint32_t[data.size()];
    std::copy(data.begin(), data.end(), sendData);
    for (int i = 1; i < mpiSize; i++) {
        err = MPI_Isend(sendData, data.size(), MPI_UNSIGNED, i, 0, MPI_COMM_WORLD, &(sendRequests[i]));
        assert(err == MPI_SUCCESS);
        #ifdef VERBOSE_DEBUG_MPI_SENDRCV
        std::cout << "-->> MPI Server Sent to " << i << " num: " << data.size() << " uint32_t -s" << std::endl;
        std::cout << "-->> MPI Server Sent to " << i << " num: " << thisSentBinData << " bins " << std::endl;
        std::cout << "-->> MPI Server Sent to " << i << " num: " << thisSentTriData << " tris " << std::endl;
        #endif
        sendRequestsFinished[i] = false;
    }
    lastSendNumGotPacket= numGotPacket;
}


//Tag of message "1"
void DataSyncServer::check_interrupt_and_forward_to_all()
{
    int err;
    MPI_Status status;
    int flag;

    //Check for interrupt data
    err = MPI_Iprobe(MPI_ANY_SOURCE, 1, MPI_COMM_WORLD, &flag, &status);
    assert(err == MPI_SUCCESS);
    if (flag == false) return;

    //Get the interrupt signal data itself
    uint32_t* buf = NULL;
    err = MPI_Recv((unsigned*)buf, 0, MPI_UNSIGNED, MPI_ANY_SOURCE, 1, MPI_COMM_WORLD, &status);
    assert(err == MPI_SUCCESS);
    int source = status.MPI_SOURCE;
    numAlreadyInterrupted++;

    //Send to all, in case they haven't received it yet. EVERYONE gets the message,
    //including the one who sent it :)
    for (int i = 1; i < mpiSize; i++) {
        if (alreadySentInterrupt[i]) {
            continue;
        }
        err = MPI_Isend(0, 0, MPI_UNSIGNED, i, 1, MPI_COMM_WORLD, &(interruptRequests[i]));
        assert(err == MPI_SUCCESS);
        alreadySentInterrupt[i] = true;
    }
}

void CMSat::DataSyncServer::print_solution()
{
    cout << "v ";
    for(uint32_t i = 0; i < solution.size(); i++) {
        if (solution[i] == l_True) {
            cout << i+1 << " ";
        } else if (solution[i] == l_False) {
            cout << "-" << i+1 << " ";
        } else {
            assert(false && "We should always have full solution");
        }
    }
    cout << endl;
}

void CMSat::DataSyncServer::send_cnf_to_solvers()
{
    cout << "c sending file to all solvers..." << endl;

    value.resize(num_vars, l_Undef);
    bins.resize(num_vars*2);
    syncMPIFinish.resize(num_vars*2, 0);

    int err;
    bool finished = false;
    Lit buf[1024];
    uint32_t at = 0;
    uint32_t i = 0;

    //first byte we send is the number of variables
    buf[i] = Lit(num_vars, false);
    i++;

    while(!finished) {
        for(;i < 1024; i++) {
            if (clauses_array.size() <= at) {
                buf[i] = lit_Error;
                finished = true;
                continue;
            }

            buf[i] = clauses_array[at];
            at++;
        }
        err = MPI_Bcast(buf, 1024, MPI_UNSIGNED, 0, MPI_COMM_WORLD);
        assert(err == MPI_SUCCESS);
        i = 0;
    }
    assert(clauses_array.size() == at);

    cout << "c sent file to all solvers" << endl;
}

void CMSat::DataSyncServer::new_var()
{
    num_vars++;
}

void CMSat::DataSyncServer::add_xor_clause(const vector<uint32_t>& vars, bool& rhs)
{
    assert(false);
}


void CMSat::DataSyncServer::new_vars(uint32_t i)
{
    num_vars+=i;
}

void CMSat::DataSyncServer::add_clause(const vector<CMSat::Lit>& lits)
{
    for(const auto& lit: lits) {
        clauses_array.push_back(lit);
    }
    clauses_array.push_back(lit_Undef);
}


bool DataSyncServer::actAsServer()
{
    while(ok) {
        syncFromMPI();

        if (numAlreadyInterrupted == 0
            && lastSendNumGotPacket+(mpiSize/2)+1 < numGotPacket)
        {
            sendDataToAll();
        }

        check_interrupt_and_forward_to_all();

        if (numAlreadyInterrupted == mpiSize-1) {
            break;
        }
    }

    return ok;
}

