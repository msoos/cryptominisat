#!/bin/bash

set -e

DIR="$(pwd)/../../../"

echo  "$(pwd)/../../tests/cnf-files" > todo
find  "$(pwd)/../../tests/cnf-files/" -printf "%f\n" | grep ".cnf$" >> todo

./server.py --noaws --dir "$DIR" --noshutdown -t 3 --extratime 2 --cnfdir todo &
./client.py --noaws --dir "$DIR" --noshutdown --host localhost --temp /tmp/ --net lo --threads 2
wait
