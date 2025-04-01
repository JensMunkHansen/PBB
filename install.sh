#!/bin/bash

set -e

# Check if `bear` is installed
if command -v bear &>/dev/null; then
    BEAR_CMD="bear -- sh -c"
else
    BEAR_CMD="sh -c"
fi

# Wrapper function to run commands with or without `bear`
bear_execute() {
    $BEAR_CMD "$1"
}

cmake --preset linux -DCMAKE_INSTALL_PREFIX=$(pwd)/install -DPBB_LIBRARY_TYPE=SHARED
cmake --build build/linux --config Release --target install
