# 附着已有的 AutoCAD（不新建实例），执行一个 PYRUNFILE 脚本，轮询结果 JSON。默认不退出 CAD。
param(
  [Parameter(Mandatory = $true)][string]$Script,
  [Parameter(Mandatory = $true)][string]$ResultJson,
  [int]$TimeoutSec = 120,
  [int]$ExpectPid = 0,
  [switch]$Quit
)

$ErrorActionPreference = 'Stop'
if (Test-Path -LiteralPath $ResultJson) { Remove-Item -LiteralPath $ResultJson -Force }

Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class CadWin {
  [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
}
'@

# Marshal.GetActiveObject 只附着、不新建；VB Interaction.GetObject 在缺失时会新建实例（严禁）
try {
  $acad = [Runtime.InteropServices.Marshal]::GetActiveObject('AutoCAD.Application')
} catch {
  Write-Output ("ATTACH_FAILED " + $_.Exception.Message)
  exit 4
}

$pid32 = 0
[CadWin]::GetWindowThreadProcessId([IntPtr]$acad.HWND, [ref]$pid32) | Out-Null
$doc = $acad.ActiveDocument
Write-Output ("attached pid=" + $pid32 + " hwnd=" + $acad.HWND + " doc=" + $doc.Name)

if ($ExpectPid -ne 0 -and $pid32 -ne $ExpectPid) {
  Write-Output ("PID_MISMATCH expected=" + $ExpectPid + " actual=" + $pid32)
  exit 5
}

$sent = $false
for ($i = 0; $i -lt 12; $i++) {
  try {
    $doc.Activate()
    $doc.SendCommand("PYRUNFILE $Script`n")
    $sent = $true
    break
  } catch {
    Write-Output ("send retry " + $i + ": " + $_.Exception.Message)
    Start-Sleep -Milliseconds 800
  }
}
if (-not $sent) { Write-Output "SEND_FAILED"; exit 3 }

$deadline = (Get-Date).AddSeconds($TimeoutSec)
$done = $false
while ((Get-Date) -lt $deadline) {
  Start-Sleep -Seconds 1
  if (Test-Path -LiteralPath $ResultJson) {
    try {
      $raw = Get-Content -LiteralPath $ResultJson -Raw -Encoding utf8
      $j = $raw | ConvertFrom-Json
      if ($j.done -or $j.ok) { $done = $true; break }
    } catch { }
  }
}

Write-Output ("done=" + $done)
if (Test-Path -LiteralPath $ResultJson) { Get-Content -LiteralPath $ResultJson -Raw -Encoding utf8 }

if ($Quit) {
  try { $acad.Quit() } catch { Write-Output ("quit warn: " + $_.Exception.Message) }
}
if ($done) { exit 0 } else { exit 2 }
