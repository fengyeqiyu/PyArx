# _export_dxf.ps1 —— 附着已有 AutoCAD 实例；只读打开样本 DWG；SaveAs ASCII DXF；关闭不保存。
# 占用探测：CMDACTIVE 非 0 则退避 45s 重试，累计等待上限 15 分钟。
$ErrorActionPreference = 'Stop'
$outDir = 'G:\PyArx\tests\dwg'
$log = Join-Path $outDir '_export_log.txt'
function Log($m) { $line = ('{0}  {1}' -f (Get-Date -Format 'HH:mm:ss'), $m); Write-Output $line; Add-Content -LiteralPath $log -Value $line -Encoding utf8 }

$samples = @(
    @{ src = 'G:\PyArx\tests\楼层平面-10KV电缆桥架.dwg'; dxf = Join-Path $outDir '楼层平面-10KV电缆桥架.dxf' },
    @{ src = 'G:\PyArx\tests\楼层平面-强电.dwg';        dxf = Join-Path $outDir '楼层平面-强电.dxf' }
)

try {
    $acad = [Runtime.InteropServices.Marshal]::GetActiveObject('AutoCAD.Application')
    Log 'attached to running AutoCAD instance'
} catch {
    Log ("NO_INSTANCE: " + $_.Exception.Message)
    exit 2
}

# ---- 占用等待 ----
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

# SaveAsType 候选：64=DXF2018(AC1032) 61=DXF2013(AC1027) 47=DXF2010 37=DXF2007
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
                Log ("  type=$code failed: " + $_.Exception.Message)
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
