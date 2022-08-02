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

#ifndef _DATASYNC_H_
#define _DATASYNC_H_

#include "solvertypes.h"
#include "watched.h"
#include "propby.h"
#include "watcharray.h"
#ifdef USE_MPI
#include "mpi.h"
#endif //USE_MPI

namespace CMSat {

class Clause;
class SharedData;
class Solver;
class DataSync
{
    public:
        DataSync(Solver* solver, SharedData* sharedData);
        void finish_up_mpi();
        bool enabled();
        void set_shared_data(SharedData* sharedData);
        void new_var(const bool bva);
        void new_vars(const size_t n);
        bool syncData();
        void save_on_var_memory();
        void rebuild_bva_map();
        void updateVars(
           const vector<uint32_t>& outerToInter
            , const vector<uint32_t>& interToOuter
        );
        void signal_new_long_clause(const vector<Lit>& clause);

        #ifdef USE_GPU
        vector<Lit> clause_tmp;
        vector<Lit> trail_tmp;
        void unsetFromGpu(uint32_t level);
        void trySendAssignmentToGpu();
        PropBy pop_clauses();
        uint32_t signalled_gpu_long_cls = 0;
        uint32_t popped_clause = 0;
        #endif

        struct Stats
        {
            uint32_t sentUnitData = 0;
            uint32_t recvUnitData = 0;
            uint32_t sentBinData = 0;
            uint32_t recvBinData = 0;
        };
        const Stats& get_stats() const;

    private:
        void extend_bins_if_needed();
        Lit map_outer_to_outside(Lit lit) const;
        bool shareUnitData();
        bool shareBinData();
        bool syncBinFromOthers();
        bool syncBinFromOthers(const Lit lit, const vector<Lit>& bins, uint32_t& finished, watch_subarray ws);
        void syncBinToOthers();
        void clear_set_binary_values();
        bool add_bin_to_threads(const Lit lit1, const Lit lit2);
        void signal_new_bin_clause(Lit lit1, Lit lit2);
        void rebuild_bva_map_if_needed();


        #ifdef USE_GPU
        uint32_t trailCopiedUntil = 0;
        #endif
        int thread_id = -1;

        //stuff to sync
        vector<std::pair<Lit, Lit> > newBinClauses;

        //stats
        uint64_t lastSyncConf = 0;
        vector<uint32_t> syncFinish;
        Stats stats;

        //Other systems
        Solver* solver = NULL;
        SharedData* sharedData = NULL;

        //MPI
        #ifdef USE_MPI
        void set_up_for_mpi();
        bool mpi_recv_from_others();
        void mpi_send_to_others();
        bool mpi_get_interrupt();
        bool mpi_get_unit(
            const lbool otherVal,
            const uint32_t var,
            uint32_t& thisGotUnitData
        );
        vector<uint32_t> syncMPIFinish;
        MPI_Request   sendReq;
        uint32_t*     mpiSendData = NULL;

        int           mpiRank = 0;
        int           mpiSize = 0;
        uint32_t      mpiRecvUnitData = 0;
        uint32_t      mpiRecvBinData = 0;
        uint32_t      mpiSentBinData = 0;
        #endif


        //misc
        uint32_t numCalls = 0;
        vector<uint32_t>& seen;
        vector<Lit>& toClear;
        vector<uint32_t> outer_to_without_bva_map;
        bool must_rebuild_bva_map = false;
};

inline const DataSync::Stats& DataSync::get_stats() const
{
    return stats;
}

inline Lit DataSync::map_outer_to_outside(const Lit lit) const
{
    return Lit(outer_to_without_bva_map[lit.var()], lit.sign());

}

inline bool DataSync::enabled()
{
    return sharedData != NULL;
}

}

#endif
