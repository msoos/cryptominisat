#wc -l */solved | sort -n | awk 'BEGIN {ORS="\t"} {print $1 " " $2; split($2,a,"/"); x="grep \"Command being\" " a[1] "/mp1-23.3.cnf.gz.timeout"; system(x)}'
wc -l */solved | sort -n | awk 'BEGIN {ORS="\t"} {print $1 " " $2; split($2,a,"/"); x="grep \"Command being\" " a[1] "/mp1-23.3.cnf.gz.timeout"; system(x)}' | grep 6727387 | sort -n

