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

set -e
set -x

function run_tmp {
    # cp stuff.cnf tmp
    ./cryptominisat5 --zero-exit-status --dumpdecformodel b tmp drat.out > out.sat
    grep "c conflicts" out.sat
    set +e
    a=$(grep "s SATIS" out.sat)
    retval=$?
    set -e
    if [[ retval -eq 1 ]]; then
        echo "-> UNSAT"
        ../utils/cnf-utils/xor_to_cnf.py tmp final.cnf
        ./tests/drat-trim/drat-trim final.cnf drat.out |tee out.drat
        grep "VERIFIED" out.drat
        grep "resol.*steps" out.drat
        continue
    fi
    echo "-> SAT"

    #cat out.sat
    rm final.cnf
    touch final.cnf
    cat tmp >> final.cnf
    cat b >> final.cnf
    cp final.cnf final_without_ban.cnf
    ../utils/cnf-utils/xor_to_cnf.py final_without_ban.cnf final_without_ban.cnf2
    mv final_without_ban.cnf2 final_without_ban.cnf
    grep ^v out.sat | sed "s/v//" | tr -d "\n" | sed "s/  / /g" | sed -e "s/ -/X/g" -e "s/ /Y/g" | sed "s/X/ /g" | sed -E "s/Y([1-9])/ -\1/g" | sed "s/Y0/ 0\n/" >> final.cnf
    ../utils/cnf-utils/xor_to_cnf.py final.cnf final2.cnf
    mv final2.cnf final.cnf
    ./cryptominisat5 --zero-exit-status --verb 0 final.cnf | tee out.unsat
    set +e
    a=$(grep "s UNSATIS" out.unsat)
    retval=$?
    set -e
    if [[ retval -eq 1 ]]; then
        echo "GRAVE ERROR!"
        exit -1
    fi

    ./tests/drat-trim/drat-trim final.cnf drat.out |tee out.drat
    grep "VERIFIED" out.drat || exit -1
    grep "resol.*steps" out.drat
}

if [[ $1 == "" ]]; then
    echo "No arguments, standard run"
    for(( c=1; c < 10000; c++ ))
    do
        r=$((1 + RANDOM % 999999))
        echo ""
        echo "------ Run $c random $r -----------"
        # ../utils/cnf-utils/cnf-fuzz-brummayer.py -s $r -l 3 -i 90 -I 180  > tmp
        ../utils/cnf-utils/cnf-fuzz-brummayer.py -s $r > tmp
        run_tmp
    done
else
    if [[ $1 == "--file" ]]; then
        echo "Using file $2"
        rm -f tmp.gz
        rm -f tmp
        cp $2 tmp.gz
        gunzip tmp.gz
        run_tmp
    else
        echo "Seed run"
        ../utils/cnf-utils/cnf-fuzz-brummayer.py -s $1 -l 3 -i 90 -I 180  > tmp
        run_tmp
        exit 0
    fi
fi
