#!/bin/bash -e

COMPILER=clang++
CCOMPILER=clang
LINKER=lld

# Go to build dir
CURDIR=$(basename $PWD)
if [[ $CURDIR == "scripts" ]]; then
  cd ..
fi
cd build

export CXX=$COMPILER
export CC=$CCOMPILER

cmake -DCMAKE_CXX_COMPILER=$COMPILER -DCMAKE_C_COMPILER=$CCOMPILER -DCMAKE_LINKER=$LINKER ..
make -j2