#!/bin/sh

rm -f memout.csv

# checking for assert/signal/error/terminate fail
xzgrep --color -i -e "assert.*fail" -e "floating" -e "signal" -e "error" -e "terminate" `ls *.out.xz` | tee issues.csv
xzgrep "signal 4"  issues.csv

# Getting SAT & UNSAT
xzgrep "^s .*SATISFIABLE" *.out.xz | sed 's/:s.*$//' > solved_xz.csv
sed 's/.gz.*/.gz/' solved_xz.csv > solved.csv
xzgrep "^s UNSATISFIABLE" $(cat solved_xz.csv) | sed 's/:s.*$//' | sed 's/.gz.*/.gz/' | sort > solvedUNSAT.csv
xzgrep "^s SATISFIABLE" $(cat solved_xz.csv)   | sed 's/:s.*$//' | sed 's/.gz.*/.gz/' | sort > solvedSAT.csv


# All files
ls -- *.out.xz > allFiles_xz.csv
ls -- *.out.xz | sed "s/.gz.*/.gz/" > allFiles.csv

# Getting solveTimes
xzgrep "Total" $(cat solved_xz.csv) | awk '{print $7 " "$1}' | sed 's/:c.*$//' > solveTimes_xz.csv
sed 's/.gz.*/.gz/' solveTimes_xz.csv | sort -n > solveTimes.csv


# adjusting/getting solved.csv, solveTimes.csv, solveTimes_rev.csv
sed "s/^/\^/" solved.csv | sed "s/$/\$/"  > solved_filtering.csv
grep -v -f solved_filtering.csv allFiles.csv | sed "s/.gz.*/.gz/" > unsolved.csv
rm solved_filtering.csv
grep -v -f solved_xz.csv allFiles_xz.csv > unsolved_xz.csv
cat unsolved.csv | awk '{print "5000.00 " $1}' >> solveTimes.csv
awk '{print $2 " " $1}' solveTimes.csv | sort > solveTimes_rev.csv


# 1500 cutoff
awk '{if ($1 < 1500) {print $2}}' solveTimes_xz.csv | sort > solved_under_1500_full_list_xz.csv
grep -f solvedUNSAT.csv solved_under_1500_full_list_xz.csv |  sed 's/:s.*$//' | sed 's/.gz.*/.gz/' | sort > solvedUNSAT1500.csv
grep -f solvedSAT.csv solved_under_1500_full_list_xz.csv   |  sed 's/:s.*$//' | sed 's/.gz.*/.gz/' | sort > solvedSAT1500.csv

# Getting "file SAT/UNSAT/UNKN" list
cat solvedSAT.csv | awk '{print $1 " SAT"}' > solved_sol.csv
cat solvedUNSAT.csv | awk '{print $1 " UNSAT"}' >> solved_sol.csv
cat unsolved.csv | awk '{print $1 " UNKN"}' >> solved_sol.csv
sort solved_sol.csv > solved_sol2.csv
rm -f solved_sol.csv
mv solved_sol2.csv solved_sol.csv

# user times
xzgrep "User time" *.timeout.xz | awk '{print $5 " " $1}' | sed "s/.timeout.xz://" > user_times.csv

#Memout or issue
grep -f unsolved.csv user_times.csv  | sort -n -r | awk '{if ($1 < 4950) {print $0}}' | sort > memout_or_issue.csv

# ReduceDB time for solved instances
xzgrep -i "ReduceDB time" $(cat solved_xz.csv) | awk '{print $1 " " $5}' | sed "s/.out.xz:c//" | sort > reducedb_times.csv
reducedb_time=$(awk '{a+=$2} END {print a}' reducedb_times.csv)
solved_total_time=$(awk '{if ($1=="5000.00") {x+=0} else {x += $1};} END {printf "%d\n", x}' solveTimes.csv)
bc <<< "scale=4; ($reducedb_time/$solved_total_time)*100.0" > reducedb_percent_time.percent

# Distill long time for solved instances
xzgrep -i "distill long time" $(cat solved_xz.csv) | awk '{print $1 " " $6}' | sed "s/.out.xz:c//" | sort > distill_long_times.csv
distill_long_time=$(awk '{a+=$2} END {print a}' distill_long_times.csv)
solved_total_time=$(awk '{if ($1=="5000.00") {x+=0} else {x += $1};} END {printf "%d\n", x}' solveTimes.csv)
bc <<< "scale=4; ($distill_long_time/$solved_total_time)*100.0" > distill_long_percent_time.percent

