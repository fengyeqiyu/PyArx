# 新测试套件运行器（tests/xml_ui）
# 与旧 pp_*.py 的区别：每个测试失败时退出码非 0；这里直接用退出码判定。

param([string]$Filter = '')

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$py = 'D:\Program Files\Python315t\python.exe'
$logDir = Join-Path $root 'log\after\xml_ui'
New-Item -ItemType Directory -Force -Path $logDir | Out-Null

$env:PYARX_D2D_RT = 'software'
$env:PYTHONIOENCODING = 'utf-8'

$tests = Get-ChildItem -LiteralPath $PSScriptRoot -Filter 't_*.py' | Sort-Object Name
if ($Filter) { $tests = $tests | Where-Object { $_.Name -match $Filter } }

$failed = @()
foreach ($t in $tests) {
    $log = Join-Path $logDir ($t.BaseName + '.txt')
    $out = & $py $t.FullName 2>&1
    $code = $LASTEXITCODE
    $out | Out-File -FilePath $log -Encoding utf8
    $summary = ($out | Select-String -Pattern '^(PASS|FAIL)=' -SimpleMatch:$false |
                Select-Object -Last 1)
    $line = ($out | Where-Object { $_ -match 'PASS=\d+ FAIL=\d+' } | Select-Object -Last 1)
    Write-Host ('{0,-16} exit={1}  {2}' -f $t.BaseName, $code, $line)
    if ($code -ne 0) { $failed += $t.BaseName }
}

Write-Host ''
if ($failed.Count -eq 0) { Write-Host 'ALL PASS' } else { Write-Host ('FAILED: ' + ($failed -join ', ')) }
if ($failed.Count -ne 0) { exit 1 }
