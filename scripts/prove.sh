#!/bin/bash
set -x

file=$1
./clean_cnf.py "$file" "clean.cnf"
file="clean.cnf"

rm -f proof proof.xfrat proof-comments
./cryptominisat5 "$file" proof-comments | tee cms.out | grep "^s"
./final_check.py proof-comments
grep -v "^c" proof-comments > proof
./frat-rs elab proof "$file" proof.xfrat
echo "frat-rs fin."
./cake_xlrup "$file" proof.xfrat
echo "cake fin."
