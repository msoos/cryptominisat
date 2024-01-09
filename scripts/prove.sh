#!/bin/bash

file=$1
rm -f proof proof.xfrat
./cryptominisat5 "$file" proof | tee cms.out | grep "^s"
./frat-rs elab proof "$file" proof.xfrat
./cake_xlrup "$file" proof.xfrat
