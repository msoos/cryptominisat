#!/bin/sh

zgrep "\%.*all" *out.gz | awk '{print $2}' > solveTimes
zgrep "\% *all" *out.gz | awk '{print $1}' | sed 's/:c$//' > solved
ls *out.gz > allFiles
zgrep "s UNSATISFIABLE" *out.gz | sed 's/:s.*$//' > solvedUNSAT
zgrep "s SATISFIABLE" *out.gz | sed 's/:s.*$//' > solvedSAT
