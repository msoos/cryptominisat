#!/usr/bin/bash

rm -f out_*
rm -f best_feats/*
git rev-parse HEAD > out_git
cat learn.sh >> out_git
md5sum *.dat >> out_git


function doit() {
../cldata_predict.py "${FNAMEOUT}-min.db-cldata-short-cut1-$cut1-cut2-$cut2-limit-${FIXED}.dat" \
--tier short --top 200 --xgboost --only "$only" \
"--${computed}computed" > "best_feats/output_short_${computed}computed"

../cldata_predict.py "${FNAMEOUT}-min.db-cldata-long-cut1-$cut1-cut2-$cut2-limit-${FIXED}.dat" \
--tier long --top 200 --xgboost --only "$only" \
"--${computed}computed" > "best_feats/output_long_${computed}computed"

../cldata_predict.py "${FNAMEOUT}-min.db-cldata-forever-cut1-$cut1-cut2-$cut2-limit-${FIXED}.dat" \
--tier forever --top 2000 --xgboost --only "$only" \
"--${computed}computed" > "best_feats/output_forever_${computed}computed"
}

only="0.3"
compuated="no"
doit

only="1"
computed="all"
doit


