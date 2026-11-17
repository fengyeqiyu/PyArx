<#
set_autoload.ps1 - make AutoCAD auto-load the PyArx plugin (PyArxEngine.arx) at startup.

How it works: AutoCAD scans, on startup,
  HKCU\Software\Autodesk\AutoCAD\<version>\<ACAD-xxxx:xxx>\Applications\<name>
Each entry needs LOADER (full path to arx/dbx/dll), LOADCTRLS (0x2 = load with AutoCAD),
MANAGED (0 for native ARX).

Usage:
  powershell -NoProfile -File set_autoload.ps1            # install / update (idempotent)
  powershell -NoProfile -File set_autoload.ps1 -Status    # report current state only
  powershell -NoProfile -File set_autoload.ps1 -Remove    # remove the autoload entry
  powershell -NoProfile -File set_autoload.ps1 -Loader "D:\x\Foo.arx" -Name "Foo"
#>
param(
  [string]$Loader = "G:\PyArx\X64\Release\PyArxEngine.arx",
  [string]$Name = "PyArx",
  [string]$ProfileKey = "",
  [switch]$Status,
  [switch]$Remove
)

$ErrorActionPreference = "Stop"

function Get-Profiles {
  $out = @()
  $roots = Get-ChildItem -Path "HKCU:\Software\Autodesk\AutoCAD" -ErrorAction SilentlyContinue
  foreach ($r in $roots) {
    # HKCU\...\AutoCAD\R24.3\ACAD-7101:804\Applications
    $vers = Get-ChildItem -Path $r.PSPath -ErrorAction SilentlyContinue
    foreach ($v in $vers) {
      $cands = @()
      if ($v.PSChildName -like "ACAD-*") { $cands += $v }
      else {
        $cands += Get-ChildItem -Path $v.PSPath -ErrorAction SilentlyContinue |
                  Where-Object { $_.PSChildName -like "ACAD-*" }
      }
      foreach ($c in $cands) {
        if (Test-Path -LiteralPath (Join-Path $c.PSPath "Applications")) { $out += $c }
      }
    }
  }
  return $out
}

if (-not $ProfileKey) {
  $profiles = Get-Profiles
  if (@($profiles).Count -eq 0) { throw "No AutoCAD profile found (..\ACAD-xxxx:xxx\Applications)" }
  $ProfileKey = @($profiles)[0].PSPath
}

$target = Join-Path $ProfileKey ("Applications\" + $Name)

if ($Status -or $Remove) {
  if (Test-Path -LiteralPath $target) {
    $item = Get-ItemProperty -LiteralPath $target
    $loader = $item.LOADER
    $exists = $false
    if ($loader) { $exists = Test-Path -LiteralPath ([Environment]::ExpandEnvironmentVariables($loader)) }
    [ordered]@{
      profile = $ProfileKey; name = $Name; installed = $true
      loader = $loader; loaderExists = $exists
      loadctrls = $item.LOADCTRLS; managed = $item.MANAGED
    } | ConvertTo-Json -Compress
  } else {
    [ordered]@{ profile = $ProfileKey; name = $Name; installed = $false } | ConvertTo-Json -Compress
  }
  if ($Remove -and (Test-Path -LiteralPath $target)) {
    Remove-Item -LiteralPath $target -Recurse -Force
    "removed: " + $target
  }
  return
}

if (-not (Test-Path -LiteralPath $Loader)) { throw "LOADER not found: $Loader" }

$appsRoot = Join-Path $ProfileKey "Applications"
if (-not (Test-Path -LiteralPath $appsRoot)) { New-Item -Path $appsRoot -Force | Out-Null }
if (-not (Test-Path -LiteralPath $target)) { New-Item -Path $target -Force | Out-Null }

New-ItemProperty -LiteralPath $target -Name "DESCRIPTION" -Value "PyArx Python bridge (PyArxEngine.arx)" -PropertyType String -Force | Out-Null
New-ItemProperty -LiteralPath $target -Name "LOADER"      -Value $Loader -PropertyType String -Force | Out-Null
New-ItemProperty -LiteralPath $target -Name "LOADCTRLS"   -Value 2 -PropertyType DWord -Force | Out-Null
New-ItemProperty -LiteralPath $target -Name "MANAGED"     -Value 0 -PropertyType DWord -Force | Out-Null

[ordered]@{ ok = $true; profile = $ProfileKey; key = $target; loader = $Loader; loadctrls = 2; managed = 0 } | ConvertTo-Json -Compress
