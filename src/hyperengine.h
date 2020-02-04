/******************************************
Copyright (c) 2016, Mate Soos

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

class HyperEngine : public PropEngine {
public:
    HyperEngine(const SolverConf *_conf, Solver* solver, std::atomic<bool>* _must_interrupt_inter);
    ~HyperEngine() override;
    size_t mem_used() const;

    bool use_depth_trick = true;
    bool perform_transitive_reduction = true;
    bool timedOutPropagateFull = false;
    Lit propagate_bfs(
        const uint64_t earlyAborTOut = std::numeric_limits<uint64_t>::max()
    );
    set<BinaryClause> needToAddBinClause;       ///<We store here hyper-binary clauses to be added at the end of propagateFull()
    set<BinaryClause> uselessBin;

    ///Add hyper-binary clause given this bin clause
    void  add_hyper_bin(Lit p);

    ///Add hyper-binary clause given this large clause
    void  add_hyper_bin(Lit p, const Clause& cl);

    void  enqueue_with_acestor_info(const Lit p, const Lit ancestor, const bool redStep);

private:
    Lit   analyzeFail(PropBy propBy);
    Lit   remove_which_bin_due_to_trans_red(Lit conflict, Lit thisAncestor, const bool thisStepRed);
    void  remove_bin_clause(Lit lit);
    bool  is_ancestor_of(
        const Lit conflict
        , Lit thisAncestor
        , const bool thisStepRed
        , const bool onlyIrred
        , const Lit lookingForAncestor
    );

    //Find lowest common ancestor, once 'currAncestors' has been filled
    Lit deepest_common_ancestor();

    PropResult prop_bin_with_ancestor_info(
        const Lit p
        , const Watched* k
        , PropBy& confl
    );

    PropResult prop_normal_cl_with_ancestor_info(
        Watched* i
        , Watched*& j
        , const Lit p
        , PropBy& confl
    );

    vector<Lit> currAncestors;
};

}
