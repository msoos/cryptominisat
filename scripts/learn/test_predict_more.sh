#!/usr/bin/bash
set -x
set -e

# (prop inside) orig: 26s learnt: 22s
FNAME="barman-pfile07-027.sas.cr.37.cnf"
FNAMEOUT="mystuff"
RATIO="1"
FIXED="6000"

FNAME="UTI-20-10p0.cnf-unz"
FNAMEOUT="mystuff2"
RATIO="0.20"
FIXED="150000"

# (prop inside) orig: 101s, learned: 120s
# no prop inside: 77s
FNAME="countbitswegner064.cnf"
FNAMEOUT="mystuff2"
RATIO="0.60"
FIXED="70000"

# (prop inside) orig: 27s learnt: 29
# FNAME="goldb-heqc-i10mul.cnf"
# FNAMEOUT="mystuff2"
# RATIO="0.60"
# FIXED="60000"
#
# # (prop inside) orig: 98s learnt: 82s
# # no prop inside: 95s
# FNAME="AProVE07-16.cnf"
# FNAMEOUT="mystuff2"
# RATIO="0.60"
# FIXED="30000"


# (prop inside) orig: 174.79 learnt: 181.35
# no prop inside: 131.36
# FNAME="UCG-20-5p0.cnf"
# FNAMEOUT="mystuff2"
# RATIO="0.60"
# FIXED="20000"

# cleanup
rm -f "$FNAMEOUT.db"
rm -f "$FNAMEOUT.lemmas-*"
rm -f "$FNAMEOUT.db-pandas*"

# get data
./build_stats.sh
./cryptominisat5 --cldatadumpratio "$RATIO" --clid --sql 2 --sqlitedb "$FNAMEOUT.db" --drat "$FNAMEOUT.drat" --zero-exit-status "$FNAME" --bva 0
./tests/drat-trim/drat-trim "$FNAME" "$FNAMEOUT.drat" -x $FNAMEOUT.lemmas -i
./add_lemma_ind.py "$FNAMEOUT.db" "$FNAMEOUT.lemmas"

./gen_pandas.py "$FNAMEOUT.db" --fixed "$FIXED"
./predict.py "$FNAMEOUT.db-pandasdata.dat" --final --conf --tree --code ../src/final_predictor.cpp
./build_final_predictor.sh
./cryptominisat5 "$FNAME"
