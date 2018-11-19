#!/bin/sh

zgrep --color -i "assert.*fail" `ls *.out.gz`
zgrep --color -i signal `ls *.out.gz`
zgrep --color -i error `ls *.out.gz`
zgrep --color -i terminate `ls *.out.gz`
zgrep "signal 4" `ls *.timeout.gz`

ls -- *.out.gz | sed 's/gz.*/gz/' > allFiles

# 1500 cutoff
zgrep "Total" *.out.gz | awk '{print $5}' > solveTimes
zgrep "Total" *.out.gz | awk '{if ($5 < 1500) {print $1}}' | sed 's/:c.*$//' | sort > solved_under_1500_full_list


# for normal
zgrep "s.*SATISFIABLE" *.out.gz | sed 's/:s.*$//' | sed 's/gz.*/gz/' | sort > solved
zgrep "s UNSATISFIABLE" *.out.gz | sed 's/:s.*$//' | sed 's/gz.*/gz/' | sort > solvedUNSAT
zgrep "s SATISFIABLE" *.out.gz   | sed 's/:s.*$//' | sed 's/gz.*/gz/' | sort > solvedSAT

# 1500 cutoff
zgrep "s.*SATISFIABLE" $(cat solved_under_1500_full_list) | sed 's/:s.*$//' | sed 's/gz.*/gz/' | sort > solved1500
zgrep "s UNSATISFIABLE" $(cat solved_under_1500_full_list) | sed 's/:s.*$//' | sed 's/gz.*/gz/' | sort > solvedUNSAT1500
zgrep "s SATISFIABLE" $(cat solved_under_1500_full_list)   | sed 's/:s.*$//' | sed 's/gz.*/gz/' | sort > solvedSAT1500
rm solved_under_1500_full_list
