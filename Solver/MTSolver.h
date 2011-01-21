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

#include "Solver.h"
#include "SharedData.h"
#include <string>

class MTSolver
{
public:
    MTSolver(const int numThreads, const SolverConf& conf = SolverConf(), const GaussConf& _gaussConfig = GaussConf());
    ~MTSolver();

    Var     newVar    (bool dvar = true);           // Add a new variable with parameters specifying variable mode.
    template<class T>
    bool    addClause (T& ps, const uint32_t group = 0, const char* group_name = NULL);  // Add a clause to the solver. NOTE! 'ps' may be shrunk by this method!
    template<class T>
    bool    addLearntClause(T& ps, const uint32_t group = 0, const char* group_name = NULL, const uint32_t glue = 10);
    template<class T>
    bool    addXorClause (T& ps, bool xorEqualFalse, const uint32_t group = 0, const char* group_name = NULL);  // Add a xor-clause to the solver. NOTE! 'ps' may be shrunk by this method!

    // Solving:
    //
    const lbool    solve       (const vec<Lit>& assumps); ///<Search for a model that respects a given set of assumptions.
    const lbool    solve       ();                        ///<Search without assumptions.
    const bool     okay        () const;                 ///<FALSE means solver is in a conflicting state

    // Variable mode:
    //
    void    setDecisionVar (Var v, bool b);         ///<Declare if a variable should be eligible for selection in the decision heuristic.

    // Read state:
    //
    const lbool        modelValue (const Lit p) const;   ///<The value of a literal in the last model. The last call to solve must have been satisfiable.
    const uint32_t     nAssigns   ()      const;         ///<The current number of assigned literals.
    const uint32_t     nClauses   ()      const;         ///<The current number of original clauses.
    const uint32_t     nLiterals  ()      const;         ///<The current number of total literals.
    const uint32_t     nLearnts   ()      const;         ///<The current number of learnt clauses.
    const uint32_t     nVars      ()      const;         ///<The current number of variables.

    // Extra results: (read-only member variable)
    //
    vec<lbool> model;             ///<If problem is satisfiable, this vector contains the model (if any).
    vec<Lit>   conflict;          ///<If problem is unsatisfiable (possibly under assumptions), this vector represent the final conflict clause expressed in the assumptions.

    //Logging
    void needStats();              // Prepares the solver to output statistics
    void needProofGraph();         // Prepares the solver to output proof graphs during solving
    void setVariableName(const Var var, const std::string& name); // Sets the name of the variable 'var' to 'name'. Useful for statistics and proof logs (i.e. used by 'logger')
    const vec<Clause*>& get_sorted_learnts(); //return the set of learned clauses, sorted according to the logic used in MiniSat to distinguish between 'good' and 'bad' clauses
    const vec<Clause*>& get_learnts() const; //Get all learnt clauses that are >1 long
    const vector<Lit> get_unitary_learnts() const; //return the set of unitary learnt clauses
    const uint32_t get_unitary_learnts_num() const; //return the number of unitary learnt clauses
    void dumpSortedLearnts(const std::string& fileName, const uint32_t maxSize); // Dumps all learnt clauses (including unitary ones) into the file
    void needLibraryCNFFile(const std::string& fileName); //creates file in current directory with the filename indicated, and puts all calls from the library into the file.
    void dumpOrigClauses(const std::string& fileNam) const;

    #ifdef USE_GAUSS
    const uint32_t get_sum_gauss_called() const;
    const uint32_t get_sum_gauss_confl() const;
    const uint32_t get_sum_gauss_prop() const;
    const uint32_t get_sum_gauss_unit_truths() const;
    #endif //USE_GAUSS

    //Printing statistics
    void printStats();
    const uint32_t getNumElimSubsume() const;       ///<Get number of variables eliminated
    const uint32_t getNumElimXorSubsume() const;    ///<Get number of variables eliminated with xor-magic
    const uint32_t getNumXorTrees() const;          ///<Get the number of trees built from 2-long XOR-s. This is effectively the number of variables that replace other variables
    const uint32_t getNumXorTreesCrownSize() const; ///<Get the number of variables being replaced by other variables
    /**
    @brief Get total time spent in Subsumer.

    This includes: subsumption, self-subsuming resolution, variable elimination,
    blocked clause elimination, subsumption and self-subsuming resolution
    using non-existent binary clauses.
    */
    const double getTotalTimeSubsumer() const;
    const double getTotalTimeFailedLitSearcher() const;
    const double getTotalTimeSCC() const;

