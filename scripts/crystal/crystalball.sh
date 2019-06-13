#!/bin/bash

FNAMEOUT="mydata"
FIXED="30000"
RATIO="0.60"

if [ "$1" == "" ]; then
    while true; do
        echo "No CNF command line parameter, running predefined CNFs"
        echo "Options are:"
        echo "1 -- countbitswegner064.cnf"
        echo "2 -- goldb-heqc-i10mul.cnf"
        echo "3 -- goldb-heqc-alu4mul.cnf"
        echo "4 -- g2-mizh-md5-48-2.cnf"
        echo "5 -- AProVE07-16.cnf"
        echo "6 -- UTI-20-10p0.cnf-unz"
        echo "7 -- UCG-20-5p0.cnf"

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
                # !!SATISFIABLE!!
                FNAME="g2-mizh-md5-48-2.cnf";
                RATIO="1.0";
                FIXED="10000";
                break;;
            [5]* )
                ORIGTIME="98s";
                FNAME="AProVE07-16.cnf";
                RATIO="0.60";
                break;;
            [6]* )
                FNAME="UTI-20-10p0.cnf-unz";
                RATIO="0.20";
                break;;
            [7]* )
                ORIGTIME="197";
                FNAME="UCG-20-5p0.cnf";
                RATIO="0.10";
                break;;
            * ) echo "Please answer 1-7";;
        esac
    done
else
    if [ "$1" == "-h" ] || [ "$1" == "--help" ]; then
        echo "You must give a CNF file as input"
        exit
    fi
    initial="$(echo $1 | head -c 1)"
    if [ "$1" == "-" ]; then
        echo "Cannot understand opion, there are no options"
        exit -1
    fi
    FNAME="$1"
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


########################
# Build statistics-gathering CryptoMiniSat
########################
./build_stats.sh


(
########################
# Obtain dynamic data in SQLite and DRAT info
########################
cd "$FNAME-dir"
../cryptominisat5 --gluecut0 100 --dumpdecformodel dec_list --cldatadumpratio "$RATIO" --clid --sql 2 --sqlitedb "$FNAMEOUT.db-raw" --drat "$FNAMEOUT.drat" --zero-exit-status "../$FNAME" | tee cms-pred-run.out
# --bva 0 --updateglueonanalysis 0 --otfsubsume 0
grep "c conflicts" cms-pred-run.out

########################
# Run our own DRAT-Trim
########################
set +e
a=$(grep "s SATIS" cms-pred-run.out)
retval=$?
set -e
if [[ retval -eq 1 ]]; then
    ../tests/drat-trim/drat-trim "../$FNAME" "$FNAMEOUT.drat" -x "$FNAMEOUT.goodCls" -o "$FNAMEOUT.usedCls" -i
else
    rm -f final.cnf
    touch final.cnf
    cat "../$FNAME" >> final.cnf
    cat dec_list >> final.cnf
    grep ^v cms-pred-run.out | sed "s/v//" | tr -d "\n" | sed "s/  / /g" | sed -e "s/ -/X/g" -e "s/ /Y/g" | sed "s/X/ /g" | sed -E "s/Y([1-9])/ -\1/g" | sed "s/Y0/ 0\n/" >> final.cnf
    ../../utils/cnf-utils/xor_to_cnf.py final.cnf final_good.cnf
    ../tests/drat-trim/drat-trim final_good.cnf "$FNAMEOUT.drat" -x "$FNAMEOUT.goodCls" -o "$FNAMEOUT.usedCls" -i
fi

########################
# Augment, fix up and sample the SQLite data
########################
../add_lemma_ind.py "$FNAMEOUT.db-raw" "$FNAMEOUT.goodCls" "$FNAMEOUT.usedCls"
cp "$FNAMEOUT.db-raw" "$FNAMEOUT.db"
../clean_data.py "$FNAMEOUT.db"
cp "$FNAMEOUT.db" "$FNAMEOUT-min.db"
../rem_data.py "$FNAMEOUT-min.db"

########################
# Denormalize the data into a Pandas Table, label it and sample it
########################
../gen_pandas.py "${FNAMEOUT}-min.db" --fixed "$FIXED" --conf 0-4

########################
# Create the classifiers
########################
mkdir -p ../../src/predict
rm -f ../../src/predict/*.h
for CONF in {0..4}; do
    ../predict.py "${FNAMEOUT}-min.db-short-conf-$CONF.dat" --name short --basedir "../../src/predict/" --final --forest --split 0.1 --conf $CONF

    ../predict.py "${FNAMEOUT}-min.db-long-conf-$CONF.dat" --name long   --basedir "../../src/predict/" --final --forest --split 0.1 --conf $CONF
done
)


########################
# Build final CryptoMiniSat with the classifier
########################
./build_final_predictor.sh
(
cd "$FNAME-dir"
../cryptominisat5 "../$FNAME" --printsol 0 --predshort 3 --predlong 3 | tee cms-final-run.out
)
exit
