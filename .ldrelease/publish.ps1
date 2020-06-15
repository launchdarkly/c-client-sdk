# Windows release script for PowerShell

$ErrorActionPreference = "Stop"

# prevents console errors from CircleCI host
$global:ProgressPreference = "SilentlyContinue"

# the built in powershell zip seems to be horribly broken when working with
# nested directories
choco install zip

$prefix = $env:LD_LIBRARY_FILE_PREFIX  # set in .ldrelease/config.yml
New-Item -ItemType Directory -Force -Path .\artifacts

cd "build-static/release"
zip -r "../../artifacts/${prefix}-static.zip" *
cd ../..

cd "build-dynamic/release"
zip -r "../../artifacts/${prefix}-dynamic.zip" *
cd ../..
