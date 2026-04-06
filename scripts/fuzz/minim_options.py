#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Minimizes solver command-line options while preserving a bug.
# Supports two bug types:
#   1. SAT solution doesn't satisfy the CNF
#   2. Solver says UNSAT but another solver (lingeling) finds SAT
#
# Usage:
#   ./minim_options.py --cmd "<full solver command>" --cnf <input.cnf>
#
# Example:
#   ./minim_options.py --cmd "../../build/cryptominisat5 --zero-exit-status \
#     --presimp 1 --schedule occ-xor,occ-bve --threads 1" \
#     --cnf out/fuzzTest_104.cnf

import argparse
import subprocess
import sys
import os
import shlex
import resource
from functools import partial

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
from verifier import solution_parser


def setlimits(maxtime):
    resource.setrlimit(resource.RLIMIT_CPU, (maxtime, maxtime))


def parse_solver_cmd(cmd_str):
    """Parse a solver command into (solver_binary, list_of_option_pairs, trailing_files).

    Returns:
        solver: str - the solver binary path
        opts: list of (key, value_or_None) tuples
        trailing: list of positional args (cnf file, frat file, etc.)
    """
    tokens = shlex.split(cmd_str)

    # skip doalarm prefix if present
    i = 0
    while i < len(tokens):
        if tokens[i] == "./doalarm" or tokens[i] == "doalarm":
            # skip: doalarm -t real <N>
            i += 1
            while i < len(tokens) and tokens[i].startswith("-"):
                i += 1  # skip -t
                i += 1  # skip "real"
                i += 1  # skip <N>
            continue
        break

    solver = tokens[i]
    i += 1

    opts = []
    trailing = []
    while i < len(tokens):
        tok = tokens[i]
        if tok.startswith("--") or (tok.startswith("-") and not tok[1:].replace('.', '').replace('-', '').isdigit()):
            key = tok
            # check if next token is a value (not another flag)
            if i + 1 < len(tokens) and not tokens[i + 1].startswith("--"):
                # could be a value or another short flag
                next_tok = tokens[i + 1]
                if next_tok.startswith("-") and len(next_tok) > 1 and next_tok[1:2].isalpha():
                    # it's another flag like -m
                    if next_tok == "-m":
                        # -m is a flag with value
                        opts.append((key, None))
                    else:
                        opts.append((key, None))
                else:
                    opts.append((key, next_tok))
                    i += 1
            else:
                opts.append((key, None))
        elif tok.startswith("-") and len(tok) == 2 and tok[1].isalpha():
            # short flag like -m
            key = tok
            if i + 1 < len(tokens) and not tokens[i + 1].startswith("-"):
                opts.append((key, tokens[i + 1]))
                i += 1
            else:
                opts.append((key, None))
        else:
            trailing.append(tok)
        i += 1

    return solver, opts, trailing


def build_cmd(solver, opts, cnf_file, extra_trailing=None):
    """Build command string from solver, options, and cnf file."""
    parts = [solver]
    for key, val in opts:
        parts.append(key)
        if val is not None:
            parts.append(str(val))
    parts.append(cnf_file)
    if extra_trailing:
        parts.extend(extra_trailing)
    return parts


