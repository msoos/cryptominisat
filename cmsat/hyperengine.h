#include "cnf.h"
#include "propby.h"
#include "solvertypes.h"
#include <vector>
#include <set>
#include "propengine.h"
#include "mystack.h"


using std::vector;
using std::set;

namespace CMSat {
using namespace CMSat;

struct HyperEngine : public PropEngine {
    HyperEngine(ClauseAllocator* clAllocator, const SolverConf& _conf);

    //For proiorty propagations
    //
    MyStack<Lit> toPropNorm;
    MyStack<Lit> toPropBin;
    MyStack<Lit> toPropRedBin;

    vector<Lit> currAncestors;

    //Find lowest common ancestor, once 'currAncestors' has been filled
    Lit deepestCommonAcestor();

    ///Add hyper-binary clause given the ancestor filled in 'currAncestors'
    void  addHyperBin(Lit p);

    ///Add hyper-binary clause given this tri-clause
    void  addHyperBin(Lit p, Lit lit1, Lit lit2);

    ///Add hyper-binary clause given this large clause
    void  addHyperBin(Lit p, const Clause& cl);

    PropResult propBin(
        const Lit p
        , vec<Watched>::const_iterator k
        , PropBy& confl
    );
    PropResult propTriClauseComplex(
        const vec<Watched>::const_iterator i
        , const Lit lit1
        , PropBy& confl
        , Solver* solver
    );
    PropResult propNormalClauseComplex(
        const vec<Watched>::iterator i
        , vec<Watched>::iterator &j
        , const Lit p
        , PropBy& confl
        , Solver* solver
    );
    PropResult propTriHelperComplex(
        const Lit lit1
        , const Lit lit2
        , const Lit lit3
        , const bool red
    );
    Lit prop_red_bin_dfs(
        StampType stampType
        , PropBy& confl
        , Lit& root
        , bool& restart
    );

    bool timedOutPropagateFull;
    Lit propagateFullBFS(const uint64_t earlyAborTOut = std::numeric_limits<uint64_t>::max());
    Lit propagateFullDFS(
        StampType stampType
        , uint64_t earlyAborTOut = std::numeric_limits<uint64_t>::max()
    );
    void closeAllTimestamps(const StampType stampType);
    set<BinaryClause> needToAddBinClause;       ///<We store here hyper-binary clauses to be added at the end of propagateFull()
    set<BinaryClause> uselessBin;

    ///Find which literal should be set when we have failed
    ///i.e. reached conflict at decision at decision lvl 1
    Lit analyzeFail(PropBy propBy);

    Lit   removeWhich(Lit conflict, Lit thisAncestor, const bool thisStepRed);
    void  remove_bin_clause(Lit lit);
    bool  isAncestorOf(
        const Lit conflict
        , Lit thisAncestor
        , const bool thisStepRed
        , const bool onlyIrred
        , const Lit lookingForAncestor
    );

    size_t getUsedMem()
    {
        size_t mem;
        mem += toPropNorm.capacity()*sizeof(Lit);
        mem += toPropBin.capacity()*sizeof(Lit);
        mem += toPropRedBin.capacity()*sizeof(Lit);

        return mem;
    }

    uint64_t stampingTime;
};

}