$ErrorActionPreference = 'Continue'
$tests = @(
  'pp_test_all','pp_split_test','pp_cell_test','pp_event_test','pp_grid_test',
  'pp_select_test','pp_real_test','pp_cursor_test','pp_click_test',
  'pp_runtime_test','pp_plugin_test'
)
$env:PYARX_D2D_RT = 'software'
$py = 'D:\Program Files\Python314\python.exe'
$dir = 'G:\PyArx\tests\mfc_xaml'
$logDir = 'G:\PyArx\docs'
$grandOk = 0; $grandBad = 0; $summary = @()
foreach ($t in $tests) {
  Get-Process python* -ErrorAction SilentlyContinue | Where-Object { $_.Path -like '*Python314*' } | Stop-Process -Force
  Start-Sleep -Milliseconds 2600
  $out = & $py (Join-Path $dir "$t.py") 2>&1
  $out | Out-File -LiteralPath (Join-Path $logDir "_regress_$t.txt") -Encoding utf8
  $ok = ($out | Where-Object { $_ -match '^\s*PASS' }).Count
  $bad = ($out | Where-Object { $_ -match '^\s*FAIL' }).Count
  $grandOk += $ok; $grandBad += $bad
  $summary += [pscustomobject]@{ Test=$t; PASS=$ok; FAIL=$bad }
  Write-Output ("{0,-18} PASS={1,-3} FAIL={2}" -f $t, $ok, $bad)
}
Write-Output ("TOTAL PASS={0} FAIL={1}" -f $grandOk, $grandBad)
$summary | Format-Table -AutoSize
