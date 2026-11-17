# Trunk pipeline: entities JSON -> recognize (pyarx_vision) -> design model -> canvas
# Usage: powershell -File G:\PyArx\tools\cadmcp\run_trunk.ps1 -SysJson C:/temp/sys.json -Keep 0
param(
  [string]$SysJson = "C:\temp\sys.json",
  [string]$Kind = "sys",
  [string]$Keep = "0"
)
$py = "G:\PyArx\X64\Release\python.exe"
$root = "G:\PyArx"

Write-Host "[1/2] recognize + export design model"
Push-Location $root
& $py -m pyarx_vision.cli --in $SysJson --kind $Kind --recog "C:\temp\recog.json" --design "G:\PyArx\docs\trunk_design.json"
$code = $LASTEXITCODE
Pop-Location
if ($code -ne 0) { Write-Host "[warn] problems found, still rendering (see recog json)" }

Write-Host "[2/2] render on canvas"
$env:DESIGN_IN = "G:\PyArx\docs\trunk_design.json"
$env:TRUNK_KEEP = $Keep
& $py G:\PyArx\tools\cadmcp\draw_trunk.py
Write-Host "done"
