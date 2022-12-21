# Windows build script for PowerShell

$ErrorActionPreference = "Stop"

Import-Module ".\scripts\helpers.psm1" -Force

SetupVSToolsEnv -architecture amd64

DownloadAndUnzip -url "https://curl.haxx.se/download/curl-7.59.0.zip" -filename "curl.zip"

Write-Host
Write-Host Building curl
Push-Location
cd curl-7.59.0/winbuild
ExecuteOrFail { nmake /f Makefile.vc mode=static BUILD=release }
Pop-Location

Write-Host
Write-Host Building Common statically debug
Push-Location
New-Item -ItemType Directory -Force -Path ".\build"
cd "build"
ExecuteOrFail {
    cmake -G "Visual Studio 16 2019" -A x64 `
        -D CURL_LIBRARY="${PSScriptRoot}/../curl-7.59.0/builds/libcurl-vc-x64-release-static-ipv6-sspi-winssl/lib/libcurl_a.lib" `
        -D CURL_INCLUDE_DIR="${PSScriptRoot}/../curl-7.59.0/builds/libcurl-vc-x64-release-static-ipv6-sspi-winssl/include" `
        ..
}
ExecuteOrFail { cmake --build . --config Debug }
Pop-Location
