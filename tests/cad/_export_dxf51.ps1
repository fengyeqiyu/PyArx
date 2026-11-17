# ASCII-only script for Windows PowerShell 5.1 (GetActiveObject available there).
# Paths come from UTF-8 JSON to avoid ANSI mis-decoding of Chinese names.
$ErrorActionPreference = 'Stop'
$outDir = 'G:\PyArx\tests\dwg'
$log = Join-Path $outDir '_export_log.txt'
function Log($m) {
    $line = ('{0}  {1}' -f (Get-Date -Format 'HH:mm:ss'), $m)
    Write-Output $line
    Add-Content -LiteralPath $log -Value $line -Encoding utf8
}

$json = [IO.File]::ReadAllText('G:\PyArx\tests\cad\_dxf_paths.json', [Text.Encoding]::UTF8)
$samples = $json | ConvertFrom-Json

try {
    $acad = [Runtime.InteropServices.Marshal]::GetActiveObject('AutoCAD.Application')
    Log 'attached to running AutoCAD instance'
} catch {
    Log ('NO_INSTANCE: ' + $_.Exception.Message)
    exit 2
}

# occupancy wait
$busy = $true; $waitedTotal = 0; $maxWait = 900
while ($busy -and $waitedTotal -lt $maxWait) {
    $busy = $false
    foreach ($d in $acad.Documents) {
        try { if ([int]$d.GetVariable('CMDACTIVE') -ne 0) { $busy = $true } } catch { $busy = $true }
    }
    if ($busy) {
        Log ("busy, sleep 45s (waited=$waitedTotal)")
        Start-Sleep -Seconds 45; $waitedTotal += 45
    }
}
Log ("occupancy wait total = ${waitedTotal}s; busy=" + $busy)

$typeCodes = @(64, 61, 47, 37)

foreach ($s in $samples) {
    Log ('=== open ' + $s.src)
    if (-not (Test-Path -LiteralPath $s.src)) { Log 'SRC_MISSING'; continue }
    $doc = $null
    try {
        $doc = $acad.Documents.Open($s.src, $true, '')
        Start-Sleep -Seconds 2
        Log ('opened: ' + $doc.Name)
        $saved = $false
        foreach ($code in $typeCodes) {
            try {
                Log ("SaveAs type=$code -> " + $s.dxf)
                $doc.SaveAs($s.dxf, $code, $null)
                Start-Sleep -Seconds 2
                if (Test-Path -LiteralPath $s.dxf) { $saved = $true; Log ("saved type=$code"); break }
            } catch {
                Log ('  type=' + $code + ' failed: ' + $_.Exception.Message)
            }
        }
        if (-not $saved) { Log 'DXF_EXPORT_FAILED' }
    } catch {
        Log ('OPEN/SAVE ERROR: ' + $_.Exception.Message)
    } finally {
        if ($doc -ne $null) {
            try { $doc.Close($false); Log 'closed without save' } catch { Log ('close err: ' + $_.Exception.Message) }
        }
        Start-Sleep -Seconds 2
    }
}
Log 'DONE'
