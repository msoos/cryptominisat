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
#   1. run the STATS build -> SQLite DB + XLRUP proof
#   2. trim the proof and mark which clauses were used, when
#   3. clean, check, sample, denormalise the data
#   4. learn the xgboost predictors
#   5. run the FINAL_PREDICTOR build with them and print a comparison
#
# usage: ballofcrystal.sh [--skip-solve] [--skip-learn] [--gather-only] file.cnf
#   --skip-solve  reuse <file>-dir/data.db-raw and data.xlrup from an earlier run
#   --skip-learn  reuse the predictors from an earlier run, only run step 5
#   --gather-only stop after step 3, the frames are for learn.sh
# KEEP_PROOF=1 keeps data.xlrup (it is deleted once used, GBs on big CNFs)
# STATS_OPTS: more options for the stats run. With a STATS=ON
#   FINAL_PREDICTOR=ON build as STATS_BIN, "--predloc DIR"
#   gathers under the learnt policy (a second round, DAgger-style)

set -e
set -o pipefail  # needed so " | tee xyz " doesn't swallow the last command's error

cd "$(dirname "$0")"
source ./setparams_ballofcrystal.sh
SCRIPTDIR="$(pwd)"

SKIP_SOLVE=0
SKIP_LEARN=0
GATHER_ONLY=0
while [[ "$1" == --* ]]; do
    case "$1" in
        --skip-solve) SKIP_SOLVE=1 ;;
        --skip-learn) SKIP_LEARN=1; SKIP_SOLVE=1 ;;
        --gather-only) GATHER_ONLY=1 ;;
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
echo "--> halflife:                 $HALFLIFE reduces"
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

    if [[ "$DUMPRATIO" == "auto" ]]; then
        if [[ -z "$CONFL" ]]; then
            stage "normal build, for the conflict count"
            "$NORMAL_BIN" --xor 0 --zero-exit-status "$FNAME" > cms-count-run.out 2>&1 || true
            CONFL=$(grep -m1 "^c conflicts" cms-count-run.out | awk '{print $4}')
        fi
        DUMPRATIO=$(awk -v c="${CONFL:-0}" -v t="$TRACKED" -v m="$MAXDUMPRATIO" \
            'BEGIN { r = (c > 0) ? t/c : m; if (r > m) r = m; printf "%.4f", r }')
        echo "--> conflicts $CONFL, dump ratio $DUMPRATIO"
    fi

    stage "solving with STATS build"
    # --xor 0: no XOR reasoning, so the proof is plain resolution. --xlrup 2: binary
    $NOBUF "$STATS_BIN" --xor 0 --presimp 1 --sqlitedboverwrite 1 \
        --cldatadumpratio "$DUMPRATIO" --cllockdatagen "$CLLOCK" \
        --everypred "$EVERYPRED" --clid --sql 2 --sqlitedb data.db-raw \
        --xlrup 2 $STATS_OPTS --zero-exit-status "$FNAME" data.xlrup | tee cms-stats-run.out
    grep -m1 "^c conflicts" cms-stats-run.out
    # the labels are uses in the trimmed UNSAT proof: a SAT run has none
    if ! grep -q "^s UNSATISFIABLE" cms-stats-run.out; then
        echo "ERROR: not UNSAT, crystalball only learns from UNSAT instances"
        exit 255
    fi

    stage "check_rawdb"
    "$SCRIPTDIR/check_rawdb.py" data.db-raw --halflife "$HALFLIFE" \
        --dumpratio "$DUMPRATIO" --proof data.xlrup | tee check_rawdb.out-stage

    # optionally check the proof
    if [[ -x "$CAKE_XLRUP" ]]; then
        stage "checking proof"
        "$CAKE_XLRUP" "$FNAME" data.xlrup | tee cake.out
        grep -q "^s VERIFIED UNSAT" cake.out
    fi
else
    cd "$DIR"
fi

