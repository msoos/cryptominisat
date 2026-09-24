# CrystalBall: learning which learnt clauses to keep

The solver's reduce (`ReduceDB::handle_reduce`) decides which redundant
clauses to delete. CrystalBall replaces the glue/size ordering of the
deletion candidates with an xgboost prediction of how much each clause
will still be used, learnt from the solver's own runs.

Three builds of the solver are involved:

- **normal**: reduce sorts candidates by glue, then size (kissat/CaDiCaL style)
- **stats** (`-DSTATS=ON`): as normal, plus at every reduce dumps every tracked
  clause's state to SQLite, and writes a FRAT proof with full hint chains
- **predictor** (`-DFINAL_PREDICTOR=ON`): as normal, but at every reduce
  predicts each clause's future use and sorts the candidates by it

Only *which* candidates are deleted differs between normal and predictor,
everything else (the `used` life, glue tiers, target fraction, flush) is
the same, so the two can be compared directly.

## One instance, end to end

```
cd scripts/crystal
./ballofcrystal.sh some-unsat.cnf          # everything below, in order
./ballofcrystal.sh --skip-solve x.cnf      # reuse the DB+proof of an earlier run
./ballofcrystal.sh --skip-learn x.cnf      # only re-run the predictor build
```

Parameters (binaries, dump ratio, label horizons, sampling) are in
`setparams_ballofcrystal.sh`, all overridable from the environment, e.g.
`SHORT=2000 LONG=6000 FOREVER=20000 ./ballofcrystal.sh small.cnf` for an
instance that takes ~100k conflicts.

The stages, each a script here:

1. **stats build** solves the CNF: `data.db-raw` (SQLite) + `data.frat`.
   `--cldatadumpratio` picks the fraction of learnt clauses that are
   tracked, `--cllockdatagen` the fraction of those that are never
   deleted. The locked ones are what tells the learner about clauses the
   current heuristic would have thrown away.
2. `fix_up_frat.py` trims the proof backwards from the empty clause and
   records, per tracked clause, at which conflict it was used to derive a
   clause of the final proof (`used_clauses`), and with a decaying weight
   for its descendants (`used_clauses_anc`).
3. `clean_update_data.py`, `check_data_quality.py`: sums and sanity checks.
4. `sample_data.py`: keeps a manageable, use-skewed sample of clauses and
   reduce rows.
5. `cldata_gen_pandas.py`: one denormalised row per (clause, reduce) with
   the labels `used_later_{short,long,forever}`: uses within the next
   SHORT/LONG/FOREVER conflicts, for both tables.
6. `cldata_predict.py`: trains an xgboost regressor per (table, tier)
   on the features of `best_features-*.txt`, writes
   `predictor-<table>-<tier>-xgb.json`. Train/test are split by clause.
7. **predictor build** run with `--predtype xgb --predloc <dir>` and the
   stats/normal conflict counts side by side.

`--predtables abc` picks per tier whether the plain (`0`) or the ancestor
(`1`) table's model is used; `--predsortby` picks what the candidates are
ranked by: the short, long or forever prediction, or (default) their sum.

## Many instances

`run_corpus.sh <outdir> a.cnf b.cnf ...` gathers each instance
(`ballofcrystal.sh --gather-only`, skipped when its frames exist), trains
on the union with `learn.sh` and prints an A/B table with
`eval_corpus.sh`: normal build vs predictor build, plain and ancestor
tables, per instance and in total. All instances must use the same
FIXED and label horizons.

## A new feature list

1. `gen_best_feats.sh <prefix> <out>` trains on all raw features and on
   all raw + computed relative features (thousands) for every
   (table, tier) and prints the xgboost importance rankings. Use the
   `comb-` frames of a `learn.sh` run as the prefix for a corpus.
2. `pick_features.py -n 30 -o best_features.txt <out>` sums the
   importances over the runs and keeps the top N features the solver can
   compute at run time (`gen_pred_features.py --list-raw` lists the raw
   columns it knows).
3. Rebuild the predictor build: `gen_pred_features.py` turns the file
   into `predict_features_gen.h` at build time (`-DPRED_FEATURES_FILE`),
   so the solver's feature code and the training expressions never drift.
   Retrain (`learn.sh`) with the new `bestf` and evaluate.

## Where the features come from

`cmsat_tablestructure.sql` is the schema. `clause_stats` holds what was
known when the clause was learnt, `reduceDB` its state at each reduce
(props/uip1 since the last reduce, discounted versions, rankings within
the DB, `used`, glue...), `reduceDB_common` the DB-wide averages. The
same numbers are computed at run time in `cl_predictors_abs.cpp`
(`--predtype xgb`) and, from the raw columns via `ml_module.py`, in
`cl_predictors_py.cpp` (`--predtype py`); the two must agree, and do:
both give the same conflict count.

`src/predict/predictor_{short,long,forever}.json` are compiled into the
predictor build as the defaults used when `--predloc` is not given.

## Results so far (2026-09-23)

Corpus of 10 UNSAT instances (count14, php10, subsetcard20, six random
3-SAT of 460k-2.2M conflicts, UTI-20-10p0), 130k training rows, 40 trees
of depth 5. Conflicts of the predictor build relative to the normal build:

| | plain tables | ancestor tables |
|---|---|---|
| old 22 features, trained on all 10 (in-sample) | 78% | 88% |
| `best_features.txt` (corpus-picked 30), in-sample | 81% | 80% |
| UTI held out, old features | 102% | 103% |
| UTI held out, `best_features.txt` | 93% | 109% |

So with enough instances the learnt ranking beats glue/size clearly on
the training instances and by 6% on an unseen one (plain tables).
