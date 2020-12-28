#!/bin/bash

check="$1"
FILE="20180321_110706599_p_cnf_320_1120.cnf"
FILE="001-80-12-sc2014.cnf"

rm out
for d in `ls | grep $check`; do
    echo $d;
    PAR2=`cat $d/PAR2score`
    name=`xzgrep "Command being" $d/${FILE}.gz.timeout.xz`
    numsolved=`wc -l $d/solved.csv | awk '{print $1}'`
    numALL=`wc -l $d/allFiles.csv | awk '{print $1}'`
    numUNSAT=`wc -l $d/solvedUNSAT.csv | awk '{print $1}'`
    numSAT=`wc -l $d/solvedSAT.csv  | awk '{print $1}'`
    rev=`xzgrep -i "revision" $d/${FILE}.gz.out.xz | awk '{print $5}' | cut -c1-7`
    reducedb_percent_time=`cat $d/reducedb_percent_time.percent`
    distill_percent_time=`cat $d/distill_percent_time.percent`
    occsimp_percent_time=`cat $d/occsimp_percent_time.percent`
    echo "$PAR2 $d $numsolved $numSAT $numUNSAT $numALL $rev $reducedb_percent_time%RDB $distill_percent_time%DIST $occsimp_percent_time%OCC $name" >> out
done
sed "s/20180321_110706.*//" out | sed "s/Command.*time.*cryptominisat5//"  | sed "s/-drat0.solved.csv//" |                 sed "s/ *Command being timed.*cryptominisat5//" | sed "s/\t/ /g" | sed "s/\t/ /g" | sort -n
