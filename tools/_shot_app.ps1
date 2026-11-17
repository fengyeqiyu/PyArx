# 打开 XML 的应用（Viewer / UIDesign 等）抓窗截图：按进程 pid 找最大可见顶层窗
param(
  [string]$Exe = "G:\PyArx\X64\Release\Viewer.exe",
  [string]$Xml = "G:\PyArx\X64\Release\power_plan_shell.xml",
  [string]$Out = "G:\PyArx\docs\_pp_shot.png",
  [int]$WaitMs = 1500
)
Add-Type @"
using System;
using System.Runtime.InteropServices;
using System.Drawing;
public class PW2 {
  [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc cb, IntPtr l);
  public delegate bool EnumProc(IntPtr h, IntPtr l);
  [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr h, IntPtr after, int x, int y, int cx, int cy, uint f);
  [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int cmd);
  [DllImport("user32.dll")] public static extern bool IsIconic(IntPtr h);
  [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr hdc, uint flags);
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
  [DllImport("user32.dll")] public static extern IntPtr GetParent(IntPtr h);
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
}
public class Pick {
  public uint pid; public IntPtr best; public int area;
}
"@ -ReferencedAssemblies System.Drawing

$env:PYARX_D2D_RT = 'software'
$p = Start-Process -FilePath $Exe -ArgumentList $Xml -PassThru -WorkingDirectory (Split-Path $Exe)
Start-Sleep -Milliseconds 400

$pick = New-Object Pick
$pick.pid = [uint32]$p.Id
[PW2]::EnumWindows({
    param($h, $l)
    if (-not [PW2]::IsWindowVisible($h)) { return $true }
    if ([PW2]::GetParent($h) -ne [IntPtr]::Zero) { return $true }
    $pid2 = [uint32]0
    [void][PW2]::GetWindowThreadProcessId($h, [ref]$pid2)
    if ($pid2 -ne $pick.pid) { return $true }
    $r = New-Object PW2+RECT
    [void][PW2]::GetWindowRect($h, [ref]$r)
    $a = ($r.Right - $r.Left) * ($r.Bottom - $r.Top)
    if (($r.Right - $r.Left) -lt 400) { return $true }
    if ($a -gt $pick.area) { $pick.area = $a; $pick.best = $h }
    return $true
  }, [IntPtr]::Zero) | Out-Null
for ($i = 0; $i -lt 40 -and $pick.best -eq [IntPtr]::Zero; $i++) {
  Start-Sleep -Milliseconds 200
  [PW2]::EnumWindows({
      param($h, $l)
      if (-not [PW2]::IsWindowVisible($h)) { return $true }
      if ([PW2]::GetParent($h) -ne [IntPtr]::Zero) { return $true }
      $pid2 = [uint32]0
      [void][PW2]::GetWindowThreadProcessId($h, [ref]$pid2)
      if ($pid2 -ne $pick.pid) { return $true }
      $r = New-Object PW2+RECT
      [void][PW2]::GetWindowRect($h, [ref]$r)
      $a = ($r.Right - $r.Left) * ($r.Bottom - $r.Top)
      if (($r.Right - $r.Left) -lt 400) { return $true }
      if ($a -gt $pick.area) { $pick.area = $a; $pick.best = $h }
      return $true
    }, [IntPtr]::Zero) | Out-Null
}
if ($pick.best -eq [IntPtr]::Zero) { "window not found"; Stop-Process -Id $p.Id -Force; exit 1 }
if ([PW2]::IsIconic($pick.best)) { [void][PW2]::ShowWindow($pick.best, 9) }
[void][PW2]::SetWindowPos($pick.best, [IntPtr](-1), 20, 20, 0, 0, 0x0001 -bor 0x0004)
[void][PW2]::SetForegroundWindow($pick.best)
Start-Sleep -Milliseconds $WaitMs
$r = New-Object PW2+RECT
[void][PW2]::GetWindowRect($pick.best, [ref]$r)
$w = $r.Right - $r.Left; $h = $r.Bottom - $r.Top
$bmp = New-Object System.Drawing.Bitmap $w, $h
$g = [System.Drawing.Graphics]::FromImage($bmp)
[void][PW2]::PrintWindow($pick.best, $g.GetHdc(), 2)
$g.ReleaseHdc()
$bmp.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
"saved $Out  ${w}x${h}"
Stop-Process -Id $p.Id -Force
