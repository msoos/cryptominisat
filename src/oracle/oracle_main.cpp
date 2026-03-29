#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cstdint>
#include <cassert>
#include <chrono>

#include "argparse.hpp"
#include "oracle.h"

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::vector;

using sspp::Lit;
using sspp::PosLit;
using sspp::NegLit;

// Convert DIMACS literal (1-based, sign) to oracle Lit (even/odd encoding)
static Lit dimacs_to_oracle_lit(int d) {
    assert(d != 0);
    if (d > 0) return PosLit(d);
    else return NegLit(-d);
}

// Convert oracle Lit back to DIMACS for display
static int oracle_lit_to_dimacs(Lit l) {
    int var = sspp::VarOf(l);
    return sspp::IsPos(l) ? var : -var;
}

struct CNF {
    int num_vars = 0;
    int num_clauses = 0;
    vector<vector<Lit>> clauses;
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
        // skip empty lines
        if (line.empty()) continue;

        // comment lines
        if (line[0] == 'c') continue;

        // problem line
        if (line[0] == 'p') {
            std::istringstream iss(line);
            string p, fmt;
            iss >> p >> fmt >> cnf.num_vars >> cnf.num_clauses;
            if (fmt != "cnf") {
                cerr << "ERROR: Expected 'cnf' format, got '" << fmt << "'" << endl;
                exit(1);
            }
            header_seen = true;
            continue;
        }

        if (!header_seen) {
            cerr << "ERROR: Clause data before p-line" << endl;
            exit(1);
        }

        // clause line(s) — may contain multiple literals, terminated by 0
        std::istringstream iss(line);
        int lit;
        vector<Lit> clause;
        while (iss >> lit) {
            if (lit == 0) {
                if (!clause.empty()) {
                    cnf.clauses.push_back(std::move(clause));
                    clause.clear();
                }
            } else {
                if (abs(lit) > cnf.num_vars) {
                    cerr << "ERROR: Literal " << lit
                         << " exceeds declared variable count " << cnf.num_vars << endl;
                    exit(1);
                }
                clause.push_back(dimacs_to_oracle_lit(lit));
            }
        }
        // Handle clauses that span multiple lines (no trailing 0 yet)
        if (!clause.empty()) {
            // Read continuation lines until we hit a 0
            while (std::getline(in, line)) {
                if (line.empty()) continue;
                if (line[0] == 'c') continue;
                std::istringstream iss2(line);
                while (iss2 >> lit) {
                    if (lit == 0) {
                        cnf.clauses.push_back(std::move(clause));
                        clause.clear();
                        break;
                    }
                    clause.push_back(dimacs_to_oracle_lit(lit));
                }
                if (clause.empty()) break;
            }
            if (!clause.empty()) {
                // EOF before 0 terminator — add remaining clause
                cnf.clauses.push_back(std::move(clause));
            }
        }
    }
    return cnf;
}

int main(int argc, char* argv[]) {
    argparse::ArgumentParser program("oracle", "1.0",
        argparse::default_arguments::help);

    program.add_description("Standalone oracle SAT solver");

    program.add_argument("input")
        .help("Input CNF file in DIMACS format")
        .required();

    program.add_argument("-v", "--verb")
        .help("Verbosity level (0=quiet, 1=stats, 2=detailed)")
        .default_value(0)
        .scan<'i', int>();

    program.add_argument("-m", "--max-mems")
        .help("Maximum memory operations (budget), 0 = unlimited")
        .default_value((int64_t)(1000LL * 1000LL * 1000LL))
        .scan<'i', int64_t>();

    try {
        program.parse_args(argc, argv);
    } catch (const std::exception& err) {
        cerr << err.what() << endl;
        cerr << program;
        return 1;
    }

    string input_file = program.get<string>("input");
    int verb = program.get<int>("--verb");
    int64_t max_mems = program.get<int64_t>("--max-mems");

    // Parse DIMACS
    if (verb >= 1) cout << "c Reading " << input_file << " ..." << endl;
    auto t0 = std::chrono::steady_clock::now();
    CNF cnf = parse_dimacs(input_file);
    auto t1 = std::chrono::steady_clock::now();
    double parse_time = std::chrono::duration<double>(t1 - t0).count();

    if (verb >= 1) {
        cout << "c Variables: " << cnf.num_vars << endl;
        cout << "c Clauses:   " << cnf.clauses.size() << endl;
        cout << "c Parse time: " << parse_time << " s" << endl;
    }

    if (cnf.num_vars == 0) {
        cout << "s SATISFIABLE" << endl;
        return 10;
    }

    // Create and configure oracle
    sspp::oracle::Oracle oracle(cnf.num_vars, cnf.clauses);
    oracle.SetVerbosity(verb >= 2 ? 1 : 0);

    // Solve with no assumptions
    if (verb >= 1) cout << "c Solving ..." << endl;
    auto t2 = std::chrono::steady_clock::now();
    vector<Lit> no_assumps;
    auto result = oracle.Solve(no_assumps, /*usecache=*/false, max_mems);
    auto t3 = std::chrono::steady_clock::now();
    double solve_time = std::chrono::duration<double>(t3 - t2).count();

    if (verb >= 1) {
        cout << "c Solve time: " << solve_time << " s" << endl;
        cout << "c Total time: " << (parse_time + solve_time) << " s" << endl;
        oracle.PrintStats();
    }

    if (result.isTrue()) {
        cout << "s SATISFIABLE" << endl;
        // Print model in DIMACS format
        cout << "v";
        for (int v = 1; v <= cnf.num_vars; v++) {
            int val = oracle.LitVal(PosLit(v));
            if (val > 0) cout << " " << v;
            else if (val < 0) cout << " " << -v;
            else cout << " " << v; // unassigned = true by convention
        }
        cout << " 0" << endl;
        return 10;
    } else if (result.isFalse()) {
        cout << "s UNSATISFIABLE" << endl;
        return 20;
    } else {
        cout << "s UNKNOWN" << endl;
        if (verb >= 1) cout << "c Resource limit reached" << endl;
        return 0;
    }
}
