#!/bin/bash

check="$1"
FILE="20180321_110706599_p_cnf_320_1120.cnf"
FILE="001-80-12-sc2014.cnf"

rm -f out
for d in `ls | grep $check`; do
    echo $d;
    FILE=`ls $d/*.out.xz | head -n1 | sed "s/.out.xz//"`
    echo "FILE: $FILE"
    PAR2=`cat $d/PAR2score | awk '{printf "%7.f", $1}'`
    PAR2score_1500=`cat $d/PAR2score_1500  | awk '{printf "%7.f", $1}'`
    name=`xzgrep "Command being" ${FILE}.timeout.xz`
    numsolved=`wc -l $d/solved.csv | awk '{printf "%3d", $1}'`
    numALL=`wc -l $d/allFiles.csv | awk '{printf "%3d", $1}'`
    numUNSAT=`wc -l $d/solvedUNSAT.csv | awk '{printf "%3d", $1}'`
    numSAT=`wc -l $d/solvedSAT.csv  | awk '{printf "%3d", $1}'`
    rev=`xzgrep -i "revision" ${FILE}.out.xz | awk '{print $5}' | cut -c1-7`
    reducedb_percent_time=`awk '{printf "%-3.1f", $1}' $d/reducedb_percent_time.percent`
    distill_bin_percent_time=`awk '{printf "%-3.1f", $1}' $d/distill_bin_percent_time.percent`
    distill_long_percent_time=`awk '{printf "%-3.1f", $1}' $d/distill_long_percent_time.percent`
    occsimp_percent_time=`awk '{printf "%-3.1f", $1}' $d/occsimp_percent_time.percent`
    avgtime_SAT=`cat $d/avgtime_SAT | awk '{printf "%5.0f", $1}'`
    avgtime_UNSAT=`cat $d/avgtime_UNSAT  | awk '{printf "%5.0f", $1}'`
    numsls=`wc -l $d/sls_sat.csv  | awk '{printf "%2d", $1}'`
    memout_or_issue=`wc -l $d/memout_or_issue.csv  | awk '{printf "%3d", $1}'`
    formatteddir=`echo $d | awk '{printf "%27s", $1}'`
    mediantime_SAT=`cat $d/mediantime_SAT | awk '{printf "%4.0f", $1}'`
    mediantime_UNSAT=`cat $d/mediantime_UNSAT | awk '{printf "%4.0f", $1}'`
    avg_tier0_size=`cat $d/avg_tier0_size | awk '{printf "%4.1f", $1}'`
    avg_tier1_size=`cat $d/avg_tier1_size | awk '{printf "%4.1f", $1}'`
    avg_tier2_size=`cat $d/avg_tier2_size | awk '{printf "%4.1f", $1}'`
    name=`echo $name | sed "s/ 001-80-12-sc2014.cnf.*//"`
    #echo "${PAR2} ${PAR2score_1500} $formatteddir $numsolved $numSAT $numUNSAT $numALL ${memout_or_issue}MO ${numsls}SLS ${avg_tier0_size}-T0 ${avg_tier1_size}-T1 ${avg_tier2_size}-T2 ${mediantime_SAT}MedS ${mediantime_UNSAT}MedU $rev $reducedb_percent_time%RDB $distill_percent_time%DIST $occsimp_percent_time%OCC $name" >> out
    echo "${PAR2} ${PAR2score_1500} $formatteddir $numsolved $numSAT $numUNSAT $numALL ${memout_or_issue}MO ${numsls}SLS ${avg_tier0_size}-T0 ${avg_tier1_size}-T1 ${avg_tier2_size}-T2 ${mediantime_SAT}MedS ${mediantime_UNSAT}MedU $rev $reducedb_percent_time%RDB $distill_long_percent_time%DISTL $distill_bin_percent_time%DISTILLB $occsimp_percent_time%OCC $name" >> out
done
sed "s/20180321_110706.*//" out | sed "s/Command.*time.*cryptominisat5//"  | sed "s/-drat0.solved.csv//" |                 sed "s/ *Command being timed.*cryptominisat5//" | sed "s/\t/ /g" | sed "s/\t/ /g" | sort -n