class BugChecker:
    def __init__(self, cnf_file, maxtime=25, verbose=False):
        self.cnf_file = cnf_file
        self.maxtime = maxtime
        self.verbose = verbose
        self.bug_type = None  # will be set after first check

    def run_solver(self, cmd_parts):
        """Run solver and return (stdout_text, returncode)."""
        if self.verbose:
            print("  Running: %s" % " ".join(cmd_parts))

        try:
            p = subprocess.Popen(
                cmd_parts,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                preexec_fn=partial(setlimits, self.maxtime),
                universal_newlines=True)
            stdout, stderr = p.communicate(timeout=self.maxtime + 10)
            return stdout, p.returncode
        except subprocess.TimeoutExpired:
            p.kill()
            p.communicate()
            return "", -1
        except Exception as e:
            if self.verbose:
                print("  Exception running solver: %s" % e)
            return "", -1

    def check_bug(self, cmd_parts):
        """Check if the bug still reproduces. Returns True if bug is present."""
        stdout, retcode = self.run_solver(cmd_parts)

        if retcode != 0:
            if self.verbose:
                print("  Non-zero return code: %d" % retcode)
            return False

        if not stdout.strip():
            if self.verbose:
                print("  Empty output")
            return False

        try:
            unsat, solution, _ = solution_parser.parse_solution_from_output(
                stdout.split("\n"), ignoreNoSolution=True)
        except SystemExit:
            if self.verbose:
                print("  Could not parse output")
            return False

        if unsat is True and solution == []:
            # timeout or no solution
            if self.verbose:
                print("  Timeout / no solution")
            return False

        if not unsat:
            # SAT - check if solution satisfies the CNF
            try:
                solution_parser.test_found_solution(solution, self.cnf_file)
                if self.verbose:
                    print("  SAT solution is correct - bug gone")
                return False
            except NameError:
                # solution doesn't satisfy the CNF - bug!
                if self.bug_type is None:
                    self.bug_type = "wrong_sat"
                if self.bug_type == "wrong_sat":
                    return True
                return False
        else:
            # UNSAT - check with lingeling
            if self.bug_type is None or self.bug_type == "wrong_unsat":
                ret = self._check_unsat_with_lingeling()
                if ret is False:
                    # lingeling found SAT, so our UNSAT is wrong
                    if self.bug_type is None:
                        self.bug_type = "wrong_unsat"
                    return True
                if self.verbose:
                    print("  UNSAT confirmed or timeout")
                return False
            return False

    def _check_unsat_with_lingeling(self):
        """Returns True if UNSAT confirmed, False if SAT found, None if timeout."""
        toexec = "../../build/utils/lingeling-ala/lingeling %s" % self.cnf_file
        try:
            p = subprocess.Popen(
                toexec.split(),
                stdout=subprocess.PIPE,
                preexec_fn=partial(setlimits, self.maxtime),
                universal_newlines=True)
            stdout, _ = p.communicate(timeout=self.maxtime + 10)
        except (subprocess.TimeoutExpired, OSError) as e:
            if self.verbose:
                print("  Lingeling error/timeout: %s" % e)
            try:
                p.kill()
                p.communicate()
            except:
                pass
            return None

        try:
            unsat, _, _ = solution_parser.parse_solution_from_output(
                stdout.split("\n"), ignoreNoSolution=True)
            return unsat
        except SystemExit:
            return None


def minimize_options(solver, opts, cnf_file, checker, verbose=False):
    """Minimize options while preserving the bug."""

    current_opts = list(opts)
    total_removed = 0

    # Phase 1: Try removing each option entirely
    print("\n=== Phase 1: Removing individual options ===")
    i = 0
    while i < len(current_opts):
        key, val = current_opts[i]

        # Never remove --zero-exit-status or --threads
        if key in ("--zero-exit-status", "--threads"):
            i += 1
            continue

        candidate = current_opts[:i] + current_opts[i + 1:]
        cmd = build_cmd(solver, candidate, cnf_file)
        print("  Try removing %s %s ..." % (key, val if val else ""), end="", flush=True)

        if checker.check_bug(cmd):
            print(" REMOVED")
            current_opts = candidate
            total_removed += 1
            # don't increment i, the next option shifted into this position
        else:
            print(" kept")
            i += 1

    # Phase 2: Minimize comma-separated schedule options
    print("\n=== Phase 2: Minimizing schedule/preschedule ===")
    schedule_keys = ("--schedule", "--preschedule")
    for sched_key in schedule_keys:
        idx = None
        for j, (key, val) in enumerate(current_opts):
            if key == sched_key:
                idx = j
                break
        if idx is None:
            continue

        val = current_opts[idx][1]
        if val is None:
            continue

        items = val.split(",")
        if len(items) <= 1:
            continue

        print("  Minimizing %s (%d items)..." % (sched_key, len(items)))

        # Try removing each item
        j = 0
        while j < len(items):
            if len(items) <= 1:
                break
            candidate_items = items[:j] + items[j + 1:]
            candidate_opts = list(current_opts)
            candidate_opts[idx] = (sched_key, ",".join(candidate_items))
            cmd = build_cmd(solver, candidate_opts, cnf_file)
            print("    Try removing '%s' ..." % items[j], end="", flush=True)

            if checker.check_bug(cmd):
                print(" REMOVED")
                items = candidate_items
                current_opts[idx] = (sched_key, ",".join(items))
                total_removed += 1
            else:
                print(" kept")
                j += 1

        print("  %s minimized to %d items: %s" % (sched_key, len(items), ",".join(items)))

    # Phase 3: Try to remove schedule/preschedule entirely (now that they're minimal)
    print("\n=== Phase 3: Try removing minimized schedules entirely ===")
    for sched_key in schedule_keys:
        idx = None
        for j, (key, val) in enumerate(current_opts):
            if key == sched_key:
                idx = j
                break
        if idx is None:
            continue

        candidate = current_opts[:idx] + current_opts[idx + 1:]
        cmd = build_cmd(solver, candidate, cnf_file)
        print("  Try removing %s entirely ..." % sched_key, end="", flush=True)
        if checker.check_bug(cmd):
            print(" REMOVED")
            current_opts = candidate
            total_removed += 1
        else:
            print(" kept")

    # Phase 4: For options with value 1, try setting to 0 (disable features)
    print("\n=== Phase 4: Try disabling features (1 -> 0) ===")
    for i, (key, val) in enumerate(current_opts):
        if key in ("--zero-exit-status", "--threads"):
            continue
        if val == "1":
            candidate = list(current_opts)
            candidate[i] = (key, "0")
            cmd = build_cmd(solver, candidate, cnf_file)
            print("  Try %s 0 ..." % key, end="", flush=True)
            if checker.check_bug(cmd):
                print(" SET TO 0")
                current_opts = candidate
                total_removed += 1
            else:
                print(" kept at 1")

    print("\n=== Minimization complete (removed/simplified %d options) ===" % total_removed)
    return current_opts


