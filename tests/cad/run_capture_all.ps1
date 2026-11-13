# 全部脚本在 CAD 内跑一遍，并在每个窗口出现后立刻抓图（验证"第一帧就是完整的"）
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName Microsoft.VisualBasic

$out = 'G:\PyArx\_com_capture.txt'
if (Test-Path $out) { Remove-Item -LiteralPath $out }
function Log($m) { Write-Output $m; Add-Content -LiteralPath $out -Value $m -Encoding utf8 }

Log "== 清场 =="
Get-Process acad, python -ErrorAction SilentlyContinue | ForEach-Object { Stop-Process -Id $_.Id -Force }
Start-Sleep -Seconds 4

Log "== 起新实例 + arxload =="
$acad = New-Object -ComObject 'AutoCAD.Application'
$acad.Visible = $true
while ($true) {
    try { $doc = $acad.ActiveDocument; if ($doc) { break } } catch { Start-Sleep -Seconds 2 }
    if (((Get-Date) - $t0).TotalSeconds -gt 200) { throw 'CAD 启动超时' }
}
$engineLog = 'G:\PyArx\X64\Release\log\pyengine.log'
$b0 = (Get-Item $engineLog).Length
$doc.SendCommand('(arxload "G:/PyArx/X64/Release/PyArxEngine.arx")' + "`n")
Start-Sleep -Seconds 8
Log ("pyengine.log " + $b0 + " -> " + (Get-Item $engineLog).Length)
if ((Get-Item $engineLog).Length -le $b0) { throw 'arxload 失败' }

$py = 'G:\PyArx\X64\Release\python.exe'
$grab = 'G:\PyArx\tests\mfc_controls\_grab_title.py'
$env:PYTHONPATH = 'G:\PyArx\X64\Release'

$jobs = @(
  @{ f = 'G:\PyArx\tests\cad\cad_form_test.py';    title = 'PyArx CAD 窗体测试'; png = 'cad_form_shown' },
  @{ f = 'G:\PyArx\tests\cad\cad_palette_test.py'; title = 'py-mfc-palette';       png = 'cad_palette_shown' },
  @{ f = 'G:\PyArx\tests\cad\cad_form_list.py';    title = 'PyArx CAD 列表与表格'; png = 'cad_list_shown' },
  @{ f = 'G:\PyArx\tests\cad\cad_form_anim.py';    title = 'PyArx CAD 动画';      png = 'cad_anim_shown' },
  @{ f = 'G:\PyArx\tests\cad\cad_form_overlay.py'; title = 'PyArx CAD 浮层';      png = 'cad_overlay_shown' },
  @{ f = 'G:\PyArx\tests\cad\cad_o2_probe.py';     title = 'PyArx O2 冒烟';       png = 'cad_o2_shown' }
)

foreach ($j in $jobs) {
    Log ("== " + (Split-Path $j.f -Leaf))
    $doc.SendCommand('PYRUNFILE ' + $j.f + "`n")
    Start-Sleep -Seconds 2      # 窗口按设计是"建完才出现"，2 秒足够，不给它补画的机会
    $bmp = 'G:\PyArx\tests\mfc_controls\_' + $j.png + '.bmp'
    $r = & $py -u $grab $j.title $bmp 2>&1
    Log ("   " + ($r -join ' '))
}

Log "== 窗口清单 =="
$r = & $py -u 'G:\PyArx\tests\mfc_controls\_nativewins.py' ((Get-Process acad | Select-Object -First 1).Id) 2>&1
$r | ForEach-Object { Log ("   " + $_) }
Log "== done =="
