#!/bin/bash
python regression_test.py -f $1 
PROC_RET=$?
echo "Returning $PROC_RET"
exit $PROC_RET
