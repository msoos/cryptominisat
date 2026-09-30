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
