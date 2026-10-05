# CrystalBall: how to run it

`README.md` says what it is, `TRAIN_AND_EVALUATE.md` is the plan for the
cluster run. This says how to build, run and evaluate it, and, at the
end, what is known.

## Builds

| dir | cmake | what it is |
|---|---|---|
| `build/` | default | normal solver, the baseline |
| `build_stats/` | `-DSTATS=ON` | dumps clause data to SQLite, writes the FRAT proof |
| `build_pred/` | `-DFINAL_PREDICTOR=ON` | ranks reduce candidates by the xgboost prediction |

Build with `-j4`. From an empty dir:

```
cd build_stats && ../scripts/build_scripts/build_stats.sh
cd build_pred  && ../scripts/build_scripts/build_final_predictor.sh
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
  `--predkeep 2` (default: the score decides which learnt clauses are
  tier1/tier2, as many of each as glue would make; 1 = fixed
  `--predkeept1`/`--predkeept2` %; 0 = glue), `--predcands 0|1|2` (what
  is ranked: 0 = the normal build's candidates, 1 = also the clauses
  kept for being used, 2 = also tier1-keep), `--predthresh T`,
  `--dumppreddistrib 1`, `--preddump FILE`.
- stats build: `--sql 2 --sqlitedb F --sqlitedboverwrite 1 --clid
  --cldatadumpratio R --cllockdatagen R --everypred N`, all set by
  `ballofcrystal.sh`.

With `--predcands 0` only WHICH candidates a reduce removes differs
between the builds, so a conflict-count A/B is clean. With 1 the DB also
shrinks: compare against the normal build with `--reducekeepused 0` too.

## The pipeline

`ballofcrystal.sh file.cnf` does all of it for one UNSAT instance, in
`<file.cnf>-dir/`:

1. **gather**: stats build with `--xor 0`, writes `data.db-raw` and
   `data.frat`.
2. **label** (`fix_up_frat.py`): trims the proof to the steps reachable
   from the empty clause and fills `used_clauses` (tracked clause X was
   in the hint chain of a kept step at conflict C) and
   `used_clauses_anc` (also credits ancestors, 0.5 per generation, down
   to 0.05). The pass over the proof is `frat_uses.cpp` (built on first
   use); `--python` is the same pass in Python, the reference of
   `test_frat_uses.py`. The proof is deleted afterwards (`KEEP_FRAT=1`
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
     `EVAL_PER_REDUCE` (3000) clauses of each, no strata. For measuring.
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
`x.<table>_disc` is the label, `x.<table>_disc_rel` (`TARGET=rel`, the
default) the share of the tracked clauses of the same reduce that score
less (0..1, never-used = 0), computed on the full data. `rel` is what
makes instances comparable: use counts differ 100-fold between families.

**Tracking.** Every tracked clause is locked (`CLLOCK=1`): never deleted
by reduce, so its rows say what it does if kept. `DUMPRATIO=auto` tracks
`TRACKED` (10k) clauses over the run, from the normal build's conflict
count (run first, or give `CONFL`), at most `MAXDUMPRATIO` (0.03) of
the learnt clauses. An eagerly subsumed clause (glue NULL) lives on only
because of the lock: its rows after the first such reduce are dropped
(`fill_used_later_X`), 14-32% of the rows otherwise.

Knobs (`setparams_ballofcrystal.sh`, all from the environment):
`STATS_BIN PRED_BIN NORMAL_BIN`, `DUMPRATIO TRACKED MAXDUMPRATIO CONFL`,
`CLLOCK`, `EVERYPRED`, `HALFLIFE`, `FIXED` (rows per stratum, 3000),
`cut1 cut2`, `EVAL_REDUCES EVAL_PER_REDUCE`, `bestf` (feature file),
`XGB_EST XGB_DEPTH XGB_MINCHILD` (40, 5, 10), `XGB_OBJ` (`squarederror`,
`log`, `poisson`), `TARGET` (`rel`, `count`), `XGB_WEIGHTS` (`none`;
`strata` = `x.weight`; `instance` = also every instance counts the same;
`family`, the default = also every family: the leading letters of the
file name, or the `FAMILIES` file of `family filename` lines),
`CAKE_XLRUP=""` (skip the optional proof check).

**Ranking quality**, in the learn output and `holdout_eval.py`: on the
fair frame, per reduce, the share of the future use kept when keeping
the best 25% / 50% of the clauses by the model, by glue then size, and
by the truth; mean over the reduces of an instance, then over the
instances (`helper.ranking_per_reduce`). "cands" = only clauses not used
since the reduce before. With three or more instances the train/test
split is by instance; the saved model is refitted on all rows.

### The gates

- `check_rawdb.py` on the stats DB: tables, one row per (clause,
  reduce), enough reduces, tracked share vs dump ratio, proof present.
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
  instance, models reproducible bit for bit, `test_frat_uses.py`, and
  the C++ features against pandas (`--preddump` +
  `check_pred_features.py`). Run it after touching any script, the
  stats/predictor code or the schema.

### Many instances

```
./learn.sh <outdir> a.cnf-dir b.cnf-dir ...  # concat the frames, train (same FIXED as the gather)
./eval_corpus.sh <preddir> a.cnf b.cnf ...   # normal vs predictor on each, and the totals
./holdout_eval.py --train a-dir .. --test c-dir ..   # offline, on the fair frames
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
`check_rawdb.py`, `fix_up_frat.py` + `frat_uses.cpp`,
`clean_update_data.py`, `check_data_quality.py`, `sample_data.py`,
`cldata_gen_pandas.py`, `check_frames.py`, `cldata_predict.py`,
`concat_pandas.py`, `learn.sh`, `eval_corpus.sh`, `eval_summary.py`,
`run_corpus.sh`, `helper.py` (shared SQL, features, weights, ranking).
Features: `gen_pred_features.py`, `gen_best_feats.sh`,
`pick_features.py`, `feature_groups.py` + `ablate_groups.sh` (group
ablation in the solver), `ccg.py`. Models: `model_report.py`, `model_variance.sh` (tree-seed spread of an A/B),
`holdout_eval.py`. Tests: `test_small.sh`, `test_frat_uses.py`,
`check_pred_features.py`. Instances: `bivium_variants.py`.

