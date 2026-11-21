# DWG -> DXF（批量）: 用本机 AutoCAD 核心控制台转换，供自研矢量渲染器读。
# 用法: .\dwg2dxf.ps1 <in.dwg> <out.dxf>   或   .\dwg2dxf.ps1 <dir> <outDir>
param(
    [Parameter(Mandatory=$true)][string]$In,
    [Parameter(Mandatory=$true)][string]$Out
)
$accore = 'G:\PyArx\ACAD\accoreconsole.exe'
if (-not (Test-Path $accore)) { throw "accoreconsole 不存在: $accore" }

function ConvertOne([string]$dwg, [string]$dxf) {
    $log = 'G:\PyArx\log'
    if (-not (Test-Path $log)) { New-Item -ItemType Directory -Path $log | Out-Null }
    $scr = Join-Path $log ('to_dxf_' + [IO.Path]::GetFileNameWithoutExtension($dwg) + '.scr')
    # FILEDIA=0 关掉文件对话框；-DXFOUT 走命令行版；16 = R2004 ASCII；6 = 精度
    $body = "_FILEDIA`r`n0`r`n_DXFOUT`r`n$dxf`r`nV`r`n2018`r`n16`r`n`r`n"
    [IO.File]::WriteAllText($scr, $body, [Text.Encoding]::ASCII)
    & $accore /i $dwg /s $scr *> (Join-Path $log 'accore_out.txt')
    if (Test-Path $dxf) { "OK  $([IO.Path]::GetFileName($dwg)) -> $dxf ($((Get-Item $dxf).Length) bytes)" }
    else { "FAIL $dwg" }
}

if (Test-Path $In -PathType Container) {
    if (-not (Test-Path $Out)) { New-Item -ItemType Directory -Path $Out | Out-Null }
    Get-ChildItem -Path $In -Filter *.dwg -Recurse | ForEach-Object {
        ConvertOne $_.FullName (Join-Path $Out ($_.BaseName + '.dxf'))
    }
} else {
    ConvertOne $In $Out
}
