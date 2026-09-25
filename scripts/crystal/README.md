# CrystalBall: learning which learnt clauses to keep

At every clause-database reduce the solver throws away the least useful
learnt clauses. Normally "least useful" means highest glue, then longest.
CrystalBall replaces that with a prediction: xgboost models, trained on
the solver's own runs, estimate how much each clause will still be used
over the next 10k / 30k / 120k conflicts, and reduce removes the clauses
with the lowest predicted use.

Three builds of the solver take part:

- **normal**: reduce sorts by glue, then size (the baseline)
- **stats** (`-DSTATS=ON`): dumps the state of tracked clauses to SQLite
  at every reduce and writes a proof, from which the training labels come
- **predictor** (`-DFINAL_PREDICTOR=ON`): sorts by the predicted use

The scripts here run the whole loop: gather data, label it from the
proof, train, embed the models in the solver, compare. See `CLAUDE.md`
for how to run it.
