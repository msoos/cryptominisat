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

#ifndef DATASYNC_H
#define DATASYNC_H

#include "SharedData.h"
#include "Solver.h"

#ifdef USE_MPI
#include "mpi.h"
#endif //USE_MPI

class DataSync
{
    public:
        DataSync(Solver& solver, SharedData* sharedData);
        void newVar();
        const bool syncData();

        //New clause signalation
        template <class T> void signalNewBinClause(const T& ps, const bool learnt = true);
        void signalNewBinClause(Lit lit1, Lit lit2, const bool learnt = true);
        template <class T> void signalNewTriClause(const T& ps, const bool learnt = true);
        void signalNewTriClause(const Lit lit1, const Lit lit2, const Lit lit3, const bool learnt = true);

        //Get methods
        const uint32_t getSentUnitData() const;
        const uint32_t getRecvUnitData() const;
        const uint32_t getSentBinData() const;
        const uint32_t getRecvBinData() const;
        const uint32_t getSentTriData() const;
        const uint32_t getRecvTriData() const;
        const int getThreadAddingVars() const;
        const bool getEREnded() const;

    private:
        //unitary shring functions
        const bool shareUnitData();
        const bool syncUnit(const lbool otherVal, const Var var, SharedData* shared, uint32_t& thisGotUnitData, uint32_t& thisSentUnitData);
        uint32_t sentUnitData;
        uint32_t recvUnitData;

        //bin sharing functions
        const bool    shareBinData();
        vector<BinClause> newBinClauses;
        const bool    syncBinFromOthers(const Lit lit, const vector<BinClause>& bins, uint32_t& finished);
        void          syncBinToOthers();
        void          addOneBinToOthers(const Lit lit1, const Lit lit2, const bool leanrt);
        vec<uint32_t> syncFinish;
        uint32_t      sentBinData;
        uint32_t      recvBinData;

        //tri sharing functions
        const bool        shareTriData();
        vector<TriClause> newTriClauses;
        const bool        syncTriFromOthers(const Lit lit1, const vector<TriClause>& tris, uint32_t& finished);
        void              syncTriToOthers();
        void              addOneTriToOthers(const Lit lit1, const Lit lit2, const Lit lit3);
        vec<uint32_t>     syncFinishTri;
        uint32_t          sentTriData;
        uint32_t          recvTriData;

        #ifdef USE_MPI
        //MPI
        const bool    syncFromMPI();
        void          syncToMPI();
        void          getNeedToInterruptFromMPI();
        vec<uint32_t> syncMPIFinish;
        vec<uint32_t> syncMPIFinishTri;
        MPI_Request   sendReq;
        uint32_t*     mpiSendData;

        int           mpiRank;
        int           mpiSize;
        uint32_t      mpiRecvUnitData;
        uint32_t      mpiRecvBinData;
        uint32_t      mpiSentBinData;
        uint32_t      mpiRecvTriData;
        uint32_t      mpiSentTriData;
        #endif

        //ER sharing
        void syncERVarsToHere();
        void syncERVarsFromHere();

        //misc
        vec<char> seen;

        //main data
        SharedData* sharedData;
        Solver& solver;
        uint32_t numCalls;
        uint64_t lastSyncConf;
};

inline const uint32_t DataSync::getSentUnitData() const
{
    return sentUnitData;
}

inline const uint32_t DataSync::getRecvUnitData() const
{
    return recvUnitData;
}

inline const uint32_t DataSync::getSentBinData() const
{
    return sentBinData;
}

inline const uint32_t DataSync::getRecvBinData() const
{
    return recvBinData;
}

inline const uint32_t DataSync::getSentTriData() const
{
    return sentTriData;
}

inline const uint32_t DataSync::getRecvTriData() const
{
    return recvTriData;
}

inline const int DataSync::getThreadAddingVars() const
{
    if (sharedData == NULL) return 0;
    else return sharedData->threadAddingVars;
}

inline const bool DataSync::getEREnded() const
{
    if (sharedData == NULL) return false;
    return sharedData->EREnded;
}

#endif //DATASYNC_H
