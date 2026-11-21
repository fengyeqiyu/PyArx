param(
    [string]$Base = 'G:\PyArx\samples\dwg_versions\base.dwg',
    [string]$OutDir = 'G:\PyArx\samples\dwg_versions',
    [string]$Accore = 'C:\Program Files\Autodesk\AutoCAD 2024\accoreconsole.exe'
)

$versions = @('2018','2013','2010','2007','2004','2000','R14')
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

foreach ($v in $versions) {
    $out  = Join-Path $OutDir "v_$v.dwg"
    $scr  = Join-Path $OutDir "s_$v.scr"
    $log  = Join-Path $OutDir "l_$v.log"
    if (Test-Path $out) { Remove-Item $out -Force }

    # SAVEAS 必须用绝对路径脚本，且命令为 SAVEAS（-SAVEAS 在 core console 被拒）
    @("FILEDIA", "0", "SAVEAS", $v, $out) |
        Set-Content -Path $scr -Encoding ASCII

    & $Accore /i $Base /s $scr > $log 2>&1

    if (Test-Path $out) {
        $code = [System.Text.Encoding]::ASCII.GetString(
            [System.IO.File]::ReadAllBytes($out)[0..5])
        Write-Output ("{0,-5} -> {1}  ({2:N0} B)" -f $v, $code, (Get-Item $out).Length)
    } else {
        Write-Output ("{0,-5} -> FAILED (see {1})" -f $v, $log)
    }
}