    /**
    @brief Get total time spent in XorSubsumer.

    This included subsumption, variable elimination through XOR,
    and local&global substitution (see Heule's Thesis)
    */
    const double   getTotalTimeXorSubsumer() const;
    const uint32_t getVerbosity() const;
    void setNeedToInterrupt();
    const bool getNeedToDumpLearnts() const;
    const bool getNeedToDumpOrig() const;

    void printNumThreads() const;

    private:
        void setUpFinish(const lbool retVal, const int threadNum);

        SolverConf conf;
        GaussConf gaussConfig;


        int numThreads;
        void setupOneSolver(const int num, const uint32_t origSeed);
        std::vector<Solver*> solvers;
        SharedData sharedData;
        int finishedThread;
};

inline const uint32_t MTSolver::nVars() const
{
    return solvers[0]->nVars();
}

inline const uint32_t MTSolver::getVerbosity() const
{
    return conf.verbosity;
}

inline const bool MTSolver::getNeedToDumpLearnts() const
{
    return conf.needToDumpLearnts;
}

inline const bool MTSolver::getNeedToDumpOrig() const
{
    return conf.needToDumpOrig;
}

#ifdef USE_GAUSS
inline const uint32_t MTSolver::get_sum_gauss_called() const
{
    return solvers[finishedThread]->get_sum_gauss_called();
}

inline const uint32_t MTSolver::get_sum_gauss_confl() const
{
    return solvers[finishedThread]->get_sum_gauss_confl();
}

inline const uint32_t MTSolver::get_sum_gauss_prop() const
{
    return solvers[finishedThread]->get_sum_gauss_prop();
}

inline const uint32_t MTSolver::get_sum_gauss_unit_truths() const
{
    return solvers[finishedThread]->get_sum_gauss_unit_truths();
}

#endif //USE_GAUSS


inline const lbool MTSolver::modelValue (const Lit p) const
{
    return solvers[finishedThread]->modelValue(p);
}

inline const uint32_t MTSolver::nAssigns() const
{
    return solvers[finishedThread]->nAssigns();
}

inline const uint32_t MTSolver::nClauses() const
{
    return solvers[finishedThread]->nClauses();
}

inline const uint32_t MTSolver::nLiterals() const
{
    return solvers[finishedThread]->nLiterals();
}

inline const uint32_t MTSolver::nLearnts() const
{
    return solvers[finishedThread]->nLearnts();
}

inline const bool MTSolver::okay() const
{
    return solvers[finishedThread]->okay();
}

inline const lbool MTSolver::solve()
{
    vec<Lit> tmp;
    return solve(tmp);
}

inline const uint32_t MTSolver::getNumElimSubsume() const
{
    return solvers[finishedThread]->getNumElimSubsume();
}

inline const uint32_t MTSolver::getNumElimXorSubsume() const
{
    return solvers[finishedThread]->getNumElimXorSubsume();
}

inline const uint32_t MTSolver::getNumXorTrees() const
{
    return solvers[finishedThread]->getNumXorTrees();
}

inline const uint32_t MTSolver::getNumXorTreesCrownSize() const
{
    return solvers[finishedThread]->getNumXorTreesCrownSize();
}

inline const double MTSolver::getTotalTimeSubsumer() const
{
    return solvers[finishedThread]->getTotalTimeSubsumer();
}

inline const double MTSolver::getTotalTimeFailedLitSearcher() const
{
    return solvers[finishedThread]->getTotalTimeFailedLitSearcher();
}

inline const double MTSolver::getTotalTimeSCC() const
{
    return solvers[finishedThread]->getTotalTimeSCC();
}

inline const double MTSolver::getTotalTimeXorSubsumer() const
{
    return solvers[finishedThread]->getTotalTimeXorSubsumer();
}
