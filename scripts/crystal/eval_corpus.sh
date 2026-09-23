#!/bin/bash

# Runs the normal build and the predictor build (plain and ancestor
# tables) on each CNF and prints conflicts/time side by side.
#
# usage: eval_corpus.sh <preddir> file1.cnf [file2.cnf ...]
#   PRED_OPTS: extra options for the predictor runs, e.g. "--predsortby 0"

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
    # options must come before the CNF: anything after it is a proof file name
    "${@:1:$#-1}" --zero-exit-status "$cnf" > "$out" 2>&1 || true
    printf "%-9s confl %9s  %7s s   " "$name" \
        "$(grep -m1 '^c conflicts' "$out" | awk '{print $4}')" \
        "$(grep -m1 'Total time (this thread)' "$out" | awk '{print $7}')"
}

printf "%-28s %s\n" "instance" "normal | pred plain tables | pred ancestor tables"
tot_n=0; tot_p=0; tot_a=0
for f in "$@"; do
    printf "%-28s " "$(basename "$f")"
    run normal "$NORMAL_BIN" "$f"
    run pred000 "$PRED_BIN" --predtype xgb --predloc "$PRED" --predtables 000 $PRED_OPTS "$f"
    run pred111 "$PRED_BIN" --predtype xgb --predloc "$PRED" --predtables 111 $PRED_OPTS "$f"
    echo
    n=$(grep -m1 '^c conflicts' "$PRED/eval/$(basename "$f").normal" | awk '{print $4}')
    p=$(grep -m1 '^c conflicts' "$PRED/eval/$(basename "$f").pred000" | awk '{print $4}')
    a=$(grep -m1 '^c conflicts' "$PRED/eval/$(basename "$f").pred111" | awk '{print $4}')
    tot_n=$((tot_n + n)); tot_p=$((tot_p + p)); tot_a=$((tot_a + a))
done
echo "total conflicts: normal $tot_n  pred000 $tot_p ($((100*tot_p/tot_n))%)  pred111 $tot_a ($((100*tot_a/tot_n))%)"
