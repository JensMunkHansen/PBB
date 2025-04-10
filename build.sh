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
#  "HeaderOnly:-DPBB_LIBRARY_TYPE=INTERFACE"
  "Static:-DPBB_LIBRARY_TYPE=STATIC"
#  "Shared:-DPBB_LIBRARY_TYPE=SHARED"
)

configs=(
#    "Release"
    "Debug"
#         "Asan"
)

if [ -d "$HOME/tspkg/ArtifactoryInstall/Linux/Release/lib/cmake/Catch2" ]; then
    Catch2_DIR="$HOME/tspkg/ArtifactoryInstall/Linux/Release/lib/cmake/Catch2"
fi

for libconfig in "${libconfigs[@]}"; do
  IFS=":" read -r name cmake_args <<< "$libconfig"
  echo "==== Building $name Configuration ===="

  # Configure
  bear_execute "cmake --preset linux -B $(pwd)/build/$name $cmake_args -DCatch2_DIR=$Catch2_DIR"

  for config in "${configs[@]}"; do
      bear_execute "cmake --build build/$name --config $config"
      ctest --test-dir "$(pwd)/build/$name" -C $config --output-on-failure
  done
done
