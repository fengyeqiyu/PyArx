# 用 Viewer 打开指定 XML 并抓窗截图（PrintWindow，比 CopyFromScreen 稳）
param(
  [string]$Xml = "G:\PyArx\X64\Release\power_plan_shell.xml",
  [string]$Out = "G:\PyArx\docs\_pp_shell_viewer.png",
  [string]$Title = "配电规划图"
)
Add-Type @"
using System;
using System.Runtime.InteropServices;
using System.Drawing;
using System.Drawing.Imaging;
public class PW {
  [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc cb, IntPtr l);
  public delegate bool EnumProc(IntPtr h, IntPtr l);
  [DllImport("user32.dll")] public static extern int GetWindowText(IntPtr h, System.Text.StringBuilder s, int n);
  [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr h, IntPtr after, int x, int y, int cx, int cy, uint f);
  [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int cmd);
  [DllImport("user32.dll")] public static extern bool IsIconic(IntPtr h);
  [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr hdc, uint flags);
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
}
"@ -ReferencedAssemblies System.Drawing

$env:PYARX_D2D_RT = 'software'
$p = Start-Process -FilePath 'G:\PyArx\X64\Release\Viewer.exe' -ArgumentList $Xml -PassThru -WorkingDirectory 'G:\PyArx\X64\Release'
Start-Sleep -Milliseconds 300
$hwnd = [IntPtr]::Zero
for ($i = 0; $i -lt 60; $i++) {
  Start-Sleep -Milliseconds 200
  $found = [IntPtr]::Zero
  [PW]::EnumWindows({ param($h, $l)
      if (-not [PW]::IsWindowVisible($h)) { return $true }
      $sb = New-Object System.Text.StringBuilder 512
      [void][PW]::GetWindowText($h, $sb, 512)
      if ($sb.ToString() -like "*$Title*") { $script:hwnd = $h; return $false }
      return $true
    }, [IntPtr]::Zero) | Out-Null
  if ($hwnd -ne [IntPtr]::Zero) { break }
}
if ($hwnd -eq [IntPtr]::Zero) { "window not found"; Stop-Process -Id $p.Id -Force; exit 1 }
if ([PW]::IsIconic($hwnd)) { [void][PW]::ShowWindow($hwnd, 9) }
[void][PW]::SetWindowPos($hwnd, [IntPtr](-1), 40, 40, 0, 0, 0x0001 -bor 0x0004)
[void][PW]::SetForegroundWindow($hwnd)
Start-Sleep -Milliseconds 1200
$r = New-Object PW+RECT
[void][PW]::GetWindowRect($hwnd, [ref]$r)
$w = $r.Right - $r.Left; $h = $r.Bottom - $r.Top
$bmp = New-Object System.Drawing.Bitmap $w, $h
$g = [System.Drawing.Graphics]::FromImage($bmp)
[void][PW]::PrintWindow($hwnd, $g.GetHdc(), 2)
$g.ReleaseHdc()
$bmp.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
"saved $Out  ${w}x${h}"
Stop-Process -Id $p.Id -Force
