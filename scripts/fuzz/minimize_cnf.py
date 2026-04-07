#!/usr/bin/env python3
# Delta debugger for CNF minimization.
# 1. First minimizes --schedule and --preschedule elements in the repro script.
# 2. Then minimizes the CNF clauses.
# Calls <repro.sh> <cnf> to check if a candidate still triggers the crash.

import sys
import os
import re
import subprocess
import tempfile


def parse_cnf(fname):
    clauses = []
    with open(fname, "r") as f:
        for line in f:
            line = line.rstrip("\n")
            if line.startswith("c") or line.startswith("p") or line.strip() == "":
                continue
            clauses.append(line)
    return clauses


def write_cnf(fname, clauses):
    max_var = 0
    for cl in clauses:
        for lit in cl.split():
            v = abs(int(lit))
            if v > max_var:
                max_var = v
    with open(fname, "w") as f:
        f.write("p cnf %d %d\n" % (max_var, len(clauses)))
        for cl in clauses:
            f.write(cl + "\n")


FAILURE_MARKERS = [
    "Checking failed",
    "panicked",
    "Unit propagation stuck",
    "verifier error",
    "empty clause not derived",
]


def run_repro(repro_script, cnf_file):
    result = subprocess.run(
        [repro_script, cnf_file],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        universal_newlines=True,
    )
    out = result.stdout
    # Reject SAT results — "UNSATISFIABLE" contains "SATISFIABLE" as substring,
    # so check both to distinguish them.
    if "s SATISFIABLE" in out and "s UNSATISFIABLE" not in out:
        return False
    return any(m in out for m in FAILURE_MARKERS)


def is_crash(clauses, tmpfile, repro_script, verbose=True):
    write_cnf(tmpfile, clauses)
    if verbose:
        print("  Running: %s %s  (%d clauses)" % (repro_script, tmpfile, len(clauses)))
    crashed = run_repro(repro_script, tmpfile)
    if verbose:
        print("  Result: %s" % ("CRASH" if crashed else "no crash"))
    return crashed


def save_intermediate(clauses, base, step):
    fname = "%s_intermediate_%03d_%dclauses.cnf" % (base, step, len(clauses))
    write_cnf(fname, clauses)
    print("  Saved intermediate CNF: %s" % fname)
    return fname


def parse_schedules(repro_script):
    with open(repro_script) as f:
        content = f.read()
    schedules = {}
    for key in ("--schedule", "--preschedule"):
        m = re.search(r"%s\s+(\S+)" % re.escape(key), content)
        if m:
            schedules[key] = m.group(1).split(",")
    return schedules


def make_repro_with_schedules(repro_script, schedules):
    with open(repro_script) as f:
        content = f.read()
    for key, elements in schedules.items():
        new_val = ",".join(elements)
        content = re.sub(r"(%s\s+)\S+" % re.escape(key),
                         lambda m, v=new_val: m.group(1) + v, content)
    script_dir = os.path.dirname(os.path.abspath(repro_script))
    tmp = tempfile.NamedTemporaryFile(suffix=".sh", delete=False, mode="w", dir=script_dir)
    tmp.write(content)
    tmp.close()
    os.chmod(tmp.name, 0o755)
    return tmp.name


def minimize_schedule(repro_script, cnf_file):
    schedules = parse_schedules(repro_script)
    if not schedules:
        print("No --schedule or --preschedule found in repro script, skipping.")
        return repro_script

    for key in ("--schedule", "--preschedule"):
        if key not in schedules:
            continue
        elements = list(schedules[key])
        print("\nMinimizing %s (%d elements): %s" % (key, len(elements), ",".join(elements)))
        i = 0
        while i < len(elements):
            candidate = elements[:i] + elements[i + 1:]
            if not candidate:
                i += 1
                continue
            schedules[key] = candidate
            tmp_repro = make_repro_with_schedules(repro_script, schedules)
            try:
                crashed = run_repro(tmp_repro, cnf_file)
            finally:
                os.unlink(tmp_repro)
            if crashed:
                print("  Removed %s element [%d]: %s  (%d -> %d elements)" % (
                    key, i, elements[i], len(elements), len(candidate)))
                elements = candidate
            else:
                schedules[key] = elements
                i += 1
        schedules[key] = elements
        print("  Final %s (%d elements): %s" % (key, len(elements), ",".join(elements)))

    minimized_repro = os.path.splitext(repro_script)[0] + "_sched_minimized.sh"
    final_repro = make_repro_with_schedules(repro_script, schedules)
    os.rename(final_repro, minimized_repro)
    os.chmod(minimized_repro, 0o755)
    print("\nMinimized repro script written to: %s" % minimized_repro)
    return minimized_repro


def ddmin(clauses, tmpfile, base, repro_script):
    chunk_size = len(clauses) // 2
    oracle_calls = 0
    step = 0

    while chunk_size > 0:
        print("\n-- chunk_size=%d, current clauses=%d --" % (chunk_size, len(clauses)))
        i = 0
        made_progress = False
        while i < len(clauses):
            end = min(i + chunk_size, len(clauses))
            candidate = clauses[:i] + clauses[end:]
            print("  Trying to remove clauses [%d:%d] (%d clauses -> %d)" % (
                i, end, len(clauses), len(candidate)))
            oracle_calls += 1
            if is_crash(candidate, tmpfile, repro_script):
                step += 1
                print("  REDUCED: %d -> %d clauses (removed %d at offset %d)" % (
                    len(clauses), len(candidate), end - i, i))
                clauses = candidate
                made_progress = True
                save_intermediate(clauses, base, step)
            else:
                i += chunk_size
        if not made_progress:
            chunk_size //= 2

    return clauses, oracle_calls


def main():
    if len(sys.argv) < 3:
        print("Usage: %s <input.cnf> <repro.sh>" % sys.argv[0])
        sys.exit(1)

    input_fname = sys.argv[1]
    repro_script = sys.argv[2]

    if not os.path.exists(input_fname):
        print("Error: file not found: %s" % input_fname)
        sys.exit(1)

    if not os.path.exists(repro_script):
        print("Error: repro script not found: %s" % repro_script)
        sys.exit(1)

    clauses = parse_cnf(input_fname)
    base = os.path.splitext(os.path.basename(input_fname))[0]
    print("Parsed %d clauses from %s" % (len(clauses), input_fname))

    tmpfile = tempfile.NamedTemporaryFile(suffix=".cnf", delete=False).name
    try:
        print("\nVerifying original CNF crashes...")
        if not is_crash(clauses, tmpfile, repro_script, verbose=False):
            print("ERROR: original CNF does not trigger the crash. Aborting.")
            sys.exit(1)
        print("Confirmed crash.\n")

        print("=== Phase 1: Minimizing schedule ===")
        repro_script = minimize_schedule(repro_script, input_fname)

        print("\n=== Phase 2: Minimizing CNF clauses ===")
        minimized, oracle_calls = ddmin(clauses, tmpfile, base, repro_script)

        out_fname = base + "_minimized.cnf"
        write_cnf(out_fname, minimized)

        print("\n=== Done ===")
        print("Original: %d clauses" % len(clauses))
        print("Minimized: %d clauses (%d oracle calls)" % (len(minimized), oracle_calls))
        print("Minimized CNF written to: %s" % out_fname)
        print("Minimized repro:  %s" % repro_script)
        print("Verify with: %s %s" % (repro_script, out_fname))
    finally:
        if os.path.exists(tmpfile):
            os.unlink(tmpfile)


if __name__ == "__main__":
    main()
