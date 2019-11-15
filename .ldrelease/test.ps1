
# Windows test script for PowerShell

$ErrorActionPreference = "Stop"

Import-Module ".\.ldrelease\helpers.psm1" -Force

# test code was already built by build script
ExecuteOrFail { .\test }
