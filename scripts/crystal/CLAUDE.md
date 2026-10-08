# CrystalBall: how to run it

`README.md` says what it is, `TRAIN_AND_EVALUATE.md` is the plan for the
cluster run. This says how to build, run and evaluate it, and, at the
end, what is known.

## Builds

| dir | cmake | what it is |
|---|---|---|
| `build/` | default | normal solver, the baseline |
| `build_stats/` | `-DSTATS=ON` | dumps clause data to SQLite, writes the XLRUP proof |
| `build_pred/` | `-DFINAL_PREDICTOR=ON` | ranks reduce candidates by the xgboost prediction |
| `build_stats_pred/` | both | the stats build with the model driving the reduce: the second gathering round |

Build with `-j4`. From an empty dir:

```
cd build_stats && ../scripts/build_scripts/build_stats.sh
cd build_pred  && ../scripts/build_scripts/build_final_predictor.sh
cd build_stats_pred && ../scripts/build_scripts/build_stats_predictor.sh
```

Both need `../cadical/build` and `../cadiback/build` next to the repo
(cadiback at 0d92b68 or later). Dependencies on this box are under
`../deps` (no sudo); export what a build needs before its script:

```
SQLITE3_INCLUDE_DIR=$HOME/development/sat_solvers/deps/sqlite-amalgamation-3450100
SQLITE3_LIBRARY=$HOME/development/sat_solvers/deps/sqlite-amalgamation-3450100/libsqlite3.a
XGBOOST_INCLUDE_DIR=$HOME/development/sat_solvers/deps/xgboost-headers
XGBOOST_LIBRARY=$HOME/development/sat_solvers/deps/xgboost-pkg/xgboost/lib/libxgboost.so.3
```

The xgboost library is the pip package's `.so` copied out (SONAME
`.so.3`, RPATH `$ORIGIN/../../xgboost.libs` for its libgomp: keep that
layout). Python: xgboost 3.4, pandas 3, scikit-learn, numpy 2, `cnfgen`
(`pip3 install --user --break-system-packages`).

The predictor build generates at build time:

- `predict_features_gen.h` from `best_features.txt`
  (`gen_pred_features.py`): the C++ of every feature. The feature file
  is the single source of truth for training and solving
  (`-DPRED_FEATURES_FILE=` for another list).
- the embedded model from `src/predict/predictor_disc.json`.
  `src/predict/` is its own git repo
  (github.com/msoos/cryptominisat-predictors), ignored by the solver's:
  clone it there before building; after retraining copy the model in,
  rebuild, commit there (never push, the owner pushes). Model and binary
  must come from the same feature list.

Solver options (all BEFORE the CNF: anything after it is the proof file):

