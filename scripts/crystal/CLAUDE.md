# CrystalBall: how to run it (current status)

Everything below is the state as of 2026-09-25. `README.md` says what it
is; this says how to build, run and evaluate it, and what came out so far.

## Builds

Three builds, each in its own dir under the repo root:

| dir | cmake | what it is |
|---|---|---|
| `build/` | default | normal solver, the baseline |
| `build_stats/` | `-DSTATS=ON` | dumps clause data to SQLite, writes the FRAT proof |
| `build_pred/` | `-DFINAL_PREDICTOR=ON` | ranks reduce candidates by the xgboost prediction |

Build with `-j4`. Scripts that do the cmake calls, run from an empty dir:

```
cd build_stats && ../scripts/build_scripts/build_stats.sh
cd build_pred  && ../scripts/build_scripts/build_final_predictor.sh
```

Both need `../cadical/build` and `../cadiback/build` next to the repo.
Commit e1916b322 (2026-09-24) needs cadiback 0d92b68 (`CadiBack::doit`
takes the output prefix), which is not on GitHub yet; until `../cadiback`
has it, HEAD does not compile here and the three binaries on this box are
from 5644e4189, which has the same crystalball code and option names.
Dependencies on this box are under `../deps` (no sudo here):

```
SQLITE3_INCLUDE_DIR=$HOME/development/sat_solvers/deps/sqlite-amalgamation-3450100
SQLITE3_LIBRARY=$HOME/development/sat_solvers/deps/sqlite-amalgamation-3450100/libsqlite3.a
XGBOOST_INCLUDE_DIR=$HOME/development/sat_solvers/deps/xgboost-headers
XGBOOST_LIBRARY=$HOME/development/sat_solvers/deps/xgboost-pkg/xgboost/lib/libxgboost.so.3
```

Export the ones a build needs before its build script. The xgboost
library is the pip package's `.so` copied out: it has SONAME `.so.3` and an
RPATH of `$ORIGIN/../../xgboost.libs` for its libgomp, so the copy keeps
that layout. Python packages (xgboost 3.4, pandas 3, scikit-learn, numpy 2)
are installed with `pip3 install --user --break-system-packages`; `cnfgen`
too, for the smoke test.

The predictor build generates two things at build time:

- `predict_features_gen.h` from `best_features.txt` (`gen_pred_features.py`):
  the C++ that computes every feature. The feature file is the single
  source of truth for training and solving. Another list:
  `cmake -DPRED_FEATURES_FILE=/path/to/list.txt`.
- the embedded default models from `src/predict/predictor_{short,long,forever}.json`.
  These are NOT in git (`.gitignore` has `src/predict`); copy them there
  from a training run before building, and re-copy after retraining. The
  models must be trained on the same feature list the binary was built
  with: the feature count and order must agree.

Solver options that matter (all must come BEFORE the CNF file name; anything
after the CNF is taken as the proof file name, and the solver errors out):

- predictor build: `--predtype xgb` (or `py`, the Python path through
  `ml_module.py`, slow, for debugging), `--predloc DIR` (models
  `DIR/predictor-<table>-<tier>-xgb.json`, empty = the embedded ones),
  `--predtables 000|111` (per short/long/forever: 0 = `used_later`, 1 =
  `used_later_anc` models), `--predsortby 0..3` (which horizon reduce
  ranks by, 3 = sum of all three, the default), `--dumppreddistrib 1`
  (predictions of all clauses at every reduce to `pred_distrib.csv`).
- stats build: `--sql 2 --sqlitedb F --sqlitedboverwrite 1 --clid
  --cldatadumpratio R --cllockdatagen R --everypred N`, all set by
  `ballofcrystal.sh`.

Only WHICH candidates a reduce removes differs between the builds (same
reduce schedule, same number removed), so a conflict-count A/B is clean.
The solver is deterministic: the same binary, options and CNF give the
same conflict count, so one run per configuration is enough.

## The pipeline

`ballofcrystal.sh file.cnf` does all of it for one UNSAT instance, in
`<file.cnf>-dir/`:

1. **gather**: stats build with `--xor 0` (plain resolution proof), writes
   `data.db-raw` and `data.frat`. Must be UNSAT.
2. **label** (`fix_up_frat.py`): trims the proof to the steps reachable
   from the empty clause and fills `used_clauses` (tracked clause X was in
   the hint chain of a kept step at conflict C) and `used_clauses_anc`
   (also credits ancestors, weight 0.5 per generation, down to 0.05). The
   proof is deleted afterwards (GBs on big instances); `KEEP_FRAT=1` keeps it.
3. **clean/check/sample** (`clean_update_data.py`, `check_data_quality.py
   --slow`, `sample_data.py`): computes the `used_later*` labels over the
   SHORT/LONG/FOREVER horizons, sanity checks, then keeps a strata-balanced
   sample in `data-min.db`.
4. **frames** (`cldata_gen_pandas.py`): denormalises into six pandas
   frames `data-min.db-cldata-<table>-<tier>-cut1-..-cut2-..-limit-N.dat`,
   table in {used_later, used_later_anc}, tier in {short, long, forever}.
5. **learn** (`cldata_predict.py`): one xgboost regressor per frame,
   `predictor-<table>-<tier>-xgb.json`.
6. **evaluate**: predictor build with `--predtables 000` and `111`, the
   normal build, conflicts side by side.

Flags: `--gather-only` stops after 4 (for corpus learning), `--skip-solve`
redoes 2-6 from the existing `data.db-raw` (needs the proof, so
`KEEP_FRAT=1` on the first run), `--skip-learn` only reruns 6.

