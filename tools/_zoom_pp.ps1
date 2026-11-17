# 设计器画布滚轮缩放验证：向画布子窗口注入滚轮，再抓整窗
param(
  [string]$Exe = "G:\PyArx\X64\Release\UIDesign.exe",
  [string]$Xml = "G:\PyArx\X64\Release\power_plan_shell.xml",
  [string]$Out = "G:\PyArx\docs\_pp_zoom.png",
  [int]$WheelSteps = 3,
  [int]$Delta = 120,
  [int]$PanX = 0,
  [int]$PanY = 0
)
Add-Type @"
using System;
using System.Runtime.InteropServices;
using System.Drawing;
public class ZP {
  [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc cb, IntPtr l);
  public delegate bool EnumProc(IntPtr h, IntPtr l);
  [DllImport("user32.dll")] public static extern bool EnumChildWindows(IntPtr p, EnumProc cb, IntPtr l);
  [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr h, ref POINT p);
  [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h, uint msg, IntPtr w, IntPtr l);
  [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr h, IntPtr after, int x, int y, int cx, int cy, uint f);
  [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int cmd);
  [DllImport("user32.dll")] public static extern bool IsIconic(IntPtr h);
  [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr hdc, uint flags);
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
  [DllImport("user32.dll")] public static extern IntPtr GetParent(IntPtr h);
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
  [StructLayout(LayoutKind.Sequential)] public struct POINT { public int X, Y; }
}
public class ZPick { public uint pid; public IntPtr best; public int area; }
public class ZChild { public IntPtr best; public int area; }
"@ -ReferencedAssemblies System.Drawing

$env:PYARX_D2D_RT = 'software'
$p = Start-Process -FilePath $Exe -ArgumentList $Xml -PassThru -WorkingDirectory (Split-Path $Exe)
Start-Sleep -Milliseconds 600
$pick = New-Object ZPick
$pick.pid = [uint32]$p.Id
for ($round = 0; $round -lt 30 -and $pick.best -eq [IntPtr]::Zero; $round++) {
  [ZP]::EnumWindows({
      param($h, $l)
      if (-not [ZP]::IsWindowVisible($h)) { return $true }
      if ([ZP]::GetParent($h) -ne [IntPtr]::Zero) { return $true }
      $pid2 = [uint32]0
      [void][ZP]::GetWindowThreadProcessId($h, [ref]$pid2)
      if ($pid2 -ne $pick.pid) { return $true }
      $r = New-Object ZP+RECT
      [void][ZP]::GetWindowRect($h, [ref]$r)
      if (($r.Right - $r.Left) -lt 400) { return $true }
      $a = ($r.Right - $r.Left) * ($r.Bottom - $r.Top)
      if ($a -gt $pick.area) { $pick.area = $a; $pick.best = $h }
      return $true
    }, [IntPtr]::Zero) | Out-Null
  Start-Sleep -Milliseconds 200
}
if ($pick.best -eq [IntPtr]::Zero) { "main window not found"; Stop-Process -Id $p.Id -Force; exit 1 }
if ([ZP]::IsIconic($pick.best)) { [void][ZP]::ShowWindow($pick.best, 9) }
[void][ZP]::SetForegroundWindow($pick.best)
Start-Sleep -Milliseconds 1500

# 画布子窗口 = 面积最大的子窗口
$ch = New-Object ZChild
[ZP]::EnumChildWindows($pick.best, {
    param($h, $l)
    if (-not [ZP]::IsWindowVisible($h)) { return $true }
    $r = New-Object ZP+RECT
    [void][ZP]::GetWindowRect($h, [ref]$r)
    $a = ($r.Right - $r.Left) * ($r.Bottom - $r.Top)
    if ($a -gt $ch.area) { $ch.area = $a; $ch.best = $h }
    return $true
  }, [IntPtr]::Zero) | Out-Null
"child hwnd=$($ch.best) area=$($ch.area)"

$cr = New-Object ZP+RECT
[void][ZP]::GetClientRect($ch.best, [ref]$cr)
$pt = New-Object ZP+POINT
$pt.X = [int](($cr.Right - $cr.Left) / 2)
$pt.Y = [int](($cr.Bottom - $cr.Top) / 2)
[void][ZP]::ClientToScreen($ch.best, [ref]$pt)
$scr = [IntPtr](($pt.Y -shl 16) -bor ($pt.X -band 0xFFFF))
for ($i = 0; $i -lt $WheelSteps; $i++) {
  [void][ZP]::PostMessage($ch.best, 0x020A, [IntPtr](($Delta -shl 16) -band 0xFFFFFFFF), $scr)
  Start-Sleep -Milliseconds 120
}
if ($PanX -ne 0 -or $PanY -ne 0) {
  [void][ZP]::PostMessage($ch.best, 0x0207, [IntPtr]1, $scr)   # WM_MBUTTONDOWN
  Start-Sleep -Milliseconds 60
  [void][ZP]::PostMessage($ch.best, 0x0200, [IntPtr]0,
      [IntPtr]((($pt.Y + $PanY) -shl 16) -bor (($pt.X + $PanX) -band 0xFFFF)))
  Start-Sleep -Milliseconds 60
  [void][ZP]::PostMessage($ch.best, 0x0208, [IntPtr]0, $scr)   # WM_MBUTTONUP
}
Start-Sleep -Milliseconds 900
$r = New-Object ZP+RECT
[void][ZP]::GetWindowRect($pick.best, [ref]$r)
$w = $r.Right - $r.Left; $h = $r.Bottom - $r.Top
$bmp = New-Object System.Drawing.Bitmap $w, $h
$g = [System.Drawing.Graphics]::FromImage($bmp)
[void][ZP]::PrintWindow($pick.best, $g.GetHdc(), 2)
$g.ReleaseHdc()
$bmp.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
"saved $Out ${w}x${h}"
Stop-Process -Id $p.Id -Force
