#!/bin/bash

module unload gcc/4.9.3
module load anaconda/3
module load openmpi/intel/1.10.2
mypids=()
for x in `ls | grep $1`; do
    cd $x
    echo At $x
    pwd
    ../solvetimes_from_output.sh &
    pid=$!
    echo "PID here is $pid"
    mypids+=("$pid")
    cd ..
done

echo "PIDS to wait for are: ${mypids[*]}"
for pid2 in "${mypids[@]}"
do
    echo "Waiting for $pid2 ..."
    wait $pid2
done


