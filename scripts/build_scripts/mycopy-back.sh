#!/usr/bin/bash

set -x
set -e
rm -rf ../src/predict
rsync -vazP nscc:/home/projects/11000744/matesoos/cryptominisat5-predict-main/headers/*.h ../src/predict/
rsync -vazP nscc:/home/projects/11000744/matesoos/cryptominisat5-predict-main/out_*.out ../src/predict/