- predictor build: `--predloc DIR` (the model
  `DIR/predictor-<table>-disc-xgb.json`; empty = the embedded one),
  `--predanc 0|1` (with `--predloc`: 0 = the `used_later` model, 1 =
  `used_later_anc`),
  `--dumppreddistrib 1`, `--preddump FILE`, `--predmimic 1`
  (self-check: the score is the glue/size order, so the run must equal
  the normal build's; `test_small.sh` checks it).
- stats build: `--sql 2 --sqlitedb F --sqlitedboverwrite 1 --clid
  --cldatadumpratio R --cllockdatagen R --everypred N`, all set by
  `ballofcrystal.sh`.

- both: `--reducerounds N` (a clause is kept for N reduces after it
  was learnt or last used; default 2, predictor builds 1), `--reducetarget P` (default 75:
  the percent of the candidates removed).

Only WHICH candidates a reduce removes differs between the builds, so a
conflict-count A/B is clean.

## The pipeline

`ballofcrystal.sh file.cnf` does all of it for one UNSAT instance, in
`<file.cnf>-dir/`:

1. **gather**: stats build with `--xor 0`, writes `data.db-raw` and
   `data.xlrup` (binary, `--xlrup 2`).
2. **label** (`fix_up_xlrup.py`): trims the proof to the steps reachable
   from the empty clause and fills `used_clauses` (tracked clause X was
   in the hint chain of a kept step at conflict C) and
   `used_clauses_anc` (also credits ancestors, 0.5 per generation, down
   to 0.05). The pass over the proof is `xlrup_uses.cpp` (built on first
   use); `--python` is the same pass in Python, the reference of
   `test_xlrup_uses.py`. The proof is deleted afterwards (`KEEP_PROOF=1`
   keeps it and `data.db`).
3. **clean/check/sample** (`clean_update_data.py`, `check_data_quality.py
   --slow`, `sample_data.py`): the labels and their rank within the
   reduce over EVERY row of the full DB, then two samples, kept in
   `data-min.db`:
   - training: cells of label stratum (top `cut1` % of the used rows, up
     to `cut2` %, the rest with the never-used) x age (`dump_no` 0, 1,
     2-5, more), at most `FIXED`/4 rows of each. `x.weight` = rows of
     the cell / rows picked.
   - fair: `EVAL_REDUCES` (10) evenly spaced reduces, up to
     `EVAL_PER_REDUCE` (3000) clauses of each that are not gone (see
     Tracking), no strata. For measuring.
4. **frames** (`cldata_gen_pandas.py`): per table in {used_later,
   used_later_anc}: `data-min.db-cldata-<table>-disc-cut1-..-limit-N.dat`
   (training) and `data-min.db-evaldata-<table>-disc.dat` (fair), then
   `check_frames.py`.
5. **learn** (`cldata_predict.py`): `predictor-<table>-disc-xgb.json`.
6. **evaluate**: predictor build (`--predanc 0` and `1`) and the
   normal build, side by side.

Flags: `--gather-only` stops after 4, `--skip-solve` redoes 2-6 (needs
the proof), `--skip-learn` only reruns 6.

**Only UNSAT instances, ever.** A label is "took part in the trimmed
UNSAT proof"; a SAT run has no proof. The pipeline refuses SAT runs.

**The label** (`disc`): the future proof uses of the clause, each
discounted by its distance, halving every `HALFLIFE` (4) reduces,
counted for two half-lives. Distance is in reduce time (the k-th reduce
is at k, linear in the conflicts in between), because the reduce
interval grows with the run. A row counts only if the clause stayed in
the solver for the two half-lives, so the last 8 reduces give no rows.
`x.<table>_disc` is the label. The ranker learns the order within a
reduce from whether it is above 0. For the regressions there is
`x.<table>_disc_rel` (`TARGET=rel`): the share of the tracked clauses of
the same reduce that score less (0..1, never-used = 0), computed on the
full data; use counts differ 100-fold between families.

**Tracking.** Every tracked clause is locked (`CLLOCK=1`): never deleted
by reduce, so its rows say what it does if kept. The reduce still
decides about it as about any other clause and remembers the verdict:
`rdb0.gone` = 1 from the first reduce that would have removed it (in a
`build_stats_pred` run: that the model would have). Most rows are of
gone clauses (Steiner: 80%); the fair frame has none, it is what the
solver holds at a reduce. `DUMPRATIO=auto` tracks
`TRACKED` (10k) clauses over the run, from the normal build's conflict
count (run first, or give `CONFL`), at most `MAXDUMPRATIO` (0.03) of
the learnt clauses. An eagerly subsumed clause (glue NULL) lives on only
because of the lock: its rows after the first such reduce are dropped
(`fill_used_later_X`), 14-32% of the rows otherwise.

Knobs (`setparams_ballofcrystal.sh`, all from the environment):
`STATS_BIN PRED_BIN NORMAL_BIN`, `DUMPRATIO TRACKED MAXDUMPRATIO CONFL`,
`CLLOCK`, `EVERYPRED`, `HALFLIFE`, `FIXED` (rows per stratum, 3000),
`cut1 cut2`, `EVAL_REDUCES EVAL_PER_REDUCE`, `bestf` (feature file),
`XGB_EST XGB_DEPTH XGB_MINCHILD` (40, 5, 10), `XGB_OBJ` (`rank`, the
default: the order within a reduce, learnt from used or not; or a
regression, `squarederror`, `log`, `poisson`, which takes `TARGET`
(`rel`, `count`) and `XGB_WEIGHTS` (`none`; `strata` = `x.weight`;
`instance` = also every instance counts the same; `family` = also every
family: the leading letters of the file name, or the `FAMILIES` file of
`family filename` lines)),
`CAKE_XLRUP=""` (skip the optional proof check).

**Use kept at reduce**, in the learn output and `holdout_eval.py`: the
solver's reduce replayed on the fair frame (`helper.policy_per_reduce`).
Per reduce, the share of the future use held by the clauses that are
kept by `normal` (the normal build: by rule the tier1 clauses with
`used` life left and the ones used since the reduce before, of the rest
the best quarter by glue, size), by `order` (the predictor build: the
same rule, the rest by the model) and by `oracle` (the same rule, the
rest by their future use: the ceiling). Only the order of the
candidates is replayed: a policy that changes the rule cannot be judged
offline (see What is known). Mean over
the reduces of an instance, then over the instances. `rule cls` / `rule
use`: the share of the clauses the rule keeps and of the use they hold,
`cls`: the share of the clauses kept. `holdout_eval.py --rounds N
--remove P --tier1 G` replays the rule with other knobs
(`--reducerounds`, `--reducetarget`, `--reducetier1glue`; the defaults
are the predictor build's).
With three or more instances the train/test split is by instance; the
saved model is refitted on all rows.

### The gates

- `check_rawdb.py` on the stats DB: tables, one row per (clause,
  reduce), enough reduces, tracked share vs dump ratio, proof present,
  no clause gone before its first reduce or back after.
- `check_data_quality.py --slow` after the labels.
- `check_frames.py` on the frames, before any learning: columns, inf,
  bad labels, `rel` in 0..1, weights >= 1, sizes, ages, per-reduce
  values constant within a reduce, features that look into the future,
  duplicate rows, labels lined up with the `used` counter (catches
  off-by-one-reduce bugs).
- `concat_pandas.py` refuses frames with different columns; `learn.sh`
  warns when the dirs were gathered by different solvers.
- The solver refuses a model whose features do not match its list.
- `gen_pred_features.py` refuses a feature that scales with the run
  length or the instance size (`SCALE` in it): such a column may only
  appear divided by one of the same class. A long run would otherwise
  feed the trees values they never saw.
- The model carries the 1st..99th percentile of every feature of its
  training data and its provenance (xgboost attributes). The solver
  prints `[pred] model trained on ...` at load and `pred feats outside
  training range` at the end; much more than 1-2% means extrapolation.
- `test_small.sh [seed]` (~3 min): the whole pipeline on a random UNSAT
  instance, models reproducible bit for bit, `test_xlrup_uses.py`, and
  the C++ features against pandas (`--preddump` +
  `check_pred_features.py`). Run it after touching any script, the
  stats/predictor code or the schema.

### Many instances

```
./learn.sh <outdir> a.cnf-dir b.cnf-dir ...  # concat the frames, train (same FIXED as the gather)
./eval_corpus.sh <preddir> a.cnf b.cnf ...   # normal vs predictor on each, and the totals
./holdout_eval.py --train a-dir .. --test c-dir ..   # offline, on the fair frames (or --model m.json)
./run_corpus.sh <outdir> a.cnf b.cnf ...     # gather + learn + eval
```

`eval_corpus.sh` writes to `<preddir>/eval/<cnf>.<normal|pred0|pred1>.s<seed>`
and takes `PRED_OPTS`, `EVAL_OPTS` (e.g. `--xor 0`), `EVAL_ANC` (which
`--predanc`, `0 1`), `EVAL_SEEDS`, `EVAL_TIMEOUT`, `EVAL_NORMAL_CACHE`.
`eval_summary.py <dir>` makes the table from such files: per instance,
then per configuration against normal the geometric mean of the
per-instance ratios with a 95% bootstrap interval, solved, PAR2, and
the noise (each seed against the first).

### Features

`best_features.txt`: 24 scale-free features. `gen_best_feats.sh` +
`pick_features.py` make importance rankings and a list from them
(memory: features x tree nodes, ~5 GB at 30% of the rows). After
changing the list: rebuild `build_pred`, retrain, copy the model to
`src/predict/`, rebuild.

`model_report.py <learn dir>/predictor-used_later-disc-xgb.json -o
report.html`: one HTML page on a model (attributes, importances, SHAP,
trees). `<learn dir>` holds the model next to its training frame.

## The scripts

Pipeline: `ballofcrystal.sh`, `setparams_ballofcrystal.sh`,
`check_rawdb.py`, `fix_up_xlrup.py` + `xlrup_uses.cpp`,
`clean_update_data.py`, `check_data_quality.py`, `sample_data.py`,
`cldata_gen_pandas.py`, `check_frames.py`, `cldata_predict.py`,
`concat_pandas.py`, `learn.sh`, `eval_corpus.sh`, `eval_summary.py`,
`run_corpus.sh`, `helper.py` (shared SQL, features, weights, ranking).
Features: `gen_pred_features.py`, `gen_best_feats.sh`,
`pick_features.py`, `feature_groups.py` + `ablate_groups.sh` (group
ablation in the solver), `ccg.py`. Models: `model_report.py`, `model_variance.sh` (tree-seed spread of an A/B),
`holdout_eval.py`. Tests: `test_small.sh`, `test_xlrup_uses.py`,
`check_pred_features.py`. Instances: `bivium_variants.py`.

## Practicalities

- Give `ballofcrystal.sh` a COPY of the CNF: it works in `<real path of
  the CNF>-dir`, and the proof (binary XLRUP, 42% of the text FRAT it
  replaced) is about 1 GB per million conflicts, 5 GB on sv-comp
  (`fix_up_xlrup.py` prints the bytes per conflict). With
  the disk full the stats run dies with an SQLite "SQL logic error".
- Never edit a script while a run uses it: write a temp file and rename.
- This box: 1 physical core, 7 GB. Time runs one at a time, and a cached
  normal run (`EVAL_NORMAL_CACHE`) must have been made under the same
  load as what it is compared to.
- A further round: `STATS_BIN=../../build_stats_pred/cryptominisat5
  STATS_OPTS="--predloc DIR --predanc 0" ballofcrystal.sh --gather-only`
  on another copy of the CNFs, then `learn.sh` on the dirs of all the
  rounds (it says what drove each). The build keeps a clause for one
  round, as the predictor build does.
- Features are float32 on both sides; ratios beyond it are "missing".
- Training is deterministic: the same frames give the same model.
- Fuzz the normal build after touching the solver:
  `cd scripts/fuzz && ./fuzz.py --fuzzlim 30`.
- `cb_test/` (not in git): `general/cnf/` and `sr19/cnf/` (the gathered
  round-1 dirs), `round2/cnf/` (round 2, `round2/gather.sh`),
  `general/models/n21r2/` (the embedded model's learn dir,
  `round2/train-both.txt` its inputs, `round2/learn-both.sh`),
  `round3/` (round 3, `learn3.sh`, its model `general/models/n21r3/`),
  `general/ladder.sh` + `lsum.py` (the hold-out A/B, 9 x 3, two at a
  time: conflicts only), `general/timed.sh` (one at a time, for times),
  `general/holdout9.txt`, `sr19/survey*.sh` and `gather.sh`.

## What is known (2026-10-07)

Data: 30 UNSAT instances, 25 families (14 of satcomp2020, 16 of
satrace19: what the normal build solves with `--xor 0` in 100-300 s on
this box, 18 of 400), `FIXED=20000`. 21 train, 9 hold out (the
families hid, jkkk, post-cbmc, schup, Steiner, sv-comp). All 30 were
gathered with glue driving the reduce (round 1), and 28 again with the
round-1 ranking model driving it (round 2; UTI ran out of disk,
ps_200_301_70 fails a frame check), and 27 with the embedded model
driving it at one protected round (round 3; SGI is 95% `gone` rows,
f6bidw and ps_200_301_70 fail a frame check).

The embedded model: a ranker on both rounds of the 21, 40 trees of
depth 5. On the 9 hold-outs, 3 seeds, `--xor 0`, against the normal
build: **conflicts 96.7% [87.0, 105.0]**, all solved. Run one at a
time, 2 seeds: conflicts 99.8% [94, 106], **time 100.9% [90, 113]**,
time per conflict 101% [94, 109]. Seed noise of the normal build:
3-9%. So at two protected rounds the model is where glue is, in
conflicts and in time; at one round, its default now, the time is 96.2%
[87, 107] (below). (Before the BVE tie-break below the same model measured
101.6% [93, 111] in conflicts and 112.5% [101, 128] in time: the
tie-break moved the normal build by as much as the model does, 106.1%
[95, 121] new vs old.)

How it got there, same 9 x 3, conflicts vs the normal build:

| | |
|---|---|
| squared-error model, the score also picking the tiers | 110.1% [106, 115] |
| + two bugs fixed (below) | 105.4% [100, 111] |
| ranking model, glue tiers | 103.5% [97, 110] |
| + trained on both rounds | 101.6% [93, 111] |

(All four against the normal build of before the tie-break.)

- **Two bugs made the predictor build lose.** An eagerly subsumed
  clause (glue = max) was ranked by the model instead of going first,
  and `lim_keptglue/lim_keptsize` (what vivification takes for likely
  kept) came from what the model kept, so every learnt clause counted.
  `--predmimic 1` now proves the plumbing: with the glue order as the
  score the predictor build is the normal build, conflict for conflict,
  on all 9 hold-outs.
- **The builds were not the same solver.** BVE sorted clauses of equal
  size by memory offset, and where a clause lies depends on the size of
  its header, so on the build: on post-cbmc and jkkk the predictor
  build made its resolvents in another order and ran 11% and 44% more
  conflicts with the very same reduce. Ties go by clause ID now. If
  `--predmimic 1` differs from the normal build, have both write a
  proof (`--xlrup 1 <cnf> <proof>`) and `cmp` them: the first differing
  line names the place.
- **Proof use cannot judge the rule.** Offline, keeping as many clauses
  as the normal reduce but all picked by glue, size (no "used since the
  last reduce" rule) holds 99.5% of the future proof use against 96.8%,
  the model the same. In the solver exactly that is 118% [106, 130] of
  the normal build's conflicts, with the model 116%. A recently used
  clause that is not in the final proof still keeps the search out of
  where it has been. So the model orders the candidates and nothing
  else, and only that is replayed. The options that let the score do
  more are gone: ranking the rule-kept clauses too (116%), picking
  which clauses are tier1/tier2 (as many as glue makes: 108% vs 104%
  with the ranking model, 105% vs 111% with the squared-error one, i.e.
  nothing, at 3.5 times the predictions), replacing glue in
  `likely_to_be_kept()` (vivification, BVE: 10 points lost).
- **What the model costs in time.** The reduce is 0.5-6.5% of the run
  (normal: 0.1-0.8%), and a conflict costs 4-17% more on Steiner, hid,
  jkkk, post-cbmc and schup. On the four sv-comp ones the reduce is
  0.5% and the time per conflict goes from 85% to 114%: there it is
  what the kept clauses cost to propagate, either way.
- **How many to keep: one round less, with the model.** At a reduce
  the clauses learnt or used since the last one are 36% of the clauses
  and 63% of the future proof use, those in their second round 24% and
  16%, tier1 19% and 17%, the candidates 21% and 5%. So the count is in
  `--reducerounds`, not in the candidates. Replayed on the hold-outs
  (clauses kept, use kept by glue, by the model):

  | rounds, removed | clauses | glue | model |
  |---|---|---|---|
  | 2, 75% (default) | 83.8% | 97.2% | 97.7% |
  | 1, 75% | 69.3% | 96.7% | 97.0% |
  | 1, 50% | 79.6% | 99.1% | 98.9% |
  | 0, 75% | 51.7% | 92.8% | 94.7% |

  In the solver, 9 x 3, against the normal build at its defaults
  (conflicts, bogoprops):

  | | conflicts | bogoprops |
  |---|---|---|
  | normal, 1 round | 107.7% [99, 116] | 100.8% [87, 116] |
  | model, 2 rounds | 96.7% [86, 105] | 89.0% [71, 105] |
  | model, 1 round | 96.4% [85, 108] | 82.4% [69, 98] |
  | model, 1 round, 50% removed | 96.1% [87, 105] | 90.8% [76, 105] |
  | model, 1 round, 90% removed | 115.6% [105, 129] | 100.8% [85, 122] |
  | model, 0 rounds | 121.4% [101, 142] | 95.9% [71, 125] |
  | model, 1 round, tier1 glue 3 | 98.0% [89, 108] | 93.0% [80, 107] |
  | model, 1 round, tier1 glue 1 | 122.6% [106, 145] | 104.3% [81, 136] |
  | normal, 1 round, 50% removed | 102.7% [91, 118] | 91.1% [73, 110] |
  | normal, 0 rounds | 130.4% [112, 155] | 99.2% [81, 121] |

  Glue cannot drop the second round, the model can: model against
  normal, both at 1 round, is 89.5% [81, 97] in conflicts and 81.7%
  [71, 94] in bogoprops. Run alone, 4 seeds, the model at 1 round
  against the normal build: conflicts 100.1% [92, 109], **time 96.2%
  [87, 107]**, time per conflict 96.2% [93, 99] (the first 2 seeds
  alone said 92.6% [83, 103]: two seeds are not enough). No protected round at
  all loses. Removing less at 1 round buys nothing (the same
  conflicts for more propagation), removing more loses on both. The
  tier1 glue is the same: 3 keeps 5 points more clauses for 113%
  [102, 124] of the bogoprops of 2, and 1 is 127% [112, 146] of its
  conflicts. One round, glue 2, 75% is a local best in all three. So the predictor builds default to
  `--reducerounds 1`; `--predmimic 1` needs `--reducerounds 2` to equal
  the normal build.
- **A count from the score lost.** Removing the candidates scored
  below a threshold (set so that the median training reduce removes
  75%, clamped to half and twice that) kept 98.5% of the use against
  96.2% offline, for 3.6 points more clauses. In the solver: 104.5%
  [98, 111] of the conflicts and 115.8% [104, 128] of the bogoprops of
  the fixed 75%. `--predthresh` is gone.
- **Bogoprops are the cost when the box is shared.** They do not move
  with the load, and correlate 0.93 with the time of a run made alone;
  the predictor build pays about 8% more seconds per bogoprop.
- **There is little to win in the order of the candidates.** The rule
  keeps 78% of the clauses with 93% of the future proof use; the normal
  reduce ends up with 97-98%, the oracle order of the candidates with
  99.9%. The whole prize is 2 points of use.
- **The model's edge is on glue's data.** Use kept, model minus normal,
  on the hold-outs: +0.5 (better on 8 of 9) on round 1, 0.0 (6 of 9) on
  round 2, where the model itself chose what stays. Training on both
  rounds does not change either number, the solver moved from 103.5%
  to 101.6% (inside the noise).
- **A third round, at one protected round, did not move the solver.**
  There the rule keeps 70% of the clauses with 90.5% of the use, the
  embedded model is at glue (-0.10, better on 6 of 9), and the model
  trained on all three rounds is ahead on every hold-out (+0.56, 9 of
  9; unchanged on rounds 1 and 2). In the solver it is 106.5% [99,
  117] of the embedded model's conflicts and 106.1% [94, 122] of its
  bogoprops. Not embedded. Half a point of use offline is below what a
  9 x 3 ladder can see, in either direction.
- Most rows are of clauses the solver would not have: 64-93% of the
  rows of an instance are `gone`. The numbers measured before `gone`
  (model +2.5 points over a glue sort) were about those.
- Ranking beats regression: offline +0.47 against +0.10..+0.36 (the
  regression moves with the row order of the frame, the ranker does
  not), in the solver 103.5% against 110.9% [101, 124].
- Training only on the candidates' rows, only on the rows not `gone`,
  or both: no better than all rows.
- **Glue is hard to beat.** `rdb0.glue` and `rdb0.size` are features,
  yet a model of only those two is 2-3 points below the glue sort on
  new families: raw glue means something else on every instance, the
  sort only compares within a reduce. A within-reduce glue/size rank as
  a feature reproduces the sort and adds nothing to the 24.
- For the regressions: `TARGET=rel` beats `count`, row weights do not
  help, squared error vs log makes no difference.
- What carries the model (group ablation in the solver): the recency
  counters, the learning-time snapshot, size/glue. Rankings, context
  ratios, age and the propagation-cost features are inside the noise.
  Importance-picked feature lists lost to the hand-kept one three times.
- The label is a use in the trimmed UNSAT proof. The solver's own
  conflict-analysis uses as a label, alone or mixed in, measured the
  same. In some runs nearly all proof uses fall in the last 10% of the
  run (ps_200_301_70: 99%), so early reduces say little there.
- Plain vs ancestor label (`--predanc 0` vs `1`): within the
  seed noise of each other. Unsettled; deleting one would simplify
  everything.
- SAT instances cannot be A/B tested one run at a time: the path to a
  model changes with every clause kept.
- Differences under ~10% between single runs are not results: the
  interval of a 9 x 3 comparison is +-7 points.
- Features that grow with the run or the instance (clause age, raw
  counters) measured the same as the 24 without them, and drift on long
  runs. They are refused by the generator.
- Three models of use counts over three horizons, summed, did what the
  one discounted model does at three times the prediction cost.
- Not tuned: trees, depth, `HALFLIFE`.
