For issues with FRAT, always recompile with DEBUG_FRAT so you can see which functions are involved with the failing step

# Fuzzing -- MANDATORY after every change

After EVERY change to the solver you MUST fuzz it with the included fuzzer:

    cd scripts/fuzz && ./fuzz.py --fuzzlim 30

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
