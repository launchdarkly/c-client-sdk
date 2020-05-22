#!/bin/bash

set -e

# Adds extra packages prior to build on Mac

HOMEBREW_NO_AUTO_UPDATE=1 brew install cmake
