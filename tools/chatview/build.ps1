# 编译 PyArx 对话窗（MFC 静态 + WebView2 静态链接）
# usage: powershell -NoProfile -File build.ps1

$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$vcvars = 'C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat'
$wv2inc = 'G:\PyArx\viewer\_sdk\wv2\build\native\include'
$wv2lib = 'G:\PyArx\viewer\_sdk\wv2\build\native\x64'
$msvcRoot = 'C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC'

if (-not (Test-Path -LiteralPath $vcvars)) { throw "vcvars64.bat missing: $vcvars" }
if (-not (Test-Path -LiteralPath $wv2inc)) { throw "WebView2 SDK missing: $wv2inc" }

$ver = (Get-ChildItem -LiteralPath $msvcRoot -Directory | Sort-Object Name -Descending |
        Select-Object -First 1).Name
$atlmfc = Join-Path $msvcRoot "$ver\atlmfc\lib\x64"
if (-not (Test-Path -LiteralPath $atlmfc)) { throw "MFC lib missing: $atlmfc" }

$cmdline = 'call "' + $vcvars + '" >nul 2>&1 && cd /d ' + $root +
  ' && cl /nologo /utf-8 /std:c++17 /EHsc /O2 /W3 /MT /DUNICODE /D_UNICODE /D_WIN32_WINNT=0x0A00' +
  ' /I "' + $wv2inc + '" src\main.cpp /Fe:PyArxChat.exe' +
  ' /link /SUBSYSTEM:WINDOWS /ENTRY:wWinMainCRTStartup "/LIBPATH:' + $wv2lib + '" "/LIBPATH:' + $atlmfc + '"' +
  ' WebView2LoaderStatic.lib uafxcw.lib user32.lib gdi32.lib ole32.lib shell32.lib' +
  ' comdlg32.lib advapi32.lib dwmapi.lib oleaut32.lib uuid.lib'

cmd /c $cmdline
if ($LASTEXITCODE -ne 0) { throw "build failed, exit code $LASTEXITCODE" }

$exe = Join-Path $root 'PyArxChat.exe'
Write-Host ("built " + $exe + "  " + [math]::Round((Get-Item -LiteralPath $exe).Length / 1024) + " KB")
