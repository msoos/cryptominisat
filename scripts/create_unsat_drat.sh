#!/bin/bash
set -x


SATFILE="$1"
UNSATFILE="${SATFILE%.cnf}_unsat.cnf"
REDCLFILE="${SATFILE%.cnf}_red_cls.cnf"
DECFILE="${SATFILE%.cnf}_decisions.cnf"
DRATFILE="${SATFILE%.cnf}_unsat.drat"
TXTDRATFILE="${SATFILE%.cnf}_unsat_drat"


./cryptominisat5 --writeunsat ${UNSATFILE} --dumpdecformodel ${DECFILE} -s 0 --dumpred ${REDCLFILE} ${SATFILE}

# TODO : the header of UNSAT file needs to contain correct clause count

# ./cryptominisat5 --gluecut0 100 --dumpdecformodel ${DECFILE} --cldatadumpratio 0.9 --clid --sql 2 --sqlitedb "$UNSATFILE.db-raw" --drat ${DRATFILE} --zero-exit-status ${UNSATFILE}

./cryptominisat5 -s 0 ${UNSATFILE} ${DRATFILE}

./drat-trim ${UNSATFILE} ${DRATFILE}

# The following uses DRAT-trim from Marijn
# The version by Mate does not support the following use

./drat-trim ${UNSATFILE} "${DRATFILE}"  -c "${TXTDRATFILE}_core" -l "${TXTDRATFILE}_lemma"

./drat-trim ${UNSATFILE} "${TXTDRATFILE}_lemma"
