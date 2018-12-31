#!/usr/bin/bash
set -e
set -x

numconfs=7
location="out-dats-8074972.wlm01"

###############
# Combine
###############

rm -f out_combine_short_*
rm -f comb-short-conf-*.dat
for (( CONF = 0; CONF < numconfs; CONF++))
do
    rm -f "comb-short-conf-${CONF}.dat"

    ./combine_dats.py -o "comb-short-conf-${CONF}.dat" ${location}/*-short-conf-${CONF}.dat > "out_combine_short_${CONF}" 2>&1 &
done

# wait all threads
FAIL=0
for job in $(jobs -p)
do
    echo "waiting for job '$job' ..."
    wait "${job}" || FAIL=$((FAIL+=1))
done

rm -f out_combine_long_*
for (( CONF = 0; CONF < numconfs; CONF++))
do
    rm -f "comb-long-conf-${CONF}.dat"

    ./combine_dats.py -o "comb-long-conf-${CONF}.dat"  ${location}/*-long-conf-${CONF}.dat  > "out_combine_long_${CONF}" 2>&1 &
done

# wait all threads
FAIL=0
for job in $(jobs -p)
do
    echo "waiting for job '$job' ..."
    wait "${job}" || FAIL=$((FAIL+=1))
done

# check for FAILs
if [ "$FAIL" == "0" ];
then
    echo "All went well!"
else
    echo "FAIL of one of the threads! ($FAIL)"
    exit 255
fi

rm -f error
touch error
for (( CONF = 0; CONF < numconfs; CONF++))
do
    egrep --color -i -e "assert.*fail" -e "signal" -e "error" -e "kill" -e "terminate" "out_combine_short_${CONF}" 2>&1 | tee -a error
    egrep --color -i -e "assert.*fail" -e "signal" -e "error" -e "kill" -e "terminate" "out_combine_long_${CONF}" 2>&1 | tee -a error
done

# exit in case of errors
if [[ -s error ]]; then
    echo "ERROR: Issues occurred!"
    exit 255
else
    echo "OK, no errors"
fi


#################
# predict.py
#################

mkdir -p ../src/predict
rm -f ../src/predict/*.h
rm -f out_pred_short_*
rm -f out_pred_long_*

# no long
./predict.py "comb-long-conf-3.dat" --name long  --basedir "../src/predict/" --final --forest --split 0.02 --clusters 9 --conf "0" --clustmin 0.03 > "out_pred_long_0" 2>&1 &

#no long, lock glue 2
./predict.py "comb-long-conf-3.dat" --name long  --basedir "../src/predict/" --final --forest --split 0.02 --clusters 9 --conf "1" --clustmin 0.03 > "out_pred_long_1" 2>&1 &

#no long, lock glue 3
./predict.py "comb-long-conf-3.dat" --name long  --basedir "../src/predict/" --final --forest --split 0.02 --clusters 9 --conf "2" --clustmin 0.03 > "out_pred_long_2" 2>&1 &

# 1 cluster
./predict.py "comb-long-conf-3.dat" --name long  --basedir "../src/predict/" --final --forest --split 0.02 --clusters 1 --conf "3" --clustmin 0.03 > "out_pred_long_3" 2>&1 &

# 3 clusters
./predict.py "comb-long-conf-3.dat" --name long  --basedir "../src/predict/" --final --forest --split 0.02 --clusters 3 --conf "4" --clustmin 0.03 > "out_pred_long_4" 2>&1 &

# 5 clusters
./predict.py "comb-long-conf-3.dat" --name long  --basedir "../src/predict/" --final --forest --split 0.02 --clusters 5 --conf "5" --clustmin 0.03 > "out_pred_long_5" 2>&1 &

# 20 clusters, min 0.2%
./predict.py "comb-long-conf-3.dat" --name long  --basedir "../src/predict/" --final --forest --split 0.02 --clusters 20 --conf "6" --clustmin 0.02 > "out_pred_long_6" 2>&1 &


# wait all threads
FAIL=0
for job in $(jobs -p)
do
    echo "waiting for job '$job' ..."
    wait "${job}" || FAIL=$((FAIL+=1))
done

# check for FAILs
if [ "$FAIL" == "0" ];
then
    echo "All went well!"
else
    echo "FAIL of one of the threads! ($FAIL)"
    exit 255
fi

# no long
./predict.py "comb-short-conf-3.dat" --name short  --basedir "../src/predict/" --final --forest --split 0.02 --clusters 9 --conf "0" --clustmin 0.03 > "out_pred_short_0" 2>&1 &

#no long, lock glue 2
./predict.py "comb-short-conf-3.dat" --name short  --basedir "../src/predict/" --final --forest --split 0.02 --clusters 9 --conf "1" --clustmin 0.03 > "out_pred_short_1" 2>&1 &

#no long, lock glue 3
./predict.py "comb-short-conf-3.dat" --name short  --basedir "../src/predict/" --final --forest --split 0.02 --clusters 9 --conf "2" --clustmin 0.03 > "out_pred_short_2" 2>&1 &

# 1 cluster
./predict.py "comb-short-conf-3.dat" --name short  --basedir "../src/predict/" --final --forest --split 0.02 --clusters 1 --conf "3" --clustmin 0.03 > "out_pred_short_3" 2>&1 &

# 3 clusters
./predict.py "comb-short-conf-3.dat" --name short  --basedir "../src/predict/" --final --forest --split 0.02 --clusters 3 --conf "4" --clustmin 0.03 > "out_pred_short_4" 2>&1 &

# 5 clusters
./predict.py "comb-short-conf-3.dat" --name short  --basedir "../src/predict/" --final --forest --split 0.02 --clusters 5 --conf "5" --clustmin 0.03 > "out_pred_short_5" 2>&1 &

# 20 clusters, min 0.2%
./predict.py "comb-short-conf-3.dat" --name short  --basedir "../src/predict/" --final --forest --split 0.02 --clusters 20 --conf "6" --clustmin 0.02 > "out_pred_short_6" 2>&1 &

# wait all threads
FAIL=0
for job in $(jobs -p)
do
    echo "waiting for job '$job' ..."
    wait "${job}" || FAIL=$((FAIL+=1))
done

# check for FAILs
if [ "$FAIL" == "0" ];
then
    echo "All went well!"
else
    echo "FAIL of one of the threads! ($FAIL)"
    exit 255
fi

rm -f error
touch error
for (( CONF = 0; CONF < numconfs; CONF++))
do
    egrep --color -i -e "assert.*fail" -e "signal" -e "error" -e "kill" -e "terminate" "out_pred_long_${CONF}" 2>&1 | tee -a error
    egrep --color -i -e "assert.*fail" -e "signal" -e "error" -e "kill" -e "terminate" "out_pred_short_${CONF}" 2>&1 | tee -a error
done

# exit in case of errors
if [[ -s error ]]; then
    echo "ERROR: Issues occurred!"
    exit 255
else
    echo "OK, no errors"
fi
