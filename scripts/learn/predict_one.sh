#!/bin/bash
set -e
set -x

if [[ $# -ne 2 ]]; then
    echo "ERROR: wrong number of arguments"
    echo "Use with ./predict_one.sh 6s153.cnf.gz DIR"
    exit -1
fi

status=`./cryptominisat5 --hhelp | grep sql`
ret=$?
if [ "$ret" -ne 0 ]; then
    echo "You must compile SQL into cryptominisat"
    exit -1
fi

FNAME=$1
OUTDIR=$2
mkdir -p ${OUTDIR}

rm -if ${OUTDIR}/drat_out
rm -if ${OUTDIR}/lemmas
rm -if ${OUTDIR}/*.sqlite
echo "Predicting file $1"

# running CNF
./cryptominisat5 ${FNAME} --verb 0 --gluecut0 10000 --ml 0 --zero-exit-status --clid --sql 2 --sqlitedb ${OUTDIR}/data.sqlite ${OUTDIR}/drat_out

# getting drat
./tests/drat-trim/drat-trim ${FNAME} ${OUTDIR}/drat_out -l ${OUTDIR}/lemmas

# add lemma indices that were good
./add_lemma_ind.py ${OUTDIR}/data.sqlite ${OUTDIR}/lemmas

# run prediction on SQLite database
./predict.py ${OUTDIR}/data.sqlite

# generate DOT and display it
dot -Tpng ${OUTDIR}/data.sqlite.tree.dot -o tree.png
okular tree.png
