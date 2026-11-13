$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName Microsoft.VisualBasic

$out = 'G:\PyArx\_com_o2.txt'
function Log($m) { Write-Output $m; Add-Content -LiteralPath $out -Value $m -Encoding utf8 }
if (Test-Path $out) { Remove-Item -LiteralPath $out }

Log "== step0 processes =="
$procs = Get-Process acad -ErrorAction SilentlyContinue
Log ("running acad: " + ($(if ($procs) { ($procs | ForEach-Object { $_.Id }) -join ',' } else { 'none' })))

if (-not $procs) {
    Log "== step1 启动 AutoCAD（COM） =="
    $acad = New-Object -ComObject 'AutoCAD.Application'
} else {
    Log "== step1 附着已运行实例 =="
    $acad = [Microsoft.VisualBasic.Interaction]::GetObject('', 'AutoCAD.Application')
}
$acad.Visible = $true

$t0 = Get-Date
while ($true) {
    try { $doc = $acad.ActiveDocument; if ($doc) { break } } catch { Start-Sleep -Seconds 2 }
    if (((Get-Date) - $t0).TotalSeconds -gt 180) { throw '等待 ActiveDocument 超时' }
}
Log ("== step2 attached hwnd=" + $acad.HWND + " doc=" + $doc.Name + " 用时 " + [int]((Get-Date) - $t0).TotalSeconds + "s")

$arx = 'G:/PyArx/X64/Release/PyArxEngine.arx'
$logPath = 'G:\PyArx\X64\Release\log\pyengine.log'
$before = 0
if (Test-Path $logPath) { $before = (Get-Item $logPath).Length }

Log ("== step3 arxload " + $arx)
$doc.SendCommand('(arxload "' + $arx + '")' + "`n")
Start-Sleep -Seconds 6

# 命令行输出读不回来，用引擎自己的日志判断是否真的加载过
$after = 0
if (Test-Path $logPath) { $after = (Get-Item $logPath).Length }
if ($after -gt $before) {
    $tail = Get-Content -LiteralPath $logPath -Encoding utf8 -Tail 12
    Log ("== log 新增 " + ($after - $before) + " 字节")
    $tail | ForEach-Object { Log ("   " + $_) }
} else {
    Log "== 警告：pyengine.log 没有新增内容（可能 arxload 未执行）"
}

Log "== step4 PYRUNFILE cad_o2_probe.py"
$doc.SendCommand('PYRUNFILE G:\PyArx\tests\cad\cad_o2_probe.py' + "`n")
Start-Sleep -Seconds 10

$probe = 'G:\PyArx\log\cad_o2_probe.log'
if (Test-Path $probe) {
    Log "== probe log =="
    Get-Content -LiteralPath $probe -Encoding utf8 -Tail 20 | ForEach-Object { Log ("   " + $_) }
} else {
    Log "== 警告：没有 cad_o2_probe.log"
}
Log "== done（CAD 保持打开）"
