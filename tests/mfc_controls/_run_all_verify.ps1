$env:PYTHONPATH = 'G:\PyArx\X64\Release'
$py  = 'G:\PyArx\X64\Release\python.exe'
$dir = 'G:\PyArx\tests\mfc_controls'
$log = 'G:\PyArx\docs\_all_verify2.txt'
if (Test-Path -LiteralPath $log) { Remove-Item -LiteralPath $log -Force }

$scripts = @(
  'test_controls.py',
  'test_real_interaction.py',
  'verify_p0_draw.py',
  '_verify_names.py',
  '_verify_flickable.py',
  '_verify_anim.py',
  '_verify_page_anim.py',
  '_verify_canvas_edit.py',
  '_verify_level_tab.py',
  '_verify_viewer_order.py',
  '_verify_designer_order.py',
  '_verify_events.py'
)

foreach ($s in $scripts) {
  # 脚本间隔：前一个脚本的窗口退净、D2D 首帧出完再开下一个。
  # 不留间隔时下一个窗口拿不到前台/鼠标捕获，拖拽类脚本会假失败（曾 60px 只走 27px）。
  Start-Sleep -Milliseconds 1800
  $path = Join-Path $dir $s
  if (-not (Test-Path -LiteralPath $path)) { Add-Content -LiteralPath $log -Value ("MISSING  " + $s) -Encoding utf8; continue }
  $o    = Join-Path $dir ('_log_' + ($s -replace '\.py$','') + '.txt')
  $p = Start-Process -FilePath $py -ArgumentList '-u', $s -WorkingDirectory $dir `
       -RedirectStandardOutput $o -RedirectStandardError ($o + '.err') -PassThru -NoNewWindow
  $done = $p.WaitForExit(300000)
  if (-not $done) {
    try { $p.Kill() } catch {}
    Add-Content -LiteralPath $log -Value ("TIMEOUT  " + $s + "  (>300s)") -Encoding utf8
    continue
  }
  $code = $p.ExitCode
  $tail = ((Get-Content -LiteralPath $o -Tail 3) -join ' | ')
  Add-Content -LiteralPath $log -Value ("$s  exit=$code  ::  $tail") -Encoding utf8
}
Add-Content -LiteralPath $log -Value 'ALL DONE' -Encoding utf8
