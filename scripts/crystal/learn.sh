#!/usr/bin/bash

#set -x
set -e

rm -f out_* classifiers/*
git-head > out_git
cat learn.sh > out_git
md5 *.dat > out_git

only=0.99
bestf="../../scripts/crystal/best_features.txt"
bestf="../../scripts/crystal/best_features.txt-ext"
../cldata_predict.py short-comb.dat --tier short --xgboost --final --only $only \
     --basedir ../../src/predict/ --name short --bestfeatfile $bestf | tee out_short

../cldata_predict.py long-comb.dat  --tier long --xgboost --final --only $only \
    --basedir ../../src/predict/ --name long --bestfeatfile $bestf | tee out_long

../cldata_predict.py forever-comb.dat --tier forever --xgboost --final --only $only \
    --basedir ../../src/predict/ --name forever --bestfeatfile $bestf | tee out_forever

#pid3=$!
#wait $pid1
#wait $pid2
#wait $pid3
cp ../../src/predict/*.boost classifiers/
cp out_* classifiers/

exit 0
