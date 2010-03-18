#!/bin/bash

TMPDIR=/tmp
mypath=.
verbosity=1
gaussuntil=0
extra=""

function usage {
    echo ""
    echo "Usage: $0 [options] <input CNF>"
    echo "  -d   directory of executables (default = .)"
    echo "  -t   directory of temporaty files (default = /tmp)"
    echo "  -g   gauss until this depth (default = 0)"
    echo "  -f   force gauss to work non-stop"
    echo "  -n   don't perform var replacement (default = replace)"
    echo "  -s   static restart strategy"
    echo "  -w   dynamic restart strategy"
    echo "  -z   don't simplify"
    echo "  -q   don't use failed var"
    echo "  -c   don't perform conglomeration (default = conglomerate)"
    echo "  -e   don't perform heule-preprocess (default = heule)"
    echo "  -p   don't perform subsumption"
    echo "  -u   don't do binary xor finding"
    echo "  -i   don't do normal xor finding"
    echo "  -v   verbosity (default = 1)"
    echo ""
}

args=`getopt g:v:t:d:ncsfzqepuiw "$@"`
if test $? != 0
then
    usage
    exit 1
fi
set -- $args

while [ $# -gt 0 ]
do
    case "$1" in
        (-d) mypath=$2; shift;;
        (-t) TMPDIR=$2; shift;;
        (-v) verbosity=$2; shift;;
        (-g) gaussuntil=$2; shift;;
        (-n) extra="$extra -novarreplace";;
        (-c) extra="$extra -noconglomerate";;
        (-f) extra="$extra -nodisablegauss";;
        (-s) extra+="$extra -restart=static";;
        (-z) extra+="$extra -nosimplify";;
        (-q) extra+="$extra -nofailedvar";;
        (-e) extra+="$extra -noheuleprocess";;
        (-p) extra+="$extra -nosubsumption";;
        (-u) extra+="$extra -nobinxorfind";;
        (-i) extra+="$extra -nonormxorfind";;
        (-w) extra+="$extra -restart=dynamic";;
        (--) shift; break;;
        (-*) echo "$0: error - unrecognized option $1" 1>&2; exit 1;;
        (*)  break;;
    esac
    shift
done

if [ "x$1" = "x" ];
then
    usage
    exit 1
fi


# To set in a normal envirnement

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
echo "c SatElite return value: $X"
if [ $X == 0 ]; then
  #SatElite terminated correctly
  echo "c SatElite terminated correctly"
  echo "c Starting CryptoMiniSat2"
  echo "c"
  $RS -gaussuntil=$gaussuntil -verbosity=$verbosity $extra $TMP.cnf $TMP.result "$@"
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
  echo "c SatElite is called for model extension"
  $SE +ext $INPUT $TMP.result $TMP.vmap $TMP.elim  "$@"
  X=$?
elif [ $X == 11 -o $X == 3 ]; then
  #SatElite died, CryptoMiniSat2 must take care of the rest
  echo "c SatElite died, CryptoMiniSat2 must take over"
  echo "c Starting CryptoMiniSat2"
  echo "c"
  $RS $extra -gaussuntil=$gaussuntil -verbosity=$verbosity $INPUT #but we must force CryptoMiniSat to print out result here!!!
  X=$?
elif [ $X == 12 ]; then
  #SatElite prints out usage message
  echo "c SatElite printed use message"
  X=0
fi

rm -f $TMP.cnf $TMP.vmap $TMP.elim $TMP.result
exit $X
