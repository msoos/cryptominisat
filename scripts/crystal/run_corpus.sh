#!/bin/bash

# Gather data from many CNFs, learn on the union, evaluate on each.
#
# usage: run_corpus.sh <outdir> file1.cnf [file2.cnf ...]
#   gathers <file>-dir for each (skipped if its frames exist), then
#   learn.sh <outdir> <dirs>, then eval_corpus.sh <outdir> <files>
# Knobs: those of setparams_ballofcrystal.sh (DUMPRATIO, tiers, FIXED...)

set -e
set -o pipefail
OUT="$1"; shift
if [[ -z "$OUT" || -z "$1" ]]; then
    echo "usage: $0 <outdir> file1.cnf [file2.cnf ...]"; exit 255
fi
cd "$(dirname "$0")"
SCRIPTDIR="$(pwd)"
cd - > /dev/null
OUT="$(realpath -m "$OUT")"
mkdir -p "$OUT"

dirs=()
for f in "$@"; do
    f="$(realpath "$f")"
    d="$f-dir"
    if ls "$d"/data-min.db-cldata-*.dat > /dev/null 2>&1; then
        echo "=== $f: frames exist, skipping gathering"
    else
        echo "=== $f: gathering"
        CAKE_XLRUP="" "$SCRIPTDIR/ballofcrystal.sh" --gather-only "$f" > "$OUT/gather-$(basename "$f").out" 2>&1 \
            || { echo "FAILED, see $OUT/gather-$(basename "$f").out"; exit 1; }
        grep -m1 "^c conflicts" "$d/cms-stats-run.out"
    fi
    dirs+=("$d")
done

echo "=== learning on ${#dirs[@]} instances"
"$SCRIPTDIR/learn.sh" "$OUT" "${dirs[@]}" | tee "$OUT/learn.out"

echo "=== evaluating"
"$SCRIPTDIR/eval_corpus.sh" "$OUT" "$@" | tee "$OUT/eval.out"
