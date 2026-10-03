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

## Two feature lists to compare in the big test

The embedded model uses `best_features.txt`: 24 features, none of which
grows with the run length or the instance size (clause age, time since
last touch, variable count...). The reason is that the training runs are
minutes long and the real ones a day: a tree model asked about a value it
never saw clamps silently. The old list, `best_features-general30.txt`,
has those age-related features. On the short hold-out instances the two
are within the noise of each other; which is better on day-long runs is
not known, so **the big test must run both**. Everything to do so is
still here: the stats build dumps every raw column whatever the list,
only the predictor build's generator refuses the age features and needs
an override.

    # train (the data needs no regathering)
    bestf=$(realpath best_features-general30.txt) FIXED=20000 ./learn.sh <out-age> <gathered dirs>
    bestf=$(realpath best_features.txt)           FIXED=20000 ./learn.sh <out-sf>  <gathered dirs>

    # a predictor build per list (the age list needs PRED_ALLOW_ABSOLUTE)
    cmake -DFINAL_PREDICTOR=ON -DPRED_FEATURES_FILE=$PWD/best_features-general30.txt -DPRED_ALLOW_ABSOLUTE=ON ../build_pred_age
    cmake -DFINAL_PREDICTOR=ON ../build_pred

    # run each with its own models
    build_pred_age/cryptominisat5 --predtype xgb --predloc <out-age> file.cnf
    build_pred/cryptominisat5     --predtype xgb --predloc <out-sf>  file.cnf

Each model carries the training range of its features; at the end of a
run the solver prints the share of values it saw outside that range
(`pred feats outside training range`). On a day-long run that line is the
first thing to compare: the age list's `cl.time_inside_solver` is the
feature that drifts. Details in `CLAUDE.md`, "Scale-free features".
