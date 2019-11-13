#!/bin/bash

set -e

TARGET_FILE=ldapi.h
TEMP_FILE=${TARGET_FILE}.tmp

sed "s/^#define LD_SDK_VERSION .*/#define LD_SDK_VERSION \"${LD_RELEASE_VERSION}\"/" "${TARGET_FILE}" > "${TEMP_FILE}"
mv "${TEMP_FILE}" "${TARGET_FILE}"
