Add-Type -AssemblyName System.Drawing
Add-Type @'
using System; using System.Runtime.InteropServices;
public static class RZ4 {
 [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
 [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr h, IntPtr a, int x, int y, int cx, int cy, uint f);
 [DllImport("user32.dll")] public static extern IntPtr SendMessage(IntPtr h, uint m, IntPtr w, IntPtr l);
 [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr hdc, uint f);
 [StructLayout(LayoutKind.Sequential)] public struct RECT { public int l,t,r,b; }
}
'@
function Shot([string]$path) {
  $r = New-Object RZ4+RECT; [RZ4]::GetWindowRect($main,[ref]$r) | Out-Null
  $bw=$r.r-$r.l; $bh=$r.b-$r.t
  $bmp = New-Object System.Drawing.Bitmap $bw,$bh
  $g = [System.Drawing.Graphics]::FromImage($bmp)
  $hdc = $g.GetHdc(); [RZ4]::PrintWindow($main,$hdc,3) | Out-Null; $g.ReleaseHdc($hdc); $g.Dispose()
  $bmp.Save($path,[System.Drawing.Imaging.ImageFormat]::Png); $bmp.Dispose()
  "$path  ${bw}x${bh}"
}
$p = Get-Process UIDesign | Select-Object -First 1
$main = $p.MainWindowHandle
[RZ4]::SetWindowPos($main,[IntPtr]::Zero,60,40,1900,1200,0x0004) | Out-Null
Start-Sleep -Milliseconds 900
Shot 'G:\PyArx\docs\_big_wide.png'

# 放大态下进入循环，缩到很窄，EXITSIZEMOVE 不发
[RZ4]::SendMessage($main,0x0231,0,0) | Out-Null
for ($i=0; $i -lt 20; $i++) {
  [RZ4]::SetWindowPos($main,[IntPtr]::Zero,60,40,(1900-$i*45),(1200-$i*26),0x0004) | Out-Null
  Start-Sleep -Milliseconds 10
}
Start-Sleep -Milliseconds 1400
Shot 'G:\PyArx\docs\_narrow_nolexit.png'
