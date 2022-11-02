# Windows test script for PowerShell

$ErrorActionPreference = "Stop"

Import-Module ".\scripts\helpers.psm1" -Force

SetupVSToolsEnv -architecture amd64

cd build
$env:GTEST_OUTPUT="xml:${PSScriptRoot}/../reports/"
ExecuteOrFail { cmake --build . --target RUN_TESTS --config Debug }
cd ..
