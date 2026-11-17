Add-Type -AssemblyName System.Drawing
Add-Type @'
using System; using System.Runtime.InteropServices;
public static class RZ3 {
 [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
 [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr h, IntPtr a, int x, int y, int cx, int cy, uint f);
 [DllImport("user32.dll")] public static extern IntPtr SendMessage(IntPtr h, uint m, IntPtr w, IntPtr l);
 [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr hdc, uint f);
 [StructLayout(LayoutKind.Sequential)] public struct RECT { public int l,t,r,b; }
}
'@
function Shot([string]$path) {
  $r = New-Object RZ3+RECT; [RZ3]::GetWindowRect($main,[ref]$r) | Out-Null
  $bw=$r.r-$r.l; $bh=$r.b-$r.t
  $bmp = New-Object System.Drawing.Bitmap $bw,$bh
  $g = [System.Drawing.Graphics]::FromImage($bmp)
  $hdc = $g.GetHdc(); [RZ3]::PrintWindow($main,$hdc,3) | Out-Null; $g.ReleaseHdc($hdc); $g.Dispose()
  $bmp.Save($path,[System.Drawing.Imaging.ImageFormat]::Png); $bmp.Dispose()
  "$path  ${bw}x${bh}"
}
$p = Get-Process UIDesign | Select-Object -First 1
$main = $p.MainWindowHandle
[RZ3]::SetWindowPos($main,[IntPtr]::Zero,160,120,1200,760,0x0004) | Out-Null
Start-Sleep -Milliseconds 900

# 关键复现：进入系统循环后只放大，EXITSIZEMOVE 故意不发（模拟 MFC 吞掉它的情形）
[RZ3]::SendMessage($main,0x0231,0,0) | Out-Null
for ($i=0; $i -lt 18; $i++) {
  [RZ3]::SetWindowPos($main,[IntPtr]::Zero,160,120,(1200+$i*40),(760+$i*24),0x0004) | Out-Null
  Start-Sleep -Milliseconds 10
}
# 再缩回去，仍不发 EXITSIZEMOVE
for ($i=0; $i -lt 18; $i++) {
  [RZ3]::SetWindowPos($main,[IntPtr]::Zero,160,120,(1880-$i*40),(1192-$i*24),0x0004) | Out-Null
  Start-Sleep -Milliseconds 10
}
Start-Sleep -Milliseconds 1400
Shot 'G:\PyArx\docs\_zoom_out_nolexit.png'
