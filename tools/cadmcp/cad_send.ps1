param(
  [Parameter(Mandatory=$true)][string]$PyFile,
  [int]$TimeoutSec = 90,
  [string]$DocName = ""
)
$ErrorActionPreference = "Continue"
$base = "G:\PyArx\tools\cadmcp"
$res = Join-Path $base "res.json"
if (Test-Path -LiteralPath $res) { Remove-Item -LiteralPath $res -Force }

$app = $null
for ($i = 0; $i -lt 20; $i++) {
  try { $app = [Runtime.InteropServices.Marshal]::GetActiveObject("AutoCAD.Application") } catch { Start-Sleep -Seconds 2 }
  if ($app) { break }
}
if (-not $app) { Write-Output '{"error":"AutoCAD not running"}'; exit 2 }

$doc = $null
if ($DocName -ne "") {
  foreach ($x in $app.Documents) { if ($x.Name -like "*$DocName") { $doc = $x } }
}
if ($null -eq $doc) { $doc = $app.ActiveDocument }
try { $doc.Activate() } catch { }
Start-Sleep -Milliseconds 500

$cmd = '(if (not (member "PyArxEngine.arx" (arx))) (arxload "G:/PyArx/X64/Release/PyArxEngine.arx")) (command "PYRUNFILE" "' + ($PyFile -replace '\\','/') + '") '
$sent = $false
for ($i = 0; $i -lt 12; $i++) {
  try { $doc.SendCommand($cmd); $sent = $true } catch { Start-Sleep -Seconds 2 }
  if ($sent) { break }
}
if (-not $sent) { Write-Output '{"error":"SendCommand rejected"}'; exit 3 }

$deadline = (Get-Date).AddSeconds($TimeoutSec)
while ((Get-Date) -lt $deadline) {
  if (Test-Path -LiteralPath $res) { Start-Sleep -Milliseconds 200; break }
  Start-Sleep -Milliseconds 400
}
if (Test-Path -LiteralPath $res) { Get-Content -LiteralPath $res -Raw -Encoding utf8 } else { Write-Output '{"error":"timeout waiting result"}' }
