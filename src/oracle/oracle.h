// SharpSAT-TD is a modification of SharpSAT (MIT License, 2019 Marc Thurley).
//
// SharpSAT-TD -- Copyright (c) 2021 Tuukka Korhonen
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
// OR OTHER DEALINGS IN THE SOFTWARE.

#pragma once

#include <array>
#include <iostream>
#include <vector>
#include <string>
#include <cassert>
#include <limits>
#include <algorithm>
#include <chrono>
#include <random>
/* #define DEBUG_ORACLE_VERB */

#ifdef DEBUG_ORACLE_VERB
#define oclv(x) do {cout << x << endl;} while(0)
#define oclv2(x) do {cout << x;} while(0)
#else
#define oclv(x) do {} while(0)
#define oclv2(x) do {} while(0)
#endif

using std::array;

namespace sspp {
typedef int Lit;
typedef int Var;

using std::vector;
using std::string;

inline Lit Neg(Lit x) {
	return x^1;
}

inline Var VarOf(Lit x) {
	return x/2;
}

inline Lit PosLit(Var x) {
	return x*2;
}

inline Lit NegLit(Var x) {
	return x*2+1;
}

inline Lit MkLit(Var var, bool phase) {
	if (phase) {
		return PosLit(var);
	} else {
		return NegLit(var);
	}
}

inline bool IsPos(Lit x) {
	return !(x&1);
}

inline bool IsNeg(Lit x) {
	return x&1;
}

inline vector<Lit> Negate(vector<Lit> vec) {
	for (Lit& lit : vec) {
		lit = Neg(lit);
	}
	return vec;
}

template<typename T>
inline T RandInt(T a, T b, std::mt19937& gen) {
	return std::uniform_int_distribution<T>(a,b)(gen);
}

template<typename T>
void SwapDel(vector<T>& vec, size_t i) {
	assert(i < vec.size());
	std::swap(vec[i], vec.back());
	vec.pop_back();
}

namespace oracle {

struct TriState {
    TriState() = default;

    TriState(bool _b) {
        if (_b) val = 1;
        else val = 0;
    }

    static TriState unknown() {
        TriState tmp;
        tmp.val = 2;
        return tmp;
    }

    bool isTrue() const {return val == 1;}
    bool isFalse() const {return val == 0;}
    bool isUnknown() const {return val == 2;}
    int val;
};

inline std::ostream& operator<<(std::ostream& os, const TriState& ts) {
    if (ts.isTrue()) os << "true";
    else if (ts.isFalse()) os << "false";
    else if (ts.isUnknown()) os << "unknown";
    else assert(false);
    return os;
}

struct Stats {
    int64_t mems = 0;
    int64_t decisions = 0;
    int64_t propagations = 0;
    int64_t learned_clauses = 0;
    int64_t learned_bin_clauses = 0;
    int64_t learned_units = 0;
    int64_t conflicts = 0;
    int64_t nontriv_redu = 0;
    int64_t forgot_clauses = 0;
    int64_t restarts = 0;
    int64_t cache_useful = 0;
    int64_t cache_added = 0;
    int64_t total_cache_lookups = 0;
    void Print() const;
};

struct Watch {
    // should align to 8+4+4=16 bytes
    size_t cls;
    Lit blit; // blocked literal
    int size;
};

struct VarC {
    size_t reason = 0;
    int level = 0;
    char phase = 0;
};

struct CInfo {
    size_t pt;
    int glue;
    int used;
    uint32_t total_used;
    bool Keep() const;
};

// Mirrors CMS's FastBackwData but in oracle's Lit/var space.
// Used by Oracle::SlowBackwSolve to run a single persistent-stack
// solve session that classifies many test_vars as indep / not-indep
// without ever tearing down the assumption stack between tests.
struct SlowBackwData {
    // Mutable assumption list. Layout:
    //   [0 .. indep_vars->size())                 — confirmed-indep indicators
    //   [indep_vars->size() .. size()-2)          — unknown indicators (still to test)
    //   [size()-2, size()-1)                      — current test pair
    //                                              (test_var positive,
    //                                               test_var+orig_num_vars negative)
    std::vector<Lit>* _assumptions = nullptr;
    // Map: indic-var (oracle internal) -> caller's "real" var index
    const std::vector<int>* indic_to_var = nullptr;
    // For each caller var, the two oracle Lits that form its test pair.
    // test_pos_lit[v] = oracle Lit (v positive)
    // test_dual_neg_lit[v] = oracle Lit (v+orig_num_vars negative)
    const std::vector<Lit>* test_pos_lit = nullptr;
    const std::vector<Lit>* test_dual_neg_lit = nullptr;
    // Output: caller's "real" var indices that were classified
    std::vector<int>* non_indep_vars = nullptr;
    // Output: caller's "real" var indices marked indep. We splice into here
    // and into _assumptions in lockstep.
    std::vector<int>* indep_vars = nullptr;
    // Current test_var info. Set by SlowBackwSolve as it walks tests.
    int* test_indic = nullptr;
    int* test_var = nullptr;
    // Per-test conflict budget. Halved by the solver if progress is slow.
    int64_t max_confl = 500;
    int64_t cur_max_confl = 0;
    int64_t indep_because_ran_out_of_confl = 0;
    int64_t start_sumConflicts = 0;
};

class Oracle {
public:
    Oracle(int vars_, const vector<vector<Lit>>& clauses_);
    Oracle(int vars_, const vector<vector<Lit>>& clauses_, const vector<vector<Lit>>& learned_clauses_);

