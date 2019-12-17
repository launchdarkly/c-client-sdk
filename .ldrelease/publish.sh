#!/bin/bash

set -e

# Releaser runs this script for both the Linux build and the Mac build. If we ever need them
# to be different in more ways than just the filenames, we can instead provide linux-publish.sh
# and mac-publish.sh

# Note that we aren't publishing anything to a remote service, just putting the files where
# Releaser will pick them up and attach them to the release.

# Note: LD_LIBRARY_FILE_PREFIX is set in .ldrelease/config.yml

mkdir -p artifacts

HEADERS="ldapi.h uthash.h"

zip artifacts/${LD_LIBRARY_FILE_PREFIX}-static.zip libldclientapi.a ${HEADERS}
zip artifacts/${LD_LIBRARY_FILE_PREFIX}-dynamic.zip libldclientapi.so ${HEADERS}
zip artifacts/${LD_LIBRARY_FILE_PREFIX}-dynamic-cpp.zip libldclientapiplus.so ${HEADERS}

tar -cf artifacts/${LD_LIBRARY_FILE_PREFIX}-static.tar libldclientapi.a ${HEADERS}
tar -cf artifacts/${LD_LIBRARY_FILE_PREFIX}-dynamic.tar libldclientapi.so ${HEADERS}
tar -cf artifacts/${LD_LIBRARY_FILE_PREFIX}-dynamic-cpp.tar libldclientapiplus.so ${HEADERS}
