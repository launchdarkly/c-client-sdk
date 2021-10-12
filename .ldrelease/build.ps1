# Windows build script for PowerShell

$ErrorActionPreference = "Stop"

$global:ProgressPreference = "SilentlyContinue"  # prevents console errors from CircleCI host

# The built in powershell zip seems to be horribly broken when working with nested directories.
# We only need this if we're doing a release as opposed to just building in CI
if ("${env:LD_RELEASE_ARTIFACTS_DIR}" -ne "") {
    choco install zip    
}

Import-Module ".\.ldrelease\helpers.psm1" -Force

SetupVSToolsEnv -architecture amd64

DownloadAndUnzip -url "https://curl.haxx.se/download/curl-7.59.0.zip" -filename "curl.zip"

Write-Host
Write-Host Building curl
Push-Location
cd curl-7.59.0/winbuild
ExecuteOrFail { nmake /f Makefile.vc mode=static BUILD=release }
Pop-Location

Write-Host
Write-Host Building SDK statically release
Push-Location
New-Item -ItemType Directory -Force -Path ".\build-static-release"
cd "build-static-release"
New-Item -ItemType Directory -Force -Path artifacts
ExecuteOrFail {
    cmake -G "Visual Studio 16 2019" -A x64 `
        -D CMAKE_INSTALL_PREFIX="${PSScriptRoot}/../build-static-release/artifacts" `
        -D CURL_LIBRARY="${PSScriptRoot}/../curl-7.59.0/builds/libcurl-vc-x64-release-static-ipv6-sspi-winssl/lib/libcurl_a.lib" `
        -D CURL_INCLUDE_DIR="${PSScriptRoot}/../curl-7.59.0/builds/libcurl-vc-x64-release-static-ipv6-sspi-winssl/include" `
        ..
}
ExecuteOrFail { cmake --build . --config Release }
ExecuteOrFail { cmake --build . --target install --config Release }
Pop-Location

Write-Host
Write-Host Building SDK dynamically release
Push-Location
New-Item -ItemType Directory -Force -Path ".\build-dynamic-release"
cd "build-dynamic-release"
New-Item -ItemType Directory -Force -Path artifacts
ExecuteOrFail {
    cmake -G "Visual Studio 16 2019" -A x64 `
        -D BUILD_TESTING=OFF `
        -D BUILD_SHARED_LIBS=ON `
        -D CMAKE_INSTALL_PREFIX="${PSScriptRoot}/../build-dynamic-release/artifacts" `
        -D CURL_LIBRARY="${PSScriptRoot}/../curl-7.59.0/builds/libcurl-vc-x64-release-static-ipv6-sspi-winssl/lib/libcurl_a.lib" `
        -D CURL_INCLUDE_DIR="${PSScriptRoot}/../curl-7.59.0/builds/libcurl-vc-x64-release-static-ipv6-sspi-winssl/include" `
        ..
}
ExecuteOrFail { cmake --build . --config Release }
ExecuteOrFail { cmake --build . --target install --config Release }
Pop-Location

Write-Host
Write-Host Building SDK statically debug
Push-Location
New-Item -ItemType Directory -Force -Path ".\build-static-debug"
cd "build-static-debug"
New-Item -ItemType Directory -Force -Path artifacts
ExecuteOrFail {
    cmake -G "Visual Studio 16 2019" -A x64 `
        -D CMAKE_INSTALL_PREFIX="${PSScriptRoot}/../build-static-debug/artifacts" `
        -D CURL_LIBRARY="${PSScriptRoot}/../curl-7.59.0/builds/libcurl-vc-x64-release-static-ipv6-sspi-winssl/lib/libcurl_a.lib" `
        -D CURL_INCLUDE_DIR="${PSScriptRoot}/../curl-7.59.0/builds/libcurl-vc-x64-release-static-ipv6-sspi-winssl/include" `
        ..
}
ExecuteOrFail { cmake --build . --config Debug }
ExecuteOrFail { cmake --build . --target install --config Debug }
Pop-Location

Write-Host
Write-Host Building SDK dynamically debug
Push-Location
New-Item -ItemType Directory -Force -Path ".\build-dynamic-debug"
cd "build-dynamic-debug"
New-Item -ItemType Directory -Force -Path artifacts
ExecuteOrFail {
    cmake -G "Visual Studio 16 2019" -A x64 `
        -D BUILD_TESTING=OFF `
        -D BUILD_SHARED_LIBS=ON `
        -D CMAKE_INSTALL_PREFIX="${PSScriptRoot}/../build-dynamic-debug/artifacts" `
        -D CURL_LIBRARY="${PSScriptRoot}/../curl-7.59.0/builds/libcurl-vc-x64-release-static-ipv6-sspi-winssl/lib/libcurl_a.lib" `
        -D CURL_INCLUDE_DIR="${PSScriptRoot}/../curl-7.59.0/builds/libcurl-vc-x64-release-static-ipv6-sspi-winssl/include" `
        ..
}
ExecuteOrFail { cmake --build . --config Debug }
ExecuteOrFail { cmake --build . --target install --config Debug }
Pop-Location

# Copy the binary archives to the artifacts directory; Releaser will find them
# there and attach them to the release (this also makes the artifacts available
# in dry-run mode)

# Check if the Releaser variable exists because this script could also be called
# from CI
if ("${env:LD_RELEASE_ARTIFACTS_DIR}" -ne "") {
    $prefix = $env:LD_LIBRARY_FILE_PREFIX  # set in .ldrelease/config.yml

    cd "build-static-release/artifacts"
    zip -r "${env:LD_RELEASE_ARTIFACTS_DIR}/${prefix}-static-release.zip" *
    cd ../..

    cd "build-dynamic-release/artifacts"
    zip -r "${env:LD_RELEASE_ARTIFACTS_DIR}/${prefix}-dynamic-release.zip" *
    cd ../..

    cd "build-static-debug/artifacts"
    zip -r "${env:LD_RELEASE_ARTIFACTS_DIR}/${prefix}-static-debug.zip" *
    cd ../..

    cd "build-dynamic-debug/artifacts"
    zip -r "${env:LD_RELEASE_ARTIFACTS_DIR}/${prefix}-dynamic-debug.zip" *
    cd ../..
}
