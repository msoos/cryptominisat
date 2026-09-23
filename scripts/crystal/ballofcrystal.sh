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

# The whole crystalball pipeline on one UNSAT CNF:
#   1. run the STATS build -> SQLite DB + FRAT proof
#   2. trim the proof and mark which clauses were used, when
#   3. clean, check, sample, denormalise the data
#   4. learn the short/long/forever xgboost predictors
#   5. run the FINAL_PREDICTOR build with them and print a comparison
#
# usage: ballofcrystal.sh [--skip-solve] [--skip-learn] file.cnf
#   --skip-solve  reuse <file>-dir/data.db-raw and data.frat from an earlier run
#   --skip-learn  reuse the predictors from an earlier run, only run step 5

set -e
set -o pipefail  # needed so " | tee xyz " doesn't swallow the last command's error

cd "$(dirname "$0")"
source ./setparams_ballofcrystal.sh
SCRIPTDIR="$(pwd)"

SKIP_SOLVE=0
SKIP_LEARN=0
while [[ "$1" == --* ]]; do
    case "$1" in
        --skip-solve) SKIP_SOLVE=1 ;;
        --skip-learn) SKIP_LEARN=1; SKIP_SOLVE=1 ;;
        *) echo "Unknown option $1"; exit 255 ;;
    esac
    shift
done
if [[ -z "$1" ]]; then
    echo "usage: $0 [--skip-solve] [--skip-learn] file.cnf"
    exit 255
fi
FNAME="$(realpath "$1")"
BASE="$(basename "$FNAME")"
DIR="$(dirname "$FNAME")/${BASE}-dir"

echo "--> CNF:                      $FNAME"
echo "--> work dir:                 $DIR"
echo "--> stats binary:             $STATS_BIN"
echo "--> predictor binary:         $PRED_BIN"
echo "--> dump ratio / lock ratio:  $DUMPRATIO / $CLLOCK"
echo "--> tiers short/long/forever: $SHORT / $LONG / $FOREVER"
echo "--> rows per strata:          $FIXED"

for f in "$STATS_BIN" "$PRED_BIN" "$bestf"; do
    if [[ ! -e "$f" ]]; then echo "ERROR: $f does not exist"; exit 255; fi
done

function stage() { echo; echo "=================== $1 ==================="; }

########################
# 1. Gather data with the STATS build
########################
if [[ $SKIP_SOLVE -eq 0 ]]; then
    rm -rf "$DIR"
    mkdir -p "$DIR"
    cd "$DIR"

    stage "solving with STATS build"
    # --xor 0: no XOR reasoning, so the proof is plain resolution
    $NOBUF "$STATS_BIN" --xor 0 --presimp 1 --sqlitedboverwrite 1 \
        --cldatadumpratio "$DUMPRATIO" --cllockdatagen "$CLLOCK" \
        --everypred "$EVERYPRED" --clid --sql 2 --sqlitedb data.db-raw \
        --xlrup 0 --zero-exit-status "$FNAME" data.frat | tee cms-stats-run.out
    grep -m1 "^c conflicts" cms-stats-run.out
    if ! grep -q "^s UNSATISFIABLE" cms-stats-run.out; then
        echo "ERROR: not UNSAT, crystalball needs an UNSAT instance"
        exit 255
    fi

    # the solver's FRAT has full hint chains, no elaboration needed.
    # Optionally check the proof anyway.
    if [[ -x "$FRAT_XOR" && -x "$CAKE_XLRUP" ]]; then
        stage "checking proof"
        "$FRAT_XOR" elab data.frat "$FNAME" data.xlrup
        "$CAKE_XLRUP" "$FNAME" data.xlrup | tee cake.out
        grep -q "^s VERIFIED UNSAT" cake.out
        rm -f data.xlrup data.frat.temp
    fi
else
    cd "$DIR"
fi

########################
# 2-4. Fill in used_clauses, clean, check, sample, learn
########################
if [[ $SKIP_LEARN -eq 0 ]]; then
    rm -f data.db data-min.db data-min.db-cldata-* predictor-*.json *.out-stage

    stage "fix_up_frat: which clause was used when"
    cp data.db-raw data.db
    "$SCRIPTDIR/fix_up_frat.py" data.frat data.db | tee fix_up_frat.out-stage

    stage "clean_update_data"
    "$SCRIPTDIR/clean_update_data.py" data.db | tee clean_update_data.out-stage

    stage "check_data_quality"
    "$SCRIPTDIR/check_data_quality.py" --slow data.db | tee check_data_quality.out-stage

    stage "sample_data"
    cp data.db data-min.db
    "$SCRIPTDIR/sample_data.py" --short "$SHORT" --long "$LONG" --forever "$FOREVER" \
        data-min.db | tee sample_data.out-stage

    stage "cldata_gen_pandas"
    "$SCRIPTDIR/cldata_gen_pandas.py" data-min.db \
        --short "$SHORT" --long "$LONG" --forever "$FOREVER" \
        --cut1 "$cut1" --cut2 "$cut2" --limit "$FIXED" ${EXTRA_GEN_PANDAS_OPTS} \
        | tee cldata_gen_pandas.out-stage

    stage "cldata_predict"
    for tier in short long forever; do
        for table in used_later used_later_anc; do
            $NOBUF "$SCRIPTDIR/cldata_predict.py" \
                "data-min.db-cldata-${table}-${tier}-cut1-${cut1}-cut2-${cut2}-limit-${FIXED}.dat" \
                --tier "$tier" --table "$table" --features best_only --regressor xgb \
                --xgboostestimators "$XGB_EST" --xboostmaxdepth "$XGB_DEPTH" \
                --xgboostminchild "$XGB_MINCHILD" \
                --basedir . --bestfeatfile "$bestf" \
                > "cldata_predict_${tier}-${table}.out-stage" 2>&1
            grep -E "Mean squared error|==> Saved" "cldata_predict_${tier}-${table}.out-stage" | head -2
        done
    done
    ls -la predictor-*.json
fi

########################
# 5. Run the predictor build with the learnt models
########################
stage "running FINAL_PREDICTOR build"
ln -fs "$SCRIPTDIR/ml_module.py" .
ln -fs "$SCRIPTDIR/ccg.py" .
for TODO in 000 111; do
    $NOBUF "$PRED_BIN" --predtype xgb --predloc . --predbestfeats "$bestf" \
        --everypred "$EVERYPRED" --predtables $TODO --zero-exit-status "$FNAME" \
        > "cms-pred-run.out-${TODO}" 2>&1 || true
    echo -n "predtables $TODO: "; grep -E "^s |^c conflicts|Total time" "cms-pred-run.out-${TODO}" | tr '\n' ' '; echo
done
echo -n "STATS build:    "; grep -E "^s |^c conflicts" cms-stats-run.out | tr '\n' ' '; echo
echo "Done. Predictors are in $DIR/predictor-*.json"
