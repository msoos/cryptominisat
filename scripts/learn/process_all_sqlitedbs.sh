#!/usr/bin/bash
set -e
set -x

numconfs=7
location="out-dats-8074972.wlm01"

###############
# Combine
###############


rm -f out_combine_short_*
rm -f out_combine_long_*
for (( CONF = 0; CONF < numconfs; CONF++))
do
    rm -f "comb-short-conf-${CONF}.dat"
    rm -f "comb-long-conf-${CONF}.dat"

    ./combine_dats.py -o "comb-short-conf-${CONF}.dat" ${location}/*-short-conf-${CONF}.dat > "out_combine_short_${CONF}" 2>&1 &
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
for (( CONF = 0; CONF < numconfs; CONF++))
do
    ./predict.py "comb-long-conf-${CONF}.dat" --name long  --basedir "../src/predict/" --final --forest --split 0.02 --clusters 9 --conf "${CONF}" --clustmin 0.03 > "out_pred_long_${CONF}" 2>&1 &
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

for (( CONF = 0; CONF < numconfs; CONF++))
do
    ./predict.py "comb-short-conf-${CONF}.dat" --name short --basedir "../src/predict/" --final --forest --split 0.02 --clusters 9 --conf "${CONF}" --clustmin 0.03 > "out_pred_short_${CONF}" 2>&1 &
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
