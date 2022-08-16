#!/bin/bash

# Copyright (C) 2009-2020 Authors of CryptoMiniSat, see AUTHORS file
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; version 2
# of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301, USA.

# This file wraps CMake invocation for TravisCI
# so we can set different configurations via environment variables.

set -e
set -x
set -o pipefail  # needed so  " | tee xyz " doesn't swallow the last command's error

. ./setparams_ballofcrystal.sh

if [ "$1" == "--skip" ]; then
    echo "Will skip running CMS stats + FRAT"
    SKIP="1"
    NEXT_OP="$2"
else
    SKIP="0"
    NEXT_OP="$1"
fi

if [ "$NEXT_OP" == "" ]; then
    while true; do
        echo "No CNF command line parameter, running predefined CNFs"
        echo "Options are:"
        echo "1 -- velev-pipe-uns-1.0-9.cnf"
        echo "2 -- goldb-heqc-i10mul.cnf"
        echo "3 -- Haystacks-ext-13_c18.cnf"
        echo "4 -- NONE"
        echo "5 -- AProVE07-16.cnf"
        echo "6 -- UTI-20-10p0.cnf-unz"
        echo "7 -- UCG-20-5p0.cnf"

        read -p "Which CNF do you want to run? " myinput
        case $myinput in
            ["1"]* )
                FNAME="velev-pipe-uns-1.0-9.cnf"
                break;;
            [2]* )
                FNAME="goldb-heqc-i10mul.cnf";
                break;;
            [3]* )
                FNAME="dist6.c-sc2018.cnf";
                break;;
            [5]* )
                FNAME="AProVE07-16.cnf";
                break;;
            [6]* )
                DUMPRATIO="0.01"
                FNAME="UTI-20-10p0.cnf-unz";
                FIXED="20000";
                break;;
            [7]* )
                DUMPRATIO="0.50"
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
set -o pipefail

echo "--> Running on file                   $FNAME"
echo "--> Outputting to data                $FNAMEOUT"
echo "--> Using clause gather ratio of      $DUMPRATIO"
echo "--> Locking in ratio of               $CLLOCK"
echo "--> with fixed number of data points  $FIXED"

if [ "$FNAMEOUT" == "" ]; then
    echo "Error: FNAMEOUT is not set, it's empty. Exiting."
    exit -1
fi


if [ "$SKIP" != "1" ]; then
    echo "Cleaning up"
    (
    rm -rf "$FNAME-dir"
    mkdir "$FNAME-dir"
    cd "$FNAME-dir"
    rm -f $FNAMEOUT.d*
    rm -f $FNAMEOUT.lemma*
    )

    set -e

    ########################
    # Build statistics-gathering CryptoMiniSat
    ########################
    if [[ SANITIZE -eq 0 ]]; then
        ./build_stats.sh
    else
        ./build_stats_sanitize.sh
    fi

    (
    ########################
    # Obtain dynamic data in SQLite and FRAT info
    ########################
    cd "$FNAME-dir"
    # for var, we need: --bva 0 --scc 0
    $NOBUF ../cryptominisat5 --maxnummatrices 0 --presimp 1 -n1 --sqlitedbover 1 --cldatadumpratio "$DUMPRATIO" --cllockdatagen $CLLOCK --clid --sql 2 --sqlitedb "$FNAMEOUT.db-raw" --frat "$FNAMEOUT.frat" --zero-exit-status "../$FNAME" | tee cms-pred-run.out
    grep "c conflicts" cms-pred-run.out

    ########################
    # Run frat-rs
    ########################
    set +e
    a=$(grep "s SATIS" cms-pred-run.out)
    retval=$?
    set -e
    if [[ retval -eq 1 ]]; then
        /usr/bin/time -v ../frat-rs elab "../$FNAME" "$FNAMEOUT.frat" -m -v

        set +e
        /usr/bin/time -v ../frat-rs elab "$FNAMEOUT.frat" - tmp.lrat
        /usr/bin/time -v ../frat-rs refrat "$FNAMEOUT.frat.temp" correct
        set -e
        #/usr/bin/time -v $NOBUF ../utils/drat-trim/drat-trim "../$FNAME" "$FNAMEOUT.drat" -o "$FNAMEOUT.usedCls" -i -O 4 -m | tee drat.out-newO4


    else
        echo "Not UNSAT!!!"
        exit -1
    fi
    echo "CMS+FRAT done now"
    )
