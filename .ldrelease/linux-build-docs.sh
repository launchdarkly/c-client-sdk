#!/bin/bash

set -e

# This only runs in the Linux build, since the docs are the same for all platforms.

PROJECT_DIR=$(pwd)

doxygen

mkdir -p $PROJECT_DIR/artifacts
cd $PROJECT_DIR/docs/build/html
zip -r $PROJECT_DIR/artifacts/docs.zip *
