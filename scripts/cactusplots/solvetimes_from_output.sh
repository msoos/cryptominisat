#!/bin/sh

zgrep -i assert *stdout* *stderr*
zgrep -i signal *stderr* *stderr*
zgrep -i error *stderr* *stderr*
zgrep -i abort *stderr* *stderr*
zgrep -i failed *stderr* *stderr*


zgrep "Total" *stdout.gz | awk '{print $5}' > solveTimes
zgrep "s.*SATISFIABLE" *stdout.gz | sed 's/:s.*$//' | sed 's/gz.*/gz/' | sort > solved
ls *stdout* | sed 's/gz.*/gz/' > allFiles
zgrep "s UNSATISFIABLE" *stdout.gz | sed 's/:s.*$//' | sed 's/gz.*/gz/' | sort > solvedUNSAT
zgrep "s SATISFIABLE" *stdout.gz   | sed 's/:s.*$//' | sed 's/gz.*/gz/' | sort > solvedSAT
#grep "v-elim" * | awk 'BEGIN {litrem=0;clauserem=0;velim=0;time=0;} {litrem+=$4; clauserem+=$6; varelim+=$8; time+=$12} END {print "time:             " time; print "lits removed:     " litrem; print "clauses subsumed: " clauserem; print "vars elimed:      " varelim}' > varelimed
