#!/bin/bash

FNAMEOUT="mydata"
FIXED="8000"
RATIO="0.99"
CONF=2

EXTRA_GEN_PANDAS_OPTS=""
if [ "$1" == "--csv" ]; then
    EXTRA_GEN_PANDAS_OPTS="--csv"
    echo "CSV will be generated (may take some disk space)"
    NEXT_OP="$2"
else
    NEXT_OP="$1"
fi

if [ "$NEXT_OP" == "" ]; then
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
                break;;
            [2]* )
                ORIGTIME="16";
                FNAME="goldb-heqc-i10mul.cnf";
                break;;
            [3]* )
                ORIGTIME="70.84";
                FNAME="goldb-heqc-alu4mul.cnf";
                break;;
            [4]* )
                # !!SATISFIABLE!!
                FNAME="g2-mizh-md5-48-2.cnf";
                FIXED="10000";
                break;;
            [5]* )
                ORIGTIME="98s";
                FNAME="AProVE07-16.cnf";
                break;;
            [6]* )
                FNAME="UTI-20-10p0.cnf-unz";
                break;;
            [7]* )
                ORIGTIME="197";
                FNAME="UCG-20-5p0.cnf";
                break;;
            * ) echo "Please answer 1-7";;
        esac
    done
else
    if [ "$NEXT_OP" == "-h" ] || [ "$NEXT_OP" == "--help" ]; then
        echo "You must give a CNF file as input"
        exit
    fi
    initial="$(echo ${NEXT_OP} | head -c 1)"
    if [ "$1" == "-" ]; then
        echo "Cannot understand opion, there are no options"
        exit -1
    fi
    FNAME="${NEXT_OP}"
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
# removed: --decbased 0 --tern 0 --bva 0 --gluecut0 100
../cryptominisat5 --breakid 0 --scc 0 --dumpdecformodel dec_list --sqlitedbover 1 --cldatadumpratio "$RATIO" --clid --sql 2 --sqlitedb "$FNAMEOUT.db-raw" --drat "$FNAMEOUT.drat" --zero-exit-status "../$FNAME" | tee cms-pred-run.out
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
    ../utils/drat-trim/drat-trim "../$FNAME" "$FNAMEOUT.drat" -o "$FNAMEOUT.usedCls" -i
else
    rm -f final.cnf
    touch final.cnf
    cat "../$FNAME" >> final.cnf
    cat dec_list >> final.cnf
    grep ^v cms-pred-run.out | sed "s/v//" | tr -d "\n" | sed "s/  / /g" | sed -e "s/ -/X/g" -e "s/ /Y/g" | sed "s/X/ /g" | sed -E "s/Y([1-9])/ -\1/g" | sed "s/Y0/ 0\n/" >> final.cnf
    ../../utils/cnf-utils/xor_to_cnf.py final.cnf final_good.cnf
    ../tests/drat-trim/drat-trim final_good.cnf "$FNAMEOUT.drat" -o "$FNAMEOUT.usedCls" -i
fi

########################
# Augment, fix up and sample the SQLite data
########################
/usr/bin/time -v ../add_lemma_ind.py "$FNAMEOUT.db-raw" "$FNAMEOUT.usedCls"
cp "$FNAMEOUT.db-raw" "$FNAMEOUT.db"
/usr/bin/time -v ../clean_update_data.py "$FNAMEOUT.db"
cp "$FNAMEOUT.db" "$FNAMEOUT-min.db"
/usr/bin/time -v ../rem_data.py "$FNAMEOUT-min.db"

########################
# Denormalize the data into a Pandas Table, label it and sample it
########################
../gen_pandas.py "${FNAMEOUT}-min.db" --limit "$FIXED" --conf $CONF-$CONF ${EXTRA_GEN_PANDAS_OPTS}
../vardata_gen_pandas.py "${FNAMEOUT}.db" --limit 10000
../vardata_predict.py mydata.db-vardata.dat --nocomputed --picktimeonly --csv -q 2

########################
# Create the classifiers
########################
mkdir -p ../../src/predict
rm -f ../../src/predict/*.h
../clustering.py mydata-min.db-short-conf-2.dat --numconfs 3 --basedir ../src/predict/ --clusters 1 --scale

../vardata_predict.py vardata-comb --final -q 20 --basedir ../src/predict/ --depth 7 --tree
for CONF in {0..2}; do
    ./predict.py short-2-comb.dat --name short --split 0.01 --final --tree --basedir ../src/predict/ --conf $CONF
    ./predict.py long-2-comb.dat --name long --split 0.01 --final --tree --basedir ../src/predict/ --conf $CONF

    #../predict.py "${FNAMEOUT}-min.db-short-conf-$CONF.dat" --name short --basedir "../../src/predict/" --final --forest --split 0.1 --conf $CONF

    # ../predict.py "${FNAMEOUT}-min.db-long-conf-$CONF.dat" --name long   --basedir "../../src/predict/" --final --forest --split 0.1 --conf $CONF
done
)


########################
# Build final CryptoMiniSat with the classifier
########################
./build_final_predictor.sh
(
cd "$FNAME-dir"
../cryptominisat5 "../$FNAME" --printsol 0 --predshort 2 --predlong 2 | tee cms-final-run.out
)
exit
