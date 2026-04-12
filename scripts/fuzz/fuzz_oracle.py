#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (C) 2009-2020 Authors of CryptoMiniSat, see AUTHORS file
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; version 2
# of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301, USA.

import subprocess
import os
import sys
import time
import random
import argparse
import tempfile
import signal


def find_binary(name, search_paths):
    """Find a binary in the given search paths."""
    for path in search_paths:
        full = os.path.join(path, name)
        if os.path.isfile(full) and os.access(full, os.X_OK):
            return os.path.abspath(full)
    return None


def auto_find_binaries(args):
    """Auto-detect binary paths relative to this script's location."""
    script_dir = os.path.dirname(os.path.abspath(__file__))
    build_dir = os.path.join(script_dir, "..", "..", "build")
    build_dir = os.path.abspath(build_dir)

    search = [
        os.path.join(build_dir, "cmsat5-src"),
        os.path.join(build_dir, "tests", "cnf-utils"),
        build_dir,
    ]

    if args.oracle is None:
        args.oracle = find_binary("oracle", search)
        if args.oracle is None:
            print("ERROR: Cannot find 'oracle' binary. Build it with: make oracle")
            print("Searched in:", search)
            sys.exit(1)

    if args.cms is None:
        args.cms = find_binary("cryptominisat5", search)
        if args.cms is None:
            print("ERROR: Cannot find 'cryptominisat5' binary.")
            print("Searched in:", search)
            sys.exit(1)

    if args.fuzzer is None:
        args.fuzzer = find_binary("cnf-fuzz-biere", search)
        if args.fuzzer is None:
            print("ERROR: Cannot find 'cnf-fuzz-biere' binary.")
            print("Searched in:", search)
            sys.exit(1)

    if args.assump_fuzz is None:
        args.assump_fuzz = find_binary("assump_fuzz_oracle", search)
        if args.assump_fuzz is None:
            print("ERROR: Cannot find 'assump_fuzz_oracle' binary. Build it with: make assump_fuzz_oracle")
            print("Searched in:", search)
            sys.exit(1)

    return args


def generate_cnf(fuzzer_bin, seed, cnf_path):
    """Generate a random CNF using cnf-fuzz-biere."""
    with open(cnf_path, "w") as f:
        ret = subprocess.run(
            [fuzzer_bin, str(seed)],
            stdout=f, stderr=subprocess.DEVNULL,
            timeout=10)
    if ret.returncode != 0:
        print("ERROR: Fuzzer failed with return code %d" % ret.returncode)
        return False
    return True


def run_solver(binary, cnf_path, timeout, extra_args=None):
    """Run a solver and return (stdout, return_code, timed_out)."""
    cmd = [binary]
    if extra_args:
        cmd.extend(extra_args)
    cmd.append(cnf_path)

    try:
        result = subprocess.run(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            timeout=timeout,
            universal_newlines=True)
        return result.stdout, result.returncode, False
    except subprocess.TimeoutExpired:
        return "", -1, True


def parse_oracle_output(output):
    """Parse oracle output. Returns (result, solution_dict).
    result: "SAT", "UNSAT", or "UNKNOWN"
    solution_dict: {var: True/False} for SAT results
    """
    result = None
    solution = {}

    for line in output.split("\n"):
        line = line.strip()
        if not line:
            continue

        if line.startswith("s "):
            if "UNSATISFIABLE" in line:
                result = "UNSAT"
            elif "SATISFIABLE" in line:
                result = "SAT"
            elif "UNKNOWN" in line:
                result = "UNKNOWN"
            continue

        if line.startswith("v "):
            parts = line.split()
            for p in parts[1:]:  # skip the 'v'
                lit = int(p)
                if lit == 0:
                    break
                solution[abs(lit)] = (lit > 0)
            continue

    return result, solution


def parse_cms_output(output):
    """Parse CryptoMiniSat output. Returns result: "SAT", "UNSAT", or None."""
    for line in output.split("\n"):
        line = line.strip()
        if line.startswith("s "):
            if "UNSATISFIABLE" in line:
                return "UNSAT"
            elif "SATISFIABLE" in line:
                return "SAT"
    return None


def parse_cnf_file(cnf_path):
    """Parse CNF file and return (num_vars, clauses).
    clauses is a list of lists of ints (DIMACS literals).
    """
    num_vars = 0
    clauses = []
    current_clause = []

    with open(cnf_path, "r") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("c"):
                continue
            if line.startswith("p"):
                parts = line.split()
                num_vars = int(parts[2])
                continue
            for tok in line.split():
                lit = int(tok)
                if lit == 0:
                    if current_clause:
                        clauses.append(current_clause)
                        current_clause = []
                else:
                    current_clause.append(lit)

    if current_clause:
        clauses.append(current_clause)

    return num_vars, clauses


