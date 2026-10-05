#!/bin/bash

# Runs the normal build and the predictor build (plain and ancestor
# tables) on each CNF, with each solver seed, and prints the A/B table
# (eval_summary.py: geometric means with intervals, solved, PAR2, noise).
#
# usage: eval_corpus.sh <preddir> file1.cnf [file2.cnf ...]
#   PRED_OPTS: extra options for the predictor runs, e.g. "--predsortby 0"
#   EVAL_OPTS: extra options for all runs, e.g. "--xor 0"
#   EVAL_TABLES: which --predtables to run, default "000 111"
#   EVAL_SEEDS: solver seeds, every build runs with each. Default "0"
#   EVAL_TIMEOUT: seconds (--maxtime), an unsolved run counts twice it in PAR2
#   EVAL_NORMAL_CACHE: dir to keep the normal build's outputs in, reused
#     by later calls with the same EVAL_OPTS, seed and timeout
# The outputs are $PRED/eval/<cnf>.<normal|predNNN>.s<seed>; a cluster can
# write them any other way and call eval_summary.py on the dir.

set -e
PRED="$(realpath "$1")"; shift
cd "$(dirname "$0")"
source ./setparams_ballofcrystal.sh
cd - > /dev/null
mkdir -p "$PRED/eval"

EVAL_SEEDS="${EVAL_SEEDS:-0}"
EVAL_TABLES="${EVAL_TABLES:-000 111}"
TIMEOUT_OPT=""
[[ -n "$EVAL_TIMEOUT" ]] && TIMEOUT_OPT="--maxtime $EVAL_TIMEOUT"

function run() { # name, seed, binary, opts..., cnf
    local name="$1"; shift
    local seed="$1"; shift
    local cnf="${@: -1}"
    local out="$PRED/eval/$(basename "$cnf").$name.s$seed"
    local cached=""
    if [[ "$name" == "normal" && -n "$EVAL_NORMAL_CACHE" ]]; then
        mkdir -p "$EVAL_NORMAL_CACHE"
        cached="$EVAL_NORMAL_CACHE/$(basename "$cnf").normal-$(echo "$EVAL_OPTS $TIMEOUT_OPT" | md5sum | cut -c1-8).s$seed"
    fi
    if [[ -n "$cached" ]] && grep -q "^s " "$cached" 2>/dev/null; then
        cp "$cached" "$out"
    else
        # options must come before the CNF: anything after it is a proof file name
        "${@:1:$#-1}" --seed "$seed" $TIMEOUT_OPT --zero-exit-status "$cnf" > "$out" 2>&1 || true
        [[ -n "$cached" ]] && cp "$out" "$cached"
    fi
    printf "%-9s s%s confl %9s  %7s s\n" "$name" "$seed" \
        "$(grep -m1 '^c conflicts' "$out" | awk '{print $4}')" \
        "$(grep -m1 'Total time (this thread)' "$out" | awk '{print $7}')"
}

rm -f "$PRED"/eval/*.s[0-9]*
for f in "$@"; do
    echo "=== $(basename "$f")"
    for seed in $EVAL_SEEDS; do
        run normal "$seed" "$NORMAL_BIN" $EVAL_OPTS "$f"
        for t in $EVAL_TABLES; do
            run "pred$t" "$seed" "$PRED_BIN" --predtype xgb --predloc "$PRED" --predtiers "${TIERS// /,}" --predtables "$t" $EVAL_OPTS $PRED_OPTS "$f"
        done
    done
done
echo
"$(dirname "$0")/eval_summary.py" "$PRED/eval" ${EVAL_TIMEOUT:+--timeout "$EVAL_TIMEOUT"}
