#!/usr/bin/bash

set -e
set -x


basename="8march-2020-3acd81dc55df3-best-feats"
rm -f out_*
mkdir -p ${basename}
rm -f ${basename}/*
git rev-parse HEAD > ${basename}/out_git
cat learn.sh >> ${basename}/out_git
md5sum *.dat >> ${basename}/out_git


function doit() {

tiers=(
"short"
"long"
"forever")

for tier in "${tiers[@]}"
do
    echo "Doing ${tier}_${computed}computed"

    /usr/bin/time --verbose -o "${basename}/output_${tier}_${computed}computed.timeout" \
    ../cldata_predict.py "${tier}-comb-cut1-$cut1-cut2-$cut2-limit-${limit}.dat" \
    --tier ${tier} --top 500 --xgboost --only "$only" \
    "--${computed}computed" > "${basename}/output_${tier}_${computed}computed"

    echo "Done with ${tier}_${computed}computed"
done

}

limit=2000
cut1="5.0"
cut2="30.0"

only="1.0"
computed="no"
doit

only="0.02"
computed="all"
doit
