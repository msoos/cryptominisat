# Determinism -- NO time-based cutoffs

The solver must be deterministic: two runs of the same binary on the same input
with the same options produce byte-identical output and proofs. So NO cutoff,
budget or limit may be driven by wall-clock or CPU time. Use deterministic step
counters -- bogoprops, ticks, `limit_to_decrease`, conflict counts, byte counts.

`cpu_time()` is for reporting only: stats lines, SQL, verbose output. The one
exception is `conf.maxTime` (`--maxtime`), a user-requested overall giving-up
point, off by default.

If a run is not reproducible, that is a bug -- do not explain a flaky result
away as timing. Diff the proofs of two runs first.

For issues with FRAT, always recompile with DEBUG_FRAT so you can see which
functions are involved with the failing step. Note that cake_xlrup cannot parse
the `c ...` lines this adds, so strip them before checking.


# Fuzzing -- MANDATORY after every change

After EVERY change to the solver you MUST fuzz it with the included fuzzer:

    cd scripts/fuzz && ./fuzz.py --fuzzlim 30

Fuzz on all 16 cores -- `unique_file()` uses O_CREAT|O_EXCL, so many `fuzz.py`
can share one `out/` dir. Launch them yourself in the background, or use
`./fuzz_session_cms.sh --num 16 [fuzz.py opts]` for an interactive tmux session.

- `--fuzzlim N` limits the number of fuzzing rounds. Use at least 30 for
  search/propagation/simplification changes, 10 for trivial ones.
- `--seed N` reproduces a run exactly (each round prints its re-create line).
- The fuzzer needs `build/` made with `-DENABLE_TESTING=ON` (generators live in
  `build/tests/cnf-utils` and `build/tests/sha1-sat`, cross-check solver in
  `build/utils/lingeling-ala`).
- Best practice is fuzzing a sanitized build: run `scripts/build_scripts/fuzz_test_sanitize.sh [rounds]`
  from an empty build dir (e.g. `build_fuzz/`). It builds with clang
  `-DSANITIZE=ON -DSLOW_DEBUG=ON` and fuzzes that binary.
- On failure the fuzzer writes a repro script; minimize with
  `scripts/fuzz/minimize_cnf.py <cnf> <repro-script>`.
- `--assumps` draws the interspersed `Solver::solve()` assumptions from a model
  of the whole CNF, so every one of them must come back SAT. This is the only
  check that catches wrong UNSAT-under-assumptions.

# building

Build with `-j16`.
