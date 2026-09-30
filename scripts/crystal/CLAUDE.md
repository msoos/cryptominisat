# CrystalBall: how to run it (current status)

Everything below is the state as of 2026-09-30. `README.md` says what it
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

Both need `../cadical/build` and `../cadiback/build` next to the repo
(cadiback at 0d92b68 or later).
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
- the embedded default models from `src/predict/predictor_<tier>.json`, one
  per tier of cmake's `PRED_TIERS` (default `disc`; `embed_models.py`
  writes the table the solver looks them up in by tier name).
  `src/predict/` is its own git repo (github.com/msoos/cryptominisat-predictors),
  ignored by the solver's repo: clone it there before building. After
  retraining, copy the new models in and commit there (never push, the
  owner pushes). The models must be trained on the same feature list
  the binary was built with: the feature count and order must agree.

Solver options that matter (all must come BEFORE the CNF file name; anything
after the CNF is taken as the proof file name, and the solver errors out):

- predictor build: `--predtype xgb` (or `py`, the Python path through
  `ml_module.py`, slow, for debugging), `--predtiers disc` (the models,
  comma separated, at most three: `disc` = one model of the discounted
  future use, the default; `short,long,forever` = the three use-count
  horizons), `--predloc DIR` (models `DIR/predictor-<table>-<tier>-xgb.json`,
  empty = the embedded ones), `--predtables 000|111` (per model: 0 =
  `used_later`, 1 = `used_later_anc`), `--predsortby 0..3` (one model's
  score, or 3 = their sum, the default), `--predkeep 1` (the score
  decides the tiers instead of glue: the `--predkeept1` % best-scored
  learnt clauses are kept while used, up to `--predkeept2` % if used
  since the last reduce; predicts for every learnt clause at every
  reduce), `--predcands 0|1|2`
  (what it ranks: 0 = the normal build's candidates, 1 = also the clauses
  the normal rules keep for being used, 2 = also the tier1-keep ones; the
  number removed stays the normal build's), `--predthresh T` (remove the
  candidates predicted below T instead of the fixed number, bounded to
  0.5x-2x of it), `--dumppreddistrib 1` (predictions of all clauses at
  every reduce to `pred_distrib.csv`).
- stats build: `--sql 2 --sqlitedb F --sqlitedboverwrite 1 --clid
  --cldatadumpratio R --cllockdatagen R --everypred N`, all set by
  `ballofcrystal.sh`.

With `--predcands 0` only WHICH candidates a reduce removes differs
between the builds (same reduce schedule, same number removed), so a
conflict-count A/B is clean. With `--predcands 1` the predictor may also
drop clauses the normal build protects, and the DB gets smaller (25% on
bivium): compare against the normal build with `--reducekeepused 0` too,
which also shrinks the DB (on bivium: 102% conflicts, 78% time).
The solver is deterministic: the same binary, options and CNF give the
same conflict count, so one run per configuration is enough.

## The pipeline

`ballofcrystal.sh file.cnf` does all of it for one UNSAT instance, in
`<file.cnf>-dir/`:

1. **gather**: stats build with `--xor 0` (plain resolution proof), writes
   `data.db-raw` and `data.frat`. Must be UNSAT (see below).
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
   Then `check_frames.py` tests them (see below); a failed test stops
   the pipeline, and `learn.sh` runs it on every input dir too.
5. **learn** (`cldata_predict.py`): one xgboost regressor per frame,
   `predictor-<table>-<tier>-xgb.json`.
6. **evaluate**: predictor build with `--predtables 000` and `111`, the
   normal build, conflicts side by side.

Flags: `--gather-only` stops after 4 (for corpus learning), `--skip-solve`
redoes 2-6 from the existing `data.db-raw` (needs the proof, so
`KEEP_FRAT=1` on the first run), `--skip-learn` only reruns 6.
**Only UNSAT instances, ever.** A label is "took part in the trimmed
UNSAT proof"; a SAT run has no empty clause, so no proof and no labels.
"Took part in learning some clause" is a different, noisier quantity
(most learnt clauses lead nowhere) and mixing the two in one training
set is wrong. The pipeline refuses SAT runs and there is no switch.

Labels: one model per tier in `TIERS`. The default tier is `disc`: every
future use of the clause, discounted by its distance, halving every
`HALFLIFE` (30k) conflicts, one horizon-free label. The old tiers
`short`, `long`, `forever` are use counts over the next `SHORT`/`LONG`/
`FOREVER` conflicts (10k/30k/120k), three models whose sum the solver
ranked by. A row counts only if the clause stayed in the solver for the
horizon (two half-lives for `disc`). `x.<table>_<tier>` is the label
itself; `x.<table>_<tier>_rel` (`TARGET=rel`, the default) is the share
of the clauses at the same reduce that score less, 0..1, never-used = 0.
Reduce only orders the clauses present at one reduce and use counts
differ 100-fold between families, so `rel` is the instance-invariant
target for a general model. The ranking quality report is always on
the label itself, whichever is learnt.

