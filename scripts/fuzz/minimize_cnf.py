#!/usr/bin/env python3
# Delta debugger for CNF minimization.
# Calls ./repro.sh <cnf> to check if a candidate CNF still triggers the crash.
# Saves every intermediate CNF that is smaller and still crashes.

import sys
import os
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


def is_crash(clauses, tmpfile, verbose=True):
    write_cnf(tmpfile, clauses)
    if verbose:
        print("  Running: ./repro.sh %s  (%d clauses)" % (tmpfile, len(clauses)))
    result = subprocess.run(
        ["./repro.sh", tmpfile],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        universal_newlines=True,
    )
    crashed = "Checking failed" in result.stdout
    if verbose:
        print("  Result: %s" % ("CRASH" if crashed else "no crash"))
    return crashed


def save_intermediate(clauses, base, step):
    fname = "%s_intermediate_%03d_%dclauses.cnf" % (base, step, len(clauses))
    write_cnf(fname, clauses)
    print("  Saved intermediate CNF: %s" % fname)
    print("  Replay with: ./repro.sh %s" % fname)
    return fname


def ddmin(clauses, tmpfile, base):
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
            if is_crash(candidate, tmpfile):
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
    if len(sys.argv) < 2:
        print("Usage: %s <input.cnf>" % sys.argv[0])
        sys.exit(1)

    input_fname = sys.argv[1]
    if not os.path.exists(input_fname):
        print("Error: file not found: %s" % input_fname)
        sys.exit(1)

    if not os.path.exists("./repro.sh"):
        print("Error: ./repro.sh not found")
        sys.exit(1)

    clauses = parse_cnf(input_fname)
    base = os.path.splitext(os.path.basename(input_fname))[0]
    print("Parsed %d clauses from %s" % (len(clauses), input_fname))

    tmpfile = tempfile.NamedTemporaryFile(suffix=".cnf", delete=False).name
    try:
        print("\nVerifying original CNF crashes...")
        print("  Running: ./repro.sh %s  (%d clauses)" % (input_fname, len(clauses)))
        if not is_crash(clauses, tmpfile, verbose=False):
            print("ERROR: original CNF does not trigger the crash. Aborting.")
            sys.exit(1)
        print("Confirmed crash. Starting minimization...\n")

        minimized, oracle_calls = ddmin(clauses, tmpfile, base)

        out_fname = base + "_minimized.cnf"
        write_cnf(out_fname, minimized)

        print("\n=== Done ===")
        print("Original: %d clauses" % len(clauses))
        print("Minimized: %d clauses (%d oracle calls)" % (len(minimized), oracle_calls))
        print("Minimized CNF written to: %s" % out_fname)
        print("Verify with: ./repro.sh %s" % out_fname)
    finally:
        if os.path.exists(tmpfile):
            os.unlink(tmpfile)


if __name__ == "__main__":
    main()
