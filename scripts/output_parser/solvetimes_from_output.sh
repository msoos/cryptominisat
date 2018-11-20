#!/bin/sh

echo "checking for assert fail"
zgrep --color -i -e "assert.*fail" -e "signal" -e "error" -e "terminate" `ls *.out.gz` > issues
echo "checking for signal 4"
zgrep "signal 4"  issues

ls -- *.out.gz | sed 's/gz.*/gz/' > allFiles

# 1500 cutoff
echo "Getting solveTimes"
zgrep "Total" *.out.gz | awk '{print $5}' > solveTimes
echo "Getting problems solved under 1500"
zgrep "Total" *.out.gz | awk '{if ($5 < 1500) {print $1}}' | sed 's/:c.*$//' | sort > solved_under_1500_full_list


# for normal
echo "Getting UNSAT"
zgrep "^s UNSATISFIABLE" *.out.gz | sed 's/:s.*$//' | sed 's/gz.*/gz/' | sort > solvedUNSAT
echo "Getting SAT"
zgrep "^s SATISFIABLE" *.out.gz   | sed 's/:s.*$//' | sed 's/gz.*/gz/' | sort > solvedSAT
rm -f solved
cat solvedUNSAT > solved
cat solvedSAT >> solved

# 1500 cutoff
echo "Getting 1500 UNSAT"
# zgrep "^s UNSATISFIABLE" $(cat solved_under_1500_full_list) | sed 's/:s.*$//' | sed 's/gz.*/gz/' | sort > solvedUNSAT1500
echo "Getting 1500 SAT"
# zgrep "^s SATISFIABLE" $(cat solved_under_1500_full_list)   | sed 's/:s.*$//' | sed 's/gz.*/gz/' | sort > solvedSAT1500
rm -f solved1500
# cat solvedUNSAT1500 > solved1500
# cat solvedSAT1500 >> solved1500

# rm solved_under_1500_full_list
