#!/bin/bash

# Smoke test of the whole crystalball pipeline on a small random UNSAT
# instance (about 100k conflicts), scaled label horizons. Needs cnfgen
# (pip install cnfgen) and the stats + predictor builds; checks the
# stats+predictor build too if it is there.
#
# usage: test_small.sh [seed]     (seeds 2, 3, 5, 6 of this generator are UNSAT)

set -e
SEED="${1:-6}"
cd "$(dirname "$0")"
DIR="$(mktemp -d /tmp/crystal-test-XXXXXX)"
CNF="$DIR/rand3-230-$SEED.cnf"
cnfgen --seed "$SEED" randkcnf 3 230 1000 > "$CNF"
echo "instance: $CNF"

# KEEP_FRAT: the --skip-solve passes below need the proof
DUMPRATIO=0.1 FIXED=3000 CAKE_XLRUP="" KEEP_FRAT=1 \
    ./ballofcrystal.sh "$CNF" > "$DIR/run.out" 2>&1 || {
    echo "FAILED, see $DIR/run.out"; tail -20 "$DIR/run.out"; exit 1; }
tail -6 "$DIR/run.out"

# the plain and ancestor predictors must differ (they had a bug where they did not)
if cmp -s "$CNF-dir/predictor-used_later-disc-xgb.json" "$CNF-dir/predictor-used_later_anc-disc-xgb.json"; then
    echo "FAILED: used_later and used_later_anc predictors are identical"; exit 1
fi
# a second learning pass must give the same predictors
DUMPRATIO=0.1 FIXED=3000 CAKE_XLRUP="" \
    KEEP_FRAT=1 ./ballofcrystal.sh --skip-solve "$CNF" > "$DIR/run2.out" 2>&1 || {
    echo "FAILED, see $DIR/run2.out"; tail -3 "$DIR/run2.out"; exit 1; }
# the C++ proof pass against the Python one, hand-made and this proof
./test_frat_uses.py --real "$CNF-dir/data.frat" "$CNF-dir/data.db-raw" || exit 1
for f in "$CNF-dir"/predictor-*.json; do
    cp "$f" "$f.first"
done
DUMPRATIO=0.1 FIXED=3000 CAKE_XLRUP="" \
    ./ballofcrystal.sh --skip-solve "$CNF" > "$DIR/run3.out" 2>&1 || {
    echo "FAILED, see $DIR/run3.out"; tail -3 "$DIR/run3.out"; exit 1; }
# (the model's train_date attribute may differ across midnight)
for f in "$CNF-dir"/predictor-*.json; do
    python3 -c "
import json, sys
a, b = [json.load(open(x)) for x in sys.argv[1:]]
for m in (a, b): m['learner']['attributes'].pop('train_date', None)
sys.exit(a != b)" "$f" "$f.first" || { echo "FAILED: $f differs between two runs"; exit 1; }
done
# the C++ feature code (generated from best_features.txt) must compute
# what pandas computes from the same expressions
SCRIPTDIR="$(pwd)"
PRED="${PRED_BIN:-$SCRIPTDIR/../../build_pred/cryptominisat5}"
MODEL="$CNF-dir/predictor-used_later-disc-xgb.json"
(cd "$CNF-dir" && "$PRED" --predloc . --preddump "$DIR/pred.dump" \
    --maxconfl 30000 --zero-exit-status "$CNF" > "$DIR/pred-dump.out" 2>&1) || {
    echo "FAILED: the predictor run, see $DIR/pred-dump.out"; exit 1; }
./check_pred_features.py "$DIR/pred.dump" "$MODEL" || exit 1
rm -f "$DIR/pred.dump"
# with the normal build's order as its score the predictor build must BE
# the normal build: the same conflicts (its default is one round less)
NORMAL="${NORMAL_BIN:-$SCRIPTDIR/../../build/cryptominisat5}"
function confl() { "$@" --zero-exit-status "$CNF" | grep -m1 "^c conflicts" | awk '{print $4}'; }
want=$(confl "$NORMAL")
got=$(confl "$PRED" --predmimic 1 --reducerounds 2)
[[ -n "$want" && "$got" == "$want" ]] || {
    echo "FAILED: --predmimic 1 gives $got conflicts, the normal build $want"; exit 1; }
echo "OK: the predictor build with the glue order as score equals the normal build ($want conflicts)"
# the same for the build that gathers under a model: with the glue order
# as its score it must be the stats build, verdicts on the tracked included
STATSPRED="${STATSPRED_BIN:-$SCRIPTDIR/../../build_stats_pred/cryptominisat5}"
if [[ -x "$STATSPRED" ]]; then
    source ./setparams_ballofcrystal.sh > /dev/null
    (cd "$DIR" && "$STATSPRED" --xor 0 --presimp 1 --sqlitedboverwrite 1 \
        --cldatadumpratio 0.1 --cllockdatagen "$CLLOCK" --everypred "$EVERYPRED" --clid --sql 2 \
        --sqlitedb round2.db --xlrup 0 --predmimic 1 --reducerounds 2 --zero-exit-status "$CNF" round2.frat > round2.out 2>&1)
    rm -f "$DIR/round2.frat"
    function gone() { python3 -c "
import sqlite3, sys
print(sqlite3.connect(sys.argv[1]).execute('select count(), sum(gone) from reduceDB').fetchone())" "$1"; }
    want="$(grep -m1 "^c conflicts" "$CNF-dir/cms-stats-run.out" | awk '{print $4}') $(gone "$CNF-dir/data.db-raw")"
    got="$(grep -m1 "^c conflicts" "$DIR/round2.out" | awk '{print $4}') $(gone "$DIR/round2.db")"
    [[ "$got" == "$want" ]] || {
        echo "FAILED: the stats+predictor build with --predmimic 1: conflicts (rows, gone) $got, the stats build $want"; exit 1; }
    echo "OK: the stats+predictor build with the glue order as score equals the stats build: $want"
else
    echo "skipped: no stats+predictor build ($STATSPRED)"
fi
echo "OK: pipeline ran, predictors differ per table and are reproducible. Output in $DIR"
