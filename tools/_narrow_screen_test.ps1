Add-Type -AssemblyName System.Drawing
Add-Type @'
using System; using System.Runtime.InteropServices;
public static class RZ5 {
 [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
 [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr h, IntPtr a, int x, int y, int cx, int cy, uint f);
 [DllImport("user32.dll")] public static extern IntPtr SendMessage(IntPtr h, uint m, IntPtr w, IntPtr l);
 [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
 [StructLayout(LayoutKind.Sequential)] public struct RECT { public int l,t,r,b; }
}
'@
function ScreenShot([string]$path) {
  [RZ5]::SetForegroundWindow($main) | Out-Null
  Start-Sleep -Milliseconds 400
  $r = New-Object RZ5+RECT; [RZ5]::GetWindowRect($main,[ref]$r) | Out-Null
  $bw=$r.r-$r.l; $bh=$r.b-$r.t
  $bmp = New-Object System.Drawing.Bitmap $bw,$bh
  $g = [System.Drawing.Graphics]::FromImage($bmp)
  $g.CopyFromScreen($r.l,$r.t,0,0,(New-Object System.Drawing.Size $bw,$bh))
  $g.Dispose()
  $bmp.Save($path,[System.Drawing.Imaging.ImageFormat]::Png); $bmp.Dispose()
  "$path  ${bw}x${bh}"
}
$p = Get-Process UIDesign | Select-Object -First 1
$main = $p.MainWindowHandle
[RZ5]::SetWindowPos($main,[IntPtr]::Zero,60,40,1900,1200,0x0004) | Out-Null
Start-Sleep -Milliseconds 900
ScreenShot 'G:\PyArx\docs\_big_wide_scr.png'

[RZ5]::SendMessage($main,0x0231,0,0) | Out-Null
for ($i=0; $i -lt 20; $i++) {
  [RZ5]::SetWindowPos($main,[IntPtr]::Zero,60,40,(1900-$i*45),(1200-$i*26),0x0004) | Out-Null
  Start-Sleep -Milliseconds 10
}
Start-Sleep -Milliseconds 1400
ScreenShot 'G:\PyArx\docs\_narrow_scr.png'
