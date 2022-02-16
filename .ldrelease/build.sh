#!/bin/bash

set -uxe

# Releaser runs this script for both the Linux build and the Mac build. If we ever need them
# to be different, we can instead provide linux-build.sh and mac-build.sh

# LD_LIBRARY_FILE_PREFIX is set in the environment variables for each build job in
# .ldrelease/config.yml

base=$(pwd)

mkdir -p build-static && cd build-static
mkdir -p release

if [ $# -ge 1 ] && [ -n "$1" ]; then
  cmake -D EXTRA_COMPILE_OPTIONS="$1" -D CMAKE_INSTALL_PREFIX=./release ..
else
  cmake -D CMAKE_INSTALL_PREFIX=./release ..
fi
cmake --build .
cmake --build . --target install
cd ..

mkdir -p build-dynamic && cd build-dynamic
mkdir -p release
cmake -D BUILD_TESTING=OFF -D BUILD_SHARED_LIBS=ON -D CMAKE_INSTALL_PREFIX=./release ..
cmake --build .
cmake --build . --target install
cd ..

# Copy the binary archives to the artifacts directory; Releaser will find them
# there and attach them to the release (this also makes the artifacts available
# in dry-run mode)

# Check if the Releaser variable exists because this script could also be called
# from CI
if [ -n "${LD_RELEASE_ARTIFACTS_DIR:-}" ]; then
  cd "${base}/build-static/release"
  zip -r "${LD_RELEASE_ARTIFACTS_DIR}/${LD_LIBRARY_FILE_PREFIX}-static.zip" *
  tar -cf "${LD_RELEASE_ARTIFACTS_DIR}/${LD_LIBRARY_FILE_PREFIX}-static.tar" *

  cd "${base}/build-dynamic/release"
  zip -r "${LD_RELEASE_ARTIFACTS_DIR}/${LD_LIBRARY_FILE_PREFIX}-dynamic.zip" *
  tar -cf "${LD_RELEASE_ARTIFACTS_DIR}/${LD_LIBRARY_FILE_PREFIX}-dynamic.tar" *
fi
