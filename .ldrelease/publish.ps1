
# Windows test script for PowerShell (just puts artifacts in output directory)

$prefix = $env:LD_LIBRARY_FILE_PREFIX  # set in .ldrelease/config.yml
New-Item -ItemType Directory -Force -Path .\artifacts

$compress = @{
LiteralPath = "ldclientapi.dll", "ldclientapi.lib", "ldapi.h", "uthash.h"
DestinationPath = ".\artifacts\$prefix-dynamic.zip"
}
Compress-Archive @compress
