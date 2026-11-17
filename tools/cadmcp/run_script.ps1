param(
  [Parameter(Mandatory=$true)][string]$PyFile,
  [string]$DocName = ""
)
# 用 CAD 原生命令 (command "PYRUNFILE" ...) 执行一个 .py 脚本：
# 脚本里的 print 会进 AutoCAD 命令行，适合带交互/长任务的脚本。
# 与 cad_send.ps1 的区别：这里不等 res.json，不要求脚本回传结果。

$app = $null
for ($i = 0; $i -lt 20; $i++) {
  try { $app = [Runtime.InteropServices.Marshal]::GetActiveObject("AutoCAD.Application") } catch { Start-Sleep -Seconds 2 }
  if ($app) { break }
}
if (-not $app) { Write-Output "AutoCAD not running"; exit 2 }

$doc = $null
if ($DocName -ne "") { foreach ($x in $app.Documents) { if ($x.Name -like "*$DocName") { $doc = $x } } }
if ($null -eq $doc) { $doc = $app.ActiveDocument }
try { $doc.Activate() } catch { }
Start-Sleep -Milliseconds 500

$cmd = '(if (not (member "PyArxEngine.arx" (arx))) (arxload "G:/PyArx/X64/Release/PyArxEngine.arx")) (command "PYRUNFILE" "' + ($PyFile -replace '\\','/') + '") '
$sent = $false
for ($i = 0; $i -lt 12; $i++) {
  try { $doc.SendCommand($cmd); $sent = $true } catch { Start-Sleep -Seconds 2 }
  if ($sent) { break }
}
if ($sent) { Write-Output ("PYRUNFILE sent: " + $PyFile) } else { Write-Output "SendCommand rejected"; exit 3 }
