#!/bin/bash

# Smoke test of the whole crystalball pipeline on a small random UNSAT
# instance (about 100k conflicts), scaled label horizons. Needs cnfgen
# (pip install cnfgen) and the stats + predictor builds.
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
SHORT=2000 LONG=6000 FOREVER=20000 HALFLIFE=5000 FIXED=3000 CAKE_XLRUP="" KEEP_FRAT=1 \
    ./ballofcrystal.sh "$CNF" > "$DIR/run.out" 2>&1 || {
    echo "FAILED, see $DIR/run.out"; tail -20 "$DIR/run.out"; exit 1; }
tail -6 "$DIR/run.out"

# the plain and ancestor predictors must differ (they had a bug where they did not)
if cmp -s "$CNF-dir/predictor-used_later-short-xgb.json" "$CNF-dir/predictor-used_later_anc-short-xgb.json"; then
    echo "FAILED: used_later and used_later_anc predictors are identical"; exit 1
fi
# a second learning pass must give the same predictors
SHORT=2000 LONG=6000 FOREVER=20000 HALFLIFE=5000 FIXED=3000 CAKE_XLRUP="" \
    KEEP_FRAT=1 ./ballofcrystal.sh --skip-solve "$CNF" > "$DIR/run2.out" 2>&1 || {
    echo "FAILED, see $DIR/run2.out"; tail -3 "$DIR/run2.out"; exit 1; }
for f in "$CNF-dir"/predictor-*.json; do
    cp "$f" "$f.first"
done
SHORT=2000 LONG=6000 FOREVER=20000 HALFLIFE=5000 FIXED=3000 CAKE_XLRUP="" \
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
# exactly what pandas computes from the same expressions: the Python
# predictor path and the xgboost C path must give the same run
SCRIPTDIR="$(pwd)"
PRED="${PRED_BIN:-$SCRIPTDIR/../../build_pred/cryptominisat5}"
run_pred() { # type
    (cd "$CNF-dir" && "$PRED" --predtype "$1" --predloc . --predbestfeats "$SCRIPTDIR/best_features.txt" \
        --zero-exit-status "$CNF" 2>&1 | grep -m1 "^c conflicts" | awk '{print $4}')
}
confl_xgb=$(run_pred xgb)
confl_py=$(run_pred py)
if [[ -z "$confl_py" ]]; then
    echo "note: the Python predictor is not built in, feature consistency not checked"
elif [[ "$confl_xgb" != "$confl_py" ]]; then
    echo "FAILED: the C++ features and the Python features give different runs: xgb $confl_xgb py $confl_py"; exit 1
else
    echo "OK: C++ and Python feature computation agree ($confl_xgb conflicts)"
fi
echo "OK: pipeline ran, predictors differ per table and are reproducible. Output in $DIR"
