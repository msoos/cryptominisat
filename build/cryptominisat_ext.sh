#!/bin/bash

if [ "x$1" = "x" ]; then
  echo "USAGE: cryptominisat_ext.sh <input CNF>"
  exit 1
fi


# To set in a normal envirnement
mypath=.
TMPDIR=/tmp

TMP=$TMPDIR/cryptominisat_$$ #set this to the location of temporary files
SE=$mypath/satelite           #set this to the executable of SatELite
RS=$mypath/cryptominisat      #set this to the executable of CryptoMiniSat
INPUT=$1;
shift 
echo "c"
echo "c Starting SatElite Preprocessing"
echo "c"
$SE $INPUT $TMP.cnf $TMP.vmap $TMP.elim
X=$?
echo "c"
echo "c Starting glucose"
echo "c"
if [ $X == 0 ]; then
  #SatElite terminated correctly
  $RS $TMP.cnf -gaussuntil=100 -verbosity=1 $TMP.result "$@" 
  #more $TMP.result
  X=$?
  if [ $X == 20 ]; then
    echo "s UNSATISFIABLE"
    rm -f $TMP.cnf $TMP.vmap $TMP.elim $TMP.result
    exit 20
    #Don't call SatElite for model extension.
  elif [ $X != 10 ]; then
    #timeout/unknown, nothing to do, just clean up and exit.
    rm -f $TMP.cnf $TMP.vmap $TMP.elim $TMP.result
    exit $X
  fi 
  #SATISFIABLE, call SatElite for model extension
  $SE +ext $INPUT $TMP.result $TMP.vmap $TMP.elim  "$@"
  X=$?
elif [ $X == 11 ]; then
  #SatElite died, glucose must take care of the rest
  $RS $INPUT -verbosity=1 #but we must force glucose to print out result here!!!
  X=$?
elif [ $X == 12 ]; then
  #SatElite prints out usage message
  #There is nothing to do here.
  X=0
fi

rm -f $TMP.cnf $TMP.vmap $TMP.elim $TMP.result
exit $X
