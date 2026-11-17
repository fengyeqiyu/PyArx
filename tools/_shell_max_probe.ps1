Add-Type -AssemblyName System.Drawing
Add-Type @'
using System; using System.Text; using System.Runtime.InteropServices;
public static class SH2 {
 [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
 [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out RECT r);
 [DllImport("user32.dll")] public static extern IntPtr SendMessage(IntPtr h, uint m, IntPtr w, IntPtr l);
 [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr hdc, uint f);
 [DllImport("user32.dll")] public static extern bool EnumChildWindows(IntPtr h, EnumProc cb, IntPtr p);
 [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetClassName(IntPtr h, StringBuilder s, int n);
 public delegate bool EnumProc(IntPtr h, IntPtr p);
 [StructLayout(LayoutKind.Sequential)] public struct RECT { public int l,t,r,b; }
}
'@
$p = Get-Process UIDesign | Select-Object -First 1
$main = $p.MainWindowHandle
[SH2]::SendMessage($main,0x0112,[IntPtr]0xF030,[IntPtr]::Zero) | Out-Null
Start-Sleep -Milliseconds 1500

$cb = [SH2+EnumProc]{ param($h,$x)
  $sb = New-Object System.Text.StringBuilder 160
  [SH2]::GetClassName($h,$sb,160) | Out-Null
  $r = New-Object SH2+RECT; [SH2]::GetWindowRect($h,[ref]$r) | Out-Null
  $c = New-Object SH2+RECT; [SH2]::GetClientRect($h,[ref]$c) | Out-Null
  Write-Host ("  {0}  win {1},{2} {3}x{4}  client {5}x{6}" -f $sb.ToString(), $r.l,$r.t,($r.r-$r.l),($r.b-$r.t),$c.r,$c.b)
  return $true }
[SH2]::EnumChildWindows($main,$cb,[IntPtr]::Zero) | Out-Null

$r = New-Object SH2+RECT; [SH2]::GetWindowRect($main,[ref]$r) | Out-Null
$bw=$r.r-$r.l; $bh=$r.b-$r.t
$bmp = New-Object System.Drawing.Bitmap $bw,$bh
$g = [System.Drawing.Graphics]::FromImage($bmp)
$hdc = $g.GetHdc(); [SH2]::PrintWindow($main,$hdc,3) | Out-Null; $g.ReleaseHdc($hdc); $g.Dispose()
$bmp.Save('G:\PyArx\docs\_shell_max_pw.png',[System.Drawing.Imaging.ImageFormat]::Png); $bmp.Dispose()
"printwindow saved ${bw}x${bh}"
