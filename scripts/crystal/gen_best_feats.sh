#!/usr/bin/env bash

# Feature importance runs, to pick the features for best_features-*.txt:
# trains on ALL raw features ("no_computed") and on all raw + computed
# relative features ("all_computed", many hundreds, slow) and prints the
# xgboost importance ranking (grep "impdf:" -A 60 in the outputs).
#
# usage: gen_best_feats.sh <cldata-prefix> [outdir]
#   e.g. gen_best_feats.sh mydir/data-min.db-cldata- feats-out
#   reads <prefix><table>-<tier>-cut1-*-cut2-*-limit-*.dat

set -e

PREFIX="$1"
OUT="${2:-best-feats-out}"
if [[ -z "$PREFIX" ]]; then
    echo "usage: $0 <cldata-prefix> [outdir]"
    exit 255
fi
cd "$(dirname "$0")"
SCRIPTDIR="$(pwd)"
cd - > /dev/null
mkdir -p "$OUT"
git -C "$SCRIPTDIR" rev-parse HEAD > "$OUT/out_git"

for tier in short long forever; do
    for table in used_later used_later_anc; do
        for computed in no all; do
            f=$(ls ${PREFIX}${table}-${tier}-cut1-*.dat | head -1)
            echo "Doing $f ${computed}_computed"
            "$SCRIPTDIR/cldata_predict.py" "$f" --tier "$tier" --table "$table" \
                --regressor xgb --topfeats --features "${computed}_computed" \
                > "$OUT/output_${table}_${tier}_${computed}computed" 2>&1
            grep -A 40 "impdf:" "$OUT/output_${table}_${tier}_${computed}computed" | head -42
        done
    done
done
echo "Outputs in $OUT/"
