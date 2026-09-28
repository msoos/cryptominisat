#!/bin/bash
# Stress congruence closure on miters: checks XLRUP proofs and SAT/UNSAT vs a run without it. Usage: fuzz_congruence.sh [N]
set -u
cd "$(dirname "$(realpath "$0")")"
N=${1:-300}
C=../../build/cryptominisat5
T=$(mktemp -d)
one() {
    s=$1; mut=$2
    [ "$mut" = plain ] && mut=""
    f=$T/m$s$mut.cnf
    ../../utils/cnf-utils/mitergen.py $mut $s > $f
    sched="scc-vrepl,congruence,sub-impl,occ-bve,congruence,distill-cls,must-scc-vrepl,congruence"
    a=$(timeout 60 $C --verb 1 --presimp 1 --preschedule "$sched" --schedule "$sched" --xlrup 1 $f $f.xlrup 2>&1)
    b=$(timeout 60 $C --congruence 0 $f 2>&1 | grep '^s ')
    ra=$(echo "$a" | grep '^s ')
    merged=$(echo "$a" | grep -oE 'merged: [0-9]+' | awk '{s+=$2} END {print s+0}')
    failed=$(echo "$a" | grep -oE 'failed: [0-9]+' | awk '{s+=$2} END {print s+0}')
    if [ "$ra" != "$b" ]; then echo "MISMATCH seed=$s $mut '$ra' vs '$b'"; return; fi
    if [ "$failed" != 0 ]; then echo "FAILED-DERIVATION seed=$s $mut"; fi
    if [ "$ra" = "s UNSATISFIABLE" ]; then
        chk=$(timeout 120 ./cake_xlrup $f $f.xlrup 2>&1 | tail -1)
        [ "$chk" = "s VERIFIED UNSAT" ] || { echo "PROOF-FAIL seed=$s $mut: $chk"; return; }
    fi
    echo "OK $ra merged=$merged"
    rm -f $f $f.xlrup
}
export -f one; export C T
{ seq 1 "$N" | sed 's/$/ plain/'; seq 1 "$N" | sed 's/$/ --mutate/'; } | xargs -P16 -L1 bash -c 'one $0 $1'
rm -rf "$T"
