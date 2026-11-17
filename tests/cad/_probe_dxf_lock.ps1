# 只读探测：附着已有 AutoCAD 实例，列出文档与命令活动状态；不打开任何新文档
$ErrorActionPreference = 'Stop'
try {
    $acad = [Runtime.InteropServices.Marshal]::GetActiveObject('AutoCAD.Application')
} catch {
    Write-Output "NO_RUNNING_INSTANCE: $($_.Exception.Message)"
    exit 0
}
Write-Output ("VERSION=" + $acad.Version)
Write-Output ("DOC_COUNT=" + $acad.Documents.Count)
foreach ($d in $acad.Documents) {
    $cmd = $null
    try { $cmd = $d.GetVariable('CMDACTIVE') } catch { $cmd = "ERR" }
    $dbmod = $null
    try { $dbmod = $d.GetVariable('DBMOD') } catch { $dbmod = "ERR" }
    Write-Output ("DOC name=" + $d.Name + " path=" + $d.FullName + " CMDACTIVE=" + $cmd + " DBMOD=" + $dbmod)
}
Write-Output ("ACTIVE=" + $acad.ActiveDocument.Name)
