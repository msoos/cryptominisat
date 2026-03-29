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
#include "cadiback.h"
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
using sspp::VarOf;
using sspp::IsPos;

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
        .help("Number of random assumption tests")
        .default_value(20)
        .scan<'i', int>();

    program.add_argument("-v", "--verb")
        .help("Verbosity level")
        .default_value(0)
        .scan<'i', int>();

    program.add_argument("-s", "--seed")
        .help("Random seed")
        .default_value(0)
        .scan<'i', int>();

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

    // 1. Parse CNF
    CNF cnf = parse_dimacs(input_file);
    if (verb >= 1)
        cout << "c Parsed: " << cnf.num_vars << " vars, "
             << cnf.clauses.size() << " clauses" << endl;

    // 2. Run cadiback to find backbones
    // Convert to cadiback's flat DIMACS format
    vector<int> cadiback_cnf;
    for (const auto& cl : cnf.dimacs_clauses) {
        for (int lit : cl) cadiback_cnf.push_back(lit);
        cadiback_cnf.push_back(0);
    }

    vector<int> drop_cands; // empty = check all variables
    vector<int> backbone_lits;
    vector<int> learned_bins;
    vector<std::pair<int, int>> eq_lits;

    int cadiback_res = CadiBack::doit(
        cadiback_cnf, verb >= 2 ? 0 : -1,
        drop_cands, backbone_lits, learned_bins, eq_lits);

    if (cadiback_res == 20) {
        cerr << "ERROR: cadiback says UNSAT, but expected SAT" << endl;
        return 1;
    }

    // Collect backbone variables
    std::set<int> backbone_vars;
    for (int bl : backbone_lits) {
        if (bl == 0) continue;
        backbone_vars.insert(abs(bl));
    }

    if (verb >= 1)
        cout << "c Backbones: " << backbone_vars.size()
             << " out of " << cnf.num_vars << " vars" << endl;

    // 3. Build oracle clause list = original clauses + backbone units
    vector<vector<Lit>> oracle_clauses = cnf.clauses;
    // Also keep combined DIMACS clauses for verification
    vector<vector<int>> all_dimacs_clauses = cnf.dimacs_clauses;
    for (int bl : backbone_lits) {
        if (bl == 0) continue;
        oracle_clauses.push_back({dimacs_to_oracle_lit(bl)});
        all_dimacs_clauses.push_back({bl});
    }

    // 4. Create oracle
    sspp::oracle::Oracle oracle(cnf.num_vars, oracle_clauses);
    if (verb >= 2) oracle.SetVerbosity(1);

    // Build list of non-backbone variables for assumption picking
    vector<int> pickable_vars;
    for (int v = 1; v <= cnf.num_vars; v++) {
        if (backbone_vars.count(v) == 0)
            pickable_vars.push_back(v);
    }

    if (pickable_vars.empty()) {
        if (verb >= 1) cout << "c All variables are backbones, nothing to fuzz" << endl;
        cout << "PASS" << endl;
        return 0;
    }

    // 5. Fuzz loop
    std::mt19937 rng(seed);
    int num_sat = 0, num_unsat = 0, num_unknown = 0;

    for (int iter = 0; iter < K; iter++) {
        // Pick 1-10 random non-backbone variables
        int num_assumps = std::uniform_int_distribution<int>(1, 10)(rng);
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
        for (int i = 0; i < num_assumps; i++) {
            int var = shuffled[i];
            bool positive = std::uniform_int_distribution<int>(0, 1)(rng);
            oracle_assumps.push_back(positive ? PosLit(var) : NegLit(var));
            dimacs_assumps.push_back(positive ? var : -var);
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
        oracle.reset_mems();
        auto result = oracle.Solve(oracle_assumps, /*usecache=*/true, max_mems);

        if (result.isUnknown()) {
            num_unknown++;
            if (verb >= 1) cout << "c  -> UNKNOWN" << endl;
            continue;
        }

        if (result.isTrue()) {
            num_sat++;
            if (verb >= 1) cout << "c  -> SAT" << endl;

            // Verify solution
            vector<int> model(cnf.num_vars + 1);
            for (int v = 1; v <= cnf.num_vars; v++) {
                int phase = oracle.GetPhase(v);
                model[v] = phase ? v : -v;
            }

            if (!verify_solution_against_clauses(
                    all_dimacs_clauses, model, cnf.num_vars, dimacs_assumps)) {
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
            // Add backbone units
            for (int bl : backbone_lits) {
                if (bl == 0) continue;
                cadical.add(bl);
                cadical.add(0);
            }
            // Add assumptions as unit clauses
            for (int alit : dimacs_assumps) {
                cadical.add(alit);
                cadical.add(0);
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
