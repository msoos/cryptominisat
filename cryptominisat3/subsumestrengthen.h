#include "solver.h"
#include "cloffset.h"
#include <vector>
using std::vector;

namespace CMSat {

class Simplifier;
class GateFinder;

class SubsumeStrengthen
{
public:
    SubsumeStrengthen(Simplifier* simplifier, Solver* solver);
    size_t memUsed() const;
    void performSubsumption();
    bool performStrengthening();
    uint32_t subsume0(ClOffset offset);
    //bool subsumeWithTris();

    struct Sub0Ret {
        Sub0Ret() :
            subsumedIrred(false)
            , numSubsumed(0)
        {}

        bool subsumedIrred;
        ClauseStats stats;
        uint32_t numSubsumed;
    };

    struct Sub1Ret {
        Sub1Ret() :
            sub(0)
            , str(0)
        {}

        Sub1Ret& operator+=(const Sub1Ret& other)
        {
            sub += other.sub;
            str += other.str;

            return *this;
        }

        size_t sub;
        size_t str;
    };

    template<class T>
    Sub0Ret subsume0AndUnlink(
        const ClOffset offset
        , const T& ps
        , const CL_ABST_TYPE abs
        , const bool removeImplicit = false
    );

    struct Stats
    {
        Stats() :
            subsumedBySub(0)
            , subsumedByStr(0)
            , litsRemStrengthen(0)

            , subsumeTime(0)
            , strengthenTime(0)
        {}

        Stats& operator+=(const Stats& other)
        {
            subsumedBySub += other.subsumedBySub;
            subsumedByStr += other.subsumedByStr;
            litsRemStrengthen += other.litsRemStrengthen;

            subsumeTime += other.subsumeTime;
            strengthenTime += other.strengthenTime;

            return *this;
        }

        void printShort() const
        {
            //STRENGTH + SUBSUME
            cout << "c [subs] long"
            << " subBySub: " << subsumedBySub
            << " subByStr: " << subsumedByStr
            << " lits-rem-str: " << litsRemStrengthen
            << " T: " << std::fixed << std::setprecision(2)
            << (subsumeTime+strengthenTime)
            << " s"
            << endl;
        }

        void print() const
        {
            cout << "c -------- SubsumeStrengthen STATS ----------" << endl;
            printStatsLine("c cl-subs"
                , subsumedBySub + subsumedByStr
                , " Clauses"
            );
            printStatsLine("c cl-str rem lit"
                , litsRemStrengthen
                , " Lits"
            );
            printStatsLine("c cl-sub T"
                , subsumeTime
                , " s"
            );
            printStatsLine("c cl-str T"
                , strengthenTime
                , " s"
            );
            cout << "c -------- SubsumeStrengthen STATS END ----------" << endl;
        }

        uint64_t subsumedBySub;
        uint64_t subsumedByStr;
        uint64_t litsRemStrengthen;

        double subsumeTime;
        double strengthenTime;
    };

    void finishedRun();
    const Stats& getStats() const;
    const Stats& getRunStats() const;

private:
    Stats globalstats;
    Stats runStats;

    Simplifier* simplifier;
    Solver* solver;

    void strengthen(ClOffset c, const Lit toRemoveLit);
    friend class GateFinder;

    template<class T>
    void findSubsumed0(
        const ClOffset offset
        , const T& ps
        , const CL_ABST_TYPE abs
        , vector<ClOffset>& out_subsumed
        , const bool removeImplicit = false
    );
    template<class T>
    size_t find_smallest_watchlist_for_clause(const T& ps) const;

    template<class T>
    void findStrengthened(
        const ClOffset offset
        , const T& ps
        , const CL_ABST_TYPE abs
        , vector<ClOffset>& out_subsumed
        , vector<Lit>& out_lits
    );

    template<class T>
    void fillSubs(
        const ClOffset offset
        , const T& ps
        , CL_ABST_TYPE abs
        , vector<ClOffset>& out_subsumed
        , vector<Lit>& out_lits
        , const Lit lit
    );

    template<class T1, class T2>
    bool subset(const T1& A, const T2& B);

    template<class T1, class T2>
    Lit subset1(const T1& A, const T2& B);
    bool subsetAbst(const CL_ABST_TYPE A, const CL_ABST_TYPE B);
    Sub1Ret subsume1(ClOffset offset);

    vector<ClOffset> subs;
    vector<Lit> subsLits;
};

inline const SubsumeStrengthen::Stats& SubsumeStrengthen::getRunStats() const
{
    return runStats;
}

inline const SubsumeStrengthen::Stats& SubsumeStrengthen::getStats() const
{
    return globalstats;
}

} //end namespace