    void SetAssumpLit(Lit lit, bool freeze);
    void SetVerbosity(uint32_t _verb) { verb=_verb;}
    void SetCacheCutoff(uint32_t c) { cache_cutoff = c; }
    int GetCacheLookupVar() const { return cache_lookup_var; }
    TriState Solve(const vector<Lit>& assumps, bool usecache=true, int64_t max_mems = 1000ULL*1000LL*1000LL);
    // Vivify all learned clauses: for each long learned clause, try to
    // remove literals by testing F ∧ NOT(clause \ {p}) for UNSAT. Must be
    // called at root level (CurLevel() == 1). Safe to call any time; if
    // the oracle is already UNSAT, this is a no-op. Returns the number of
    // lits removed across all vivified clauses.
    int Vivify(int64_t max_mems = 200LL*1000LL*1000LL);
    // Failed literal probing. At root level, for each unassigned var v
    // try v=TRUE and v=FALSE; if either side propagates to a conflict,
    // freeze the other polarity as a root unit. Returns the number of
    // units learned. Budget-capped.
    int FailedLiteralProbe(int64_t max_mems = 100LL*1000LL*1000LL);
    // Bounded variable elimination (BVE). For each variable v where
    // eliminable[v] is true and where the number of non-tautological
    // resolvents does not exceed (#pos + #neg + grow_cap), remove v and
    // replace its clauses with the resolvents. Caller is responsible for
    // ensuring eliminable[v] is ONLY true for variables that are NOT
    // needed downstream (e.g., for arjun: vars that are known not to be
    // in the independent support). Can be called between SlowBackwSolve
    // sessions — resets to root level automatically if needed. Returns
    // the number of variables eliminated.
    int BVE(const vector<bool>& eliminable, int grow_cap = 0,
            int64_t max_mems = 500LL*1000LL*1000LL);
    // SCC-based equivalent literal replacement. Finds strongly connected
    // components in the binary implication graph and replaces equivalent
    // literals throughout the clause DB. Must be called at root level.
    // Returns number of variables eliminated via equivalence.
    int SCCEquivLitElim();
    // Persistent-stack independence-test solve. See SlowBackwData above.
    // Mirrors CMSat::Searcher::find_fast_backw + new_decision_fast_backw,
    // but using the oracle's CDCL machinery.
    // Returns:
    //   true  — all tests classified, _assumptions is at the indep boundary
    //   false — formula became globally UNSAT
    //   unknown — mems budget exceeded; caller can resume by calling again
    TriState SlowBackwSolve(SlowBackwData& d, int64_t max_mems = 1000ULL*1000LL*1000LL);
    bool FreezeUnit(Lit unit);
    bool AddClauseIfNeededAndStr(vector<Lit> clause, bool entailed);
    void AddClause(const vector<Lit>& clause, bool entailed);
    void PrintStats() const;
    vector<vector<Lit>> GetLearnedClauses() const;
    vector<Lit> GetLearnedUnits(int max_var) const;

    int CurLevel() const;
    int LitVal(Lit lit) const;
    // Get the phase (polarity) of a variable from the last solution.
    // Returns 1 for true, 0 for false. Only valid after a SAT Solve().
    int GetPhase(Var v) const {
        if (cached_solution) return cached_solution[v];
        return vs[v].phase;
    }
    const Stats& getStats() const {return stats;}
    void reset_mems() {stats.mems = 0;}

