#!/bin/sh

zgrep "CPU time" -- *stdout.gz | awk '{print $5}' > solveTimes
zgrep "CPU time" -- *stdout.gz | awk '{print $1}' | sed 's/:c$//' | sed 's/gz.*/gz/' | sort  > solved
ls -- *.stdout.gz | sed 's/gz.*/gz/' | sort > allFiles
zgrep "s UNSATISFIABLE" -- *stdout.gz | sed 's/:s.*$//' | sed 's/gz.*/gz/' | sort > solvedUNSAT
zgrep "s SATISFIABLE" -- *stdout.gz   | sed 's/:s.*$//' | sed 's/gz.*/gz/' | sort > solvedSAT
