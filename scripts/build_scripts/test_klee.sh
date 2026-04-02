#!/usr/bin/env bash

set -euo pipefail

rm -rf cm* CM* lib* cryptomini* Testing* tests* pycryptosat include
rm -rf tests

CXX=/home/soos/development/smt/klee/scripts/klee-clang cmake -DNOZLIB=ON  -DENABLE_PYTHON_INTERFACE=OFF -DUSE_PTHREADS=OFF -Dcadical_DIR=../../cadical/build -Dcadiback_DIR=../../cadiback ..
make -j$(nproc) VERBOSE=1

lvm-link-3.4 cryptominisat5_simple /home/soos/development/smt/libcxx/build/lib/libc++.so.1.0 -o out
klee --libc=uclibc --posix-runtime out

# /home/soos/development/smt/klee/scripts/klee-clang -DKLEE -std=c++11 -I../ ../tests/distill_klee.cpp -o klee.o
# llvm-link-3.4 lib/libcryptominisat5.so /home/soos/development/smt/libcxx/build/lib/libc++.so.1.0 klee.o -o out
# klee --libc=uclibc --posix-runtime out
