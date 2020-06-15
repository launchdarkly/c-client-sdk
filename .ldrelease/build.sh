#!/bin/bash

set -uxe

# Releaser runs this script for both the Linux build and the Mac build. If we
# ever need them to be different, we can instead provide linux-build.sh and
# mac-build.sh

mkdir -p build-static && cd build-static
mkdir -p release
cmake -D CMAKE_INSTALL_PREFIX=./release ..
cmake --build .
cmake --build . --target install
cd ..

mkdir -p build-dynamic && cd build-dynamic
mkdir -p release
cmake -D BUILD_TESTING=OFF -D CMAKE_INSTALL_PREFIX=./release ..
cmake --build .
cmake --build . --target install
cd ..
