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

set +x

module unload gcc/4.9.3
module load anaconda/3
module load openmpi/intel/1.10.2

function concat() {
./concat_pandas.py $dir/*-cldata-short.dat   -o short-comb.dat
./concat_pandas.py $dir/*-cldata-long.dat    -o long-comb.dat
./concat_pandas.py $dir/*-cldata-forever.dat -o forever-comb.dat
}

# run with new fixed setup, but only 100 instances, no sum proapgations
dir="out-dats-9974801.wlm01"
dir="out-dats-9987959.wlm01"
dir="out-dats-9997175.wlm01"

# new run, with sum propagations and 125 instances instead of 100
dir="out-dats-37633.wlm01" # less samples
dir="out-dats-41199.wlm01" # more samples
dir="out-dats-43977.wlm01" # slightly more samples, different combinations

# new run, glue is correct, the CONF doesn't really matter now
dir="out-dats-232072.wlm01"

#fixed stratification
dir="out-dats-814249.wlm01"
dir="out-dats-992340.wlm01" # -- incorrect, HACK not present
dir="out-dats-992539.wlm01" # HACK present, 2000


rm -f short-comb*
rm -f long-comb*
rm -f forever-comb*
echo "Concatenating..."
concat

echo "Zipping..."
xz *.dat