def main():
    parser = argparse.ArgumentParser(
        description="Minimize solver options while preserving a bug.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""Examples:
  %(prog)s --cmd "../../build/cryptominisat5 --zero-exit-status --presimp 1 --threads 1" --cnf out/fuzzTest_104.cnf
  %(prog)s --cmd "../../build/cryptominisat5 --zero-exit-status --schedule occ-xor,occ-bve --threads 1" --cnf out/fuzzTest_104.cnf -t 30
""")

    parser.add_argument("--cmd", required=True,
                        help="Full solver command line (without CNF file, or it will be replaced)")
    parser.add_argument("--cnf", required=True,
                        help="CNF file to test against")
    parser.add_argument("-t", "--timeout", type=int, default=25,
                        help="Timeout per solver run in seconds (default: 25)")
    parser.add_argument("-v", "--verbose", action="store_true", default=False,
                        help="Verbose output")
    parser.add_argument("--bug-type", choices=["wrong_sat", "wrong_unsat"],
                        default=None,
                        help="Expected bug type. If not set, auto-detected from first run.")

    args = parser.parse_args()

    if not os.path.exists(args.cnf):
        print("Error: CNF file not found: %s" % args.cnf)
        sys.exit(1)

    # Parse the command
    solver, opts, trailing = parse_solver_cmd(args.cmd)

    if not os.path.exists(solver):
        print("Error: solver not found: %s" % solver)
        sys.exit(1)

    # Filter out CNF/FRAT files from trailing (we use --cnf instead)
    extra_trailing = [t for t in trailing if not t.endswith(".cnf")]

    print("Solver: %s" % solver)
    print("CNF: %s" % args.cnf)
    print("Options (%d):" % len(opts))
    for key, val in opts:
        print("  %s %s" % (key, val if val else ""))
    if extra_trailing:
        print("Extra trailing args: %s" % extra_trailing)

    checker = BugChecker(args.cnf, maxtime=args.timeout, verbose=args.verbose)
    if args.bug_type:
        checker.bug_type = args.bug_type

    # Verify bug reproduces with original options
    print("\n=== Verifying bug reproduces with original options ===")
    cmd = build_cmd(solver, opts, args.cnf, extra_trailing if extra_trailing else None)
    if not checker.check_bug(cmd):
        print("ERROR: Bug does not reproduce with the given command!")
        print("Command: %s" % " ".join(cmd))
        sys.exit(1)

    print("Bug confirmed! Type: %s" % checker.bug_type)

    # Minimize
    minimized_opts = minimize_options(
        solver, opts, args.cnf, checker, verbose=args.verbose)

    # Print result
    final_cmd = build_cmd(solver, minimized_opts, args.cnf,
                          extra_trailing if extra_trailing else None)
    print("\n" + "=" * 60)
    print("MINIMIZED COMMAND:")
    print("=" * 60)
    print(" ".join(final_cmd))
    print("=" * 60)

    # Also print just the options for easy copy-paste
    print("\nMinimized options:")
    for key, val in minimized_opts:
        print("  %s %s" % (key, val if val else ""))


if __name__ == "__main__":
    main()
