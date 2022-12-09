#!/bin/bash

set -e

# This only runs in the Linux build, since the docs are the same for all platforms.

PROJECT_DIR=$(pwd)

doxygen

cp -r "${PROJECT_DIR}"/docs/build/html/* "${LD_RELEASE_DOCS_DIR}"