## Practicalities

- Give `ballofcrystal.sh` a COPY of the CNF: it works in `<real path of
  the CNF>-dir`, and the proof is about 2 GB per million conflicts. With
  the disk full the stats run dies with an SQLite "SQL logic error".
- Never edit a script while a run uses it: write a temp file and rename.
- This box: 1 physical core, 7 GB. Time runs one at a time, and a cached
  normal run (`EVAL_NORMAL_CACHE`) must have been made under the same
  load as what it is compared to.
- `build_stats_pred/` (STATS=ON and FINAL_PREDICTOR=ON) with
  `STATS_OPTS="--predloc DIR"` gathers under the learnt
  policy.
- Features are float32 on both sides; ratios beyond it are "missing".
- Training is deterministic: the same frames give the same model.
- Fuzz the normal build after touching the solver:
  `cd scripts/fuzz && ./fuzz.py --fuzzlim 30`.
- `cb_test/` (not in git): `general/cnf/` and `sr19/cnf/` (the gathered
  dirs), `general/models/n21/` (the embedded model's learn dir,
  `train21.txt` its inputs), `general/ab.sh` (the hold-out A/B),
  `general/lofo.py` (leave-one-family-out, offline), `sr19/survey*.sh`
  and `gather.sh`.

## What is known (2026-10-05)

Data: 30 UNSAT instances, 25 families (14 of satcomp2020, 16 of
satrace19: what the normal build solves with `--xor 0` in 100-300 s on
this box, 18 of 400), `FIXED=20000`.

The embedded model: 21 of them (all but the hold-out families: hid,
jkkk, post-cbmc, schup, Steiner, sv-comp), `rel`, squared error, no
weights, 40 trees of depth 5. On the 9 hold-out instances, 3 seeds,
`--xor 0`, against the normal build: **conflicts 110% [105, 115], time
118% [110, 125]**, all solved. Seed noise of the normal build: 3-9%.
So the model loses to glue in the solver.

Offline, leave-one-family-out over the 30, model minus the glue sort at
25% / 50% kept: +2.5 [+0.4, +4.4] / +2.7 [+0.9, +4.8].

- **Offline and solver disagree.** +2.5 offline is 10% worse in the
  solver, and more instances (8 -> 21) moved the offline number (0 ->
  +2.5) and not the solver's (108 -> 110%). Why is open: find out what
  the normal reduce does that a glue/size sort does not.
- **Glue is hard to beat.** `rdb0.glue` and `rdb0.size` are features,
  yet a model of only those two is 2-3 points below the glue sort on
  new families: raw glue means something else on every instance, the
  sort only compares within a reduce. A within-reduce glue/size rank as
  a feature reproduces the sort and adds nothing to the 24.
- `TARGET=rel` beats `count`, offline (count: 10 points below glue on
  new families) and in the solver.
- Row weights do not help; `family` is the worst at 25%, `none` the
  best, intervals overlapping. The default is still `family`.
- Squared error vs log, training on the stratified vs the fair frame:
  no difference seen.
- What carries the model (group ablation in the solver): the recency
  counters, the learning-time snapshot, size/glue. Rankings, context
  ratios, age and the propagation-cost features are inside the noise.
  Importance-picked feature lists lost to the hand-kept one three times.
- Amount vs order: with a fixed share of tier1/tier2 clauses the A/B
  measured how many clauses are protected, not which. `--predkeep 2`
  (as many as glue would) is the default for that reason.
- The score decides the reduce tiers and nothing else. Letting it also
  replace glue in `likely_to_be_kept()` (vivification, BVE) cost 10
  points. Do not add it back without an A/B.
- The label is a use in the trimmed UNSAT proof. The solver's own
  conflict-analysis uses as a label, alone or mixed in, measured the
  same. In some runs nearly all proof uses fall in the last 10% of the
  run (ps_200_301_70: 99%), so early reduces say little there.
- Plain vs ancestor label (`--predanc 0` vs `1`): within the
  seed noise of each other. Unsettled; deleting one would simplify
  everything.
- A second gathering round under the learnt policy, trained with the
  first: not a loss, not a demonstrated gain. Unsettled.
- The objective and `--predcands` are family-dependent: on bivium `log`
  with `--predcands 1` gave 74% / 65% of the normal build, on a mixed
  corpus the same was a loss. The defaults are the general ones.
- SAT instances cannot be A/B tested one run at a time: the path to a
  model changes with every clause kept.
- Differences under ~10% between single runs are not results.
- Features that grow with the run or the instance (clause age, raw
  counters) measured the same as the 24 without them, and drift on long
  runs. They are refused by the generator.
- Three models of use counts over three horizons, summed, did what the
  one discounted model does at three times the prediction cost.
- Not tuned: trees, depth, `HALFLIFE`.
