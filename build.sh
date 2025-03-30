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
#  "Static:-DBUILD_SHARED_LIBS=OFF -DPBB_HEADER_ONLY=OFF"
#  "Shared:-DBUILD_SHARED_LIBS=ON -DPBB_HEADER_ONLY=OFF"
)

configs=("Release"
#         "Debug"
#         "Asan"
)

for libconfig in "${libconfigs[@]}"; do
  IFS=":" read -r name cmake_args <<< "$libconfig"
  echo "==== Building $name Configuration ===="

  # Configure
  bear_execute "cmake --preset linux -B $(pwd)/build/$name $cmake_args"

  for config in "${configs[@]}"; do
      bear_execute "cmake --build build/$name --config $config"
      ctest --preset core-test --test-dir $(pwd)/build/$name -C $config
  done
done
