#!/bin/sh

echo "checking for assert/signal/error/terminate fail"
xzgrep --color -i -e "assert.*fail" -e "signal" -e "error" -e "terminate" `ls *.out.xz` > issues
echo "checking for signal 4"
xzgrep "signal 4"  issues

# 1500 cutoff
echo "Getting solveTimes"
xzgrep "Total" *.out.xz | awk '{print $7 " "$1}' | sed 's/:c.*$//' > solveTimes
echo "Getting problems solved under 1500"
awk '{if ($1 < 1500) {print $2}}' solveTimes | sort > solved_under_1500_full_list
awk '{print $2}' solveTimes | sort > solved
sed 's/.gz.*/.gz/' solveTimes | sort -n > solveTimes2
rm solveTimes
mv solveTimes2 solveTimes
ls -- *.out.xz | sed "s/.gz.*/.gz/" > allFiles

# for normal
echo "Getting UNSAT"
xzgrep "^s UNSATISFIABLE" $(cat solved) | sed 's/:s.*$//' | sed 's/.gz.*/.gz/' | sort > solvedUNSAT
echo "Getting SAT"
xzgrep "^s SATISFIABLE" $(cat solved)   | sed 's/:s.*$//' | sed 's/.gz.*/.gz/' | sort > solvedSAT


# adjusting solved, solveTimes, solveTimes_rev
sed -i 's/.gz.*/.gz/' solved
grep -v -f solved allFiles | sed "s/.gz.*/.gz/" > unsolved
cat unsolved | awk '{print "5000.00 " $1}' >> solveTimes
awk '{print $2 " " $1}' solveTimes | sort > solveTimes_rev

# 1500 cutoff
echo "Getting 1500 UNSAT"
xzgrep "^s UNSATISFIABLE" $(cat solved_under_1500_full_list) | sed 's/:s.*$//' | sed 's/.gz.*/.gz/' | sort > solvedUNSAT1500
echo "Getting 1500 SAT"
xzgrep "^s SATISFIABLE" $(cat solved_under_1500_full_list)   | sed 's/:s.*$//' | sed 's/.gz.*/.gz/' | sort > solvedSAT1500
rm -f solved1500
cat solvedUNSAT1500 > solved1500
cat solvedSAT1500 >> solved1500

cat solvedSAT | awk '{print $1 " SAT"}' > solved_sol
cat solvedUNSAT | awk '{print $1 " UNSAT"}' >> solved_sol
cat unsolved | awk '{print $1 " UNKN"}' >> solved_sol
sort solved_sol > solved_sol2
rm solved_sol
mv solved_sol2 solved_sol
