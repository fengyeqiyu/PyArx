# 旁路单文件编译验证：include/宏/语言标准全部从 vcxproj 现读，产物只落临时目录。
# 不写 G:\PyArx\X64\Release、不生成 pch、不链接、不跑回归。
# 用法：check_one.bat PyDbViewport.cpp [更多.cpp ...] [-ShowWarnings]
# 说明：不声明 param 块，全部走 $args，避免 PowerShell 按位置绑定把源文件当参数、或弹交互提问卡死。

$ErrorActionPreference = "Stop"
$ShowWarnings = $false
$ShowAll = $false
$Sources = @()
foreach ($a in $args) {
  if ($a -eq '-ShowWarnings' -or $a -eq '-sw') { $ShowWarnings = $true }
  elseif ($a -eq '-ShowAll') { $ShowAll = $true; $ShowWarnings = $true }
  else { $Sources += $a }
}
if ($Sources.Count -eq 0) { throw "用法: check_one.bat <源文件.cpp> [更多.cpp ...] [-ShowWarnings]" }

# 可按需用环境变量覆盖：PYARX_CHECK_PROJ(vcxproj 路径) / PYARX_CHECK_OBJDIR(obj 输出目录)
$Project = if ($env:PYARX_CHECK_PROJ) { $env:PYARX_CHECK_PROJ } else { "G:\PyArx\PyArxCore\PyArxCore.vcxproj" }
$ObjDir = if ($env:PYARX_CHECK_OBJDIR) { $env:PYARX_CHECK_OBJDIR } else { "" }

$projDir = Split-Path -Parent $Project
$proj = [IO.File]::ReadAllText($Project)

function Get-First($pattern) {
  $m = [regex]::Match($proj, $pattern)
  if ($m.Success) { $m.Groups[1].Value } else { "" }
}
$inc = Get-First '<AdditionalIncludeDirectories>([^<]+)<'
$def = Get-First '<PreprocessorDefinitions>([^<]+)<'
$std = Get-First '<LanguageStandard>([^<]+)<'
$opt = Get-First '<AdditionalOptions>([^<]+)<'

$inc = ($inc -replace '%\(AdditionalIncludeDirectories\)', '') -replace '\$\(ProjectDir\)', $projDir
$def = ($def -replace '%\(PreprocessorDefinitions\)', '')
$opt = ($opt -replace '%\(AdditionalOptions\)', '')

# ---- 源文件 ----
$full = @()
foreach ($s in $Sources) {
  $p = if ([IO.Path]::IsPathRooted($s)) { $s } else { Join-Path $projDir $s }
  if (-not (Test-Path -LiteralPath $p)) { throw "源文件不存在: $p" }
  $full += (Resolve-Path -LiteralPath $p).Path
}

if ($ObjDir -eq "") { $ObjDir = Join-Path $env:TEMP 'pyarx_check_obj' }
if (-not (Test-Path $ObjDir)) { New-Item -ItemType Directory -Path $ObjDir -Force | Out-Null }

# ---- 找 VS 并导入 vcvars64 环境（INCLUDE/LIB/PATH），之后直接起 cl ----
$vswhere = 'C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path $vswhere)) { throw "找不到 vswhere，无法定位 MSVC" }
$vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vs) { throw "未找到带 VC 工具链的 Visual Studio" }
$vcvars = Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'
if (-not (Test-Path $vcvars)) { throw "找不到 vcvars64.bat: $vcvars" }

$envDump = & cmd /c ('call "{0}" >nul && set' -f $vcvars)
foreach ($line in $envDump) {
  $i = $line.IndexOf('=')
  if ($i -gt 0) { Set-Item -Path ('Env:' + $line.Substring(0, $i)) -Value $line.Substring($i + 1) -ErrorAction SilentlyContinue }
}
$cl = (Get-Command cl.exe -ErrorAction SilentlyContinue).Source
if (-not $cl) { throw "vcvars64 之后仍找不到 cl.exe" }

# ---- 组装 cl 参数 ----
$clArgs = @('/nologo', '/c')
if ($std) { $clArgs += '/std:' + ($std -replace 'stdcpp', 'c++') }
$clArgs += (($opt -split ' ') | Where-Object { $_ -like '/*' })
$clArgs += (($def -split ';') | Where-Object { $_ } | ForEach-Object { $_.Trim() } |
  Where-Object { $_ -ne 'PyEdModule' } |        # stdafx.h 内已 #define，重复会 C4005
  ForEach-Object { '/D' + $_ })
if ($proj -match '<UseOfMfc>Dynamic<') { $clArgs += '/D_AFXDLL' }   # MFC 共享 dll 必配 /MD
$clArgs += (($inc -split ';') | Where-Object { $_ } | ForEach-Object { '/I' + ($_.Trim() -replace '\\$', '') })
$clArgs += @('/MD', '/bigobj', '/FS')
$clArgs += $full

# ---- 编译（obj 落在 ObjDir，不影响正式构建目录）----
$sw = [Diagnostics.Stopwatch]::StartNew()
Push-Location $ObjDir
try { $out = & $cl @clArgs 2>&1 } finally { Pop-Location }
$code = $LASTEXITCODE
$sw.Stop()

# ---- 汇总：滤掉 ARX 头 gemat3d.h 既有的 C4828 噪声 ----
$keep = $out | Where-Object {
  ($_ -match 'error' -or ($ShowWarnings -and $_ -match 'warning')) -and $_ -notmatch 'C4828'
}
foreach ($f in $full) {
  $obj = Join-Path $ObjDir ([IO.Path]::GetFileNameWithoutExtension($f) + '.obj')
  $ok = Test-Path -LiteralPath $obj
  $size = if ($ok) { (Get-Item -LiteralPath $obj).Length } else { 0 }
  Write-Output ("{0}  {1}  {2}s  obj={3}KB" -f `
    $(if ($code -eq 0 -and $ok) { 'PASS' } else { 'FAIL' }), (Split-Path -Leaf $f), `
    [int]$sw.Elapsed.TotalSeconds, [int]($size / 1024))
}
if ($code -ne 0) {
  if ($keep) { $keep | Select-Object -First 10 | ForEach-Object { Write-Output $_.Trim() } }
  else { $out | Where-Object { $_.Trim() } | Select-Object -Last 6 | ForEach-Object { Write-Output $_.Trim() } }
}
exit $code
