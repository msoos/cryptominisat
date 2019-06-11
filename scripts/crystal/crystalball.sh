#!/bin/bash

FNAMEOUT="mydata"
FIXED="30000";
if [ "$1" == "" ]; then
    while true; do
        echo "No CNF command line parameter, running predefined CNFs"
        echo "Options are:"
        echo "1 -- countbitswegner064.cnf"
        echo "2 -- goldb-heqc-i10mul.cnf"
        echo "3 -- goldb-heqc-alu4mul.cnf"
        echo "4 -- AProVE07-16.cnf"
        echo "5 -- UTI-20-10p0.cnf-unz"
        echo "6 -- UCG-20-5p0.cnf"

        read -p "Which CNF do you want to run? " myinput
        case $myinput in
            ["1"]* )
                ORIGTIME="101";
                FNAME="countbitswegner064.cnf";
                RATIO="0.30";
                break;;
            [2]* )
                ORIGTIME="27";
                FNAME="goldb-heqc-i10mul.cnf";
                RATIO="0.60";
                break;;
            [3]* )
                ORIGTIME="70.84";
                FNAME="goldb-heqc-alu4mul.cnf";
                RATIO="0.60";
                break;;
            [4]* )
                ORIGTIME="98s";
                FNAME="AProVE07-16.cnf";
                RATIO="0.60";
                break;;
            [5]* )
                FNAME="UTI-20-10p0.cnf-unz";
                RATIO="0.20";
                break;;
            [6]* )
                ORIGTIME="197";
                FNAME="UCG-20-5p0.cnf";
                RATIO="0.10";
                break;;
            * ) echo "Please answer 1-7";;
        esac
    done
else
    FNAME="$1"
    RATIO="0.60"
    FIXED="20000"
fi

set -e

echo "--> Running on file                   $FNAME"
echo "--> Outputting to data                $FNAMEOUT"
echo "--> Using clause gather ratio of      $RATIO"
echo "--> with fixed number of data points  $FIXED"

if [ "$FNAMEOUT" == "" ]; then
    echo "Error: FNAMEOUT is not set, it's empty. Exiting."
    exit -1
fi

echo "Cleaning up"
(
rm -rf "$FNAME-dir"
mkdir "$FNAME-dir"
cd "$FNAME-dir"
rm -f $FNAMEOUT.d*
rm -f $FNAMEOUT.lemma*
)

set -x

# get data
./build_stats.sh
(
cd "$FNAME-dir"
../cryptominisat5 --gluecut0 100 --dumpdecformodel dec_list --cldatadumpratio "$RATIO" --clid --sql 2 --sqlitedb "$FNAMEOUT.db" --drat "$FNAMEOUT.drat" --zero-exit-status "../$FNAME" | tee cms-pred-run.out
# --bva 0 --updateglueonanalysis 0 --otfsubsume 0
grep "c conflicts" cms-pred-run.out
set +e
a=$(grep "s SATIS" cms-pred-run.out)
retval=$?
set -e
if [[ retval -eq 1 ]]; then
    ../tests/drat-trim/drat-trim "../$FNAME" "$FNAMEOUT.drat" -x "$FNAMEOUT.goodCls" -o "$FNAMEOUT.usedCls" -i
else
    echo "ERROR: The problem you gave is SATISFIABLE"
    echo "ERROR: CrystalBall cannot work with satisfiable instances"
    exit -1
fi

../add_lemma_ind.py "$FNAMEOUT.db" "$FNAMEOUT.goodCls" "$FNAMEOUT.usedCls"
../clean_data.py "$FNAMEOUT.db"
cp "$FNAMEOUT.db" "$FNAMEOUT-min.db"
../rem_data.py "$FNAMEOUT-min.db"


../gen_pandas.py "${FNAMEOUT}-min.db" --fixed "$FIXED" --conf 0-4

mkdir -p ../../src/predict
rm -f ../../src/predict/*.h
for CONF in {0..4}; do
    ../predict.py "${FNAMEOUT}-min.db-short-conf-$CONF.dat" --name short --basedir "../../src/predict/" --final --forest --split 0.1 --conf $CONF

    ../predict.py "${FNAMEOUT}-min.db-long-conf-$CONF.dat" --name long   --basedir "../../src/predict/" --final --forest --split 0.1 --conf $CONF
done
)

./build_final_predictor.sh
(
cd "$FNAME-dir"
../cryptominisat5 "../$FNAME" --printsol 0 --predshort 3 --predlong 3 | tee cms-final-run.out
)
exit

