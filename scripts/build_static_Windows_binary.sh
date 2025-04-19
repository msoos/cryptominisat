#!/usr/bin/env bash
# Run this script from 'ucrt64.exe' MSYS2 shell

set -o xtrace -o errexit -o nounset -o pipefail

readonly currentScriptDir=`dirname "$(realpath -s "${BASH_SOURCE[0]}")"`
readonly buildDir="${currentScriptDir}/../build"

# Install prerequisites (the prerequisites are installed by Github Actions)
# pacman --sync --sysupgrade --refresh --noconfirm
# pacman --sync --needed --noconfirm \
#   mingw-w64-ucrt-x86_64-toolchain base-devel cmake git zlib gmp-devel msys2-runtime-devel ninja

# Create build directory
rm -rf "${buildDir}"
mkdir "${buildDir}"

# Build Cadical
git clone \
  --branch mate-only-libraries-1.8.0 \
  --depth 1 \
  https://github.com/meelgroup/cadical \
  "${buildDir}/cadical"
( cd "${buildDir}/cadical"
./configure -static
make -j `nproc` cadical
)

# Build Cadiback
git clone \
  --branch synthesis \
  --depth 1 \
  https://github.com/meelgroup/cadiback \
  "${buildDir}/cadiback"
( cd "${buildDir}/cadiback"
./configure
make -j `nproc` libcadiback.a
)

# Build Cryptominisat
git clone \
  --depth 1 \
  https://github.com/msoos/cryptominisat \
  "${buildDir}/cryptominisat"
cmake \
  -S "${buildDir}/cryptominisat" \
  -B "${buildDir}/cryptominisat/build" \
  -G Ninja \
  -D CMAKE_BUILD_TYPE=Release \
  -D GMP_LIBRARY=/ucrt64/lib/libgmp.a \
  -D GMPXX_LIBRARY=/ucrt64/lib/libgmpxx.a \
  -D STATICCOMPILE=ON \
  -D CMAKE_EXE_LINKER_FLAGS="-static -static-libgcc -static-libstdc++"
cmake --build "${buildDir}/cryptominisat/build"

echo "Successfully built '${buildDir}/cryptominisat/build/cryptominisat5.exe'"
