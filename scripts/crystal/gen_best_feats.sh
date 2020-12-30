#!/usr/bin/bash

rm -f out_*
rm -f best_feats/*
git rev-parse HEAD > best_feats/out_git
cat learn.sh >> best_feats/out_git
md5sum *.dat >> best_feats/out_git


function doit() {

tiers=("short" "long" "forever")
for tier in "${tiers[@]}"
do
    echo "Doing ${tier}_${computed}computed"

    /usr/bin/time --verbose -o "best_feats/output_${tier}_${computed}computed.timeout" \
    ../cldata_predict.py "${tier}-comb-cut1-$cut1-cut2-$cut2-limit-${limit}.dat" \
    --tier ${tier} --top 2000 --xgboost --only "$only" \
    "--${computed}computed" > "best_feats/output_${tier}_${computed}computed"

    echo "Done with ${tier}_${computed}computed"
done

}

limit=8000
cut1="10.0"
cut2="40.0"

only="1.0"
computed="no"
doit

only="0.05"
computed="all"
doit
