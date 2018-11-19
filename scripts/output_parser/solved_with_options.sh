#!/usr/bin/bash

#wc -l */solved | sort -n | awk 'BEGIN {ORS="\t"} {print $1 " " $2; split($2,a,"/"); x="grep \"Command being\" " a[1] "/mp1-23.3.cnf.gz.timeout"; system(x)}'
#wc -l */solved | sort -n | awk 'BEGIN {ORS="\t"} {print $1 " " $2; split($2,a,"/"); x="grep \"Command being\" " a[1] "/mp1-23.3.cnf.gz.timeout"; system(x)}' | grep 6727387 | sort -n

check="6912668"
wc -l *${check}*/solved | sort -n | awk 'BEGIN {ORS="\t"} {print $1 " " $2; split($2,a,"/"); x="zgrep \"Command being\" " a[1] "/mp1-23.3.cnf.gz.timeout.gz"; system(x)}' | sort -n
wc -l *${check}*/solvedUNSAT | sort -n
wc -l *${check}*/solvedSAT | sort -n

echo "On SATCOMP 17 nolimits problems:"
rm tmp
for f in $(ls | grep ${check}); do
	g=$(grep -f out-new-${check}.wlm01-5/allFiles $f/solved | wc -l)
	echo "$g for $f" >> tmp
done
sort -n tmp

