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

#ifndef DATASYNC_SERVER_H
#define DATASYNC_SERVER_H

#include <vector>
#include "mpi.h"

#include "solvertypes.h"
#include "shareddata.h"

namespace CMSat {

class DataSyncServer {
    public:
        DataSyncServer();
        bool actAsServer();
        void print_solution();
        void send_cnf_to_solvers();
        void add_clause(const vector<Lit>& lits);
        void new_vars(uint32_t i);
        void new_var();

    private:
        void syncFromMPI();
        void addOneBinToOthers(const Lit lit1, const Lit lit2);
        void sendDataToAll();
        void check_interrupt_and_forward_to_all();

        std::vector<uint32_t> syncMPIFinish;
        std::vector<std::vector<Lit> > bins;
        std::vector<lbool> value;
        std::vector<Lit> clauses_array;

        uint32_t *sendData = NULL;

        std::vector<MPI_Request> sendRequests;
        std::vector<bool> sendRequestsFinished;

        std::vector<bool> alreadyInterrupted;
        std::vector<bool> alreadySentInterrupt;
        std::vector<MPI_Request> interruptRequests;
        int numAlreadyInterrupted;

        vector<lbool> solution;

        int mpiSize;
        bool ok = true;
        uint32_t nVars = 0;
        uint32_t recvBinData = 0;
        uint32_t sentBinData = 0;
        uint32_t numGotPacket = 0;
        uint32_t lastSendNumGotPacket = 0;
};

}

#endif //DATASYNC_SERVER_H
