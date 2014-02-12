#!/bin/sh

grep "\%.*all" *.out | awk '{print $2}' > solveTimes
grep "\% *all" *.out | awk '{print $1}' | sed 's/:c$//' > solved
ls *.gz.out > allFiles
grep "s UNSATISFIABLE" *out | sed 's/:s.*$//' > solvedUNSAT
grep "s SATISFIABLE" *out | sed 's/:s.*$//' > solvedSAT
