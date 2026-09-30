#!/bin/bash

# Runs the normal build and the predictor build (plain and ancestor
# tables) on each CNF and prints conflicts/time side by side.
#
# usage: eval_corpus.sh <preddir> file1.cnf [file2.cnf ...]
#   PRED_OPTS: extra options for the predictor runs, e.g. "--predsortby 0"
#   EVAL_OPTS: extra options for all runs, e.g. "--xor 0"
#   EVAL_TABLES: which --predtables to run, default "000 111"
#   EVAL_NORMAL_CACHE: dir to keep the normal build's outputs in, reused
#     by later calls with the same EVAL_OPTS

set -e
PRED="$(realpath "$1")"; shift
cd "$(dirname "$0")"
source ./setparams_ballofcrystal.sh
cd - > /dev/null
mkdir -p "$PRED/eval"

function run() { # name, binary, opts..., cnf
    local name="$1"; shift
    local cnf="${@: -1}"
    local out="$PRED/eval/$(basename "$cnf").$name"
    local cached=""
    if [[ "$name" == "normal" && -n "$EVAL_NORMAL_CACHE" ]]; then
        mkdir -p "$EVAL_NORMAL_CACHE"
        cached="$EVAL_NORMAL_CACHE/$(basename "$cnf").normal-$(echo "$EVAL_OPTS" | md5sum | cut -c1-8)"
    fi
    if [[ -n "$cached" ]] && grep -q "^s " "$cached" 2>/dev/null; then
        cp "$cached" "$out"
    else
        # options must come before the CNF: anything after it is a proof file name
        "${@:1:$#-1}" --zero-exit-status "$cnf" > "$out" 2>&1 || true
        [[ -n "$cached" ]] && cp "$out" "$cached"
    fi
    printf "%-9s confl %9s  %7s s   " "$name" \
        "$(grep -m1 '^c conflicts' "$out" | awk '{print $4}')" \
        "$(grep -m1 'Total time (this thread)' "$out" | awk '{print $7}')"
}

EVAL_TABLES="${EVAL_TABLES:-000 111}"
printf "%-28s %s\n" "instance" "normal | predictor, per --predtables"
declare -A tot
for f in "$@"; do
    printf "%-28s " "$(basename "$f")"
    run normal "$NORMAL_BIN" $EVAL_OPTS "$f"
    for t in $EVAL_TABLES; do
        run "pred$t" "$PRED_BIN" --predtype xgb --predloc "$PRED" --predtables "$t" $EVAL_OPTS $PRED_OPTS "$f"
    done
    echo
    for t in normal $(for t in $EVAL_TABLES; do echo "pred$t"; done); do
        n=$(grep -m1 '^c conflicts' "$PRED/eval/$(basename "$f").$t" | awk '{print $4}')
        s=$(grep -m1 'Total time (this thread)' "$PRED/eval/$(basename "$f").$t" | awk '{print $7}')
        tot[$t-confl]=$(( ${tot[$t-confl]:-0} + ${n:-0} ))
        tot[$t-time]=$(echo "${tot[$t-time]:-0} + ${s:-0}" | bc -l)
    done
done
printf "total: normal confl %s  %.0f s" "${tot[normal-confl]}" "${tot[normal-time]}"
for t in $EVAL_TABLES; do
    printf "  |  pred%s confl %s (%s%%)  %.0f s (%s%%)" "$t" "${tot[pred$t-confl]}" \
        "$((100*${tot[pred$t-confl]}/${tot[normal-confl]}))" "${tot[pred$t-time]}" \
        "$(echo "100*${tot[pred$t-time]}/${tot[normal-time]}" | bc -l | cut -d. -f1)"
done
echo
