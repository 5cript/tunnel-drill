#!/bin/bash -e

COMPILER=g++
CCOMPILER=gcc
LINKER=lld

# Go to build dir
CURDIR=$(basename $PWD)
if [[ $CURDIR == "scripts" ]]; then
  cd ..
fi
mkdir -p build
cd build

export CXX=$COMPILER
export CC=$CCOMPILER

cmake \
  -G"MSYS Makefiles" \
  -DCMAKE_CXX_COMPILER=$COMPILER \
  -DCMAKE_C_COMPILER=$CCOMPILER \
  -DCMAKE_LINKER=$LINKER \
  -DCMAKE_CXX_STANDARD=20 \
  ..
make -j2