def verify_solution(cnf_path, solution, verbose=False):
    """Verify that the solution satisfies all clauses in the CNF.
    Returns (ok, error_message).
    """
    num_vars, clauses = parse_cnf_file(cnf_path)

    # Check all variables have values
    for v in range(1, num_vars + 1):
        if v not in solution:
            return False, "Variable %d has no value in solution" % v

    # Check all clauses are satisfied
    for i, clause in enumerate(clauses):
        satisfied = False
        for lit in clause:
            var = abs(lit)
            if var not in solution:
                continue
            if (lit > 0) == solution[var]:
                satisfied = True
                break

        if not satisfied:
            return False, "Clause %d not satisfied: %s" % (i + 1, clause)

    return True, None


def save_failing_cnf(cnf_path, test_num, seed):
    """Copy the failing CNF to a permanent location for investigation."""
    fail_dir = "oracle_fuzz_failures"
    os.makedirs(fail_dir, exist_ok=True)
    dst = os.path.join(fail_dir, "fail_test%d_seed%d.cnf" % (test_num, seed))
    with open(cnf_path, "r") as src_f:
        with open(dst, "w") as dst_f:
            dst_f.write(src_f.read())
    return dst


def fuzz_one(args, test_num, seed, counters):
    """Run one fuzz iteration. Returns True if passed, False if bug found."""
    verbose = args.verbose

    if verbose:
        print("  Generating CNF with seed %d..." % seed)

    # Create temp file for CNF
    fd, cnf_path = tempfile.mkstemp(suffix=".cnf", prefix="oracle_fuzz_")
    os.close(fd)

    try:
        # Generate CNF
        if not generate_cnf(args.fuzzer, seed, cnf_path):
            return True  # fuzzer failure, skip

        if verbose:
            num_vars, clauses = parse_cnf_file(cnf_path)
            print("  CNF: %d vars, %d clauses" % (num_vars, len(clauses)))

        # Run oracle (randomly enable Vivify to exercise that path)
        rng = random.Random(seed)
        vivify = 1 if rng.random() < 0.3 else 0
        oracle_verb = "2" if verbose else "0"
        oracle_args = ["-v", oracle_verb, "--vivify", str(vivify)]
        if verbose:
            print("  Running oracle (vivify=%d)..." % vivify)
        oracle_out, oracle_rc, oracle_timeout = run_solver(
            args.oracle, cnf_path, args.tlimit, oracle_args)

        if oracle_timeout:
            if verbose:
                print("  Oracle timed out, skipping")
            counters["timeout"] += 1
            return True

        if verbose:
            for line in oracle_out.split("\n"):
                if "Vivify" in line or "Re-solving" in line:
                    print("    %s" % line)

        oracle_result, oracle_solution = parse_oracle_output(oracle_out)

        if oracle_result is None:
            print("ERROR: Oracle produced no result line!")
            print("Output:", oracle_out)
            saved = save_failing_cnf(cnf_path, test_num, seed)
            print("CNF saved to:", saved)
            return False

        if oracle_result == "UNKNOWN":
            if verbose:
                print("  Oracle returned UNKNOWN (resource limit), skipping")
            counters["unknown"] += 1
            return True

        if verbose:
            print("  Oracle says: %s" % oracle_result)

        # If SAT: verify the solution
        if oracle_result == "SAT":
            ok, err = verify_solution(cnf_path, oracle_solution, verbose)
            if not ok:
                print("BUG: Oracle says SAT but solution is invalid!")
                print("Error: %s" % err)
                saved = save_failing_cnf(cnf_path, test_num, seed)
                print("CNF saved to:", saved)
                print("Oracle output:")
                print(oracle_out)
                return False

            if verbose:
                print("  Solution verified OK (%d vars)" % len(oracle_solution))
            counters["sat"] += 1

            # Run assumption-based fuzzing on SAT instances
            if verbose:
                print("  Running assumption-based fuzz (K=20)...")
            assump_out, assump_rc, assump_timeout = run_solver(
                args.assump_fuzz, cnf_path, args.tlimit,
                ["-k", "20", "-s", str(seed)])

            if assump_timeout:
                if verbose:
                    print("  assump_fuzz_oracle timed out, skipping")
            elif assump_rc != 0:
                print("BUG: assump_fuzz_oracle failed!")
                print("Output:")
                print(assump_out)
                saved = save_failing_cnf(cnf_path, test_num, seed)
                print("CNF saved to:", saved)
                print("To reproduce directly: %s -k 20 -s %d -v 2 %s" % (
                    args.assump_fuzz, seed, saved))
                return False
            else:
                # Parse SAT/UNSAT/UNKNOWN counts from assump_fuzz output
                for line in assump_out.split("\n"):
                    if line.startswith("c Results:"):
                        import re
                        m = re.search(r'SAT=(\d+)\s+UNSAT=(\d+)\s+UNKNOWN=(\d+)', line)
                        if m:
                            counters["assump_sat"] += int(m.group(1))
                            counters["assump_unsat"] += int(m.group(2))
                            counters["assump_unknown"] += int(m.group(3))
                if verbose:
                    for line in assump_out.split("\n"):
                        if line.startswith("c Results") or line.startswith("PASS"):
                            print("  assump: %s" % line.strip())

        # If UNSAT: cross-check with CryptoMiniSat
        if oracle_result == "UNSAT":
            if verbose:
                print("  Cross-checking UNSAT with CryptoMiniSat...")

            cms_out, cms_rc, cms_timeout = run_solver(
                args.cms, cnf_path, args.tlimit,
                ["--verb", "0", "--zero-exit-status"])

            if cms_timeout:
                if verbose:
                    print("  CryptoMiniSat timed out, cannot verify UNSAT")
                return True

            cms_result = parse_cms_output(cms_out)

            if cms_result is None:
                if verbose:
                    print("  CryptoMiniSat produced no result, skipping UNSAT check")
                return True

            if verbose:
                print("  CryptoMiniSat says: %s" % cms_result)

            if cms_result == "SAT":
                print("BUG: Oracle says UNSAT but CryptoMiniSat says SAT!")
                saved = save_failing_cnf(cnf_path, test_num, seed)
                print("CNF saved to:", saved)
                print("Oracle output:")
                print(oracle_out)
                print("CryptoMiniSat output:")
                print(cms_out)
                return False

            if verbose:
                print("  UNSAT confirmed by CryptoMiniSat")
            counters["unsat"] += 1

        return True

    finally:
        try:
            os.unlink(cnf_path)
        except OSError:
            pass


