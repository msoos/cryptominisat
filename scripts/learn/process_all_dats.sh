#!/usr/bin/bash
set -e
set -x

numconfs=5
location="out-dats-8091519.wlm01"

function wait_threads {
    FAIL=0
    for job in $(jobs -p)
    do
        echo "waiting for job '$job' ..."
        wait "${job}" || FAIL=$((FAIL+=1))
    done
}

function check_fails {
    if [ "$FAIL" == "0" ];
    then
        echo "All went well!"
    else
        echo "FAIL of one of the threads! ($FAIL)"
        exit 255
    fi
}

function check_error {
    # exit in case of errors
    if [[ -s error ]]; then
        echo "ERROR: Issues occurred!"
        exit 255
    else
        echo "OK, no errors"
    fi
}

###############
# Combine
###############

rm -f out_combine_short_*
rm -f comb-short-conf-*.dat
rm -f out_combine_long_*
rm -f comb-long-conf-*.dat
for (( CONF = 0; CONF < numconfs; CONF++))
do
    rm -f "comb-short-conf-${CONF}.dat"
    rm -f "comb-long-conf-${CONF}.dat"

    ./combine_dats.py -o "comb-short-conf-${CONF}.dat" ${location}/*-short-conf-${CONF}.dat > "out_combine_short_${CONF}" 2>&1 &
    ./combine_dats.py -o "comb-long-conf-${CONF}.dat"  ${location}/*-long-conf-${CONF}.dat  > "out_combine_long_${CONF}" 2>&1 &

    wait_threads
    check_fails
done

# populate error
rm -f error
touch error
for (( CONF = 0; CONF < numconfs; CONF++))
do
    grep -E --color -i -e "assert.*fail" -e "signal" -e "error" -e "kill" -e "terminate" "out_combine_short_${CONF}" 2>&1 | tee -a error
    grep -E --color -i -e "assert.*fail" -e "signal" -e "error" -e "kill" -e "terminate" "out_combine_long_${CONF}" 2>&1 | tee -a error
done
check_error


#################
# predict.py
#################

mkdir -p ../src/predict
rm -f ../src/predict/*.h
rm -f out_pred_short_*
rm -f out_pred_long_*

# scaler conf
for (( CONF = 0; CONF < numconfs; CONF++))
do
    ./predict.py "comb-long-conf-${CONF}.dat" --scale --name long  --basedir "../src/predict/" --final --forest --split 0.01 --clusters 9 --conf "${CONF}" --clustmin 0.03 > "out_pred_long_${CONF}" 2>&1 &
    ./predict.py "comb-short-conf-${CONF}.dat" --scale --name short --basedir "../src/predict/" --final --forest --split 0.01 --clusters 9 --conf "${CONF}" --clustmin 0.03 > "out_pred_short_${CONF}" 2>&1 &
    wait_threads
    check_fails
done


# no scaler conf 1 (using conf 1)
CONF=5
./predict.py "comb-long-conf-0.dat" --name long  --basedir "../src/predict/" --final --forest --split 0.01 --clusters 9 --conf "${CONF}" --clustmin 0.03 > "out_pred_long_${CONF}" 2>&1 &
./predict.py "comb-short-conf-0.dat" --name short  --basedir "../src/predict/" --final --forest --split 0.01 --clusters 9 --conf "${CONF}" --clustmin 0.03 > "out_pred_short_${CONF}" 2>&1 &
wait_threads
check_fails


# populate error
rm -f error
touch error
for (( CONF = 0; CONF < numconfs+1; CONF++))
do
    grep -E --color -i -e "assert.*fail" -e "signal" -e "error" -e "kill" -e "terminate" "out_pred_long_${CONF}" 2>&1 | tee -a error
    grep -E --color -i -e "assert.*fail" -e "signal" -e "error" -e "kill" -e "terminate" "out_pred_short_${CONF}" 2>&1 | tee -a error
done
check_error
