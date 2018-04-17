#!/bin/bash
set -e
set -x

# Copyright (C) 2017  Mate Soos
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

if [[ $# -ne 2 ]]; then
    echo "ERROR: wrong number of arguments"
    echo "Use with ./predict_one.sh 6s153.cnf.gz DIR"
    exit -1
fi

status=`./cryptominisat5 --hhelp | grep sql`
ret=$?
if [ "$ret" -ne 0 ]; then
    echo "You must compile SQL into cryptominisat"
    exit -1
fi

FNAME=$1
OUTDIR=$2
mkdir -p ${OUTDIR}

rm -if ${OUTDIR}/drat_out
rm -if ${OUTDIR}/lemmas
rm -if ${OUTDIR}/*.sqlite
echo "Predicting file $1"

# running CNF
./cryptominisat5 ${FNAME} --cldatadumpratio 1.0 --gluecut0 10000 --presimp 1 -n 1 --zero-exit-status --clid --sql 2 --sqlitedb ${OUTDIR}/data.sqlite ${OUTDIR}/drat_out > ${OUTDIR}/cms_output.txt

# getting drat
./tests/drat-trim/drat-trim ${FNAME} ${OUTDIR}/drat_out -x ${OUTDIR}/lemmas -i

# add lemma indices that were good
./add_lemma_ind.py ${OUTDIR}/data.sqlite ${OUTDIR}/lemmas

# run prediction on SQLite database
./predict.py --csv ${OUTDIR}/data.sqlite

# generate DOT and display it
dot -Tpng ${OUTDIR}/data.sqlite.tree.dot -o tree.png
display tree.png
