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
    SLOW_DEBUG)
        sudo apt-get install libboost-program-options-dev
        eval cmake ${COMMON_CMAKE_ARGS} \
                   -DSLOW_DEBUG:BOOL=ON \
                   ${SOURCE_DIR}
    ;;

    NORMAL|AWS)
        sudo apt-get install libboost-program-options-dev
        eval cmake ${COMMON_CMAKE_ARGS} \
                   ${SOURCE_DIR}
    ;;

    COVERAGE)
        sudo apt-get install libboost-program-options-dev
        eval cmake ${COMMON_CMAKE_ARGS} \
                    -DCOVERAGE:BOOL=ON \
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

if [ "$CMS_CONFIG" != "AWS" ]; then
    make
    ctest -V
    sudo make install
fi

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

#do regression testing
if [ "$CMS_CONFIG" != "ONLY_SIMPLE" ] && [ "$CMS_CONFIG" != "AWS" ] && [ "$CMS_CONFIG" != "WEB" ] && [ "$CMS_CONFIG" != "PYTHON" ]; then
    cd ../scripts/
    ./regression_test.py -f --novalgrind --fuzzlim 30
    cd ..
fi

case $CMS_CONFIG in
    WEB)
        cd web
        sudo apt-get install python-software-properties
        sudo add-apt-repository -y ppa:chris-lea/node.js
        sudo apt-get update
        sudo apt-get install -y nodejs
        ./install_web.sh
    ;;

    AWS)
        sudo apt-get install -y python-boto
        sudo apt-get install -y python-pip
        sudo pip install awscli
        cd scripts/aws
        ./local_test.sh
        exit 0
    ;;

    *)
        echo "\"${STP_CONFIG}\" No further testing"
    ;;
esac

if [ "$CMS_CONFIG" -eq "COVERAGE" ]; then
  lcov --directory ../scripts/ --capture --output-file coverage.info # capture coverage info
  lcov --remove coverage.info 'scripts/*' 'tests/*' '/usr/*' --output-file coverage.info # filter out system and test code
  lcov --list coverage.info # debug before upload
  coveralls-lcov --repo-token $COVERTOKEN coverage.info # uploads to coveralls
fi
