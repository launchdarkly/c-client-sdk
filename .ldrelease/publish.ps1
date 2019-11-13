
# Windows test script for PowerShell (just puts artifacts in output directory)

$prefix = $env:LD_LIBRARY_FILE_PREFIX  # set in .ldrelease/config.yml
New-Item -ItemType Directory -Force -Path .\artifacts
cp ldclientapi.dll ".\artifacts\$prefix-ldclientapi.dll"
cp ldclientapi.lib ".\artifacts\$prefix-ldclientapi.lib"