fi

(
set -e
cd "$FNAME-dir"

########################
# Augment, fix up and sample the SQLite data
########################

rm -f "$FNAMEOUT.db"
rm -f "$FNAMEOUT-min.db"
rm -f "${FNAMEOUT}-min.db-cldata-*"
rm -f out_pred_*
rm -f sample.out
rm -f check_quality.out
rm -f clean_update.out
rm -f fill_clauses.out

../fix_up_frat.py correct $FNAMEOUT.db-raw | tee fill_used_clauses.out
cp "$FNAMEOUT.db-raw" "$FNAMEOUT.db"
/usr/bin/time -v ../clean_update_data.py "$FNAMEOUT.db"  | tee clean_update_data.out
../check_data_quality.py --slow "$FNAMEOUT.db" | tee check_data_quality.out
cp "$FNAMEOUT.db" "$FNAMEOUT-min.db"
/usr/bin/time -v ../sample_data.py "$FNAMEOUT-min.db" | tee sample_data.out


########################
# Denormalize the data into a Pandas Table, label it and sample it
########################
../cldata_gen_pandas.py "${FNAMEOUT}-min.db" --cut1 $cut1 --cut2 $cut2 --limit "$FIXED" ${EXTRA_GEN_PANDAS_OPTS} | tee cldata_gen_pandas.out
# ../vardata_gen_pandas.py "${FNAMEOUT}.db" --limit 1000


####################################
# Clustering for cldata, using cldata dataframe
####################################
# ../clustering.py ${FNAMEOUT}-min.db-cldata-*short*.dat ${FNAMEOUT}-min.db-cldata-*long*.dat --basedir ../../src/predict/ --clusters 4 --scale --nocomputed


####################################
# Create the classifiers
####################################

# TODO: add --csv  to dump CSV
#       then you can play with Weka
#../vardata_predict.py mydata.db-vardata.dat --picktimeonly -q 2 --only 0.99
#../vardata_predict.py vardata-comb --final -q 20 --basedir ../src/predict/ --depth 7 --tree

set -e
set -o pipefail
rm -f .*.json
regressors=("xgb") # "lgbm" )
tiers=("short" "long" "forever")
tables=("used_later" "used_later_anc")
for tier in "${tiers[@]}"; do
    for table in "${tables[@]}"; do
        for regressor in "${regressors[@]}"; do
            $NOBUF ../cldata_predict.py "${FNAMEOUT}-min.db-cldata-${table}-${tier}-cut1-$cut1-cut2-$cut2-limit-${FIXED}.dat" --tier ${tier} --table ${table} --features best_only --regressor $regressor --basedir "." --bestfeatfile $bestf | tee "cldata_predict_${tier}-${table}-${regressor}.out"
        done
    done
done

############################
# To get feature importances
############################
# ../cldata_predict.py "${FNAMEOUT}-min.db-cldata-long-cut1-$cut1-cut2-$cut2-limit-${FIXED}.dat" --tier long --top 200 --xgb --allcomputed > output_long
# ../cldata_predict.py "${FNAMEOUT}-min.db-cldata-long-cut1-$cut1-cut2-$cut2-limit-${FIXED}.dat" --tier long --top 200 --xgb --nocomputed > output_long_nocomputed
# ../cldata_predict.py "${FNAMEOUT}-min.db-cldata-short-cut1-$cut1-cut2-$cut2-limit-${FIXED}.dat" --tier short --top 200 --xgb --allcomputed > output_short
# ../cldata_predict.py "${FNAMEOUT}-min.db-cldata-short-cut1-$cut1-cut2-$cut2-limit-${FIXED}.dat" --tier short --top 200 --xgb --nocomputed > output_short_nocomputed
# ../cldata_predict.py "${FNAMEOUT}-min.db-cldata-forever-cut1-$cut1-cut2-$cut2-limit-${FIXED}.dat" --tier forever --top 2000 --xgb --nocomputed --topperc > "output_forever_nocomputed"
# ../cldata_predict.py "${FNAMEOUT}-min.db-cldata-forever-cut1-$cut1-cut2-$cut2-limit-${FIXED}.dat" --tier forever --top 2000 --xgb --allcomputed --topperc > "output_forever"


)


