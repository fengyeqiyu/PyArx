# 附着现有 AutoCAD，执行一个 PYRUNFILE；不 CreateObject；finally Quit 并等待退出。
param(
  [Parameter(Mandatory = $true)][string]$Script,
  [Parameter(Mandatory = $true)][string]$ResultJson,
  [string]$Tag = "existing",
  [int]$TimeoutSec = 60
)

$ErrorActionPreference = 'Stop'
$log = "G:\PyArx\_com_$Tag.txt"
function Log($m) { Write-Output $m; Add-Content -LiteralPath $log -Value $m -Encoding utf8 }

Add-Type -AssemblyName Microsoft.VisualBasic
$acad = [Microsoft.VisualBasic.Interaction]::GetObject('', 'AutoCAD.Application')
$doc = $acad.ActiveDocument
$hwnd = [int64]$acad.HWND
Log ("attached hwnd=" + $hwnd + " doc=" + $doc.Name)

Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class ExistingCad {
  [DllImport("user32.dll", CharSet=CharSet.Auto)]
  public static extern IntPtr SendMessage(IntPtr h, uint msg, IntPtr w, IntPtr l);
  [DllImport("user32.dll")]
  public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
}
'@

$pid32 = 0
[ExistingCad]::GetWindowThreadProcessId([IntPtr]$hwnd, [ref]$pid32) | Out-Null
[ExistingCad]::SendMessage([IntPtr]$hwnd, 0x0006, [IntPtr]1, [IntPtr]0) | Out-Null
Log ("attached pid=" + $pid32)

$procBefore = @(Get-Process acad -ErrorAction SilentlyContinue)
Log ("acad before=" + $procBefore.Count)

$exitCode = 0
try {
  $doc.SendCommand("PYRUNFILE $Script`n")
  $deadline = (Get-Date).AddSeconds($TimeoutSec)
  $done = $false
  while ((Get-Date) -lt $deadline) {
    Start-Sleep -Seconds 1
    if (Test-Path -LiteralPath $ResultJson) {
      try {
        $j = Get-Content -LiteralPath $ResultJson -Raw -Encoding utf8 | ConvertFrom-Json
        if ($j.done -or $j.ok -or $j.steps) { $done = $true; break }
      } catch { }
    }
  }
  Log ("script done=" + $done)
  if (-not $done) { $exitCode = 2 }
}
finally {
  try { $acad.Quit() } catch { Log ("quit warn: " + $_.Exception.Message) }
  Start-Sleep -Seconds 3
  $p = Get-Process -Id $pid32 -ErrorAction SilentlyContinue
  if ($p) { Start-Sleep -Seconds 5; $p.Refresh(); if (-not $p.HasExited) { Log "pid still alive" } }
}
exit $exitCode
