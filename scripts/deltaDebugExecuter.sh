#!/bin/bash
python regression_test.py --numStart 1 --num 1 --file $1 
PROC_RET=$?
echo "Returning $PROC_RET"
exit $PROC_RET
