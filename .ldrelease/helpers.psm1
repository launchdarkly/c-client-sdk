
# PowerShell helper functions for Windows build. These can't be built into Releaser because we also
# need to use them for our regular CI job.

$ProgressPreference = "SilentlyContinue"  # prevents console errors from CircleCI host

$vcBaseDir = "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC"
$env:Path += ";$vcBaseDir\Common7\Tools"

# This helper function is based on https://github.com/psake/psake - it allows us to terminate the
# script if any external command (like "aws" or "dotnet", rather than a PowerShell cmdlet) returns
# an error code, which PowerShell won't otherwise do.
function ExecuteOrFail {
    [CmdletBinding()]
    param(
        [Parameter(Position=0,Mandatory=1)][scriptblock]$cmd,
        [Parameter(Position=1,Mandatory=0)][string]$errorMessage = ("Error executing command {0}" -f $cmd)
    )
    & $cmd
    if ($lastexitcode -ne 0) {
        throw ($errorMessage)
    }
}

function DownloadAndUnzip {
    param(
        [Parameter(Mandatory)][string]$url,
        [Parameter(Mandatory)][string]$filename
    )
    Write-Host Downloading and expanding $url
    ExecuteOrFail { iwr -outf $filename $url }
    ExecuteOrFail { unzip $filename | Out-Null }
}

# Using vcvarsall.bat from PowerShell is not straightforward - see
# https://stackoverflow.com/questions/41399692/running-a-build-script-after-calling-vcvarsall-bat-from-powershell
# Invokes a Cmd.exe shell script and updates the environment.
function Invoke-CmdScript {
  param(
    [String] $scriptName
  )
  $cmdLine = """$scriptName"" $args & set"
  & $Env:SystemRoot\system32\cmd.exe /c $cmdLine |
  select-string '^([^=]*)=(.*)$' | foreach-object {
    $varName = $_.Matches[0].Groups[1].Value
    $varValue = $_.Matches[0].Groups[2].Value
    set-item Env:$varName $varValue
  }
}

function SetupVSToolsEnv {
    param(
        [Parameter(Mandatory)][string]$architecture
    )
    Write-Host "Setting environment for $architecture"
    Invoke-CmdScript "$vcBaseDir\Auxiliary\Build\vcvarsall.bat" $architecture
}
