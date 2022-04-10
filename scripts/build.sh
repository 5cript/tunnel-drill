#!/bin/bash -e

# Arguments:
#   -b Debug/Release debug or release build, debug if not passed
#   -j Threads to build with, 1 if not passed.

IS_MSYS2_CLANG=off
COMPILER=clang++
CCOMPILER=clang
LINKER=lld
THREADS=1
BUILD_TYPE=Debug

while getopts b:j: opts; do
   case ${opts} in
      b) BUILD_TYPE=${OPTARG} ;;
      j) THREADS=${OPTARG} ;;
   esac
done

# Go to build dir
CURDIR=$(basename $PWD)
if [[ $CURDIR == "scripts" ]]; then
  cd ..
fi
mkdir -p build
mkdir -p "build/${CCOMPILER}_${BUILD_TYPE,,}"
cd "build/${CCOMPILER}_${BUILD_TYPE,,}"

export CXX=$COMPILER
export CC=$CCOMPILER

CMAKE_GENERATOR="Unix Makefiles"
if [[ ! -z "${MSYSTEM}" ]]; then
  CMAKE_GENERATOR="MSYS Makefiles"
  if [[ $COMPILER == clang ]]; then
    IS_MSYS2_CLANG=on
  fi
fi

CMAKE_CXX_FLAGS=""
if [[ $COMPILER == clang ]]; then
  CMAKE_CXX_FLAGS="-fuse-ld=lld"
fi

cmake \
  -G"${CMAKE_GENERATOR}" \
  -DMSYS2_CLANG=$IS_MSYS2_CLANG \
  -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
  -DJWT_DISABLE_PICOJSON=on \
  -DJWT_BUILD_EXAMPLES=off \
  -DCMAKE_CXX_FLAGS=$CMAKE_CXX_FLAGS \
  -DCMAKE_CXX_COMPILER=$COMPILER \
  -DCMAKE_C_COMPILER=$CCOMPILER \
  -DCMAKE_LINKER=$LINKER \
  -DCMAKE_CXX_STANDARD=20 \
  ../..

make -j$THREADS