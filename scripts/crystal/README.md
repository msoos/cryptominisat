# CrystalBall: learning which learnt clauses to keep

At every clause-database reduce the solver throws away the least useful
learnt clauses. Normally "least useful" means highest glue, then longest.
CrystalBall replaces that with a prediction: an xgboost model, trained on
the solver's own UNSAT runs, ranks each clause by how much it will still
be used (every future use counted, discounted by how far away it is), and
reduce removes the clauses ranked lowest. With `--predkeep` the rank also
decides which clauses are protected as tier1/tier2, instead of glue.

Three builds of the solver take part:

- **normal**: reduce sorts by glue, then size (the baseline)
- **stats** (`-DSTATS=ON`): dumps the state of tracked clauses to SQLite
  at every reduce and writes a proof, from which the training labels come.
  Only UNSAT instances: a label is "took part in the UNSAT proof", and a
  SAT run has no proof
- **predictor** (`-DFINAL_PREDICTOR=ON`): sorts by the predicted use

The scripts here run the whole loop: gather data, label it from the
proof, train, embed the models in the solver, compare. See `CLAUDE.md`
for how to run it.

To see what a trained model does, `model_report.py` writes one HTML page
about it (its provenance, xgboost's tree statistics and importances, and
SHAP values: the direction and size of every feature's effect, dependence
plots, interactions, the first trees drawn):

    pip install --user shap graphviz    # once; the dot binary for drawn trees
    ./model_report.py <learn dir>/predictor-used_later-disc-xgb.json -o report.html

`<learn dir>` is the output directory of `learn.sh`, which holds the
model next to the training frame (`comb-*.dat`) SHAP needs; for a model
elsewhere (`src/predict/`) give that frame as the second argument.

The report is regenerated, not committed.

## Open questions for a big test

Two choices could not be settled on the short hold-out instances here
(both are within the seed noise); a day-long test should run both sides:

- **Age features or not.** The default `best_features.txt` (24) has
  nothing that grows with the run length or the instance size;
  `best_features-general30.txt` has the age features. Train each with
  `bestf=$(realpath <list>) ./learn.sh <out> <dirs>` on the same gathered
  data, build a predictor per list (the age list needs
  `-DPRED_FEATURES_FILE=<list> -DPRED_ALLOW_ABSOLUTE=ON`), and compare
  the `pred feats outside training range` line at the end of the runs.
- **Plain or ancestor label.** `learn.sh` trains both; `--predtables 0`
  (default) uses the plain one, `--predtables 1` the ancestor one, from
  the same `--predloc`. Delete the loser from the pipeline afterwards.

Details and the numbers so far: `CLAUDE.md`.
