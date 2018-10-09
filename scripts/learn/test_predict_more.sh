#!/usr/bin/bash
set -x
set -e

FNAME="barman-pfile07-027.sas.cr.37.cnf"
FNAMEOUT="mystuff"
RATIO="1"
FIXED="40000"

FNAME="UTI-20-10p0.cnf-unz"
FNAMEOUT="mystuff2"
RATIO="0.20"
FIXED="150000"

./build_stats.sh
rm -f "$FNAMEOUT.db"
rm -f "$FNAMEOUT.lemmas-*"
rm -f "$FNAMEOUT.db-pandas*"
./cryptominisat5 --cldatadumpratio "$RATIO" --clid --sql 2 --sqlitedb "$FNAMEOUT.db" --drat "$FNAMEOUT.drat" --zero-exit-status "$FNAME" --bva 0
./tests/drat-trim/drat-trim "$FNAME" "$FNAMEOUT.drat" -x $FNAMEOUT.lemmas -i
./add_lemma_ind.py "$FNAMEOUT.db" "$FNAMEOUT.lemmas"
./gen_pandas.py "$FNAMEOUT.db" --fixed "$FIXED"

./predict.py "$FNAMEOUT.db-pandasdata.dat" --final --conf --tree --code ../src/final_predictor.cpp
./build_final_predictor.sh
./cryptominisat5 "$FNAME"
