#!/usr/bin/bash
set -x

#xzgrep "s SATIS" *.out.xz > solved
cat solved | sed "s/.out.*//" > solved_files
rm -f solveTimes.csv
for f in `cat solved_files`; do
    xzgrep "time" "$f.out.xz" | awk '{print $6 " x"}' | sed "s/x/$f/" >> solveTimes_unsorted
done
sort -n solveTimes_unsorted > solveTimes.csv
rm solveTimes_unsorted
