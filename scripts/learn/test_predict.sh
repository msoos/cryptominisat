#!/bin/bash
set -e
set -x

DIR=test_predict
rm -f drat_test2.cnf.*
./predict_one.sh drat_test2.cnf ${DIR} 1.0

echo "Execute:"
echo "display tree.png"
