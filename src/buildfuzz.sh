#!/bin/bash

set -e

COV=" -fsanitize-coverage=edge,indirect-calls,8bit-counters"

clang++   -DBOOST_TEST_DYN_LINK -DUSE_VALGRIND -DUSE_ZLIB $COV  -fsanitize=address -Wno-bitfield-constant-conversion -Wshadow -Wextra-semi -Wdeprecated -std=c++11 -pthread -g -Wall -Wextra -Wunused -pedantic -Wsign-compare -Wtype-limits -Wuninitialized -Wno-deprecated -Wstrict-aliasing -Wpointer-arith -fvisibility=hidden -Wpointer-arith -Wformat-nonliteral -Winit-self -Wparentheses -Wunreachable-code -ggdb3 -fPIC -O2 -mtune=native -I/usr/include/valgrind -I/home/soos/development/sat_solvers/cryptominisat -I/home/soos/development/sat_solvers/cryptominisat/build/cmsat4-src    -o fuzz.o -c fuzz.cpp

clang++ -g -fsanitize=address -Wl,--whole-archive ../build/lib/libcryptominisat4.a -Wl,-no-whole-archive ../scripts/libfuzz/Fuzzer*.o fuzz.o -o fuzz
