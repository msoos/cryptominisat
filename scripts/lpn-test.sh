#!/bin/bash

echo "Usage: ./todo.sh OUTPUTS FUNLEN NOISE"

outputs=$1
funlen=$2
noise=$3
cont=$4
mkdir -p "lpn-out"

echo "outputs: $outputs"
echo "funlen : $funlen"
echo "noise  : $noise"

for seed in {1..20}; do
  tmpfname="tmp-$1-$2-$3"
  ./lpn-gen.py -m $outputs -n $funlen --noise $noise -s $seed > "lpn-out/$tmpfname-$seed"
  /usr/bin/time --verbose ./cryptominisat5 "lpn-out/$tmpfname-$seed" > "lpn-out/$tmpfname-$seed-out"
  ./check_lpn_solution.py "lpn-out/$tmpfname-$seed" "lpn-out/$tmpfname-$seed-out"
  out=$?

  if [[ "$out" -eq "0" ]]; then
      echo "OK, test: $seed"
  else
      echo "NOT OK!!"
      if [[ "$cont" -eq "1" ]]; then
        echo "continuing, as requested"
      else
        exit -1
      fi
  fi
done
echo "Finished"

for seed in {1..20}; do
  tmpfname="tmp-$1-$2-$3"
  ./lpn-gen.py -m $outputs -n $funlen --noise $noise -s $seed --pb > "lpn-out/$tmpfname-$seed-pb"
  sed "s/^c.*//" "lpn-out/$tmpfname-$seed-pb" > "lpn-out/$tmpfname-$seed-pb-cleaned"
  /usr/bin/time --verbose ./linpb --print-sol=1 "lpn-out/$tmpfname-$seed-pb-cleaned" > "lpn-out/$tmpfname-$seed-out"
  egrep "(^s )|(^v )" "lpn-out/$tmpfname-$seed-out" > "lpn-out/$tmpfname-$seed-out-filt"
  sed "s/x//g" -i "lpn-out/$tmpfname-$seed-out-filt"
  ./check_lpn_solution.py "lpn-out/$tmpfname-$seed-pb" "lpn-out/$tmpfname-$seed-out-filt"
  out=$?

  if [[ "$out" -eq "0" ]]; then
      echo "OK, test: $seed"
  else
      echo "NOT OK!!"
      if [[ "$cont" -eq "1" ]]; then
        echo "continuing, as requested"
      else
        exit -1
      fi
  fi
done
echo "Finished"
