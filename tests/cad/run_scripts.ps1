# run_scripts.ps1 — 新建一个 AutoCAD 实例：arxload 一次，然后逐条 "PYRUNFILE <路径>"（单行，不留悬空提示）
param(
    [Parameter(Mandatory = $true)][string[]]$Scripts,
    [string]$Tag = "run"
)

$ErrorActionPreference = 'Stop'
$out = "G:\PyArx\_com_$Tag.txt"

function Log($m) {
    Write-Output $m
    Add-Content -LiteralPath $out -Value $m -Encoding utf8
}

$acad = New-Object -ComObject 'AutoCAD.Application'
$acad.Visible = $true
Log ("version=" + $acad.Version + " hwnd=" + $acad.HWND)
Start-Sleep -Seconds 3

$doc = $acad.ActiveDocument
Log ("doc=" + $doc.Name)
$doc.SendCommand("(setvar `"FILEDIA`" 0)`n")

Log "arxload ..."
$doc.SendCommand("(arxload `"G:/PyArx/X64/Release/PyArxEngine.arx`")`n")
Start-Sleep -Seconds 5

foreach ($s in $Scripts) {
    Log ("PYRUNFILE " + $s)
    $doc.SendCommand("PYRUNFILE $s`n")
    Start-Sleep -Seconds 7
}
Log "done"
