#!/usr/bin/bash

rm -f out_*
rm -f best_feats/*
git rev-parse HEAD > best_feats/out_git
cat learn.sh >> best_feats/out_git
md5sum *.dat >> best_feats/out_git


function doit() {
../cldata_predict.py "short-comb-cut1-$cut1-cut2-$cut2-limit-${limit}.dat" \
--tier short --top 200 --xgboost --only "$only" \
"--${computed}computed" > "best_feats/output_short_${computed}computed"

../cldata_predict.py "long-comb-cut1-$cut1-cut2-$cut2-limit-${limit}.dat" \
--tier long --top 200 --xgboost --only "$only" \
"--${computed}computed" > "best_feats/output_long_${computed}computed"

../cldata_predict.py "forever-comb-cut1-$cut1-cut2-$cut2-limit-${limit}.dat" \
--tier forever --top 2000 --xgboost --only "$only" \
"--${computed}computed" > "best_feats/output_forever_${computed}computed"
}

limit=2000
cut1="40.0"
cut2="70.0"
only="1.0"
computed="no"
doit

only="0.3"
computed="all"
doit


