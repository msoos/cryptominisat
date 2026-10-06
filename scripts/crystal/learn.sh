#!/bin/bash

# Copyright (C) 2020  Mate Soos
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

# Learn predictors from MANY instances: concatenates the per-instance
# pandas frames (data-min.db-cldata-*.dat, made by ballofcrystal.sh) and
# trains the predictors on the union. The dirs may come from several
# gathering rounds (see STATS_OPTS in ballofcrystal.sh).
#
# usage: learn.sh <outdir> <cnf-dir> [<cnf-dir> ...]
#   <cnf-dir> are the <file.cnf>-dir directories ballofcrystal.sh made
# Knobs, same as setparams_ballofcrystal.sh: cut1 cut2 FIXED bestf
#   XGB_EST XGB_DEPTH XGB_MINCHILD XGB_SEED XGB_SUBSAMPLE TARGET XGB_OBJ
#   XGB_WEIGHTS, FAMILIES (a file of 'family filename' lines)

set -e
set -o pipefail

OUT="$1"; shift
if [[ -z "$OUT" || -z "$1" ]]; then
    echo "usage: $0 <outdir> <cnf-dir> [<cnf-dir> ...]"
    exit 255
fi
cd "$(dirname "$0")"
source ./setparams_ballofcrystal.sh
SCRIPTDIR="$(pwd)"
cd - > /dev/null
mkdir -p "$OUT"
git -C "$SCRIPTDIR" rev-parse HEAD > "$OUT/out_git"
echo "$@" >> "$OUT/out_git"

# provenance: which solver gathered each dir (frames of different
# schemas are refused by concat_pandas.py, this says why)
: > "$OUT/gathered_by"
for d in "$@"; do
    sha=$(grep -m1 "^c CMS SHA1:" "$d/cms-stats-run.out" 2>/dev/null | awk '{print $4}')
    drv=$(grep -m1 "^c \[pred\] \(loaded the\|compiled-in\) model" "$d/cms-stats-run.out" 2>/dev/null | sed 's/^c \[pred\] //')
    echo "${sha:-unknown} $d ${drv:-glue}" >> "$OUT/gathered_by"
done
echo "the reduce of the gathering runs was driven by:"
awk '{$1=""; $2=""; print}' "$OUT/gathered_by" | sort | uniq -c
if [[ $(awk '{print $1}' "$OUT/gathered_by" | sort -u | wc -l) -gt 1 ]]; then
    echo "WARNING: the dirs were gathered by different solver versions, see $OUT/gathered_by"
    awk '{print $1}' "$OUT/gathered_by" | sort | uniq -c
fi

echo "=== checking the frames"
for d in "$@"; do
    "$SCRIPTDIR/check_frames.py" -f "$bestf" "$d"/data-min.db-cldata-*.dat "$d"/data-min.db-evaldata-*.dat > "$OUT/check_frames-$(basename "$d").out" 2>&1 \
        || { echo "FAILED: $d, see $OUT/check_frames-$(basename "$d").out"; grep FAIL "$OUT/check_frames-$(basename "$d").out"; exit 1; }
done

for table in used_later used_later_anc; do
    name="${table}-disc-cut1-${cut1}-cut2-${cut2}-limit-${FIXED}"
    dats=()
    evals=()
    for d in "$@"; do
        f="$d/data-min.db-cldata-${name}.dat"
        # a run shorter than two half-lives has no frame
        if [[ ! -f "$f" ]]; then echo "WARNING: $f missing, skipped"; continue; fi
        dats+=("$f")
        e="$d/data-min.db-evaldata-${table}-disc.dat"
        [[ -f "$e" ]] && evals+=("$e")
    done
    if [[ ${#dats[@]} -eq 0 ]]; then echo "ERROR: no frames for $table"; exit 255; fi
    echo "=== $table: ${#dats[@]} frames"
    "$SCRIPTDIR/concat_pandas.py" -o "$OUT/comb-${name}.dat" "${dats[@]}" | tail -1
    "$SCRIPTDIR/concat_pandas.py" -o "$OUT/comb-eval-${table}-disc.dat" "${evals[@]}" | tail -1
    $NOBUF "$SCRIPTDIR/cldata_predict.py" "$OUT/comb-${name}.dat" \
        --table "$table" --features best_only --regressor xgb \
        --xgboostestimators "$XGB_EST" --xboostmaxdepth "$XGB_DEPTH" \
        --xgboostminchild "$XGB_MINCHILD" --objective "$XGB_OBJ" --target "$TARGET" --seed "$XGB_SEED" --xgboostsubsample "$XGB_SUBSAMPLE" \
        --weights "$XGB_WEIGHTS" --evalframe "$OUT/comb-eval-${table}-disc.dat" \
        --basedir "$OUT" --bestfeatfile "$bestf" \
        --gatheredby "$(awk '{print $1}' "$OUT/gathered_by" | sort -u | tr '\n' ' ' | sed 's/ $//')" \
        > "$OUT/out-${table}" 2>&1
    grep -E "Train/test split|Mean squared error|==> Saved" "$OUT/out-${table}" | head -3
    grep -E "^use kept at reduce" "$OUT/out-${table}" || true
done
echo "Predictors in $OUT/predictor-*.json. Use: cryptominisat5 --predloc $OUT file.cnf"
