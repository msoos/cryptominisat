#!/bin/sh

grep "Total" *.out | awk '{print $5}' > solveTimes
grep "Total" *out | awk '{print $1}' | sed 's/:c$//' > solved
# grep "Total" *out | awk '{if ($5 < 1000) {print $1}}' | sed 's/:c$//' > solvedAll
ls *.gz.out > allFiles
#grep "CPU time .*:" *out | awk '{ if ($5<5000) {print $1}}' | sed 's/:c$//' > solved
grep "s UNSATISFIABLE" *out | sed 's/:s.*$//' > solvedUNSAT
grep "s SATISFIABLE" *out | sed 's/:s.*$//' > solvedSAT
#grep "v-elim" * | awk 'BEGIN {litrem=0;clauserem=0;velim=0;time=0;} {litrem+=$4; clauserem+=$6; varelim+=$8; time+=$12} END {print "time:             " time; print "lits removed:     " litrem; print "clauses subsumed: " clauserem; print "vars elimed:      " varelim}' > varelimed
