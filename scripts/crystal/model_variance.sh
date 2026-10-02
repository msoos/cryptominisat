#!/bin/bash

# The noise floor of the A/B: the same feature list and the same data,
# N models that differ only in the seed (the train/test split and, with
# XGB_SUBSAMPLE < 1, the rows each tree sees), each run on the test CNFs.
# A feature or option whose effect is inside this spread was not measured.
#
# usage: model_variance.sh <outdir> <train-dirs.txt> <test-cnfs.txt> [N=3]
#   knobs of setparams_ballofcrystal.sh and eval_corpus.sh apply;
#   PRED_BIN must have been built with the list in bestf

set -e
OUT="$(realpath -m "$1")"; TRAIN="$(realpath "$2")"; TEST="$(realpath "$3")"; N="${4:-3}"
if [[ -z "$OUT" || -z "$TRAIN" || -z "$TEST" ]]; then
    echo "usage: $0 <outdir> <train-dirs.txt> <test-cnfs.txt> [N]"; exit 255
fi
cd "$(dirname "$0")"
SCRIPTDIR="$(pwd)"
mkdir -p "$OUT"
mapfile -t TRAIN_DIRS < "$TRAIN"
mapfile -t TEST_CNFS < "$TEST"
for seed in $(seq 1 "$N"); do
    echo "=== seed $seed $(date +%T)"
    XGB_SEED=$seed XGB_SUBSAMPLE="${XGB_SUBSAMPLE:-0.8}" "$SCRIPTDIR/learn.sh" "$OUT/models-seed$seed" "${TRAIN_DIRS[@]}" > "$OUT/learn-seed$seed.out" 2>&1
    "$SCRIPTDIR/eval_corpus.sh" "$OUT/models-seed$seed" "${TEST_CNFS[@]}" > "$OUT/eval-seed$seed.out" 2>&1
    echo "seed $seed: $(grep '^total' "$OUT/eval-seed$seed.out" | sed 's/total: normal confl [0-9]* *[0-9]* s *| *//')"
done
echo "--- per instance, conflicts of each seed's model:"
paste <(grep -v "^instance\|^total" "$OUT/eval-seed1.out" | awk '{print substr($1,1,30)}') \
      <(for seed in $(seq 1 "$N"); do grep -v "^instance\|^total" "$OUT/eval-seed$seed.out" | awk '{print $9}'; done | paste $(for i in $(seq 1 "$N"); do echo -n "- "; done)) | column -t
