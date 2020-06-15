#!/bin/bash

set -e

# Releaser runs this script for both the Linux build and the Mac build. If we
# ever need them to be different, we can instead provide linux-test.sh and
# mac-test.sh

cd build-static
CTEST_OUTPUT_ON_FAILURE=1 make test
