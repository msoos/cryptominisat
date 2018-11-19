#!/usr/bin/bash

for x in `ls | grep out-141617`; do
    (cd $x
    echo At $x
    pwd
    /home/soos/cryptominisat/scripts/output_parser/solvetimes_from_output.sh
    );
done

