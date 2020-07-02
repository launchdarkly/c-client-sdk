# Windows test script for PowerShell

$ErrorActionPreference = "Stop"

Import-Module ".\.ldrelease\helpers.psm1" -Force

SetupVSToolsEnv -architecture amd64

cd build-static
ExecuteOrFail { cmake --build . --target RUN_TESTS --config Release }
