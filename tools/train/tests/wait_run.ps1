param([string]$Script, [string]$Out, [string]$Tag)
$log = "C:\temp\wait_run_$Tag.log"
for ($i = 0; $i -lt 60; $i++) {
    $n = (Get-Process accoreconsole -ErrorAction SilentlyContinue | Measure-Object).Count
    "$(Get-Date -Format HH:mm:ss) count=$n" | Out-File $log -Append -Encoding utf8
    if ($n -lt 2) { break }
    Start-Sleep 10
}
$n = (Get-Process accoreconsole -ErrorAction SilentlyContinue | Measure-Object).Count
if ($n -ge 2) { "TIMEOUT waiting slot" | Out-File $log -Append -Encoding utf8; exit 2 }
& 'G:\PyArx\X64\Release\python.exe' 'G:\PyArx\tools\train\tests\run.py' $Script $Out *>> $log
"EXIT=$LASTEXITCODE" | Out-File $log -Append -Encoding utf8
