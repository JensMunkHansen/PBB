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

# Define configurations
libconfigs=(
  "HeaderOnly:-DPBB_LIBRARY_TYPE=INTERFACE"
  "Static:-DPBB_LIBRARY_TYPE=STATIC"
  "Shared:-DPBB_LIBRARY_TYPE=SHARED"
)

configs=(
    #"Release"
    #"Debug"
    "Asan"
)

compilers=(
    "clang:clang++"
    "gcc:g++")


if [ -d "$HOME/tspkg/ArtifactoryInstall/Linux/Release/lib/cmake/Catch2" ]; then
    Catch2_DIR="$HOME/tspkg/ArtifactoryInstall/Linux/Release/lib/cmake/Catch2"
fi

for compiler in "${compilers[@]}"; do
  IFS=":" read -r cc cxx <<< "$compiler"
  for libconfig in "${libconfigs[@]}"; do
    IFS=":" read -r name cmake_args <<< "$libconfig"
    echo "==== Building $name Configuration ===="
  
    # Configure
    bear_execute "cmake -G Ninja -S . -B $(pwd)/build/$cc/$name $cmake_args -DCMAKE_C_COMPILER=$cc -DCMAKE_CXX_COMPILER=$cxx"
    for config in "${configs[@]}"; do
        bear_execute "cmake --build build/$cc/$name --config $config"
        ctest --test-dir $(pwd)/build/$cc/$name -C $config
    done
  done
done
