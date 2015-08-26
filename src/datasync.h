/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2015, Mate Soos. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation
 * version 2.0 of the License.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301  USA
*/

#include "solvertypes.h"
#include "watched.h"
#include "watcharray.h"

namespace CMSat {

class SharedData;
class Solver;
class DataSync
{
    public:
        DataSync(Solver* solver, SharedData* sharedData);
        bool enabled();
        void new_var(const bool bva);
        void new_vars(const size_t n);
        bool syncData();
        void save_on_var_memory();
        void rebuild_bva_map();
        void updateVars(
           const vector<uint32_t>& outerToInter
            , const vector<uint32_t>& interToOuter
        );

        template <class T> void signalNewBinClause(T& ps);
        void signalNewBinClause(Lit lit1, Lit lit2);

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
        Lit map_outside_without_bva(Lit lit) const;
        bool shareUnitData();
        bool syncBinFromOthers();
        bool syncBinFromOthers(const Lit lit, const vector<Lit>& bins, uint32_t& finished, watch_subarray ws);
        void syncBinToOthers();
        void clear_set_binary_values();
        void addOneBinToOthers(const Lit lit1, const Lit lit2);
        bool shareBinData();

        //stuff to sync
        vector<std::pair<Lit, Lit> > newBinClauses;

        //stats
        uint64_t lastSyncConf = 0;
        vector<uint32_t> syncFinish;
        Stats stats;

        //Other systems
        Solver* solver;
        SharedData* sharedData;

        //misc
        vector<uint16_t>& seen;
        vector<Lit>& toClear;
        vector<uint32_t> outer_to_without_bva_map;
        bool must_rebuild_bva_map = false;
};

inline const DataSync::Stats& DataSync::get_stats() const
{
    return stats;
}

template <class T>
inline void DataSync::signalNewBinClause(T& ps)
{
    if (sharedData == NULL) {
        return;
    }
    //assert(ps.size() == 2);
    signalNewBinClause(ps[0], ps[1]);
}

inline Lit DataSync::map_outside_without_bva(const Lit lit) const
{
    return Lit(outer_to_without_bva_map[lit.var()], lit.sign());

}

inline bool DataSync::enabled()
{
    return sharedData != NULL;
}

}
