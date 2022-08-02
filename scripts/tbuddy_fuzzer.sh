#!/bin/bash
set -x

from=$1
to=$((from+1000))
i=$1
while [[ ${to} -gt ${i} ]]; do
    echo "Doing $i"
    rm -f test_FRAT-${i}
    rm -f test_FRAT-${i}.cnf

    # ../utils/cnf-utils/cnf-fuzz-brummayer.py > test_FRAT-${i}.cnf
    # timeout 5s ./cryptominisat5 test_FRAT-${i}.cnf b --distill 0 --scc 0 --varelim 0 --presimp 0 --xor 0 --confbtwsimp 10000000 --occsimp 0 --sls 0 --bva 0 --intree 0 > out_test
    # timeout 5s ./cryptominisat5 test_FRAT-${i}.cnf b --distill 1 --scc 1 --varelim 1 --presimp 1 --xor 0 --confbtwsimp 100 --occsimp 1 --sls 0 --bva 0 --intree 0 > out_test

    ../utils/cnf-utils/xortester.py -s $i --varsmin 35 > test_FRAT-${i}.cnf
    # ../utils/cnf-utils/xortester.py -s $i --varsmin 35 > test_FRAT-${i}.cnf
    # timeout 30s ./cryptominisat5 test_FRAT-${i}.cnf b-${i} --distill 1 --scc 1 --varelim 1 --presimp 1 --xor 1 --confbtwsimp 100 --occsimp 1 --sls 1 --bva 1 --intree 1 --maxmatrixcols 10000 --maxmatrixrows 10000 --strmaxt 0 --mustconsolidate 1 > out_test-${i}

    timeout 30s ./cryptominisat5 test_FRAT-${i}.cnf b-${i} --presimp 1 --maxmatrixcols 10000 --maxmatrixrows 10000 > out_test-${i}

    a=`grep "UNSATIS" out_test-${i}`
    if [[ $? -eq 0 ]]; then
        ./frat-rs stat b-${i}
        ./frat-rs elab b-${i} -m test_FRAT-${i}.cnf -v
        if [[ $? == 0 ]]; then
            echo "OK, verification good"
        else
            echo "Verification error"
            exit -1
        fi
    else
        echo "not UNSAT"
    fi
    rm -f b-${i}
    rm -f out_test-${i}
    rm -f ELAB-${i}
    rm -f test_FRAT-${i}
    i=$((i+1))
done