# Distill bin time for solved instances
xzgrep -i "distill bin time" $(cat solved_xz.csv) | awk '{print $1 " " $6}' | sed "s/.out.xz:c//" | sort > distill_bin_times.csv
distill_bin_time=$(awk '{a+=$2} END {print a}' distill_bin_times.csv)
solved_total_time=$(awk '{if ($1=="5000.00") {x+=0} else {x += $1};} END {printf "%d\n", x}' solveTimes.csv)
bc <<< "scale=4; ($distill_bin_time/$solved_total_time)*100.0" > distill_bin_percent_time.percent

# Occsimp time for solved instances
xzgrep -i "OccSimplifier time " $(cat solved_xz.csv) | awk '{print $1 " " $5}' | sed "s/.out.xz:c//" | sort > occsimp_times.csv
occsimp_time=$(awk '{a+=$2} END {print a}' occsimp_times.csv)
solved_total_time=$(awk '{if ($1=="5000.00") {x+=0} else {x += $1};} END {printf "%d\n", x}' solveTimes.csv)
bc <<< "scale=4; ($occsimp_time/$solved_total_time)*100.0" > occsimp_percent_time.percent

xzgrep signal  *.timeout.xz | sed -E "s/.timeout.*signal (.*)/ \1/" > signals.csv
xzgrep signal  *.timeout.xz | sed -E "s/.timeout.*signal (.*)//" > signals_files.csv
sed "s/^/\^/" signals_files.csv | sed "s/$/\$/"  > signals_files_filtering.csv
grep -v -f signals_files_filtering.csv allFiles.csv | awk '{print $1 " -1"}' >> signals.csv
rm -f signals_files_filtering.csv
sort signals.csv > signals_sorted.csv
rm -f signals.csv signals_files.csv
mv signals_sorted.csv signals.csv
grep " 11" signals.csv

# PAR 2 score 5000s timeout
awk '{if ($1=="5000.00") {x+=10000} else {x += $1};} END {printf "%7.0f\n", x}' solveTimes.csv > PAR2score
mypwd=`pwd`
echo "$mypwd PAR2 score is: " `cat PAR2score`
awk '{if ($1 >= 1500) {x+=3000} else {x += $1};} END {printf "%6.0f\n", x}' solveTimes.csv > PAR2score_1500
echo "$mypwd PAR2_1500 score is: " `cat PAR2score_1500`
xzgrep "avg cls in red 0" $(cat solved_xz.csv) | awk '{k+=$8;x+=1} END {printf "%3.1fK", (k/(x*1000))}' > avg_tier0_size
xzgrep "avg cls in red 1" $(cat solved_xz.csv) | awk '{k+=$8;x+=1} END {printf "%3.1fK", (k/(x*1000))}' > avg_tier1_size
xzgrep "avg cls in red 2" $(cat solved_xz.csv) | awk '{k+=$8;x+=1} END {printf "%3.1fK", (k/(x*1000))}' > avg_tier2_size

# avg times SAT/UNSAT
grep -f solvedSAT.csv solveTimes.csv | sort -n | awk -f ../median.awk > mediantime_SAT
grep -f solvedUNSAT.csv solveTimes.csv | sort -n | awk -f ../median.awk > mediantime_UNSAT
grep -f solvedSAT.csv solveTimes.csv | awk '{{x+=$1; y+=1}} END {printf "%-7.2f\n", (x/y)}' > avgtime_SAT
grep -f solvedUNSAT.csv solveTimes.csv | awk '{{x+=$1; y+=1}} END {printf "%-7.2f\n", (x/y)}' > avgtime_UNSAT

xzgrep "ASSIGNMENT FOUND" $(cat solved_xz.csv | grep -f solvedSAT.csv) | sed 's/:s.*$//' | sed 's/.gz.*/.gz/' | sort > sls_sat.csv



../concat_files.py > combined.csv


################

# xzgrep "total elapsed seconds" *.out.xz  | awk '{if ($7 > 5) {print $1 " - " $7;}; a+=$7;} END {print a}'
# 9093.88

# expensive ones make up HALF of the time used:
# xzgrep "total elapsed seconds" *.out.xz  | awk '{if ($7 > 5) {print $1 " - " $7; a+=$7;}} END {print a}'
# 5417.6


