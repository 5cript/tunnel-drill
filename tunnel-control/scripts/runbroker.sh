#!/bin/bash

if [[ ! -z "${MSYSTEM}" ]]; then
    BROKER="../build/clang_debug/bin/broker.exe"
else
    BROKER="../build/clang_debug/bin/broker"
fi

#eval "gdbserver :12772 ${BROKER} --served-directory ${PWD}/public"
eval "${BROKER} --served-directory ${PWD}/public"