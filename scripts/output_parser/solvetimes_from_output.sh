#!/bin/sh

echo "checking for assert/signal/error/terminate fail"
xzgrep --color -i -e "assert.*fail" -e "signal" -e "error" -e "terminate" `ls *.out.xz` > issues
echo "checking for signal 4"
xzgrep "signal 4"  issues

ls -- *.out.xz | sed 's/xz.*/xz/' > allFiles

# 1500 cutoff
echo "Getting solveTimes"
xzgrep "Total" *.out.xz | awk '{print $5}' > solveTimes
echo "Getting problems solved under 1500"
xzgrep "Total" *.out.xz | awk '{if ($7 < 1500) {print $1}}' | sed 's/:c.*$//' | sort > solved_under_1500_full_list


# for normal
echo "Getting UNSAT"
xzgrep "^s UNSATISFIABLE" *.out.xz | sed 's/:s.*$//' | sed 's/xz.*/xz/' | sort > solvedUNSAT
echo "Getting SAT"
xzgrep "^s SATISFIABLE" *.out.xz   | sed 's/:s.*$//' | sed 's/xz.*/xz/' | sort > solvedSAT
rm -f solved
cat solvedUNSAT > solved
cat solvedSAT >> solved

# 1500 cutoff
echo "Getting 1500 UNSAT"
xzgrep "^s UNSATISFIABLE" $(cat solved_under_1500_full_list) | sed 's/:s.*$//' | sed 's/xz.*/xz/' | sort > solvedUNSAT1500
echo "Getting 1500 SAT"
xzgrep "^s SATISFIABLE" $(cat solved_under_1500_full_list)   | sed 's/:s.*$//' | sed 's/xz.*/xz/' | sort > solvedSAT1500
rm -f solved1500
cat solvedUNSAT1500 > solved1500
cat solvedSAT1500 >> solved1500

# rm solved_under_1500_full_list
