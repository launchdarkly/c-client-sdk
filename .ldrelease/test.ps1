# Windows test script for PowerShell

$ErrorActionPreference = "Stop"

Import-Module ".\.ldrelease\helpers.psm1" -Force

SetupVSToolsEnv -architecture amd64

cd build-static-release
$env:GTEST_OUTPUT="xml:${PSScriptRoot}/../reports/"
ExecuteOrFail { cmake --build . --target RUN_TESTS --config Release }
cd ..