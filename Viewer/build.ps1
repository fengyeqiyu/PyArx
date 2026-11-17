# Build PyArx Viewer (VS vcvars64 + cl, WebView2 static link)
# usage: powershell -NoProfile -File build.ps1

$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$vcvars = 'C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat'
$inc = Join-Path $root '_sdk\wv2\build\native\include'
$lib = Join-Path $root '_sdk\wv2\build\native\x64'

if (-not (Test-Path -LiteralPath $inc)) { throw "WebView2 SDK missing: $inc" }
if (-not (Test-Path -LiteralPath $vcvars)) { throw "vcvars64.bat missing: $vcvars" }

$cmdline = 'call "' + $vcvars + '" >nul 2>&1 && cd /d ' + $root + ' && cl /nologo /utf-8 /std:c++17 /EHsc /O2 /W3 /DUNICODE /D_UNICODE /I "' + $inc + '" src\main.cpp /Fe:PyArxViewer.exe /link /SUBSYSTEM:WINDOWS /LIBPATH:"' + $lib + '" WebView2LoaderStatic.lib user32.lib gdi32.lib ole32.lib shell32.lib comdlg32.lib advapi32.lib dwmapi.lib'

cmd /c $cmdline
if ($LASTEXITCODE -ne 0) { throw "build failed, exit code $LASTEXITCODE" }

$exe = Join-Path $root 'PyArxViewer.exe'
Write-Host ("built " + $exe + "  " + [math]::Round((Get-Item -LiteralPath $exe).Length / 1024) + " KB")
