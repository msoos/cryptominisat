#!/bin/bash

check="$1"

rm out
for d in `ls | grep $check`; do
    echo $d;
    PAR2=`cat $d/PAR2score`
    name=`xzgrep "Command being" $d/20180321_110706599_p_cnf_320_1120.cnf.gz.timeout.xz`
    numsolved=`wc -l $d/solved.csv | awk '{print $1}'`
    numUNSAT=`wc -l $d/solvedUNSAT.csv | awk '{print $1}'`
    numSAT=`wc -l $d/solvedSAT.csv  | awk '{print $1}'`
    rev=`xzgrep -i "revision" $d/20180321_110706599_p_cnf_320_1120.cnf.gz.out.xz | awk '{print $5}' | cut -c1-7`
    echo "$PAR2 $d $numsolved $numSAT $numUNSAT $rev $name" >> out
done
sed "s/20180321_110706.*//" out | sed "s/Command.*time.*cryptominisat5//"  | sed "s/-drat0.solved.csv//" |                 sed "s/ *Command being timed.*cryptominisat5//" | sed "s/\t/ /g" | sed "s/\t/ /g" | sort -n
