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
  "HeaderOnly:-DBUILD_SHARED_LIBS=OFF -DPBB_HEADER_ONLY=ON"
  "Static:-DBUILD_SHARED_LIBS=OFF -DPBB_HEADER_ONLY=OFF"
  "Shared:-DBUILD_SHARED_LIBS=ON -DPBB_HEADER_ONLY=OFF"
)

configs=("Release"
         "Debug"
         "Asan")

compilers=("clang:clang++"
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
        if [[ "$cc" == gcc && "$config" == "Asan" ]]; then
            echo "Skipping GCC with Asan: not supported"
            continue
        fi        
        bear_execute "cmake --build build/$cc/$name --config $config"
        ctest --preset core-test --test-dir $(pwd)/build/$cc/$name -C $config
    done
  done
done
