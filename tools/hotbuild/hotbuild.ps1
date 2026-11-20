param(
  [string]$Project = "G:\PyArx\PyArxCore\PyArxCore.vcxproj",
  [string]$Tag = "",
  [switch]$NoRestore
)
$ErrorActionPreference = "Continue"
$msbuild = "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe"
$hotGlobals = "G:\PyArx\PyArxCore\HotModuleName.h"
$hotName = "G:\PyArx\PyArxCore\HotModName.h"
$outDir = "G:\PyArx\X64\Release"
$modTxt = "G:\PyArx\tools\hotbuild\module.txt"

if ($Tag -eq "") { $Tag = (Get-Date).ToString("yyyyMMdd_HHmmss") }
$modName = "PyArxCoreH" + $Tag
Write-Output ("hot module: " + $modName)

$globals = @"
#pragma once

#ifndef PYARX_HOT_INTERNALS
#define PYARX_HOT_INTERNALS "_dev"
#endif

#ifndef PYBIND11_INTERNALS_KIND
#define PYBIND11_INTERNALS_KIND PYARX_HOT_INTERNALS
#endif
"@
# Only rewrite when content differs; a touched force-included header causes a full 9-min rebuild.
$cur = ""
if (Test-Path -LiteralPath $hotGlobals) { $cur = [IO.File]::ReadAllText($hotGlobals) }
if ($cur.Replace("`r`n", "`n").Trim() -ne $globals.Replace("`r`n", "`n").Trim()) {
  [IO.File]::WriteAllText($hotGlobals, $globals, (New-Object Text.UTF8Encoding($false)))
  Write-Output "globals: switched to hot mode (one full rebuild)"
} else {
  Write-Output "globals: unchanged (incremental)"
}

$nameHdr = @"
#pragma once

#ifndef PYARX_HOT_MODULE
#define PYARX_HOT_MODULE $modName
#endif
"@
[IO.File]::WriteAllText($hotName, $nameHdr, (New-Object Text.UTF8Encoding($false)))

$log = "G:\PyArx\tools\hotbuild\build_" + $Tag + ".log"
$sw = [Diagnostics.Stopwatch]::StartNew()
$null = & $msbuild $Project /t:Build /p:Configuration=Release /p:Platform=x64 /p:TargetName=$modName /v:m /nologo *> $log
$code = $LASTEXITCODE
$sw.Stop()
Write-Output ("msbuild exit=" + $code + " in " + [int]$sw.Elapsed.TotalSeconds + "s")

if (-not $NoRestore) {
  $restore = @"
#pragma once

#ifndef PYARX_HOT_MODULE
#define PYARX_HOT_MODULE PyArxCore
#endif
"@
  [IO.File]::WriteAllText($hotName, $restore, (New-Object Text.UTF8Encoding($false)))
}

$pyd = Join-Path $outDir ($modName + ".pyd")
if (Test-Path -LiteralPath $pyd) {
  [IO.File]::WriteAllText($modTxt, $modName, (New-Object Text.UTF8Encoding($false)))
  Write-Output ("pyd: " + $pyd + " bytes=" + (Get-Item -LiteralPath $pyd).Length)
  Write-Output ("MODULE=" + $modName)
} else {
  Write-Output "build failed"
  if (Test-Path -LiteralPath $log) { Select-String -LiteralPath $log -Pattern "error" | Select-Object -First 8 | ForEach-Object { $_.Line.Trim() } }
}
