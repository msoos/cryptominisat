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
    STATIC_BIN)
        eval cmake ${COMMON_CMAKE_ARGS} \
                   -DSTATICCOMPILE:BOOL=OFF \
                   ${SOURCE_DIR}
    ;;

    SIMPLE)
        eval cmake ${COMMON_CMAKE_ARGS} \
                   -DSIMPLE=ON \
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

    *)
        echo "\"${STP_CONFIG}\" configuration not recognised"
        exit 1
esac

exit $?

