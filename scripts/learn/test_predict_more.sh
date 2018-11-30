#!/usr/bin/bash
set -x
set -e

# orig: 26s
# prop inside learnt: 22s
# FNAME="barman-pfile07-027.sas.cr.37.cnf"
# FNAMEOUT="mystuff2"
# RATIO="1"
# FIXED="6000"
#
FNAME="UTI-20-10p0.cnf-unz"
FNAMEOUT="mystuff2"
RATIO="0.20"
FIXED="15000"

# orig: 101s
# prop inside learned: 120s
# no prop inside: 77s
# locking in: 131s
FNAME="countbitswegner064.cnf"
FNAMEOUT="mystuff2"
RATIO="0.30"
FIXED="30000"

# orig: 27s
# prop inside learnt: 29
# FNAME="goldb-heqc-i10mul.cnf"
# FNAMEOUT="mystuff2"
# RATIO="0.60"
# FIXED="10000"

# orig time: 70.84
# no prop inside 33.24
# FNAME="goldb-heqc-alu4mul.cnf"
# FNAMEOUT="mystuff2"
# RATIO="0.60"
# FIXED="40000"


# # orig: 98s
# prop inside learnt: 82s
# no prop inside: 95s
# locked: 69s -- 737d7a5dcf32c2c2e07e2f5edf09819ee5fb0dfb
# FNAME="AProVE07-16.cnf"
# FNAMEOUT="mystuff2"
# RATIO="0.60"
# FIXED="30000"


# orig: 197.10
# prop inside learnt: 181.35
# no prop inside: 131.36
# locking in: 158.23
# FNAME="UCG-20-5p0.cnf"
# FNAMEOUT="mystuff2"
# RATIO="0.10" #used 0.6 before lock-in
# FIXED="20000" #must use 4000 for lock-in

# cleanup
rm -f $FNAMEOUT.d*
rm -f $FNAMEOUT.lemmas
rm -f $FNAMEOUT.lemmas-0
rm -f $FNAMEOUT.lemmas-1
rm -f $FNAMEOUT.lemmas-2
rm -f $FNAMEOUT.db-short-pandasdata.dat
rm -f $FNAMEOUT.db-long-pandasdata.dat
rm -f $FNAMEOUT.db-vardata-pandasdata.dat
rm -f ../src/final_predictor*

# get data
./build_stats.sh
mkdir "$FNAME-dir"

(
cd "$FNAME-dir"
../cryptominisat5 --gluecut0 100 --cldatadumpratio "$RATIO" --clid --sql 2 --sqlitedb "$FNAMEOUT.db" --drat "$FNAMEOUT.drat" --zero-exit-status "../$FNAME"
# --bva 0 --updateglueonanalysis 0 --otfsubsume 0

../tests/drat-trim/drat-trim "../$FNAME" "$FNAMEOUT.drat" -x $FNAMEOUT.lemmas -i
../add_lemma_ind.py "$FNAMEOUT.db" "$FNAMEOUT.lemmas"

../gen_pandas.py "$FNAMEOUT.db" --fixed "$FIXED"
# ./gen_pandas.py "$FNAMEOUT.db" --fixed "10000" --csv


../predict.py "$FNAMEOUT.db-short-pandasdata.dat" --basedir "../../src/" --final --conf --tree --code short --split 0.1 --clust 1
../predict.py "$FNAMEOUT.db-long-pandasdata.dat"  --basedir "../../src/" --final --conf --tree --code long  --split 0.1 --clust 1
)

./build_final_predictor.sh

(
cd "$FNAME-dir"
../cryptominisat5 "../$FNAME"
)

#--bva 0 --updateglueonanalysis 0 --otfsubsume 0
