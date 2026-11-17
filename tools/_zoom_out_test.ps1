Add-Type -AssemblyName System.Drawing
Add-Type @'
using System; using System.Runtime.InteropServices;
public static class RZ2 {
 [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
 [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr h, IntPtr a, int x, int y, int cx, int cy, uint f);
 [DllImport("user32.dll")] public static extern IntPtr SendMessage(IntPtr h, uint m, IntPtr w, IntPtr l);
 [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr hdc, uint f);
 [StructLayout(LayoutKind.Sequential)] public struct RECT { public int l,t,r,b; }
}
'@
function Shot([string]$path) {
  $r = New-Object RZ2+RECT; [RZ2]::GetWindowRect($main,[ref]$r) | Out-Null
  $bw=$r.r-$r.l; $bh=$r.b-$r.t
  $bmp = New-Object System.Drawing.Bitmap $bw,$bh
  $g = [System.Drawing.Graphics]::FromImage($bmp)
  $hdc = $g.GetHdc(); [RZ2]::PrintWindow($main,$hdc,3) | Out-Null; $g.ReleaseHdc($hdc); $g.Dispose()
  $bmp.Save($path,[System.Drawing.Imaging.ImageFormat]::Png); $bmp.Dispose()
  "$path  ${bw}x${bh}"
}
$p = Get-Process UIDesign | Select-Object -First 1
$main = $p.MainWindowHandle
[RZ2]::SetWindowPos($main,[IntPtr]::Zero,160,120,1200,760,0x0004) | Out-Null
Start-Sleep -Milliseconds 900

# 放大：进入循环 -> 逐步放大 -> 退出
[RZ2]::SendMessage($main,0x0231,0,0) | Out-Null
for ($i=0; $i -lt 20; $i++) {
  [RZ2]::SetWindowPos($main,[IntPtr]::Zero,160,120,(1200+$i*32),(760+$i*20),0x0004) | Out-Null
  Start-Sleep -Milliseconds 10
}
[RZ2]::SendMessage($main,0x0232,0,0) | Out-Null
Start-Sleep -Milliseconds 800
Shot 'G:\PyArx\docs\_zoom_in.png'

# 缩小：同样一次完整循环，逐步缩回到起点
[RZ2]::SendMessage($main,0x0231,0,0) | Out-Null
for ($i=0; $i -lt 20; $i++) {
  [RZ2]::SetWindowPos($main,[IntPtr]::Zero,160,120,(1840-$i*32),(1160-$i*20),0x0004) | Out-Null
  Start-Sleep -Milliseconds 10
}
[RZ2]::SendMessage($main,0x0232,0,0) | Out-Null
Start-Sleep -Milliseconds 800
Shot 'G:\PyArx\docs\_zoom_out.png'
