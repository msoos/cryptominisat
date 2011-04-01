#!/bin/bash
myret="`./cryptominisat --nosolprint --verbosity=1 AProVE09-12.cnf.gz`"
if [ "$myret" == "c SATISFIABLE" ]
then
exit 0
fi
