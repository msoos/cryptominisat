#!/bin/bash

echo "Usage: ./todo.sh OUTPUTS FUNLEN NOISE"

outputs=$1
funlen=$2
noise=$3

echo "outputs: $outputs"
echo "funlen : $funlen"
echo "noise  : $noise"


for seed in {1..20}; do
  ./lpn-gen.py -m $outputs -n $funlen --noise $noise -s $seed > tmp
  ../build/cryptominisat5 tmp > tmp_out
  ./check_lpn_solution.py tmp tmp_out
  out=$?
  
  if [[ "$out" -eq "0" ]]; then
      echo "OK, test: $seed"
  else
      echo "NOT OK!!"
      exit -1
  fi
done
echo "Finished"
