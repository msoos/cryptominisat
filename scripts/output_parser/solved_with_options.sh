#!/bin/bash

check="7846479"
wc -l *${check}*/solved.csv | sort -n | awk 'BEGIN {ORS="\t"} {print $1 " " $2; split($2,a,"/"); x="xzgrep \"Command being\" " a[1] "/mp1-23.3.cnf.gz.timeout.xz"; system(x)}' | sort -n
wc -l *${check}*/solvedUNSAT.csv | sort -n
wc -l *${check}*/solvedSAT.csv | sort -n

echo "On SATCOMP 17 nolimits problems:"
rm tmp
for f in $(ls | grep ${check}); do
	g=$(grep -f out-new-${check}.wlm01-5/allFiles.csv $f/solved.csv | wc -l)
	echo "$g for $f" >> tmp
done
sort -n tmp

