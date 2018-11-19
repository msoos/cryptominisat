#!/bin/bash

for x in `ls | grep 7846479`; do
    (cd $x
    echo At $x
    pwd
    ../solvetimes_from_output.sh
    );
done

