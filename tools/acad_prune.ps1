# ACAD 精简脚本：默认只预览，加 -Apply 才真删
param([switch]$Apply)
$root = 'G:\PyArx\ACAD'
$exts = @('.arx', '.bmp', '.bnd', '.css', '.ctb', '.dat', '.dcl', '.drc', '.dvb', '.dwg', '.fas', '.h', '.hdi', '.hdr', '.htm', '.html', '.js', '.json', '.lsp', '.map', '.mln', '.mnc', '.mns', '.mnu', '.ogsf', '.onnx', '.otf', '.pak', '.pc3', '.plt', '.pmp', '.png', '.qm', '.shx', '.ttf', '.txt', '.vlx', '.xml', '.xmx')
$pref = @('qt', 'adsk', 'adlm', 'adsso', 'aec')
$n = 0; $b = 0
Get-ChildItem -LiteralPath $root -Recurse -File -Force | ForEach-Object {
    $low = $_.Name.ToLower(); $e = [IO.Path]::GetExtension($low)
    $hit = $false
    if ($exts -contains $e) { $hit = $true }
    elseif ($e -eq '.dll' -and ($pref | Where-Object { $low.StartsWith($_) })) { $hit = $true }
    elseif ($e -eq '.exe' -and $low -ne 'accoreconsole.exe') { $hit = $true }
    if ($hit) {
        $n++; $b += $_.Length
        if ($Apply) { Remove-Item -LiteralPath $_.FullName -Force }
        else { Write-Output $_.FullName }
    }
}
Write-Host ("{0} files / {1:N2} MB {2}" -f $n, ($b/1MB), $(if ($Apply) {'deleted'} else {'would delete (-Apply to execute)'}))
