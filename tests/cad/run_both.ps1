# run_both.ps1 — 一次 COM 会话内：arxload 一次 → 依次 PYRUNFILE 多个脚本（解释器常驻，不重启 CAD）
param(
    [string[]]$Scripts = @(
        'G:/PyArx/tests/cad/cad_form_test.py',
        'G:/PyArx/tests/cad/cad_palette_test.py'
    ),
    [string]$Tag = "both"
)

$ErrorActionPreference = 'Stop'
$out = "G:\PyArx\_com_$Tag.txt"

function Log($m) {
    Write-Output $m
    Add-Content -LiteralPath $out -Value $m -Encoding utf8
}

try {
    Log "create AutoCAD.Application ..."
    $acad = New-Object -ComObject 'AutoCAD.Application'
    $acad.Visible = $true
    Log ("version=" + $acad.Version + " hwnd=" + $acad.HWND)
    Start-Sleep -Seconds 3

    $doc = $acad.ActiveDocument
    Log ("doc=" + $doc.Name)

    $doc.SendCommand("(setvar `"FILEDIA`" 0)`n")
    Log "arxload ..."
    $doc.SendCommand("(arxload `"G:/PyArx/X64/Release/PyArxEngine.arx`")`n")
    Start-Sleep -Seconds 4

    foreach ($s in $Scripts) {
        Log ("PYRUNFILE " + $s)
        $doc.SendCommand("PYRUNFILE`n$s`n")
        Start-Sleep -Seconds 6
    }

    Log ("done; hwnd=" + $acad.HWND)
}
catch {
    Log ("ERROR: " + $_.Exception.Message)
    exit 1
}
