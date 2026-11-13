# run_com.ps1 — 用 COM 驱动 AutoCAD：附着已有实例（没有则新建）→ 打开新文档 → arxload → PYRUNFILE
param(
    [Parameter(Mandatory = $true)][string]$PyScript,
    [string]$Tag = "com",
    [switch]$NewDoc
)

$ErrorActionPreference = 'Stop'
$out = "G:\PyArx\_com_$Tag.txt"

function Log($m) {
    Write-Output $m
    Add-Content -LiteralPath $out -Value $m -Encoding utf8
}

try {
    Log "attach AutoCAD.Application ..."
    try {
        $acad = [Runtime.InteropServices.Marshal]::GetActiveObject('AutoCAD.Application')
        Log "attached to running instance"
    }
    catch {
        $acad = New-Object -ComObject 'AutoCAD.Application'
        Log "created new instance"
    }
    $acad.Visible = $true
    Log ("version=" + $acad.Version + " hwnd=" + $acad.HWND)
    Start-Sleep -Seconds 3

    if ($NewDoc -or ($acad.Documents.Count -eq 0)) {
        $doc = $acad.Documents.Add()
        Log ("Documents.Add -> " + $doc.Name)
        Start-Sleep -Seconds 2
    }
    else {
        $doc = $acad.ActiveDocument
        Log ("use ActiveDocument -> " + $doc.Name)
    }

    $doc.SendCommand("(setvar `"FILEDIA`" 0)`n")
    Log "arxload ..."
    $doc.SendCommand("(arxload `"G:/PyArx/X64/Release/PyArxEngine.arx`")`n")
    Start-Sleep -Seconds 4

    Log ("PYRUNFILE " + $PyScript)
    $doc.SendCommand("PYRUNFILE`n$PyScript`n")
    Start-Sleep -Seconds 6

    Log ("done; doc=" + $doc.Name + " hwnd=" + $acad.HWND)
}
catch {
    Log ("ERROR: " + $_.Exception.Message)
    exit 1
}