 private:
    uint32_t verb = 0;
    uint32_t cache_cutoff = 1000;
    size_t total_confls = 0;
    size_t last_db_clean = 0;

    void AddOrigClause(vector<Lit> clause, bool entailed);
    vector<Lit> clauses;
    vector<int> clause_pos; // watch replacement search start position (per clause start)
    vector<vector<Watch>> watches;
    vector<signed char> lit_val;
    vector<VarC> vs;

    bool unsat = false;
    const uint8_t* cached_solution = nullptr;

    const int vars;
    size_t orig_clauses_size = 0;
    Stats stats;
    vector<Lit> prop_q;
    vector<Var> decided;
    vector<char> in_cc;

    std::mt19937 rand_gen;

    int64_t redu_it = 1;
    vector<char> seen;
    vector<int64_t> redu_seen;
    vector<Lit> redu_s;

    // Poison/removable marks for conflict minimization memoization
    // 0=unmarked, 1=removable, 2=poison
    vector<char> minimize_mark;
    vector<Var> minimize_marked_vars; // track which vars to clear
    vector<Var> redu_visited; // scratch space for LitReduntant (avoid heap allocs)

    int64_t lvl_it = 1;
    vector<int64_t> lvl_seen; // for computing LBD

    vector<Lit> learned_units;

    int restart_factor = 0;
    vector<int> luby;
    void InitLuby();
    int NextLuby();

    size_t num_lbd2_red_cls = 0;
    size_t num_used_red_cls = 0;
    void ResizeClauseDb();
    void BumpClause(size_t cls);
    vector<CInfo> cla_info;

    double var_inc = 1;
    double var_fact = 0;
    size_t heap_N;
    vector<double> var_act_heap;
    void BumpVar(Var v);
    void ActivateActivity(Var v);
    Var PopVarHeap();

    bool LitSat(Lit lit) const;
    bool LitAssigned(Lit lit) const;
    void Assign(Lit dec, size_t reason_clause, int level);
    void Decide(Lit dec, int level);
    void UnDecide(int level);

    TriState HardSolve(int64_t max_mems = 1000LL*1000LL*1000LL, int64_t mems_startup = 0);
    // True if conflict
    size_t Propagate(int level);

    size_t AddLearnedClause(const vector<Lit>& clause);
    bool LitReduntant(Lit lit);
    void PopLit(vector<Lit>& clause, int& confl_levels, int& impl_lits, int level);
    vector<Lit> LearnUip(size_t conflict_clause);
    int CDCLBT(size_t confl_clause, int min_level=0);

    void rebuild_cache_lookup();
    vector<uint8_t> sol_cache; // Caches found FULL solutions
    array<vector<uint32_t>, 2> cache_lookup; //0/1 for literal cache_lookup_lit
    int cache_lookup_var = 0; // 0 = unset
    vector<uint64_t> cache_lookup_frequencies;
    void AddSolToCache();
    bool SatByCache(const vector<Lit>& assumps);
    void ClearSolCache();
    // Drop cached solutions that contradict a newly-frozen literal.
    // Must be called whenever a var becomes permanently assigned at level 1
    // after the cache has accumulated entries, otherwise SatByCache may
    // return a stale solution that violates the freeze.
    void PruneSolCacheForVar(Var v, uint8_t phase);
};


inline int Oracle::LitVal(Lit lit) const {
    return lit_val[lit];
}

inline bool Oracle::LitSat(Lit lit) const {
    return LitVal(lit) > 0;
}

inline bool Oracle::LitAssigned(Lit lit) const {
    return LitVal(lit) != 0;
}

// TOP level is 1.
inline int Oracle::CurLevel() const {
    if (decided.empty()) {
        return 1;
    }
    return vs[decided.back()].level;
}

inline void Oracle::Decide(Lit dec, int level) {
    assert(LitVal(dec) == 0);
    stats.decisions++;
    Assign(dec, 0, level);
}

inline void Oracle::AddClause(const vector<Lit>& clause, bool entailed) {
    AddOrigClause(clause, entailed);
}

inline void Oracle::PrintStats() const {
    stats.Print();
}

inline bool CInfo::Keep() const {
    return glue <= 2;
}

} // namespace oracle
} // namespace sspp
