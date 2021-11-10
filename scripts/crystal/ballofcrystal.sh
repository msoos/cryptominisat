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

. ./setparams_ballofcrystal.sh

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
        echo "4 -- NONE"
        echo "5 -- AProVE07-16.cnf"
        echo "6 -- UTI-20-10p0.cnf-unz"
        echo "7 -- UCG-20-5p0.cnf"

        read -p "Which CNF do you want to run? " myinput
        case $myinput in
            ["1"]* )
                FNAME="countbitswegner064.cnf";
                break;;
            [2]* )
                FNAME="goldb-heqc-i10mul.cnf";
                break;;
            [3]* )
                FNAME="goldb-heqc-alu4mul.cnf";
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

echo "--> Running on file                   $FNAME"
echo "--> Outputting to data                $FNAMEOUT"
echo "--> Using clause gather ratio of      $DUMPRATIO"
echo "--> Locking in ratio of               $CLLOCK"
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
if [[ SANITIZE -eq 1 ]]; then
    ./build_stats.sh
else
    ./build_stats_sanitize.sh
fi

NOBUF="stdbuf -oL -eL "

(
########################
# Obtain dynamic data in SQLite and DRAT info
########################
cd "$FNAME-dir"
# for var, we need: --bva 0 --scc 0
$NOBUF ../cryptominisat5 --presimp 1 -n1 ${EXTRA_CMS_OPTS} --sqlitedbover 1 --cldatadumpratio "$DUMPRATIO" --cllockdatagen $CLLOCK --clid --sql 2 --sqlitedb "$FNAMEOUT.db-raw" --drat "$FNAMEOUT.drat" --zero-exit-status "../$FNAME" | tee cms-pred-run.out
grep "c conflicts" cms-pred-run.out

########################
# Run our own DRAT-Trim
########################
set +e
a=$(grep "s SATIS" cms-pred-run.out)
retval=$?
set -e
if [[ retval -eq 1 ]]; then
    rm -f $FNAMEOUT.usedCls-*
    /usr/bin/time -v $NOBUF ../utils/drat-trim/drat-trim "../$FNAME" "$FNAMEOUT.drat" -o "$FNAMEOUT.usedCls" -i -O 3 | tee drat.out-newO3
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
../fill_used_clauses.py "$FNAMEOUT.db-raw" "$FNAMEOUT.usedCls"
cp "$FNAMEOUT.db-raw" "$FNAMEOUT.db"
/usr/bin/time -v ../clean_update_data.py "$FNAMEOUT.db"
../check_data_quality.py --slow "$FNAMEOUT.db"
cp "$FNAMEOUT.db" "$FNAMEOUT-min.db"
/usr/bin/time -v ../sample_data.py "$FNAMEOUT-min.db"

########################
# Denormalize the data into a Pandas Table, label it and sample it
########################
../cldata_gen_pandas.py "${FNAMEOUT}-min.db" --cut1 $cut1 --cut2 $cut2 --limit "$FIXED" ${EXTRA_GEN_PANDAS_OPTS}
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

rm -f .*.json
regressors=("xgb") # "lgbm" )
tiers=("short" "long" "forever")
tables=("used_later" "used_later_anc")
for tier in "${tiers[@]}"; do
    for table in "${tables[@]}"; do
        for regressor in "${regressors[@]}"; do
            ../cldata_predict.py "${FNAMEOUT}-min.db-cldata-${table}-${tier}-cut1-$cut1-cut2-$cut2-limit-${FIXED}.dat" --tier ${tier} --table ${table} --features best_only --regressor $regressor --basedir "." --bestfeatfile $bestf | tee "out_pred_${tier}-${table}-${regressor}"
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
ln -s ../ml_module.py .
ln -s ../crystalcodegen.py .
$NOBUF ../cryptominisat5 "../$FNAME" ${EXTRA_CMS_OPTS} --predtype py --simdrat 1 --printsol 0 --predloc "./" --predbestfeats "$bestf" --predtables 111 --distillsort 3 | tee cms-final-run.out-111-distillsort3

TODO="000"
$NOBUF ../cryptominisat5 "../$FNAME" ${EXTRA_CMS_OPTS} --predtype py --simdrat 1 --printsol 0 --predloc "./" --predbestfeats "$bestf" --predtables $TODO --distillsort 3 | tee cms-final-run.out-${TODO}-distillsort3

TODO="001"
$NOBUF ../cryptominisat5 "../$FNAME" ${EXTRA_CMS_OPTS} --predtype py --simdrat 1 --printsol 0 --predloc "./" --predbestfeats "$bestf" --predtables $TODO --distillsort 3 | tee cms-final-run.out-${TODO}-distillsort3

TODO="010"
$NOBUF ../cryptominisat5 "../$FNAME" ${EXTRA_CMS_OPTS} --predtype py --simdrat 1 --printsol 0 --predloc "./" --predbestfeats "$bestf" --predtables $TODO --distillsort 3 | tee cms-final-run.out-${TODO}-distillsort3

TODO="011"
$NOBUF ../cryptominisat5 "../$FNAME" ${EXTRA_CMS_OPTS} --predtype py --simdrat 1 --printsol 0 --predloc "./" --predbestfeats "$bestf" --predtables $TODO --distillsort 3 | tee cms-final-run.out-${TODO}-distillsort3

TODO="100"
$NOBUF ../cryptominisat5 "../$FNAME" ${EXTRA_CMS_OPTS} --predtype py --simdrat 1 --printsol 0 --predloc "./" --predbestfeats "$bestf" --predtables $TODO --distillsort 3 | tee cms-final-run.out-${TODO}-distillsort3

TODO="101"
$NOBUF ../cryptominisat5 "../$FNAME" ${EXTRA_CMS_OPTS} --predtype py --simdrat 1 --printsol 0 --predloc "./" --predbestfeats "$bestf" --predtables $TODO --distillsort 3 | tee cms-final-run.out-${TODO}-distillsort3

TODO="110"
$NOBUF ../cryptominisat5 "../$FNAME" ${EXTRA_CMS_OPTS} --predtype py --simdrat 1 --printsol 0 --predloc "./" --predbestfeats "$bestf" --predtables $TODO --distillsort 3 | tee cms-final-run.out-${TODO}-distillsort3

TODO="111"
$NOBUF ../cryptominisat5 "../$FNAME" ${EXTRA_CMS_OPTS} --predtype py --simdrat 1 --printsol 0 --predloc "./" --predbestfeats "$bestf" --predtables $TODO --distillsort 3 | tee cms-final-run.out-${TODO}-distillsort3

)
exit


# old
# ./cryptominisat5 goldb-heqc-i10mul.cnf --simdrat 1 --printsol 0 --predloc ./data/14-april-2021-69bad529f962c-cut1-3.0-cut2-25.0-limit-1000-est10-w0-xbmin50-xbmd6/ --predtype py --predbestfeats ./best_features-b93210018231ab04d2.txt

# current
# ./cryptominisat5 goldb-heqc-i10mul.cnf --simdrat 1 --printsol 0 --predloc ./data/18-sept-2021-a408d53c665f9305b-cut1-3.0-cut2-25.0-limit-1000-est15-w0-xbmin50-xbmd4-regxgb/ --predtype py --predbestfeats ./best_features-rdb0-only.txt

# newest
# ./cryptominisat5 goldb-heqc-i10mul.cnf --simdrat 1 --printsol 0 --predloc ./data/29-sept-a408d53c665f9305b-ccf121255372-cut1-3.0-cut2-25.0-limit-3000-est8-w0-xbmin10-xbmd6-regxgb/ --predtype py --predbestfeats ./best_features-correlaton.txt
