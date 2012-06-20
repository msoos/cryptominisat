/***************************************************************************************[Solver.cc]
Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
Copyright (c) 2007-2009, Niklas Sorensson
Copyright (c) 2009-2012, Mate Soos

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/

#include "cmsat/SharedData.h"
#include "cmsat/Solver.h"

namespace CMSat {

class DataSync
{
    public:
        DataSync(Solver& solver, SharedData* sharedData);
        void newVar();
        bool syncData();


        template <class T> void signalNewBinClause(T& ps);
        void signalNewBinClause(Lit lit1, Lit lit2);

        uint32_t getSentUnitData() const;
        uint32_t getRecvUnitData() const;
        uint32_t getSentBinData() const;
        uint32_t getRecvBinData() const;

    private:
        //functions
        bool shareUnitData();
        bool syncBinFromOthers(const Lit lit, const vector<Lit>& bins, uint32_t& finished, vec<Watched>& ws);
        void syncBinToOthers();
        void addOneBinToOthers(const Lit lit1, const Lit lit2);
        bool shareBinData();

        //stuff to sync
        vector<std::pair<Lit, Lit> > newBinClauses;

        //stats
        uint64_t lastSyncConf;
        vec<uint32_t> syncFinish;
        uint32_t sentUnitData;
        uint32_t recvUnitData;
        uint32_t sentBinData;
        uint32_t recvBinData;

        //misc
        vec<char> seen;

        //main data
        SharedData* sharedData;
        Solver& solver;
};

inline uint32_t DataSync::getSentUnitData() const
{
    return sentUnitData;
}

inline uint32_t DataSync::getRecvUnitData() const
{
    return recvUnitData;
}

inline uint32_t DataSync::getSentBinData() const
{
    return sentBinData;
}

inline uint32_t DataSync::getRecvBinData() const
{
    return recvBinData;
}

template <class T>
inline void DataSync::signalNewBinClause(T& ps)
{
    if (sharedData == NULL) return;
    assert(ps.size() == 2);
    signalNewBinClause(ps[0], ps[1]);
}

inline void DataSync::signalNewBinClause(Lit lit1, Lit lit2)
{
    if (sharedData == NULL) return;
    if (lit1.toInt() > lit2.toInt()) std::swap(lit1, lit2);
    newBinClauses.push_back(std::make_pair(lit1, lit2));
}

}
