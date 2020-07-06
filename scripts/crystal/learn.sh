#!/bin/bash

# Copyright (C) 2020  Mate Soos
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; version 2
# of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301, USA.

# This file wraps CMake invocation for TravisCI
# so we can set different configurations via environment variables.

#set -x
set -e

rm -f out_* classifiers/*
git rev-parse HEAD > out_git
cat learn.sh >> out_git
md5sum *.dat >> out_git

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
cp ../../src/predict/*.json classifiers/
cp out_* classifiers/

exit 0
