# CrystalBall: learning which learnt clauses to keep

CrystalBall replaces the glue/size ordering of the clause deletion candidates
at reduce (`ReduceDB::handle_reduce`) with an xgboost prediction of how much
each clause will still be used, learnt from the solver's own runs.

Three builds of the solver are involved:

- **normal**: reduce sorts candidates by glue, then size
- **stats** (`-DSTATS=ON`): dumps the state of tracked clauses to SQLite at
  every reduce, to generate training data
- **predictor** (`-DFINAL_PREDICTOR=ON`): sorts the candidates by the
  predicted use

## Usage

```
cd scripts/crystal
./ballofcrystal.sh some-unsat.cnf          # gather data, train, evaluate
./run_corpus.sh <outdir> a.cnf b.cnf ...   # the same over many instances
```

Parameters are in `setparams_ballofcrystal.sh`, all overridable from the
environment. The features used are listed in `best_features.txt`;
`gen_best_feats.sh` and `pick_features.py` generate a new list.

The predictor build compiles in the models at
`src/predict/predictor_{short,long,forever}.json`, which are not in the
repository: copy them there from a training run before building.
