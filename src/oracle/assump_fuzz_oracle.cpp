#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cstdint>
#include <cassert>
#include <random>
#include <algorithm>
#include <set>

#include "argparse.hpp"
#include "oracle.h"
#include "cadical.hpp"

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::vector;

using sspp::Lit;
using sspp::Var;
using sspp::PosLit;
using sspp::NegLit;

// Convert DIMACS literal (1-based, sign) to oracle Lit (even/odd encoding)
static Lit dimacs_to_oracle_lit(int d) {
    assert(d != 0);
    if (d > 0) return PosLit(d);
    else return NegLit(-d);
}

struct CNF {
    int num_vars = 0;
    int num_clauses = 0;
    vector<vector<Lit>> clauses;      // oracle format
    vector<vector<int>> dimacs_clauses; // DIMACS format (for cadical)
};

static CNF parse_dimacs(const string& filename) {
    CNF cnf;
    std::ifstream in(filename);
    if (!in.is_open()) {
        cerr << "ERROR: Cannot open file: " << filename << endl;
        exit(1);
    }

    string line;
    bool header_seen = false;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        if (line[0] == 'c') continue;
        if (line[0] == 'p') {
            std::istringstream iss(line);
            string p, fmt;
            iss >> p >> fmt >> cnf.num_vars >> cnf.num_clauses;
            header_seen = true;
            continue;
        }
        if (!header_seen) continue;

        std::istringstream iss(line);
        int lit;
        vector<Lit> clause;
        vector<int> dimacs_clause;
        while (iss >> lit) {
            if (lit == 0) {
                if (!clause.empty()) {
                    cnf.clauses.push_back(std::move(clause));
                    cnf.dimacs_clauses.push_back(std::move(dimacs_clause));
                    clause.clear();
                    dimacs_clause.clear();
                }
            } else {
                clause.push_back(dimacs_to_oracle_lit(lit));
                dimacs_clause.push_back(lit);
            }
        }
        if (!clause.empty()) {
            while (std::getline(in, line)) {
                if (line.empty()) continue;
                if (line[0] == 'c') continue;
                std::istringstream iss2(line);
                while (iss2 >> lit) {
                    if (lit == 0) {
                        cnf.clauses.push_back(std::move(clause));
                        cnf.dimacs_clauses.push_back(std::move(dimacs_clause));
                        clause.clear();
                        dimacs_clause.clear();
                        break;
                    }
                    clause.push_back(dimacs_to_oracle_lit(lit));
                    dimacs_clause.push_back(lit);
                }
                if (clause.empty()) break;
            }
            if (!clause.empty()) {
                cnf.clauses.push_back(std::move(clause));
                cnf.dimacs_clauses.push_back(std::move(dimacs_clause));
            }
        }
    }
    return cnf;
}

// Verify that solution satisfies all clauses (DIMACS format)
static bool verify_solution_against_clauses(
    const vector<vector<int>>& dimacs_clauses,
    const vector<int>& model, // model[var] = +var or -var
    int num_vars,
    const vector<int>& assumptions)
{
    // Build assignment: assignment[var] = true/false
    vector<int8_t> assignment(num_vars + 1, 0); // 0=unset, 1=true, -1=false
    for (int v = 1; v <= num_vars; v++) {
        assignment[v] = (model[v] > 0) ? 1 : -1;
    }

    // Check assumptions are respected
    for (int alit : assumptions) {
        int var = abs(alit);
        bool expected = (alit > 0);
        if ((assignment[var] == 1) != expected) {
            cerr << "BUG: assumption " << alit << " not respected in model" << endl;
            return false;
        }
    }

    // Check all clauses
    for (size_t ci = 0; ci < dimacs_clauses.size(); ci++) {
        bool satisfied = false;
        for (int dlit : dimacs_clauses[ci]) {
            int var = abs(dlit);
            if (var > num_vars) continue;
            bool positive = (dlit > 0);
            if ((assignment[var] == 1) == positive) {
                satisfied = true;
                break;
            }
        }
        if (!satisfied) {
            cerr << "BUG: clause " << ci << " not satisfied: ";
            for (int dlit : dimacs_clauses[ci]) cerr << dlit << " ";
            cerr << endl;
            return false;
        }
    }
    return true;
}

