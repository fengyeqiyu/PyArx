# run_into.ps1 — 附着到已运行的 AutoCAD，逐条 "PYRUNFILE <路径>"（单行，不留悬空提示）
param(
    [Parameter(Mandatory = $true)][string[]]$Scripts,
    [string]$Tag = "into"
)

$ErrorActionPreference = 'Stop'
$out = "G:\PyArx\_com_$Tag.txt"

function Log($m) {
    Write-Output $m
    Add-Content -LiteralPath $out -Value $m -Encoding utf8
}

Add-Type -AssemblyName Microsoft.VisualBasic
$acad = [Microsoft.VisualBasic.Interaction]::GetObject('', 'AutoCAD.Application')
$doc = $acad.ActiveDocument
Log ("attached hwnd=" + $acad.HWND + " doc=" + $doc.Name)

foreach ($s in $Scripts) {
    Log ("PYRUNFILE " + $s)
    $doc.SendCommand("PYRUNFILE $s`n")
    Start-Sleep -Seconds 7
}
Log "done"
