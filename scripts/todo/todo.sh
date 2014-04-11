#!/bin/bash
#filespos="~/media/sat/examples/satcop09"
#files=`ls $filespos/*.cnf.gz`
#FILES="~/media/sat/examples/satcop09/*.cnf.gz"

ulimit -t unlimited
ulimit -a
shopt -s nullglob
rm -f todo
touch todo
fileloc="/home/soos/sat/examples/satcomp091113/*cnf.gz"
solver="/home/soos/cryptominisat/build/cryptominisat"
#solver="/home/soos/cmsat-satrace/solver"
#solver="/home/soos/lingeling-ala-b02aa1a-121013/lingeling -v"
#solver="/home/soos/lingeling-ala-b02aa1a-121013/lingeling -v"
#solver="/home/soos/glucose-3.0/simp/glucose"
#solver="/home/soos/glucose2.2/simp/glucose"
#opts="--restart glue --clean glue --flippolarfreq 0"
#opts="--calcpolarall 1 --locktop 0 --printsol 0"
#opts="--freq 0"
opts="--printsol 0"
#output="/home/soos/sat/out/lingeling-aqw-satcomp091113"
#output="/home/soos/sat/out/lingeling-aqw-satcomp091113"
#output="/home/soos/sat/out/glucose3-satcomp091113"
#output="/home/soos/sat/out/glucose-shuf-satcomp091113"
#output="/home/soos/sat/out/cmsat-satrace10-satcomp091113"
output="/home/soos/sat/out/26-satcomp091113"
tlimit="900"
#5GB mem limit
memlimit="5000000"
numthreads=4

mkdir -p $output

# remove everything that has been done
for file in `ls $output`
do
    echo "deleting file $output/$file"
    rm -i $output/$file
    status=$?
    if [ $status -ne 0 ]; then
        echo "error, can't delete file $ouput/$file"
        exit 112
    fi

done

# create todo
echo -ne "Creating todo..."
for file in  $fileloc
do
    filename=$(basename "$file")
    #todo="zcat $file | shuf --random-source=myrnd | /usr/bin/time --verbose -o $output/$filename.timeout $solver $opts > $output/$filename.out 2>&1"
    todo="/usr/bin/time --verbose -o $output/$filename.timeout $solver $opts $file > $output/$filename.out 2>&1"
    #todo="$solver $file > $output/$filename.out"
    echo $todo >> todo
    # $todo
done
numlines=`wc -l todo |  awk '{print $1}'`
echo "Done creating todo with $numlines of problems"

# create random order
echo -ne "Randomizing order of execution of $val files"
shuf --random-source=myrnd todo > todo_rnd
echo "Done."

# create per-core todos
echo "numlines:" $numlines
let numper=numlines/numthreads
remain=$((numlines-numper*numthreads))
mystart=0
echo -ne "Creating per-core TODOs"
for ((myi=0; myi < numthreads ; myi++))
do
    rm -f todo_rnd_$myi.sh
    echo todo_rnd_$myi.sh
    echo "ulimit -t $tlimit" > todo_rnd_$myi.sh
    echo "ulimit -v $memlimit" >> todo_rnd_$myi.sh
    echo "ulimit -a" >> todo_rnd_$myi.sh
    typeset -i myi
    typeset -i numper
    typeset -i mystart
    echo "myi: $myi, numper: $numper,"
    mystart=$((mystart + numper))
    echo "mystart: $mystart"
    head -n $mystart todo_rnd | tail -n $numper>> todo_rnd_$myi.sh
    chmod +x todo_rnd_$myi.sh
done
echo "Done."
let myi--
tail -n $remain todo_rnd >> todo_rnd_$myi.sh

#check that todos match original
# rm valami
# cat todo_rnd_*.sh >> valami
# diff todo_rnd valami
# status=$?
# if [ $status -ne 0 ]; then
#     echo "error, files don't match"
#     exit 113
# fi

# Execute todos
rm -f out_*
echo -ne "executing todos..."
for ((myi=0; myi < numthreads ; myi++))
do
    nohup ./todo_rnd_$myi.sh > out_$myi &
    echo "OK"
done
echo  "done."

