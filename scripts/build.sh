#!/bin/bash -e

ISCLANG=on
COMPILER=clang++
CCOMPILER=clang
LINKER=lld
THREADS=1

# Go to build dir
CURDIR=$(basename $PWD)
if [[ $CURDIR == "scripts" ]]; then
  cd ..
fi
mkdir -p build
mkdir -p build/clang_debug
cd build/clang_debug

export CXX=$COMPILER
export CC=$CCOMPILER

cmake \
  -G"MSYS Makefiles" \
  -DMSYS2_CLANG=$ISCLANG \
  -DCMAKE_BUILD_TYPE=Debug \
  -DJWT_DISABLE_PICOJSON=on \
  -DJWT_BUILD_EXAMPLES=off \
  -DCMAKE_CXX_FLAGS="-fuse-ld=lld" \
  -DCMAKE_CXX_COMPILER=$COMPILER \
  -DCMAKE_C_COMPILER=$CCOMPILER \
  -DCMAKE_LINKER=$LINKER \
  -DCMAKE_CXX_STANDARD=20 \
  ../..

make -j$THREADS