########################
# 2-4. Fill in used_clauses, clean, check, sample, learn
########################
if [[ $SKIP_LEARN -eq 0 ]]; then
    rm -f data.db data-min.db data-min.db-cldata-* data-min.db-evaldata-* predictor-*.json *.out-stage
    if [[ ! -f data.xlrup ]]; then
        echo "ERROR: data.xlrup is gone (deleted once used unless KEEP_PROOF=1), re-run without --skip-solve"
        exit 255
    fi

    stage "fix_up_xlrup: which clause was used when"
    cp data.db-raw data.db
    "$SCRIPTDIR/fix_up_xlrup.py" data.xlrup data.db | tee fix_up_xlrup.out-stage
    # the proof is GBs on big instances and not needed any more
    [[ "$KEEP_PROOF" == "1" ]] || rm -f data.xlrup

    stage "clean_update_data"
    "$SCRIPTDIR/clean_update_data.py" data.db | tee clean_update_data.out-stage

    stage "use_stats"
    "$SCRIPTDIR/use_stats.py" data.db --halflife "$HALFLIFE" | tee use_stats.out-stage

    stage "check_data_quality"
    "$SCRIPTDIR/check_data_quality.py" --slow data.db | tee check_data_quality.out-stage

    stage "sample_data"
    cp data.db data-min.db
    "$SCRIPTDIR/sample_data.py" \
        --halflife "$HALFLIFE" \
        --cut1 "$cut1" --cut2 "$cut2" --limit "$FIXED" \
        --evalreduces "$EVAL_REDUCES" --evalperreduce "$EVAL_PER_REDUCE" \
        data-min.db | tee sample_data.out-stage
    # the full labelled DB is only needed for the sampling
    [[ "$KEEP_PROOF" == "1" ]] || rm -f data.db

    stage "cldata_gen_pandas"
    "$SCRIPTDIR/cldata_gen_pandas.py" data-min.db \
        --halflife "$HALFLIFE" \
        --cut1 "$cut1" --cut2 "$cut2" --limit "$FIXED" ${EXTRA_GEN_PANDAS_OPTS} \
        | tee cldata_gen_pandas.out-stage
    if ! ls data-min.db-cldata-*.dat > /dev/null 2>&1; then
        echo "ERROR: no frames at all: no tracked clause was ever used within a horizon"
        exit 255
    fi
    stage "check_frames"
    "$SCRIPTDIR/check_frames.py" -f "$bestf" data-min.db-cldata-*.dat data-min.db-evaldata-*.dat | tee check_frames.out-stage | grep -E "FAIL|failed"
    if [[ $GATHER_ONLY -eq 1 ]]; then
        echo "Done, --gather-only: frames are in $DIR/data-min.db-cldata-*.dat"
        exit 0
    fi

    stage "cldata_predict"
    for table in used_later used_later_anc; do
        f="data-min.db-cldata-${table}-disc-cut1-${cut1}-cut2-${cut2}-limit-${FIXED}.dat"
        if [[ ! -f "$f" ]]; then echo "no frame for $table (run shorter than two half-lives), no model"; continue; fi
        $NOBUF "$SCRIPTDIR/cldata_predict.py" "$f" \
            --table "$table" --features best_only --regressor xgb \
            --xgboostestimators "$XGB_EST" --xboostmaxdepth "$XGB_DEPTH" \
            --xgboostminchild "$XGB_MINCHILD" --objective "$XGB_OBJ" --target "$TARGET" --seed "$XGB_SEED" --xgboostsubsample "$XGB_SUBSAMPLE" \
            --weights "$XGB_WEIGHTS" --evalframe "data-min.db-evaldata-${table}-disc.dat" \
            --basedir . --bestfeatfile "$bestf" \
            > "cldata_predict_${table}.out-stage" 2>&1
        grep -E "^use kept at reduce|==> Saved" "cldata_predict_${table}.out-stage" || true
    done
    ls -la predictor-*.json
fi

########################
# 5. Run the predictor build with the learnt models
########################
stage "running FINAL_PREDICTOR build"
function summary() {
    printf "%-16s" "$1:"
    grep -m1 "^s " "$2" | tr -d '\n'
    printf "  conflicts %s  time %s s\n" \
        "$(grep -m1 '^c conflicts' "$2" | awk '{print $4}')" \
        "$(grep -m1 'Total time (this thread)' "$2" | awk '{print $7}')"
}
for ANC in 0 1; do
    $NOBUF "$PRED_BIN" --predloc . \
        --predanc $ANC --zero-exit-status "$FNAME" \
        > "cms-pred-run.out-anc${ANC}" 2>&1 || true
done
if [[ -x "$NORMAL_BIN" ]]; then
    $NOBUF "$NORMAL_BIN" --zero-exit-status "$FNAME" > cms-normal-run.out 2>&1 || true
fi
echo
echo "predictor build vs the others (same reduce, only the candidate order differs):"
summary "pred plain" cms-pred-run.out-anc0
summary "pred ancestor" cms-pred-run.out-anc1
[[ -f cms-normal-run.out ]] && summary "normal" cms-normal-run.out
summary "stats" cms-stats-run.out
echo "Done. Predictors are in $DIR/predictor-*.json"
