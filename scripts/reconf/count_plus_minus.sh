#!/usr/bin/bash

#rm outfile*.data
#rm outs/*.data
#./reconf.py -n 18 -i 1,2,8,9,10,11,15,5,14,13,3 /home/soos/media/sat/out/new/out-reconf-6776906.wlm01-*/*.out


for f in `ls outfile*.data`; do
    echo "$f:"
    grep ",+" $f  | wc -l;
    grep ",-" $f  | wc -l;
    echo "---"
done


echo "this is solved by everyone:"
grep "total-10-13-u.cnf" outfile*.data

echo "this is solved by nobody:"
grep "partial-10-13-u.cnf" outfile*.data

echo "this is solved by some only, small diff:"
grep "mp1-22.5.cnf.gz" outfile*.data

echo "this is solved by everyone except 7, large diff:"
grep "mp1-klieber2017s-1000-024-eq.cnf" outfile*.data

echo "given    neg : reconf17, reconf7, reconf16"
echo "which is eqiv: 10. 4, 9, (note: 8-drat = 15, which is ignored)"

echo 'mapping:
num -> reconf
0      0
1      3
2      4
3      6
4      7
5      12
6      13
7      14
8      15
9      16
10     17'
