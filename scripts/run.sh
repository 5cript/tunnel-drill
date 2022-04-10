#!/bin/bash -e

CCOMPILER=clang
BUILD_TYPE=Debug

CURDIR=$(basename $PWD)
if [[ $CURDIR == "scripts" ]]; then
  cd ..
fi

EXE=broker
if [[ ! -z "${MSYSTEM}" ]]; then
  EXE=broker.exe
fi

bash ./scripts/build.sh && \
    "./build/${CCOMPILER}_${BUILD_TYPE,,}/bin/${EXE}"