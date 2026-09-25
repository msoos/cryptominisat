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
SHORT=2000 LONG=6000 FOREVER=20000 FIXED=3000 CAKE_XLRUP="" KEEP_FRAT=1 \
    ./ballofcrystal.sh "$CNF" > "$DIR/run.out" 2>&1 || {
    echo "FAILED, see $DIR/run.out"; tail -20 "$DIR/run.out"; exit 1; }
tail -6 "$DIR/run.out"

# the plain and ancestor predictors must differ (they had a bug where they did not)
if cmp -s "$CNF-dir/predictor-used_later-short-xgb.json" "$CNF-dir/predictor-used_later_anc-short-xgb.json"; then
    echo "FAILED: used_later and used_later_anc predictors are identical"; exit 1
fi
# a second learning pass must give the same predictors
SHORT=2000 LONG=6000 FOREVER=20000 FIXED=3000 CAKE_XLRUP="" \
    KEEP_FRAT=1 ./ballofcrystal.sh --skip-solve "$CNF" > "$DIR/run2.out" 2>&1 || {
    echo "FAILED, see $DIR/run2.out"; tail -3 "$DIR/run2.out"; exit 1; }
for f in "$CNF-dir"/predictor-*.json; do
    cp "$f" "$f.first"
done
SHORT=2000 LONG=6000 FOREVER=20000 FIXED=3000 CAKE_XLRUP="" \
    ./ballofcrystal.sh --skip-solve "$CNF" > "$DIR/run3.out" 2>&1 || {
    echo "FAILED, see $DIR/run3.out"; tail -3 "$DIR/run3.out"; exit 1; }
for f in "$CNF-dir"/predictor-*.json; do
    cmp -s "$f" "$f.first" || { echo "FAILED: $f differs between two runs"; exit 1; }
done
echo "OK: pipeline ran, predictors differ per table and are reproducible. Output in $DIR"
