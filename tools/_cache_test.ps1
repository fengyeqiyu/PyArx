Add-Type -AssemblyName System.Drawing
Add-Type @'
using System; using System.Runtime.InteropServices;
public static class CV {
 [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr h, IntPtr a, int x, int y, int cx, int cy, uint f);
 [DllImport("user32.dll")] public static extern IntPtr SendMessage(IntPtr h, uint m, IntPtr w, IntPtr l);
 [DllImport("user32.dll")] public static extern IntPtr WindowFromPoint(POINT p);
 [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
 [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr hdc, uint f);
 [StructLayout(LayoutKind.Sequential)] public struct RECT { public int l,t,r,b; }
 [StructLayout(LayoutKind.Sequential)] public struct POINT { public int x,y; }
}
'@

$w = Get-Process UIDesign | Select-Object -First 1
$h = $w.MainWindowHandle
[CV]::SetWindowPos($h,[IntPtr]::Zero,200,200,1400,900,0x0004) | Out-Null
Start-Sleep -Milliseconds 1400

$p = New-Object CV+POINT; $p.x = 560; $p.y = 420
$canvas = [CV]::WindowFromPoint($p)
Write-Host "canvas=$canvas"

$lp0 = 300 -bor (300 -shl 16)
$lp1 = 420 -bor (380 -shl 16)
[CV]::SendMessage($canvas,0x0207,0,[IntPtr]$lp0) | Out-Null
[CV]::SendMessage($canvas,0x0200,[IntPtr]1,[IntPtr]$lp1) | Out-Null
[CV]::SendMessage($canvas,0x0208,0,[IntPtr]$lp1) | Out-Null
Start-Sleep -Milliseconds 700

$r = New-Object CV+RECT; [CV]::GetWindowRect($h,[ref]$r) | Out-Null
$bmp = New-Object System.Drawing.Bitmap ($r.r-$r.l),($r.b-$r.t)
$g = [System.Drawing.Graphics]::FromImage($bmp)
$hdc = $g.GetHdc()
[CV]::PrintWindow($h,$hdc,3) | Out-Null
$g.ReleaseHdc($hdc); $g.Dispose()
$bmp.Save('G:\PyArx\docs\_cache_pan1.png',[System.Drawing.Imaging.ImageFormat]::Png)
$bmp.Dispose()

# 反向平移回去，再截一张，确认左/上方向也不丢内容
$lp2 = 300 -bor (300 -shl 16)
$lp3 = 180 -bor (220 -shl 16)
[CV]::SendMessage($canvas,0x0207,0,[IntPtr]$lp2) | Out-Null
[CV]::SendMessage($canvas,0x0200,[IntPtr]1,[IntPtr]$lp3) | Out-Null
[CV]::SendMessage($canvas,0x0208,0,[IntPtr]$lp3) | Out-Null
Start-Sleep -Milliseconds 700
$r2 = New-Object CV+RECT; [CV]::GetWindowRect($h,[ref]$r2) | Out-Null
$b2 = New-Object System.Drawing.Bitmap ($r2.r-$r2.l),($r2.b-$r2.t)
$g2 = [System.Drawing.Graphics]::FromImage($b2)
$hd2 = $g2.GetHdc(); [CV]::PrintWindow($h,$hd2,3) | Out-Null
$g2.ReleaseHdc($hd2); $g2.Dispose()
$b2.Save('G:\PyArx\docs\_cache_pan2.png',[System.Drawing.Imaging.ImageFormat]::Png)
$b2.Dispose()
Write-Host "shots done"
