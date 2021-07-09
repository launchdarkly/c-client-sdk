# Windows test script for PowerShell

$ErrorActionPreference = "Stop"

Import-Module ".\scripts\helpers.psm1" -Force

SetupVSToolsEnv -architecture amd64

cd build
ExecuteOrFail { cmake --build . --target RUN_TESTS --config Debug }