def main():
    parser = argparse.ArgumentParser(
        description="Fuzz-test the oracle SAT solver")

    parser.add_argument("--verbose", "-v", action="store_true", default=False,
                        help="Print detailed output for each test")
    parser.add_argument("--tlimit", "-t", type=int, default=10,
                        help="Time limit in seconds for each solver run (default: 10)")
    parser.add_argument("--seed", "-s", type=int, default=None,
                        help="Starting seed (default: random)")
    parser.add_argument("--num", "-n", type=int, default=None,
                        help="Number of tests to run (default: infinite)")
    parser.add_argument("--oracle", type=str, default=None,
                        help="Path to oracle binary (auto-detected if not given)")
    parser.add_argument("--cms", type=str, default=None,
                        help="Path to cryptominisat5 binary (auto-detected if not given)")
    parser.add_argument("--fuzzer", type=str, default=None,
                        help="Path to cnf-fuzz-biere binary (auto-detected if not given)")
    parser.add_argument("--assump-fuzz", type=str, default=None, dest="assump_fuzz",
                        help="Path to assump_fuzz_oracle binary (auto-detected if not given)")

    args = parser.parse_args()
    args = auto_find_binaries(args)

    print("Oracle fuzzer")
    print("  oracle:      %s" % args.oracle)
    print("  cms:         %s" % args.cms)
    print("  fuzzer:      %s" % args.fuzzer)
    print("  assump_fuzz: %s" % args.assump_fuzz)
    print("  tlimit:      %d s" % args.tlimit)
    print()

    seed = args.seed
    if seed is None:
        seed = random.randint(0, 10**8)
    print("Starting seed: %d" % seed)

    test_num = 0
    counters = {"sat": 0, "unsat": 0, "unknown": 0, "timeout": 0,
                 "assump_sat": 0, "assump_unsat": 0, "assump_unknown": 0}
    start_time = time.time()

    try:
        while True:
            test_num += 1
            if args.num is not None and test_num > args.num:
                break

            if not args.verbose:
                elapsed = time.time() - start_time
                skip = counters["unknown"] + counters["timeout"]
                a_sat = counters["assump_sat"]
                a_unsat = counters["assump_unsat"]
                a_unk = counters["assump_unknown"]
                print("\rTest %d | seed %d | SAT:%d UNSAT:%d skip:%d | assump S:%d U:%d ?:%d | %.0fs"
                      % (test_num, seed, counters["sat"], counters["unsat"],
                         skip, a_sat, a_unsat, a_unk, elapsed),
                      end="", flush=True)
            else:
                print("Test %d (seed %d):" % (test_num, seed))

            ok = fuzz_one(args, test_num, seed, counters)
            if not ok:
                print("\n\nBUG FOUND at test %d, seed %d" % (test_num, seed))
                print("To reproduce: %s --seed %d --num 1 -v" % (sys.argv[0], seed))
                sys.exit(1)

            seed += 1

    except KeyboardInterrupt:
        print("\n\nInterrupted after %d tests" % test_num)

    elapsed = time.time() - start_time
    print("\n\nCompleted %d tests in %.1f s (%.1f tests/s)"
          % (test_num, elapsed, test_num / max(elapsed, 0.001)))
    print("All tests passed.")


if __name__ == "__main__":
    main()