Knobs are in `setparams_ballofcrystal.sh`, all overridable from the
environment: `STATS_BIN PRED_BIN NORMAL_BIN`, `DUMPRATIO` (fraction of
learnt clauses tracked, 0.1), `CLLOCK` (fraction of tracked clauses never
deleted, 0.3, so the labels see what a kept clause does), `EVERYPRED`,
`SHORT LONG FOREVER` (10k/30k/120k conflicts), `FIXED` (rows per strata,
3000), `cut1 cut2`, `bestf` (feature file), `XGB_EST XGB_DEPTH
XGB_MINCHILD` (40 trees, depth 5, min child 10). `CAKE_XLRUP=""` skips the
optional proof check (`../frat-xor` + cake_xlrup, pointless on big proofs).

### Many instances

One instance is not enough: the models memorise it. Use a corpus:

```
./run_corpus.sh <outdir> a.cnf b.cnf ...   # gather each (skips dirs that have frames), learn.sh, eval_corpus.sh
./learn.sh <outdir> a.cnf-dir b.cnf-dir ...  # concat the frames (concat_pandas.py), train the six models
./eval_corpus.sh <preddir> a.cnf b.cnf ...   # normal vs pred 000 vs pred 111 on each, and the totals
```

`eval_corpus.sh` writes the solver outputs to `<preddir>/eval/` and takes
`PRED_OPTS` for extra predictor options. A hold-out test = `learn.sh` on
all dirs but one, `eval_corpus.sh` on the one left out.

### Choosing features

```
ONLY=0.1 ./gen_best_feats.sh <preddir>/comb- <outdir>     # importance rankings, 12 runs
./pick_features.py -n 30 --no-context -o best_features.txt <outdir>
```

`gen_best_feats.sh` trains on all raw columns and on all raw plus
computed relative features (thousands of columns, ~5 GB at 10% of the
rows; earlyoom kills anything much bigger on this box). It skips runs it
already has, so it can be restarted. `pick_features.py` sums the
importances over the runs, drops features the solver cannot compute (the
raw column table is in `gen_pred_features.py`, `--list-raw` prints it)
and, with `--no-context`, features made only of `rdb0_common.*` columns,
which are the same for every clause at a reduce and only identify the
instance. After changing the list: rebuild `build_pred`, retrain, copy the
models to `src/predict/`, rebuild again for the embedded defaults.

### Smoke test

```
./test_small.sh [seed]     # cnfgen random 3-SAT, ~100k conflicts, scaled horizons, ~3 min
```

Runs the whole pipeline, checks the plain and ancestor models differ and
that a second learning pass reproduces the models bit for bit. Run it
after touching any script, the stats/predictor code, or the schema.

## Practicalities

- Options before the CNF, always (see above).
- Labels need conflicts beyond the horizon: instances under ~200k
  conflicts need scaled-down `SHORT LONG FOREVER` (the smoke test uses
  2000/6000/20000).
- `DUMPRATIO=0.02` for instances of 1-2M conflicts, else the SQLite DB
  and the proof get out of hand (UTI: 0.01, 4.9 GB proof, ~1 h). A corpus
  dir is 0.2-1.2 GB after the proof is deleted.
- Features are float32 on both sides: ratios beyond float32 are "missing"
  in training and in the solver (no FE_OVERFLOW trap). The solver's FP
  traps are off while xgboost runs (`NoFPTraps`).
- The training is deterministic (fixed seed, sorted sampling); the same
  frames give the same models.
- Fuzz the normal build after touching `clause.h`, `searcher.cpp`,
  `reducedb.cpp` or the stats dumping: `cd scripts/fuzz && ./fuzz.py --fuzzlim 30`.
- `cb_test/` and `UTI-20-10p0.cnf.gz-dir/` in the repo root hold the runs
  below; neither is in git.

## Results so far

Corpus of 10 UNSAT instances in `cb_test/corpus/` (count14, php10,
subsetcard20, six random 3-SAT of 460k-2.2M conflicts) plus UTI-20-10p0;
130k training rows, 40 trees of depth 5, `FIXED=10000`. Conflicts of the
predictor build relative to the normal build:

| training | plain tables (000) | ancestor tables (111) |
|---|---|---|
| all 10, evaluated on the same 10 (in-sample) | 81% | 80% |
| 9 without UTI, evaluated on UTI (hold-out) | 93% | 109% |

UTI hold-out in numbers: normal 1,508,855 conflicts / 261 s, predictor
1,416,778 / 244 s. Per-instance in-sample results range from 53% (php10)
to 107% (r3-270-2). Models: `cb_test/learn-all-newfeats/` (all 10, these
are the embedded defaults), `cb_test/learn-noUTI-newfeats/` (hold-out).
Rankings for the current `best_features.txt`: `cb_test/feats-corpus/`.

With the previous hand-written 22-feature list the hold-out was 102%, so
the gain on unseen instances comes from the corpus-picked features.

## Open

- The ancestor-weighted labels generalise worse than the plain ones (109%
  hold-out); `--predtables 000` is the default for a reason.
- One hold-out instance is thin evidence. More families in the corpus
  (cnfgen kcolor/tseitin were trivial or XOR-heavy; matching, ec, cliqcol
  in `cb_test/corpus/` are untried).
- Tree count/depth and `FIXED` were not tuned; `--predsortby` 0/1/2 vs 3
  was not compared on the corpus.
- `src/predict/*.json` are not tracked, so a fresh checkout cannot build
  the predictor. Tracking them (320 KB of JSON) with the feature list they
  belong to would fix that.
