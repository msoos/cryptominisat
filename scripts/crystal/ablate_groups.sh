#!/bin/bash

# Group ablation in the solver: for the full feature list and for the
# list without each group (feature_groups.py), build a predictor binary
# with that list, learn the models on the training dirs, A/B on the test
# CNFs. The offline ranking quality has misled the feature choice twice;
# this is the number that counts.
#
# usage: ablate_groups.sh <outdir> <features.txt> <train-dirs.txt> <test-cnfs.txt>
#   train-dirs.txt / test-cnfs.txt: one path per line
#   ABLATE_GROUPS="recency snapshot ..." limits the groups (default: all;
#     not GROUPS, which is bash's own list of the user's group IDs)
#   knobs of setparams_ballofcrystal.sh (TIERS, TARGET, FIXED ...) and of
#   eval_corpus.sh (EVAL_OPTS, EVAL_NORMAL_CACHE ...) apply
# Needs build_pred/ configured: the xgboost paths are taken from its cache.
# Each list costs a partial rebuild (2-3 min), the learning (minutes) and
# the test runs.

set -e
set -o pipefail
OUT="$(realpath -m "$1")"; FEATS="$(realpath "$2")"; TRAIN="$(realpath "$3")"; TEST="$(realpath "$4")"
if [[ -z "$OUT" || -z "$FEATS" || -z "$TRAIN" || -z "$TEST" ]]; then
    echo "usage: $0 <outdir> <features.txt> <train-dirs.txt> <test-cnfs.txt>"; exit 255
fi
cd "$(dirname "$0")"
SCRIPTDIR="$(pwd)"
TOP="$(realpath ../..)"
mkdir -p "$OUT"
CACHE="$TOP/build_pred/CMakeCache.txt"
XGB_INC=$(grep "^XGBOOST_INCLUDE_DIR" "$CACHE" | cut -d= -f2)
XGB_LIB=$(grep "^XGBOOST_LIBRARY" "$CACHE" | cut -d= -f2)
CADICAL=$(grep "^cadical_DIR" "$CACHE" | cut -d= -f2)
CADIBACK=$(grep "^cadiback_DIR" "$CACHE" | cut -d= -f2)
BUILD="$TOP/build_pred_ablate"
mkdir -p "$BUILD"
mapfile -t TRAIN_DIRS < "$TRAIN"
mapfile -t TEST_CNFS < "$TEST"

function one() { # name, feature file
    local name="$1" feats="$2"
    echo "=== $name: $(grep -vc '^#' "$feats") features $(date +%T)"
    (cd "$BUILD" && cmake -DFINAL_PREDICTOR=ON -DENABLE_TESTING=ON -DPRED_FEATURES_FILE="$feats" \
        -DXGBOOST_INCLUDE_DIR="$XGB_INC" -DXGBOOST_LIBRARY="$XGB_LIB" \
        -Dcadical_DIR="$CADICAL" -Dcadiback_DIR="$CADIBACK" "$TOP" > "$OUT/cmake-$name.out" 2>&1 \
        && make -j4 > "$OUT/make-$name.out" 2>&1) || { echo "build FAILED, see $OUT/make-$name.out"; return 1; }
    bestf="$feats" "$SCRIPTDIR/learn.sh" "$OUT/models-$name" "${TRAIN_DIRS[@]}" > "$OUT/learn-$name.out" 2>&1 \
        || { echo "learn FAILED, see $OUT/learn-$name.out"; return 1; }
    PRED_BIN="$BUILD/cryptominisat5" "$SCRIPTDIR/eval_corpus.sh" "$OUT/models-$name" "${TEST_CNFS[@]}" > "$OUT/eval-$name.out" 2>&1
    echo "$name: $(grep '^total' "$OUT/eval-$name.out")"
}

one full "$FEATS"
for g in ${ABLATE_GROUPS:-$("$SCRIPTDIR/feature_groups.py" "$FEATS" | awk '$2+0 > 0 {print $1}')}; do
    "$SCRIPTDIR/feature_groups.py" "$FEATS" --without "$g" -o "$OUT/without-$g.txt"
    one "without-$g" "$OUT/without-$g.txt"
done
echo "--- summary (conflicts / time of the predictor build vs the normal build):"
grep -h "^total" "$OUT"/eval-*.out | sed 's/total: normal confl [0-9]* *[0-9]* s *| *//' | paste -d' ' <(ls "$OUT"/eval-*.out | xargs -n1 basename | sed 's/eval-//; s/.out//') - | column -t