########################
# Build final CryptoMiniSat with the classifier
########################
if [[ $SANITIZE -eq 1 ]]; then
    ./build_final_predictor_sanitize.sh
else
    ./build_final_predictor.sh
fi

(
cd "$FNAME-dir"
ln -fs ../ml_module.py .

TODO="000"
../cryptominisat5 "../$FNAME" --predtype py --simfrat 1 --printsol 0 --predloc "./" --predbestfeats "$bestf" --predtables $TODO --distillsort 3 > cms-final-run.out-${TODO}-distillsort3 &

TODO="001"
../cryptominisat5 "../$FNAME" --predtype py --simfrat 1 --printsol 0 --predloc "./" --predbestfeats "$bestf" --predtables $TODO --distillsort 3 > cms-final-run.out-${TODO}-distillsort3 &

TODO="010"
../cryptominisat5 "../$FNAME" --predtype py --simfrat 1 --printsol 0 --predloc "./" --predbestfeats "$bestf" --predtables $TODO --distillsort 3 > cms-final-run.out-${TODO}-distillsort3 &

TODO="011"
../cryptominisat5 "../$FNAME" --predtype py --simfrat 1 --printsol 0 --predloc "./" --predbestfeats "$bestf" --predtables $TODO --distillsort 3 > cms-final-run.out-${TODO}-distillsort3 &

TODO="100"
../cryptominisat5 "../$FNAME" --predtype py --simfrat 1 --printsol 0 --predloc "./" --predbestfeats "$bestf" --predtables $TODO --distillsort 3 > cms-final-run.out-${TODO}-distillsort3 &

TODO="101"
../cryptominisat5 "../$FNAME" --predtype py --simfrat 1 --printsol 0 --predloc "./" --predbestfeats "$bestf" --predtables $TODO --distillsort 3 > cms-final-run.out-${TODO}-distillsort3 &

TODO="110"
../cryptominisat5 "../$FNAME" --predtype py --simfrat 1 --printsol 0 --predloc "./" --predbestfeats "$bestf" --predtables $TODO --distillsort 3 > cms-final-run.out-${TODO}-distillsort3 &

TODO="111"
../cryptominisat5 "../$FNAME" --predtype py --simfrat 1 --printsol 0 --predloc "./" --predbestfeats "$bestf" --predtables $TODO --distillsort 3 > cms-final-run.out-${TODO}-distillsort3 &

)
exit


# old
# ./cryptominisat5 goldb-heqc-i10mul.cnf --simfrat 1 --printsol 0 --predloc ./data/14-april-2021-69bad529f962c-cut1-3.0-cut2-25.0-limit-1000-est10-w0-xbmin50-xbmd6/ --predtype py --predbestfeats ./best_features-b93210018231ab04d2.txt

# current
# ./cryptominisat5 goldb-heqc-i10mul.cnf --simfrat 1 --printsol 0 --predloc ./data/18-sept-2021-a408d53c665f9305b-cut1-3.0-cut2-25.0-limit-1000-est15-w0-xbmin50-xbmd4-regxgb/ --predtype py --predbestfeats ./best_features-rdb0-only.txt

# newest
# ./cryptominisat5 goldb-heqc-i10mul.cnf --simfrat 1 --printsol 0 --predloc ./data/29-sept-a408d53c665f9305b-ccf121255372-cut1-3.0-cut2-25.0-limit-3000-est8-w0-xbmin10-xbmd6-regxgb/ --predtype py --predbestfeats ./best_features-correlaton.txt
