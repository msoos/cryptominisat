/***************************************************************************
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
*****************************************************************************/

#include "DataSyncServer.h"
using std::vector;

//#define VERBOSE_DEBUG_MPI_SENDRCV

DataSyncServer::DataSyncServer() :
    ok(true)
    , sendData(NULL)
    , nVars(0)
    , recvBinData(0)
    , sentBinData(0)
    , recvTriData(0)
    , sentTriData(0)
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

const bool DataSyncServer::syncFromMPI()
{
    int err;
    MPI_Status status;
    int flag;
    int count;
    uint32_t thisRecvBinData = 0;
    uint32_t thisRecvTriData = 0;

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
        tris.resize(nVars*2);
        syncMPIFinish.resize(nVars*2, 0);
        syncMPIFinishTri.resize(nVars*2, 0);
    }
    assert(nVars == buf[at]);
    at++;
    for (Var var = 0; var < nVars; var++, at++) {
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

    assert(buf[at] == nVars*2);
    at++;
    for (uint32_t wsLit = 0; wsLit < nVars*2; wsLit++) {
        Lit lit1 = ~Lit::toLit(wsLit);
        uint32_t num = buf[at];
        at++;
        for (uint32_t i = 0; i < num; i++, at++) {
            Lit lit2 = Lit::toLit(buf[at]);
            at++;
            Lit lit3 = Lit::toLit(buf[at]);
            addOneTriToOthers(lit1, lit2, lit3);
            thisRecvTriData++;
        }
    }
    recvTriData += thisRecvTriData;

    #ifdef VERBOSE_DEBUG_MPI_SENDRCV
    std::cout << "-->> MPI Server Received " << thisRecvBinData << " bins" << std::endl;
    std::cout << "-->> MPI Server Received " << thisRecvTriData << " tris" << std::endl;
    #endif

    end:
    delete buf;
    numGotPacket++;
    return ok;
}

void DataSyncServer::addOneTriToOthers(const Lit lit1, const Lit lit2, const Lit lit3)
{
    assert(lit1 < lit2);
    assert(lit2 < lit3);
    assert(lit1.var() != lit2.var());
    assert(lit2.var() != lit3.var());

    vector<TriClause>& triCls = tris[(~lit1).toInt()];
    for (vector<TriClause>::const_iterator it = triCls.begin(), end = triCls.end(); it != end; it++) {
        if (it->lit2 == lit2
            && it->lit3 == lit3) return;
    }

    triCls.push_back(TriClause(lit1, lit2, lit3));
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
    if (sendData != NULL) {
        int numFinished = 0;
        for (int i = 1; i < mpiSize; i++) {
            if (sendRequestsFinished[i]) {
                numFinished++;
                continue;
            }
            MPI_Status status;
            int flag;
            int err = MPI_Test(&(sendRequests[i]), &flag, &status);
            assert(err == MPI_SUCCESS);
            if (flag == true) {
                int err = MPI_Request_free(&(sendRequests[i]));
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
    uint32_t thisSentTriData = 0;
    vector<uint32_t> data;
    data.push_back((uint32_t)nVars);
    for (Var var = 0; var < nVars; var++) {
        data.push_back((uint32_t)value[var].getchar());
    }

    data.push_back((uint32_t)nVars*2);
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

    data.push_back((uint32_t)nVars*2);
    at = 0;
    for(vector<vector<TriClause> >::const_iterator it = tris.begin(), end = tris.end(); it != end; it++, at++) {
        const vector<TriClause>& triSet = *it;
        assert(triSet.size() >= syncMPIFinishTri[at]);
        uint32_t sizeToSend = triSet.size() - syncMPIFinishTri[at];
        data.push_back(sizeToSend);
        for (uint32_t i = syncMPIFinishTri[at]; i < triSet.size(); i++) {
            data.push_back(triSet[i].lit2.toInt());
            data.push_back(triSet[i].lit3.toInt());
            thisSentTriData++;
        }
        syncMPIFinishTri[at] = triSet.size();
    }
    sentTriData += thisSentTriData;

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
        int err = MPI_Isend(0, 0, MPI_UNSIGNED, i, 1, MPI_COMM_WORLD, &(interruptRequests[i]));
        assert(err == MPI_SUCCESS);
        alreadySentInterrupt[i] = true;
    }
}

const bool DataSyncServer::actAsServer()
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