### The data tests (`check_frames.py`)

Run before any learning. Fails on: missing columns, other tiers' labels
in a frame, inf, NaN/negative/non-integer labels, all-zero labels, size
< 3, `used` outside 0..31, negative ages, ternary resolvents with
learning-time data or learnt clauses without it, a clause larger than
when learnt, per-reduce (`rdb0_common.*`) values that differ within one
reduce, rate columns that are always ~0, features that look into the
future or need a column that is not there, duplicate (clause, reduce)
rows, and labels not lined up with the `used` counter (a clause used
since the last reduce must be used more later than a never-used one:
the off-by-one-reduce bugs break this). With the three tiers given
together, short <= long <= forever per clause and reduce. Warns on
constant columns, glue > size (glue is not refreshed when a clause
shrinks) and glue above orig_glue. It found the `avg_sum_*_per_time`
bug (divided by the clause count twice, always ~1e-8).

Knobs are in `setparams_ballofcrystal.sh`, all overridable from the
environment: `STATS_BIN PRED_BIN NORMAL_BIN`, `DUMPRATIO` (fraction of
learnt clauses tracked, 0.1), `CLLOCK` (fraction of tracked clauses never
deleted, 0.3, so the labels see what a kept clause does), `EVERYPRED`,
`SHORT LONG FOREVER` (10k/30k/120k conflicts), `FIXED` (rows per strata,
3000), `cut1 cut2`, `bestf` (feature file), `XGB_EST XGB_DEPTH
XGB_MINCHILD` (40 trees, depth 5, min child 10), `XGB_OBJ` (training
objective: `squarederror`, `log` = squared error of log(1+use), `poisson`).
`CAKE_XLRUP=""` skips the optional proof check (`../frat-xor` +
cake_xlrup, pointless on big proofs).

