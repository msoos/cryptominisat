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

    NOPYTHON)
        sudo apt-get install libboost-program-options-dev
        sudo apt-get remove python2.7-dev python-dev
        eval cmake ${COMMON_CMAKE_ARGS} \
                   ${SOURCE_DIR}
    ;;

    MYSQL)
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
        sudo apt-get install sqlite sqlite-dev

        eval cmake ${COMMON_CMAKE_ARGS} \
                    -DSTATS:BOOL=ON \
                   ${SOURCE_DIR}
    ;;

    *)
        echo "\"${STP_CONFIG}\" configuration not recognised"
        exit 1
esac

make
ctest -V
sudo make install

case $CMS_CONFIG in
    MYSQL)
        echo "1 2 0" ./cryptominisat --sql 2 --wsql 2 --zero-exit-status
    ;;

     SQLITE)
        echo "1 2 0" ./cryptominisat --sql 2 --wsql 3 --zero-exit-status
    ;;

    *)
        echo "\"${STP_CONFIG}\" configuration not recognised"
        exit 1
esac
