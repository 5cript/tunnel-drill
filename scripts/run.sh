#!/bin/bash -e

# See build.sh for arguments

CCOMPILER=clang
BUILD_TYPE=Debug

while getopts b: opts; do
   case ${opts} in
      b) BUILD_TYPE=${OPTARG} ;;
   esac
done

CURDIR=$(basename $PWD)
if [[ $CURDIR == "scripts" ]]; then
  cd ..
fi

EXE=broker
if [[ ! -z "${MSYSTEM}" ]]; then
  EXE=broker.exe
fi

bash ./scripts/build.sh "$@" && \
    "./build/${CCOMPILER}_${BUILD_TYPE,,}/bin/${EXE}"