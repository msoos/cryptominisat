/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2013, Mate Soos. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
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
#include "shareddata.h"

namespace CMSat {

class Solver;
class DataSync
{
    public:
        DataSync(Solver* solver, SharedData* sharedData);
        void new_var();
        bool syncData();
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
        const Stats& getStats() const;

    private:
        //functions
        bool shareUnitData();
        bool syncBinFromOthers(const Lit lit, const vector<Lit>& bins, uint32_t& finished, watch_subarray ws);
        void syncBinToOthers();
        void addOneBinToOthers(const Lit lit1, const Lit lit2);
        bool shareBinData();

        //stuff to sync
        vector<std::pair<Lit, Lit> > newBinClauses;

        //stats
        uint64_t lastSyncConf = 0;
        vector<uint32_t> syncFinish;
        Stats stats;

        //misc
        vector<Lit> toClear;
        vector<char> seen;

        //main data
        Solver* solver;
        SharedData* sharedData;
};

inline const DataSync::Stats& DataSync::getStats() const
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

inline void DataSync::signalNewBinClause(Lit lit1, Lit lit2)
{
    if (sharedData == NULL) {
        return;
    }
    if (lit1.toInt() > lit2.toInt()) {
        std::swap(lit1, lit2);
    }
    newBinClauses.push_back(std::make_pair(lit1, lit2));
}


}
