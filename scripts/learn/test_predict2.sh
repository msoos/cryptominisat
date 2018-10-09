#!/bin/bash
set -e
set -x

DIR=test_predict2
rm -f q_query_3_L200_coli.sat.cnf.*
./predict_one.sh q_query_3_L200_coli.sat.cnf ${DIR} 0.3

echo "Execute:"
echo "display tree.png"
