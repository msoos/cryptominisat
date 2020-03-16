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

#include "datasyncserver.h"
#include "solvertypes.h"
#include <cassert>
using std::vector;

//#define VERBOSE_DEBUG_MPI_SENDRCV

using namespace CMSat;

DataSyncServer::DataSyncServer() :
    ok(true)
    , sendData(NULL)
    , nVars(0)
    , recvBinData(0)
    , sentBinData(0)
    , numGotPacket(0)
    , lastSendNumGotPacket(0)
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

bool DataSyncServer::syncFromMPI()
{
    int err;
    MPI_Status status;
    int flag;
    int count;
    uint32_t thisRecvBinData = 0;

    err = MPI_Iprobe(MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &flag, &status);
    assert(err == MPI_SUCCESS);
    if (flag == false) return true;

    err = MPI_Get_count(&status, MPI_UNSIGNED, &count);
    assert(err == MPI_SUCCESS);
    #ifdef VERBOSE_DEBUG_MPI_SENDRCV
    std::cout << "-->> MPI Server Received " << count << " uint32_t-s" << std::endl;
    #endif

    uint32_t* buf = new uint32_t[count];
    err = MPI_Recv((unsigned*)buf, count, MPI_UNSIGNED, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
    assert(err == MPI_SUCCESS);

    #ifdef VERBOSE_DEBUG_MPI_SENDRCV
    std::cout << "-->> MPI Server Received data from " << status.MPI_SOURCE << std::endl;
    #endif

    int at = 0;
    if (nVars == 0) {
        nVars = buf[at];
        value.resize(nVars, l_Undef);
        bins.resize(nVars*2);
        syncMPIFinish.resize(nVars*2, 0);
    }

    //Unit
    assert(nVars == buf[at]);
    at++;
    for (uint32_t var = 0; var < nVars; var++, at++) {
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
            goto end;
        }
    }

    //Binary
    assert(buf[at] == nVars*2);
    at++;
    for (uint32_t wsLit = 0; wsLit < nVars*2; wsLit++) {
        Lit lit = ~Lit::toLit(wsLit);
        uint32_t num = buf[at];
        at++;
        for (uint32_t i = 0; i < num; i++, at++) {
            Lit otherLit = Lit::toLit(buf[at]);
            addOneBinToOthers(lit, otherLit);
            thisRecvBinData++;
        }
    }
    recvBinData += thisRecvBinData;

    #ifdef VERBOSE_DEBUG_MPI_SENDRCV
    std::cout << "-->> MPI Server Received " << thisRecvBinData << " bins" << std::endl;
    std::cout << "-->> MPI Server Received " << thisRecvTriData << " tris" << std::endl;
    #endif

    end:
    delete[] buf;
    numGotPacket++;
    return ok;
}

void DataSyncServer::addOneBinToOthers(const Lit lit1, const Lit lit2)
{
    assert(lit1 < lit2);

    vector<Lit>& thisBins = bins[(~lit1).toInt()];
    for (vector<Lit>::const_iterator it = thisBins.begin(), end = thisBins.end(); it != end; it++) {
        if (*it == lit2) return;
    }

    thisBins.push_back(lit2);
}

void DataSyncServer::sendDataToAll()
{
    int err;
    if (sendData != NULL) {
        int numFinished = 0;
        for (int i = 1; i < mpiSize; i++) {
            if (sendRequestsFinished[i]) {
                numFinished++;
                continue;
            }
            MPI_Status status;
            int flag;
            err = MPI_Test(&(sendRequests[i]), &flag, &status);
            assert(err == MPI_SUCCESS);
            if (flag == true) {
                err = MPI_Request_free(&(sendRequests[i]));
                assert(err == MPI_SUCCESS);
                sendRequestsFinished[i] = true;
                numFinished++;
                #ifdef VERBOSE_DEBUG_MPI_SENDRCV
                std::cout << "-->> MPI Server Sending finished to " << i << std::endl;
                #endif
            }
        }
        if (numFinished != mpiSize-1) return;
        delete sendData;
        sendData = NULL;
    }

    uint32_t thisSentBinData = 0;
    vector<uint32_t> data;
    data.push_back((uint32_t)nVars);
    for (uint32_t var = 0; var < nVars; var++) {
        data.push_back(toInt(value[var]));
    }

    data.push_back((uint32_t)nVars*2);

    //Binaries
    uint32_t at = 0;
    for(vector<vector<Lit> >::const_iterator it = bins.begin(), end = bins.end(); it != end; it++, at++) {
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

    uint32_t *sendData = new uint32_t[data.size()];
    std::copy(data.begin(), data.end(), sendData);
    for (int i = 1; i < mpiSize; i++) {
        int err = MPI_Isend(sendData, data.size(), MPI_UNSIGNED, i, 0, MPI_COMM_WORLD, &(sendRequests[i]));
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

void DataSyncServer::forwardNeedToInterrupt()
{
    int err;
    MPI_Status status;
    int flag;

    err = MPI_Iprobe(MPI_ANY_SOURCE, 1, MPI_COMM_WORLD, &flag, &status);
    assert(err == MPI_SUCCESS);
    if (flag == false) return;

    uint32_t* buf = NULL;
    err = MPI_Recv((unsigned*)buf, 0, MPI_UNSIGNED, MPI_ANY_SOURCE, 1, MPI_COMM_WORLD, &status);
    assert(err == MPI_SUCCESS);
    //int source = status.MPI_SOURCE;
    numAlreadyInterrupted++;

    for (int i = 1; i < mpiSize; i++) {
        if (alreadySentInterrupt[i]) continue;
        err = MPI_Isend(0, 0, MPI_UNSIGNED, i, 1, MPI_COMM_WORLD, &(interruptRequests[i]));
        assert(err == MPI_SUCCESS);
        alreadySentInterrupt[i] = true;
    }
}

bool DataSyncServer::actAsServer()
{
    while(ok) {
        if (!syncFromMPI()) return false;

        if (numAlreadyInterrupted == 0
            && lastSendNumGotPacket+(mpiSize/2)+1 < numGotPacket) sendDataToAll();

        forwardNeedToInterrupt();

        if (numAlreadyInterrupted == mpiSize-1) break;
    }

    return ok;
}

