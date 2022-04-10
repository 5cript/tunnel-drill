#!/bin/bash -e

CURDIR=$(basename $PWD)
if [[ $CURDIR == "scripts" ]]; then
  cd ..
fi

bash ./scripts/build.sh && \
gdb ./build/clang_debug/bin/broker.exe