int main(int argc, char* argv[]) {
    argparse::ArgumentParser program("assump_fuzz_oracle", "1.0",
        argparse::default_arguments::help);

    program.add_description("Assumption-based oracle fuzzer");

    program.add_argument("input")
        .help("Input CNF file in DIMACS format (must be SAT)")
        .required();

    program.add_argument("-k", "--iterations")
        .help("Number of random assumption tests (-1 = random 1..1000)")
        .default_value(-1)
        .scan<'i', int>();

    program.add_argument("-v", "--verb")
        .help("Verbosity level")
        .default_value(0)
        .scan<'i', int>();

    program.add_argument("-s", "--seed")
        .help("Random seed")
        .default_value(0)
        .scan<'i', int>();

    program.add_argument("--vivify")
        .help("Enable Vivify calls (0/1)")
        .default_value(1)
        .scan<'i', int>();

    program.add_argument("--vivify-freq")
        .help("Percent chance of running Vivify per iteration (0..100)")
        .default_value(70)
        .scan<'i', int>();

    program.add_argument("--vivify-max-runs")
        .help("Max number of Vivify runs per iteration (1..N)")
        .default_value(3)
        .scan<'i', int>();

    program.add_argument("--vivify-before")
        .help("Also run Vivify BEFORE each Solve (0/1)")
        .default_value(1)
        .scan<'i', int>();

    program.add_argument("--vivify-initial")
        .help("Run Vivify immediately after oracle construction (0/1)")
        .default_value(1)
        .scan<'i', int>();

    program.add_argument("--soundness-check")
        .help("After every Vivify, Solve(no_assumps) and verify the "
              "SAT/UNSAT answer still matches the baseline (1=on)")
        .default_value(1)
        .scan<'i', int>();

    program.add_argument("--sparsify-mode")
        .help("Mimic oracle_sparsify: each original clause gets an indicator "
              "variable appended, and indicators are kept 'active' via "
              "SetAssumpLit(..., freeze=false) (level-2 soft assumption). "
              "Vivify calls between Solves are then expected to preserve the "
              "active state. Mismatch vs CaDiCaL on the original CNF + tmp "
              "asserts the soft-assumption / Vivify interaction bug.")
        .default_value(0)
        .scan<'i', int>();

    program.add_argument("--sparsify-cumulative")
        .help("In sparsify-mode, persistently remove clauses the oracle "
              "decides are redundant (SetAssumpLit ind=TRUE, freeze=true), "
              "and run Vivify between probes. At the end, build the reduced "
              "CNF (orig minus removed clauses) and verify with CaDiCaL that "
              "its satisfiability matches the original. Catches the case "
              "where Vivify's strengthening misleads the oracle into "
              "removing a clause that wasn't actually redundant in the "
              "*original* CNF (the user's hypothesis).")
        .default_value(0)
        .scan<'i', int>();

    program.add_argument("--freeze-rate")
        .help("Probability (0..100) per iteration of FreezeUnit'ing a random "
              "lit consistent with the baseline model — mimics the "
              "SetAssumpLit pattern in oracle_sparsify.")
        .default_value(20)
        .scan<'i', int>();

    program.add_argument("--cache-cutoff")
        .help("Solution-cache lookup cadence")
        .default_value(1000).scan<'i', int>();
    program.add_argument("--db-clean-interval")
        .help("Conflicts between learned-clause DB cleans")
        .default_value(20000).scan<'i', int>();
    program.add_argument("--tier1-max-glue")
        .help("Max glue for tier-1 (always-keep) learned clauses")
        .default_value(5).scan<'i', int>();
    program.add_argument("--tier2-max-glue")
        .help("Max glue for tier-2 (keep-if-used) learned clauses")
        .default_value(6).scan<'i', int>();
    program.add_argument("--restart-factor")
        .help("Luby restart-factor multiplier")
        .default_value(400).scan<'i', int>();

    try {
        program.parse_args(argc, argv);
    } catch (const std::exception& err) {
        cerr << err.what() << endl;
        cerr << program;
        return 1;
    }

    string input_file = program.get<string>("input");
    int K = program.get<int>("--iterations");
    int verb = program.get<int>("--verb");
    int seed = program.get<int>("--seed");
    int vivify_enabled = program.get<int>("--vivify");
    int vivify_freq = program.get<int>("--vivify-freq");
    int vivify_max_runs = program.get<int>("--vivify-max-runs");
    int vivify_before = program.get<int>("--vivify-before");
    int vivify_initial = program.get<int>("--vivify-initial");
    int soundness_check = program.get<int>("--soundness-check");
    int freeze_rate = program.get<int>("--freeze-rate");
    int sparsify_mode = program.get<int>("--sparsify-mode");
    int sparsify_cumulative = program.get<int>("--sparsify-cumulative");
    vector<int8_t> frozen_var; // frozen_var[v] = 1 if already SetAssumpLit'd

    // Baseline: ground truth for the input CNF (no assumptions). If Vivify
    // is unsound and adds a non-entailed clause, the oracle's answer to
    // Solve({}, ...) can flip away from this baseline — which is exactly
    // the symptom the user sees in oracle_sparsify.
    int baseline = 0; // 0=unknown, 10=SAT, 20=UNSAT — set after parse.

    auto check_base_sound = [&](sspp::oracle::Oracle& o, const char* where) {
        if (!soundness_check || baseline == 0) return true;
        o.reset_mems();
        vector<Lit> no_assumps;
        auto r = o.Solve(no_assumps, /*usecache=*/false, /*max_mems=*/1000000000LL);
        if (r.isUnknown()) return true; // can't conclude
        int got = r.isTrue() ? 10 : 20;
        if (got != baseline) {
            cerr << "UNSOUND: after " << where
                 << ": oracle reports " << (got == 10 ? "SAT" : "UNSAT")
                 << " but baseline is " << (baseline == 10 ? "SAT" : "UNSAT")
                 << endl;
            return false;
        }
        return true;
    };

    auto run_vivify = [&](sspp::oracle::Oracle& o, std::mt19937& r, const char* tag) -> bool {
        if (!vivify_enabled) return true;
        constexpr int64_t viv_mems_choices[] =
            {0, 10, 100, 1000, 100000, 100000000, 100000000, 100000000};
        int nruns = std::uniform_int_distribution<int>(1, std::max(1, vivify_max_runs))(r);
        for (int vi = 0; vi < nruns; vi++) {
            int64_t vm = viv_mems_choices[
                std::uniform_int_distribution<int>(0, 7)(r)];
            const auto& st_before = o.getStats();
            size_t forgot_before = st_before.forgot_clauses;
            if (verb >= 1) cout << "c  Vivify[" << tag << "] #" << vi
                                << " max_mems=" << vm << endl;
            o.reset_mems();
            o.Vivify(vm);
            if (verb >= 1) {
                cout << "c   forgot_cls delta=" << (o.getStats().forgot_clauses - forgot_before)
                     << endl;
            }
            if (!check_base_sound(o, "Vivify")) return false;
        }
        return true;
    };

    // 1. Parse CNF
    CNF cnf = parse_dimacs(input_file);
    if (verb >= 1)
        cout << "c Parsed: " << cnf.num_vars << " vars, "
             << cnf.clauses.size() << " clauses" << endl;

    // Ground truth for no-assumption Solve — needed to catch a Vivify that
    // makes the formula spuriously UNSAT. If SAT, also capture the model so
    // we can FreezeUnit lits known to be compatible with a satisfying
    // assignment (safe way to mimic SetAssumpLit from oracle_sparsify).
    vector<int> baseline_model; // baseline_model[v] = +v or -v (1..num_vars)
    if (soundness_check) {
        CaDiCaL::Solver cad;
        for (const auto& cl : cnf.dimacs_clauses) {
            for (int lit : cl) cad.add(lit);
            cad.add(0);
        }
        int r = cad.solve();
        if (r == 10) {
            baseline = 10;
            baseline_model.assign(cnf.num_vars + 1, 0);
            for (int v = 1; v <= cnf.num_vars; v++) {
                baseline_model[v] = (cad.val(v) > 0) ? v : -v;
            }
        } else if (r == 20) baseline = 20;
        if (verb >= 1)
            cout << "c Baseline (no assumps): "
                 << (baseline == 10 ? "SAT" : baseline == 20 ? "UNSAT" : "UNKNOWN")
                 << endl;
    }

    std::mt19937 rng(seed);

    // ============================================================
    // Sparsify-mode: mimic oracle_sparsify's indicator-var pattern.
    // Each original clause gets a fresh indicator var v_i; the clause becomes
    // [orig_lits, PosLit(v_i)]. We then soft-assign NegLit(v_i) (i.e. v_i = false)
    // via SetAssumpLit(..., freeze=false), which lives at level 2. With every
    // indicator forced false, every augmented clause is equivalent to the
    // original — so Solve(tmp) on the augmented oracle must agree with
    // CaDiCaL on (orig CNF + tmp).
    //
    // Bug under test: Vivify calls UnDecide(2), which clears all level-2
    // assignments — wiping the soft indicator state. Subsequent Solves can
    // then trivially satisfy any clause by setting its indicator true,
    // returning SAT where the original is UNSAT.
    // ============================================================
    if (sparsify_mode) {
        if (K < 0) {
            K = std::uniform_int_distribution<int>(1, 200)(rng);
            if (verb >= 1) cout << "c [sparsify-mode] Random iterations: " << K << endl;
        }
        const int orig_vars = cnf.num_vars;
        const int n_cls = (int)cnf.clauses.size();
        const int aug_vars = orig_vars + n_cls;

        vector<vector<Lit>> aug_clauses;
        aug_clauses.reserve(n_cls);
        for (int i = 0; i < n_cls; i++) {
            auto cl = cnf.clauses[i];
            cl.push_back(PosLit(orig_vars + 1 + i));
            aug_clauses.push_back(std::move(cl));
        }

        sspp::oracle::Oracle oracle(aug_vars, aug_clauses);
        if (verb >= 2) oracle.SetVerbosity(1);
        oracle.SetCacheCutoff(program.get<int>("--cache-cutoff"));
        oracle.SetDbCleanInterval(program.get<int>("--db-clean-interval"));
        oracle.SetTier1MaxGlue(program.get<int>("--tier1-max-glue"));
        oracle.SetTier2MaxGlue(program.get<int>("--tier2-max-glue"));
        oracle.SetRestartFactor(program.get<int>("--restart-factor"));

        // Soft-assign every indicator var to FALSE → every clause is "active".
        for (int i = 0; i < n_cls; i++) {
            oracle.SetAssumpLit(NegLit(orig_vars + 1 + i), /*freeze=*/false);
        }
        cout << "c [sparsify-mode] Soft-assigned " << n_cls
             << " indicators; aug_vars=" << aug_vars
             << " CurLevel after SetAssumpLit loop=" << oracle.CurLevel() << endl;

        // Optional initial Vivify (most likely to trip the bug immediately).
        if (vivify_initial && vivify_enabled) {
            if (verb >= 1) cout << "c [sparsify-mode] initial Vivify" << endl;
            oracle.reset_mems();
            oracle.Vivify(100000000);
        }

        // ----------------------------------------------------------------
        // Cumulative variant: persistently remove clauses, Vivify between,
        // then verify reduced CNF ≡ original CNF (in satisfiability).
        // ----------------------------------------------------------------
        if (sparsify_cumulative) {
            vector<int8_t> removed(n_cls, 0);
            int s_removed = 0, s_kept = 0, s_unknown = 0;
            // Walk each clause once (mirrors oracle_sparsify's for-loop).
            for (int ci = 0; ci < n_cls; ci++) {
                // Periodic Vivify, like the line under investigation.
                if (vivify_enabled
                    && std::uniform_int_distribution<int>(0, 99)(rng) < vivify_freq) {
                    if (verb >= 1) cout << "c [sparsify-cum] ci=" << ci << " Vivify" << endl;
                    oracle.reset_mems();
                    oracle.Vivify(100000000);
                }

                // Tentatively flip indicator to TRUE (removed).
                const Lit pos_ind = PosLit(orig_vars + 1 + ci);
                oracle.SetAssumpLit(pos_ind, /*freeze=*/false);

                vector<Lit> tmp;
                for (int dl : cnf.dimacs_clauses[ci]) {
                    tmp.push_back(dimacs_to_oracle_lit(-dl));
                }

                oracle.reset_mems();
                auto res = oracle.Solve(tmp, /*usecache=*/false, 100000000LL);

                if (res.isUnknown()) {
                    s_unknown++;
                    // Restore active.
                    oracle.SetAssumpLit(NegLit(orig_vars + 1 + ci), /*freeze=*/false);
                } else if (res.isFalse()) {
                    // UNSAT → clause is entailed by the rest. Persistently remove
                    // (freeze ind=TRUE), exactly like oracle_sparsify does.
                    oracle.SetAssumpLit(pos_ind, /*freeze=*/true);
                    removed[ci] = 1;
                    s_removed++;
                } else {
                    // SAT → keep clause active (freeze ind=FALSE).
                    oracle.SetAssumpLit(NegLit(orig_vars + 1 + ci), /*freeze=*/true);
                    s_kept++;
                }
            }

            if (verb >= 1)
                cout << "c [sparsify-cum] removed=" << s_removed
                     << " kept=" << s_kept << " unknown=" << s_unknown << endl;

            // Verify equivalence: orig CNF and reduced CNF must agree on SAT/UNSAT.
            CaDiCaL::Solver cad_orig;
            for (const auto& cl : cnf.dimacs_clauses) {
                for (int l : cl) cad_orig.add(l);
                cad_orig.add(0);
            }
            int r_orig = cad_orig.solve();

            CaDiCaL::Solver cad_red;
            for (int k = 0; k < n_cls; k++) {
                if (removed[k]) continue;
                for (int l : cnf.dimacs_clauses[k]) cad_red.add(l);
                cad_red.add(0);
            }
            int r_red = cad_red.solve();

            if (r_orig != 0 && r_red != 0 && r_orig != r_red) {
                cerr << "BUG [sparsify-cum]: reduced CNF disagrees with original!" << endl
                     << "  original = " << (r_orig == 10 ? "SAT" : "UNSAT") << endl
                     << "  reduced  = " << (r_red  == 10 ? "SAT" : "UNSAT") << endl
                     << "  removed " << s_removed << " of " << n_cls << " clauses" << endl;
                // Dump the indices of removed clauses for repro.
                cerr << "  removed indices:";
                for (int k = 0; k < n_cls; k++) if (removed[k]) cerr << " " << k;
                cerr << endl;
                return 1;
            }
            cout << "c [sparsify-cum] Results: removed=" << s_removed
                 << " kept=" << s_kept << " unknown=" << s_unknown
                 << " orig=" << (r_orig == 10 ? "SAT" : r_orig == 20 ? "UNSAT" : "?")
                 << " reduced=" << (r_red == 10 ? "SAT" : r_red == 20 ? "UNSAT" : "?")
                 << endl;
            cout << "PASS" << endl;
            return 0;
        }

        int s_sat = 0, s_unsat = 0, s_unknown = 0;
        for (int iter = 0; iter < K; iter++) {
            // Optional Vivify before Solve.
            if (vivify_enabled && vivify_before
                && std::uniform_int_distribution<int>(0, 99)(rng) < vivify_freq) {
                if (verb >= 1) cout << "c [sparsify-mode] iter " << iter << " Vivify(before)" << endl;
                oracle.reset_mems();
                oracle.Vivify(100000000);
            }

            // Pick a random clause ci and "tentatively remove" it: flip its
            // indicator from FALSE (soft, active) to TRUE (soft, removed) via
            // SetAssumpLit(PosLit(ind_ci), freeze=false). Then Solve(tmp=~lits(ci)).
            // This mirrors the oracle_sparsify "is this clause needed?" probe.
            //
            // Ground truth: CaDiCaL on (original CNF \ {clause ci} + tmp).
            // Because clause ci is removed, the relaxed problem can be SAT
            // even though the original was UNSAT — exactly the regime where
            // a Vivify that produced a spurious unconditional copy of clause
            // ci ("orig_lits" without the indicator) would make the oracle
            // incorrectly report UNSAT.
            int ci = std::uniform_int_distribution<int>(0, n_cls - 1)(rng);
            const Lit pos_ind = PosLit(orig_vars + 1 + ci);
            oracle.SetAssumpLit(pos_ind, /*freeze=*/false);

            vector<Lit> tmp;
            vector<int> dimacs_tmp;
            for (int dl : cnf.dimacs_clauses[ci]) {
                int neg = -dl;
                dimacs_tmp.push_back(neg);
                tmp.push_back(dimacs_to_oracle_lit(neg));
            }

            int64_t mems = 100000000LL;
            oracle.reset_mems();
            auto res = oracle.Solve(tmp, /*usecache=*/false, mems);

            // Restore indicator back to FALSE (active) so successive probes
            // start from a consistent state.
            const Lit neg_ind = NegLit(orig_vars + 1 + ci);
            oracle.SetAssumpLit(neg_ind, /*freeze=*/false);

            if (vivify_enabled
                && std::uniform_int_distribution<int>(0, 99)(rng) < vivify_freq) {
                if (verb >= 1) cout << "c [sparsify-mode] iter " << iter << " Vivify(after)" << endl;
                oracle.reset_mems();
                oracle.Vivify(100000000);
            }

            if (res.isUnknown()) { s_unknown++; continue; }
            int got = res.isTrue() ? 10 : 20;

            // Cross-check vs CaDiCaL on (orig CNF \ {clause ci}) + tmp.
            CaDiCaL::Solver cad;
            for (int k = 0; k < (int)cnf.dimacs_clauses.size(); k++) {
                if (k == ci) continue;
                for (int l : cnf.dimacs_clauses[k]) cad.add(l);
                cad.add(0);
            }
            for (int l : dimacs_tmp) { cad.add(l); cad.add(0); }
            int cr = cad.solve();

            if (cr != 0 && cr != got) {
                cerr << "BUG [sparsify-mode] iter " << iter
                     << ": oracle=" << (got == 10 ? "SAT" : "UNSAT")
                     << " cadical=" << (cr == 10 ? "SAT" : "UNSAT")
                     << " probe-clause-idx=" << ci << " tmp:";
                for (int l : dimacs_tmp) cerr << " " << l;
                cerr << endl;
                return 1;
            }
            if (got == 10) s_sat++; else s_unsat++;
            if (verb >= 1)
                cout << "c [sparsify-mode] iter " << iter << " ci=" << ci
                     << " -> " << (got == 10 ? "SAT" : "UNSAT")
                     << " (cadical " << (cr == 10 ? "SAT" : cr == 20 ? "UNSAT" : "?") << ")" << endl;
        }

        cout << "c [sparsify-mode] Results: SAT=" << s_sat
             << " UNSAT=" << s_unsat << " UNKNOWN=" << s_unknown << endl;
        cout << "PASS" << endl;
        return 0;
    }

    // 2. Create oracle
    if (K < 0) {
        K = std::uniform_int_distribution<int>(1, 1000)(rng);
        if (verb >= 1) cout << "c Random iterations: " << K << endl;
    }
    sspp::oracle::Oracle oracle(cnf.num_vars, cnf.clauses);
    if (verb >= 2) oracle.SetVerbosity(1);

    // Apply cutoffs from the command line. Fuzzing harnesses (e.g. fuzz_oracle.py)
    // are expected to vary these across runs to exercise extreme regimes.
    int cf_cache    = program.get<int>("--cache-cutoff");
    int cf_db_clean = program.get<int>("--db-clean-interval");
    int cf_t1       = program.get<int>("--tier1-max-glue");
    int cf_t2       = program.get<int>("--tier2-max-glue");
    int cf_restart  = program.get<int>("--restart-factor");
    oracle.SetCacheCutoff(cf_cache);
    oracle.SetDbCleanInterval(cf_db_clean);
    oracle.SetTier1MaxGlue(cf_t1);
    oracle.SetTier2MaxGlue(cf_t2);
    oracle.SetRestartFactor(cf_restart);
    if (verb >= 1) cout << "c Cutoffs: cache=" << cf_cache
                        << " db_clean=" << cf_db_clean
                        << " tier1<=" << cf_t1 << " tier2<=" << cf_t2
                        << " restart_factor=" << cf_restart << endl;

    // Exercise Vivify on the freshly-built oracle — hits the branch where
    // no learned clauses exist yet (og == true inside AddOrigClause).
    if (vivify_initial && !run_vivify(oracle, rng, "initial")) return 1;

    // Build list of variables for assumption picking
    vector<int> pickable_vars;
    for (int v = 1; v <= cnf.num_vars; v++)
        pickable_vars.push_back(v);

    // 3. Fuzz loop
    int num_sat = 0, num_unsat = 0, num_unknown = 0;

    frozen_var.assign(cnf.num_vars + 1, 0);

    for (int iter = 0; iter < K; iter++) {
        // Mimic oracle_sparsify: occasionally SetAssumpLit(v, freeze=true)
        // using the baseline model's polarity, so we never make the formula
        // inconsistent with the baseline. Bugs in Vivify's interaction with
        // frozen lits will then show up in the soundness check.
        if (baseline == 10
            && std::uniform_int_distribution<int>(0, 99)(rng) < freeze_rate) {
            for (int attempt = 0; attempt < 5; attempt++) {
                int v = std::uniform_int_distribution<int>(1, cnf.num_vars)(rng);
                if (frozen_var[v]) continue;
                int lit_sign = baseline_model[v]; // +v or -v
                Lit ol = (lit_sign > 0) ? PosLit(v) : NegLit(v);
                oracle.SetAssumpLit(ol, /*freeze=*/true);
                frozen_var[v] = 1;
                if (verb >= 1) cout << "c  Freeze var " << v
                                    << " = " << (lit_sign > 0 ? "T" : "F") << endl;
                break;
            }
            if (!check_base_sound(oracle, "SetAssumpLit")) return 1;
        }

        // Pick 1-10 random variables
        int max_assumps_cap = std::uniform_int_distribution<int>(0, 3)(rng) == 0 ? 200 : 50;
        int num_assumps = std::uniform_int_distribution<int>(1, max_assumps_cap)(rng);
        num_assumps = std::min(num_assumps, (int)pickable_vars.size());

        // Shuffle and take first num_assumps
        vector<int> shuffled = pickable_vars;
        for (int i = 0; i < num_assumps; i++) {
            int j = std::uniform_int_distribution<int>(i, (int)shuffled.size() - 1)(rng);
            std::swap(shuffled[i], shuffled[j]);
        }

        // Build assumptions with random polarity
        vector<Lit> oracle_assumps;
        vector<int> dimacs_assumps;
        std::set<int> used_vars;
        for (int i = 0; i < num_assumps; i++) {
            int var = shuffled[i];
            used_vars.insert(var);
            bool positive = std::uniform_int_distribution<int>(0, 1)(rng);
            oracle_assumps.push_back(positive ? PosLit(var) : NegLit(var));
            dimacs_assumps.push_back(positive ? var : -var);
        }

        // 50% of the time, include the cache lookup variable to exercise
        // the indexed cache lookup path
        int clv = oracle.GetCacheLookupVar();
        if (clv != 0 && used_vars.count(clv) == 0
                && std::uniform_int_distribution<int>(0, 1)(rng) == 0) {
            bool positive = std::uniform_int_distribution<int>(0, 1)(rng);
            oracle_assumps.push_back(positive ? PosLit(clv) : NegLit(clv));
            dimacs_assumps.push_back(positive ? clv : -clv);
        }

        // Pick a random max_mems limit: sometimes very tight, sometimes generous
        int64_t max_mems;
        int mems_choice = std::uniform_int_distribution<int>(0, 9)(rng);
        if (mems_choice < 2)       max_mems = 10;
        else if (mems_choice < 4)  max_mems = 100;
        else if (mems_choice < 6)  max_mems = 1000;
        else                       max_mems = 100000000;

        if (verb >= 1) {
            cout << "c Iter " << iter << ": assumps =";
            for (int a : dimacs_assumps) cout << " " << a;
            cout << " max_mems=" << max_mems << endl;
        }

        // Solve
        // Before-Solve Vivify: exercises the DB right before a query, so
        // any lost/corrupted clause surfaces in *this* iteration's answer.
        if (vivify_before
            && std::uniform_int_distribution<int>(0, 99)(rng) < vivify_freq) {
            if (!run_vivify(oracle, rng, "before")) return 1;
        }

        oracle.reset_mems();
        bool usecache = std::uniform_int_distribution<int>(0, 9)(rng) < 7;
        auto result = oracle.Solve(oracle_assumps, usecache, max_mems);

        // Snapshot the model BEFORE any subsequent mutating call (e.g. Vivify)
        vector<int> model;
        if (result.isTrue()) {
            model.assign(cnf.num_vars + 1, 0);
            for (int v = 1; v <= cnf.num_vars; v++) {
                int phase = oracle.GetPhase(v);
                model[v] = phase ? v : -v;
            }
        }

        // After-Solve Vivify: most bugs show up here because we've learned
        // clauses and have a live model/phase to compare against.
        if (std::uniform_int_distribution<int>(0, 99)(rng) < vivify_freq) {
            if (!run_vivify(oracle, rng, "after")) return 1;
        }

        if (result.isUnknown()) {
            num_unknown++;
            if (verb >= 1) cout << "c  -> UNKNOWN" << endl;
            continue;
        }

        if (result.isTrue()) {
            num_sat++;
            if (verb >= 1) cout << "c  -> SAT" << endl;

            if (!verify_solution_against_clauses(
                    cnf.dimacs_clauses, model, cnf.num_vars, dimacs_assumps)) {
                cerr << "BUG at iteration " << iter << "!" << endl;
                cerr << "Assumptions:";
                for (int a : dimacs_assumps) cerr << " " << a;
                cerr << endl;
                return 1;
            }
        } else {
            num_unsat++;
            if (verb >= 1) cout << "c  -> UNSAT" << endl;

            // Cross-check with CaDiCaL
            CaDiCaL::Solver cadical;
            // Add all original clauses
            for (const auto& cl : cnf.dimacs_clauses) {
                for (int lit : cl) cadical.add(lit);
                cadical.add(0);
            }
            // Add assumptions as unit clauses
            for (int alit : dimacs_assumps) {
                cadical.add(alit);
                cadical.add(0);
            }
            // Frozen vars are part of the oracle's effective formula;
            // CaDiCaL must see them too, otherwise it solves a strictly
            // less-constrained problem and produces false-positive bugs.
            for (int v = 1; v <= cnf.num_vars; v++) {
                if (frozen_var[v]) {
                    cadical.add(baseline_model[v]);
                    cadical.add(0);
                }
            }

            int cad_res = cadical.solve();
            if (cad_res == 10) {
                cerr << "BUG: oracle says UNSAT but CaDiCaL says SAT!" << endl;
                cerr << "Iteration " << iter << ", assumptions:";
                for (int a : dimacs_assumps) cerr << " " << a;
                cerr << endl;
                return 1;
            }
            if (cad_res == 20) {
                if (verb >= 1) cout << "c  UNSAT confirmed by CaDiCaL" << endl;
            }
            // cad_res == 0 (unknown) — skip
        }
    }

    cout << "c Results: SAT=" << num_sat << " UNSAT=" << num_unsat
         << " UNKNOWN=" << num_unknown << endl;

    cout << "PASS" << endl;
    return 0;
}
