#!/bin/bash -x
# This file wraps CMake invocation for TravisCI
# so we can set different configurations via environment variables.
#
# We could modify our CMake scripts to read environment variables directly but
# that would create two ways of setting the same thing which doesn't seem like
# a good idea.

# export CC="gcc-4.7"
# export CXX="g++-4.7"

SOURCE_DIR="$1"
THIS_DIR="$2"
COMMON_CMAKE_ARGS="-G \"Unix Makefiles\" -DENABLE_TESTING:BOOL=ON"
set -e

# Note eval is needed so COMMON_CMAKE_ARGS is expanded properly
case $CMS_CONFIG in
    LATEX)
        sudo apt-get install texlive-latex-base
        cd desc/satcomp14
        make
        exit 0
    ;;

    AWS)
        sudo apt-get install python-boto
        cd scripts/aws
        echo  `pwd`/../../tests/cnf-files > todo
        find  `pwd`/../../tests/cnf-files/ -printf "%f\n" | grep ".cnf$" >> todo
        ./server -t 10 --cnfdir todo &
        ./client --host localhost --temp /tmp/
        wait
        exit 0
    ;;

    NORMAL)
        sudo apt-get install libboost-program-options-dev
        eval cmake ${COMMON_CMAKE_ARGS} \
                   ${SOURCE_DIR}
    ;;
    STATIC_BIN)
        sudo apt-get install libboost-program-options-dev
        eval cmake ${COMMON_CMAKE_ARGS} \
                   -DSTATICCOMPILE:BOOL=ON \
                   ${SOURCE_DIR}
    ;;

    ONLY_SIMPLE)
        eval cmake ${COMMON_CMAKE_ARGS} \
                   -DONLY_SIMPLE:BOOL=ON \
                   ${SOURCE_DIR}
    ;;

    ONLY_SIMPLE_STATIC)
        eval cmake ${COMMON_CMAKE_ARGS} \
                   -DONLY_SIMPLE:BOOL=ON \
                   -DSTATICCOMPILE:BOOL=ON \
                   ${SOURCE_DIR}
    ;;

    MORE_DEBUG)
        sudo apt-get install libboost-program-options-dev
        eval cmake ${COMMON_CMAKE_ARGS} \
                   -DMORE_DEBUG:BOOL=ON \
                   ${SOURCE_DIR}
    ;;

    STATS)
        sudo apt-get install libboost-program-options-dev
        eval cmake ${COMMON_CMAKE_ARGS} \
                   -DSTATS:BOOL=ON \
                   ${SOURCE_DIR}
    ;;

    NOZLIB)
        sudo apt-get install libboost-program-options-dev
        eval cmake ${COMMON_CMAKE_ARGS} \
                   -DNOZLIB:BOOL=ON \
                   ${SOURCE_DIR}
    ;;

    RELEASE)
        sudo apt-get install libboost-program-options-dev
        eval cmake ${COMMON_CMAKE_ARGS} \
                   -DCMAKE_BUILD_TYPE:STRING=Release \
                   ${SOURCE_DIR}
    ;;

    NOSQLITE)
        sudo apt-get install libboost-program-options-dev
        sudo apt-get remove libsqlite3-dev
        eval cmake ${COMMON_CMAKE_ARGS} \
                   ${SOURCE_DIR}
    ;;

    NOMYSQL)
        sudo apt-get install libboost-program-options-dev
        sudo apt-get remove libmysqlclient-dev
        eval cmake ${COMMON_CMAKE_ARGS} \
                   ${SOURCE_DIR}
    ;;

    NOMYSQLNOSQLITE)
        sudo apt-get install libboost-program-options-dev
        sudo apt-get remove libmysqlclient-dev
        sudo apt-get remove libsqlite3-dev
        eval cmake ${COMMON_CMAKE_ARGS} \
                   ${SOURCE_DIR}
    ;;

    PYTHON)
        sudo apt-get install libboost-program-options-dev
        sudo apt-get remove python2.7-dev python-dev
        eval cmake ${COMMON_CMAKE_ARGS} \
                   ${SOURCE_DIR}
    ;;

    MYSQL|WEB)
        sudo apt-get install libboost-program-options-dev
        sudo debconf-set-selections <<< 'mysql-server mysql-server/root_password password '
        sudo debconf-set-selections <<< 'mysql-server mysql-server/root_password_again password '
        sudo apt-get install mysql-server
        sudo apt-get install mysql-client
        sudo apt-get install libmysqlclient-dev
        cd $SOURCE_DIR
        ./cmsat_mysql_setup.sh
        cd $THIS_DIR

        eval cmake ${COMMON_CMAKE_ARGS} \
                    -DSTATS:BOOL=ON \
                   ${SOURCE_DIR}
    ;;

    SQLITE)
        sudo apt-get install libboost-program-options-dev
        sudo apt-get install libsqlite3-dev

        eval cmake ${COMMON_CMAKE_ARGS} \
                    -DSTATS:BOOL=ON \
                   ${SOURCE_DIR}
    ;;

    M4RI)
        sudo apt-get install libboost-program-options-dev
        wget https://bitbucket.org/malb/m4ri/downloads/m4ri-20140914.tar.gz
        tar xzvf m4ri-20140914.tar.gz
        cd m4ri-20140914/
        ./configure
        make
        sudo make install
        cd ..

        eval cmake ${COMMON_CMAKE_ARGS} \
            ${SOURCE_DIR}
    ;;

    *)
        echo "\"${STP_CONFIG}\" configuration not recognised"
        exit 1
    ;;
esac

make
ctest -V
sudo make install

case $CMS_CONFIG in
    MYSQL|WEB)
        echo "1 2 0" | ./cryptominisat --sql 2 --wsql 2 --zero-exit-status
    ;;

    SQLITE)
        echo "1 2 0" | ./cryptominisat --sql 2 --wsql 3 --zero-exit-status
    ;;

    M4RI)
        echo "1 2 0" | ./cryptominisat --xor 1 --zero-exit-status
    ;;

    *)
        echo "\"${STP_CONFIG}\" Binary no extra testing (sql, xor, etc), skipping this part"
    ;;
esac

cd ../scripts/

case $CMS_CONFIG in
    ONLY_SIMPLE)
    ;;

    *)
        ./regression_test.py -f --fuzzlim 30
    ;;
esac

cd ..


case $CMS_CONFIG in
    WEB)
        sudo apt-get install nodejs-legacy
        sudo apt-get install npm
        cd web/dygraphs
        npm install
        npm install gulp
        gulp dist
        gulp test

        cd ..
        wget http://code.jquery.com/jquery-1.11.3.min.js
    ;;

    *)
        echo "\"${STP_CONFIG}\" No web testing"
    ;;
esac

exit 0
