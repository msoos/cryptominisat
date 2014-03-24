#!/bin/sh

grep "CPU time" *.out | awk '{print $5}' > solveTimes
grep "CPU time" *out | awk '{print $1}' | sed 's/:c$//' > solved
ls *.gz.out > allFiles
grep "s UNSATISFIABLE" *out | sed 's/:s.*$//' > solvedUNSAT
grep "s SATISFIABLE" *out | sed 's/:s.*$//' > solvedSAT
