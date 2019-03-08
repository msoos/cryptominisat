#!/bin/bash

module unload gcc/4.9.3
module load anaconda/3
module load openmpi/intel/1.10.2
for x in `ls | grep $1`; do
    (cd $x
    echo At $x
    pwd
    ../solvetimes_from_output.sh
    );
done

