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

#ifndef DATASYNC_SERVER_H
#define DATASYNC_SERVER_H

#include <vector>
#include "mpi.h"

#include "SolverTypes.h"
#include "SharedData.h"

class DataSyncServer {
    public:
        DataSyncServer();
        const bool actAsServer();
    private:
        const bool syncFromMPI();
        void addOneBinToOthers(const Lit lit1, const Lit lit2);
        void addOneTriToOthers(const Lit lit1, const Lit lit2, const Lit lit3);
        void sendDataToAll();
        void forwardNeedToInterrupt();

        std::vector<uint32_t> syncMPIFinish;
        std::vector<uint32_t> syncMPIFinishTri;
        std::vector<std::vector<Lit> > bins;
        std::vector<std::vector<TriClause> > tris;
        std::vector<lbool> value;

        bool ok;

        uint32_t *sendData;

        std::vector<MPI_Request> sendRequests;
        std::vector<bool> sendRequestsFinished;

        std::vector<bool> alreadyInterrupted;
        std::vector<bool> alreadySentInterrupt;
        std::vector<MPI_Request> interruptRequests;
        int numAlreadyInterrupted;

        int mpiSize;
        uint32_t nVars;
        uint32_t recvBinData;
        uint32_t sentBinData;
        uint32_t recvTriData;
        uint32_t sentTriData;
        uint32_t numGotPacket;
        uint32_t lastSendNumGotPacket;
};

#endif //DATASYNC_SERVER_H