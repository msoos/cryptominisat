#!/bin/sh

zgrep --color -i "assert.*fail" -- *stdout* *stderr*
zgrep --color -i signal -- *stderr* *stderr*
zgrep --color -i error -- *stderr* *stderr*
zgrep --color -i abort -- *stderr* *stderr*
zgrep --color -i failed -- *stderr* *stderr*

ls -- *stdout* | sed 's/gz.*/gz/' > allFiles

# 1500 cutoff
zgrep "Total" -- *stdout.gz | awk '{print $5}' > solveTimes
zgrep "Total" -- *stdout.gz | awk '{if ($5 < 1500) {print $1}}' | sed 's/:c.*$//' | sort > solved_under_1500_full_list


# for normal
zgrep "s.*SATISFIABLE" -- *stdout.gz | sed 's/:s.*$//' | sed 's/gz.*/gz/' | sort > solved
zgrep "s UNSATISFIABLE" -- *stdout.gz | sed 's/:s.*$//' | sed 's/gz.*/gz/' | sort > solvedUNSAT
zgrep "s SATISFIABLE" -- *stdout.gz   | sed 's/:s.*$//' | sed 's/gz.*/gz/' | sort > solvedSAT

# 1500 cutoff
zgrep "s.*SATISFIABLE" -- `cat solved_under_1500_full_list` | sed 's/:s.*$//' | sed 's/gz.*/gz/' | sort > solved1500
zgrep "s UNSATISFIABLE" -- `cat solved_under_1500_full_list` | sed 's/:s.*$//' | sed 's/gz.*/gz/' | sort > solvedUNSAT1500
zgrep "s SATISFIABLE" -- `cat solved_under_1500_full_list`   | sed 's/:s.*$//' | sed 's/gz.*/gz/' | sort > solvedSAT1500
rm solved_under_1500_full_list
