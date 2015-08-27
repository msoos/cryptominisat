#!/bin/bash

# Copyright (C) 2014  Mate Soos
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

set -e

NUM=14
NUM_MIN_ONE=`expr $NUM - 1`

rm -f outs/*
./reconf.py -i 5,0,8,11,9,10 -n $NUM -f outs/out  ~/media/sat/out/satcomp091113/reconf*/*stdout* > output
for i in $(seq 0 $NUM_MIN_ONE); do
    echo "reconf with $i"
    cp reconf.names outs/out${i}.names
    c5.0 -u 20 -f outs/out${i} -r > outs/out${i}.c50.out
done
./tocpp.py -i 5,0,8,11,9,10 -n $NUM > ../../cryptominisat4/features_to_reconf.cpp
read -r -p "Upload to AWS? [y/N] " response
case $response in
    [yY][eE][sS]|[yY])
        aws s3 cp ../../cryptominisat4/features_to_reconf.cpp s3://msoos-solve-data/solvers/
        echo "Uploded to AWS"
        ;;
    *)
        echo "Not uploaded to AWS"
        ;;
esac
