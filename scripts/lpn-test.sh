#!/bin/bash

echo "Usage: ./todo.sh OUTPUTS FUNLEN NOISE"

outputs=$1
funlen=$2
noise=$3

echo "outputs: $outputs"
echo "funlen : $funlen"
echo "noise  : $noise"


for seed in {1..20}; do
  tmpfname="tmp-$1-$2-$3"
  ./lpn-gen.py -m $outputs -n $funlen --noise $noise -s $seed > "$tmpfname-$seed"
  /usr/bin/time --verbose ../build/cryptominisat5 "$tmpfname-$seed" > "$tmpfname-$seed-out"
  ./check_lpn_solution.py "$tmpfname-$seed" "$tmpfname-$seed-out"
  out=$?
  
  if [[ "$out" -eq "0" ]]; then
      echo "OK, test: $seed"
  else
      echo "NOT OK!!"
      exit -1
  fi
done
echo "Finished"