The learn output reports, next to the squared error, the **ranking
quality**: the share of the future use kept when keeping the best 25% /
50% of the test clauses by the model, by glue then size (the normal
build's order) and by the truth. "cands" restricts it to clauses not used
since the reduce before, which is what reduce picks from. Reduce only
ranks, so this is the number to watch; it is on the strata-balanced
sample, so only the comparison between the orders means something.

### Many instances

One instance is not enough: the models memorise it. Use a corpus:

```
./run_corpus.sh <outdir> a.cnf b.cnf ...   # gather each (skips dirs that have frames), learn.sh, eval_corpus.sh
./learn.sh <outdir> a.cnf-dir b.cnf-dir ...  # concat the frames (concat_pandas.py), train the six models
./eval_corpus.sh <preddir> a.cnf b.cnf ...   # normal vs pred 000 vs pred 111 on each, and the totals
```

`eval_corpus.sh` writes the solver outputs to `<preddir>/eval/` and takes
`PRED_OPTS` (extra predictor options), `EVAL_OPTS` (options for all runs,
e.g. `--xor 0`), `EVAL_TABLES` (default `000 111`) and
`EVAL_NORMAL_CACHE` (a dir where the normal build's runs are kept and
reused). A hold-out test = `learn.sh` on all dirs but one,
`eval_corpus.sh` on the one left out.

### Choosing features

```
ONLY=0.2 ./gen_best_feats.sh <preddir>/comb- <outdir>     # importance rankings, 12 runs
./pick_features.py -n 30 --no-context -o best_features.txt <outdir>
```

`gen_best_feats.sh` trains on all raw columns and on all raw plus
computed relative features (thousands of columns). xgboost's memory is
features x tree nodes, so it uses the models' depth (`XGB_DEPTH`, 5):
about 5 GB at 30% of the rows, and earlyoom kills anything much bigger
on this box. `COMPUTED=no` does only the quick raw runs, `TABLES=used_later`
only the plain label tables, `XGB_OBJ` the objective. It skips runs it
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
  dir is 0.2-1.2 GB after the proof is deleted. The proof is about 2 GB
  per million conflicts: with the disk full the stats run dies with an
  SQLite "SQL logic error" on an insert (schur-triples, 3.7M conflicts).
- Never edit a script while a run uses it: bash reads scripts
  incrementally and a truncated file gives a syntax error mid-run. Write
  a temp file and rename it over the old one.
- Gathering a mixed corpus: `STATS_BIN` with `STATS_OPTS="--predtype
  xgb --predloc DIR"` and the `build_stats_pred/` build (STATS=ON and
  FINAL_PREDICTOR=ON together) gathers under the learnt policy.
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

## Bivium (2026-09-30)

The two SAT Competition 2020 Bivium CNFs (`~/media/satcomp2020/bivium-*`)
have their guessed state bits propagated into the clauses, so
`bivium_variants.py base.cnf.gz -n 6 --seed S` makes UNSAT variants by
guessing N more state bits at random; every bit halves the work (n=6:
0.2-2M conflicts, n=8: 300k). Everything is in `cb_test/bivium/`:
`pool/` the measured candidates, `train/` 10 instances (5 per base CNF,
n=5/6, 220k-2.1M conflicts, gathered with `DUMPRATIO` 0.06 or 0.03 for
the >1M ones, `FIXED=10000`), `test/` 4 others (1.3-1.8M), `models/<name>/`
the models and `results-*.out` the A/B tables, all `--xor 0` (the stats
run has no XOR reasoning). `exp.sh <name>` learns and evaluates one
configuration, `chain*.sh` are the runs made. Conflicts / time of the
predictor build relative to the normal build on the 4 held-out test
instances (time is under load unless marked quiet, so indicative):

| models | `--predcands 0` | `--predcands 1` |
|---|---|---|
| squared error (the default) | 90% / 90% | 82% / 65% |
| log objective | 75% / 71% | 74% / 65% (quiet: 262 s vs 400 s) |
| poisson | 88% / 117% | 85% / 72% |
| log, features picked on bivium | 84% / 78% | 87% / 70% |
| log, `--predthresh 1` | | 71% / 77% |
| poisson, `--predthresh 3` | | 72% / 75% |
| log, `--predcands 1`, default options (XOR reasoning on) | | 77% / 80% |

In-sample on the 10 training instances: log + predcands 1 gives 70% /
70%, squared error 83% / 88%. Offline, glue/size is already a strong order
on bivium (93.5% of the future use kept at 25% for the short horizon, vs
75% on the mixed corpus) and `rdb0.size` carries most of the importance.
A normal build with `--reducekeepused 0` gets 102% / 78%, so about half
of the time gain of `--predcands 1` is the smaller DB (25% smaller), the
conflict gain is the ranking.

**But not on the mixed corpus.** Same comparisons on the 10-instance
corpus of `cb_test/corpus/` + UTI, with the solver of 2026-09-30
(`cb_test/eval-all-*.out`, models `learn-all-log`, `learn-all-newfeats` =
squared error, same frames):

| models | in-sample, `--predcands 0` | in-sample, `--predcands 1` | UTI held out, 0 | UTI held out, 1 |
|---|---|---|---|---|
| squared error | 87% / 93% | 95% / 104% | | |
| log | 91% / 113% | 88% / 103% | 98% / 107% | 160% / 154% |

So the objective and the candidate set are family-dependent: the
defaults stay squared error and `--predcands 0`, and `XGB_OBJ=log` with
`--predcands 1` is what to use for a bivium-like family. Features picked
on bivium rank better offline but do not help in the solver, so
`best_features.txt` stays the shared list (`best_features-bivium.txt` is
kept for reference).

Prediction cost: the predictor build predicts only for the clauses
reduce ranks (a third of the DB with `--predcands 0`; `--predkeep 1`
needs all of them), with one model instead of three since `disc`, and
sorts the learnt DB at reduce only for the ranking features the list
uses (the generated header says which; two of five for the general
list). On php10 the overhead went from 3 s of 11 to about 1.4 s of 9.8.

## A general model from satcomp2020 (2026-09-30)

`cb_test/general/`: `survey.txt` is the normal build on
`~/media/satcomp2020` with `--xor 0` (`survey.sh`: 30 s on 100 files
found 7 solvable; `survey2.sh`: the smallest file of each of 91
families at 300 s found 30 solvable with 30k-2.5M conflicts, half of
them SAT). `gather_all.sh` gathered them into `cnf/<file>-dir`
(`FIXED=6000`, `DUMPRATIO` 0.1/0.05/0.03 by conflicts), 26 usable: 4
have no rows in any horizon or no used clause at all, 1 filled the disk.
12 of the 26 were SAT, gathered by a since-removed switch that counted
every derivation as a use; they are parked in `cnf-sat/` and are not to
be learnt from. `families.py` names the family, `run_general.sh <name>`
learns on all families but `HOLDOUT` and evaluates on those (the normal
build's runs come from the survey via `normal-cache/`).

Offline (`holdout_eval.py`, 14 training and 6 held-out families, short
horizon, share of the future use kept when keeping 25% of the reduce
candidates): glue/size 81.3%; count target, shared list: 79.5%; rank
target (`TARGET=rel`), shared list plus glue/used: 88.3%; rank target,
`best_features-general.txt`: 90.1%. The count target is below glue/size
across families, the rank target above it on 5 of the 6, sometimes by a
lot (course: 92% vs 64%, post-cbmc-aes: 76% vs 67%).

In the solver, on the same held-out families (`results-rel-*.out`): see
the table below. SAT instances swing wildly either way (sgp: 20x more
conflicts with the model, course: 60% of the time), so only the UNSAT
ones say something.

Models trained on 17 families (`models/rel-gen`), rank target,
`best_features-general.txt`, `--xor 0`, conflicts / time of the predictor
build relative to the normal build:

| held-out instance | `--predcands 0` | `--predcands 1` |
|---|---|---|
| hid-uns-enc (UNSAT) | 97% / 82% | 100% / 91% |
| jkkk-one-one (UNSAT) | 99% / 80% | 117% / 97% |
| post-cbmc-aes (UNSAT) | 98% / 106% | 101% / 111% |
| schup-l2s (UNSAT) | 131% / 95% | 97% / 76% |
| Steiner-45 (UNSAT) | 101% / 121% | 120% / 178% |
| sv-comp19 (UNSAT) | 109% / 91% | 125% / 97% |
| the six UNSAT together | 102% / 90% | 105% / 90% |
| combined-crypto (SAT) | 403% / 466% | 143% / 115% |
| course0.2 (SAT) | 55% / 42% | 15% / 19% |
| sgp_5-6-8 (SAT) | 310% / 210% | 2340% / 1650% |

That model was trained with the 12 SAT instances in, which is now
forbidden. Trained on the 8 UNSAT training instances only
(`models/rel-unsat`), same held-out UNSAT instances, `--predcands 0`:

| held-out instance | UNSAT-only model | (mixed model) |
|---|---|---|
| hid-uns-enc | 98% / 84% | 97% / 82% |
| jkkk-one-one | 94% / 76% | 99% / 80% |
| post-cbmc-aes | 104% / 111% | 98% / 106% |
| schup-l2s | 85% / 72% | 131% / 95% |
| Steiner-45 | 108% / 135% | 101% / 121% |
| sv-comp19 | 104% / 85% | 109% / 91% |
| the six together | 97% / 81% | 102% / 90% |

Offline the UNSAT-only model looks weaker (73.6% vs 85.2% of the future
use kept at 25%, short horizon: 8 training instances instead of 20), in
the solver it is better. `--predcands 0` stays the default.

**One discounted model, score-driven tiers (2026-10-01).** The 14 UNSAT
instances regathered with all four tiers (`TIERS="disc short long
forever"`), models trained on the 8 training families, same 6 held-out
UNSAT families, `--xor 0`, conflicts / time relative to the normal build:

| models | `--predkeep 0` | `--predkeep 1` (25% / 47%) | `--predkeep 1` (15% / 40%) |
|---|---|---|---|
| one `disc` model | 100% / 82% | 98% / 73% | 129% / 102% |
| three horizons, same data | 108% / 81% | | |

Per instance with `--predkeep 1`: time 68-75% on hid, jkkk, schup and
sv, 106% on post-cbmc-aes, 130% on Steiner (6 s runs). Offline the disc
label does not look better than the short one (the offline metric keeps
disagreeing with the solver on anything but the target scale), in the
solver one model does what three did at a third of the prediction cost.
So the defaults are: `TIERS=disc`, `TARGET=rel`, `--predtiers disc`,
`--predkeep 1`, `best_features.txt` = the general list (the old
corpus-picked one is `best_features-mixed.txt`), `src/predict/predictor_disc.json`
= `models/disc-all/` (all 14 UNSAT instances). The bivium and
mixed-corpus tables above were made with the old list, the count target
and three models.


## Open

- The general model's offline gain (90% vs 81% of the future use kept)
  does not show up as fewer conflicts in the solver. Which candidates
  reduce drops seems to matter less than what the deletions do to the
  later search; a second gathering round under the learnt policy
  (`build_stats_pred/`, `STATS_OPTS`) is set up but not run.
- SAT instances cannot be A/B tested one run at a time: the path to a
  model changes with every clause kept. Several seeds, or UNSAT only.
- 14 UNSAT instances from 14 families is thin; the survey found 30
  solvable in 300 s on this box out of 370, and only UNSAT ones count. More families need more time or a
  bigger machine (the proof is ~2 GB per million conflicts).
- The ancestor-weighted labels generalise worse than the plain ones;
  `--predtables 000` is the default for a reason.
- `--predsortby` 0/1/2 vs 3 and tree count/depth were not tuned.
