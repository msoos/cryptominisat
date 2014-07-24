#!/bin/bash -x
# This file wraps CMake invocation for TravisCI
# so we can set different configurations via environment variables.
#
# We could modify our CMake scripts to read environment variables directly but
# that would create two ways of setting the same thing which doesn't seem like
# a good idea.

export CC="gcc-4.7"
export CXX="g++-4.7"

SOURCE_DIR="$1"
COMMON_CMAKE_ARGS="-G \"Unix Makefiles\" -DENABLE_TESTING:BOOL=ON"

# Note eval is needed so COMMON_CMAKE_ARGS is expanded properly
case $CMS_CONFIG in
    NORMAL)
        eval cmake ${COMMON_CMAKE_ARGS} \
                   ${SOURCE_DIR}
    ;;
    STATIC_BIN)
        eval cmake ${COMMON_CMAKE_ARGS} \
                   -DSTATICCOMPILE:BOOL=ON \
                   ${SOURCE_DIR}
    ;;

    SIMPLE)
        eval cmake ${COMMON_CMAKE_ARGS} \
                   -DSIMPLE:BOOL=ON \
                   ${SOURCE_DIR}
    ;;

    SIMPLE_STATIC)
        eval cmake ${COMMON_CMAKE_ARGS} \
                   -DSIMPLE:BOOL=ON \
                   -DSTATICCOMPILE:BOOL=ON \
                   ${SOURCE_DIR}
    ;;

    MORE_DEBUG)
        eval cmake ${COMMON_CMAKE_ARGS} \
                   -DMORE_DEBUG:BOOL=ON \
                   ${SOURCE_DIR}
    ;;

    NOSTATS)
        eval cmake ${COMMON_CMAKE_ARGS} \
                   -DNOSTATS:BOOL=ON \
                   ${SOURCE_DIR}
    ;;

    NOZLIB)
        eval cmake ${COMMON_CMAKE_ARGS} \
                   -DNOZLIB:BOOL=ON \
                   ${SOURCE_DIR}
    ;;

    RELEASE)
        eval cmake ${COMMON_CMAKE_ARGS} \
                   -DCMAKE_BUILD_TYPE:STRING=Release \
                   ${SOURCE_DIR}
    ;;

    NOSQLITE)
        sudo apt-get remove libsqlite3-dev
        eval cmake ${COMMON_CMAKE_ARGS} \
                   ${SOURCE_DIR}
    ;;

    NOMYSQL)
        sudo apt-get remove libmysqlclient-dev
        eval cmake ${COMMON_CMAKE_ARGS} \
                   ${SOURCE_DIR}
    ;;

    NOMYSQLNOSQLITE)
        sudo apt-get remove libmysqlclient-dev
        sudo apt-get remove libsqlite3-dev
        eval cmake ${COMMON_CMAKE_ARGS} \
                   ${SOURCE_DIR}
    ;;

    NOPYTHON)
        sudo apt-get remove python2.7-dev python-dev
        eval cmake ${COMMON_CMAKE_ARGS} \
                   ${SOURCE_DIR}
    ;;

    *)
        echo "\"${STP_CONFIG}\" configuration not recognised"
        exit 1
esac

exit $?

