# 一次性：清掉所有 cad 实例 → 起一个新实例 → arxload → 依次 PYRUNFILE 全部脚本 → 校验每个日志都新增了内容

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName Microsoft.VisualBasic

$out = 'G:\PyArx\_com_runall.txt'
if (Test-Path $out) { Remove-Item -LiteralPath $out }
function Log($m) { Write-Output $m; Add-Content -LiteralPath $out -Value $m -Encoding utf8 }

Log "== 1 结束所有 cad 进程 =="
Get-Process acad -ErrorAction SilentlyContinue | ForEach-Object { "   kill PID $($_.Id)"; Stop-Process -Id $_.Id -Force }
Start-Sleep -Seconds 4
$left = Get-Process acad -ErrorAction SilentlyContinue
if ($left) { Log ("   残留：" + (($left | ForEach-Object { $_.Id }) -join ',')) } else { Log "   已清空" }

Log "== 2 启动单一实例 =="
$acad = New-Object -ComObject 'AutoCAD.Application'
$acad.Visible = $true
$t0 = Get-Date
while ($true) {
    try { $doc = $acad.ActiveDocument; if ($doc) { break } } catch { Start-Sleep -Seconds 2 }
    if (((Get-Date) - $t0).TotalSeconds -gt 210) { throw '等待 ActiveDocument 超时' }
}
Log ("   pid=" + (Get-Process acad | Select-Object -ExpandProperty Id) + " hwnd=" + $acad.HWND + " doc=" + $doc.Name)

Log "== 3 arxload =="
$engineLog = 'G:\PyArx\X64\Release\log\pyengine.log'
$before = if (Test-Path $engineLog) { (Get-Item $engineLog).Length } else { 0 }
$doc.SendCommand('(arxload "G:/PyArx/X64/Release/PyArxEngine.arx")' + "`n")
Start-Sleep -Seconds 8
$after = if (Test-Path $engineLog) { (Get-Item $engineLog).Length } else { 0 }
Log ("   pyengine.log " + $before + " -> " + $after)
if ($after -le $before) { throw 'arxload 没有写日志，中止' }
(Get-Content -LiteralPath $engineLog -Encoding utf8 -Tail 3) | ForEach-Object { Log ("   " + $_) }

$scripts = @(
  @{ f = 'G:\PyArx\tests\cad\cad_form_test.py';    log = 'G:\PyArx\log\cad_form_test.log';    title = 'PyArx CAD 窗体测试' },
  @{ f = 'G:\PyArx\tests\cad\cad_palette_test.py'; log = 'G:\PyArx\log\cad_palette_test.log'; title = '' },
  @{ f = 'G:\PyArx\tests\cad\cad_form_list.py';    log = 'G:\PyArx\log\cad_form_list.log';    title = 'PyArx CAD 列表与表格' },
  @{ f = 'G:\PyArx\tests\cad\cad_form_anim.py';    log = 'G:\PyArx\log\cad_form_anim.log';    title = 'PyArx CAD 动画' },
  @{ f = 'G:\PyArx\tests\cad\cad_form_overlay.py'; log = 'G:\PyArx\log\cad_form_overlay.log'; title = 'PyArx CAD 浮层' },
  @{ f = 'G:\PyArx\tests\cad\cad_o2_probe.py';     log = 'G:\PyArx\log\cad_o2_probe.log';     title = 'PyArx O2 冒烟' }
)

foreach ($s in $scripts) {
    $beforeLen = if (Test-Path $s.log) { (Get-Item $s.log).Length } else { 0 }
    Log ("== PYRUNFILE " + (Split-Path $s.f -Leaf))
    $doc.SendCommand('PYRUNFILE ' + $s.f + "`n")
    Start-Sleep -Seconds 9
    $afterLen = if (Test-Path $s.log) { (Get-Item $s.log).Length } else { 0 }
    Log ("   log " + $beforeLen + " -> " + $afterLen)
    if ($afterLen -gt $beforeLen) {
        $delta = [int](($afterLen - $beforeLen) / 1)
        $text = Get-Content -LiteralPath $s.log -Encoding utf8 -Tail 8
        $text | ForEach-Object { Log ("   " + $_) }
        Log "   RESULT: OK（有新增输出）"
    } else {
        Log "   RESULT: 没有新增输出 —— 该脚本未执行"
    }
}

Log "== 4 现有窗口 =="
Get-Process acad -ErrorAction SilentlyContinue | ForEach-Object { Log ("   pid=" + $_.Id) }
Log "== done（CAD 保持打开，窗口不关）"
