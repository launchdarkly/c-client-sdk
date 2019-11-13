
# Windows build script for PowerShell

$ProgressPreference = "SilentlyContinue"  # prevents console errors from CircleCI host

$vcBaseDir = "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC"
$env:Path += ";$vcBaseDir\Common7\Tools"

Write-Host Downloading curl
iwr -outf curl.zip https://curl.haxx.se/download/curl-7.59.0.zip
unzip curl.zip | Out-Null

Write-Host
Write-Host Setting environment

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
Invoke-CmdScript "$vcBaseDir\Auxiliary\Build\vcvarsall.bat" amd64

Write-Host
Write-Host Running nmake
nmake /f Makefile.win
