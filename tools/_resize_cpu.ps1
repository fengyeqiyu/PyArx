Add-Type -AssemblyName System.Drawing
Add-Type @'
using System; using System.Runtime.InteropServices;
public static class RZ {
 [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
 [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr h, IntPtr a, int x, int y, int cx, int cy, uint f);
 [DllImport("user32.dll")] public static extern IntPtr SendMessage(IntPtr h, uint m, IntPtr w, IntPtr l);
 [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr hdc, uint f);
 [StructLayout(LayoutKind.Sequential)] public struct RECT { public int l,t,r,b; }
}
'@
$p = Get-Process UIDesign | Select-Object -First 1
$main = $p.MainWindowHandle
[RZ]::SetWindowPos($main,[IntPtr]::Zero,100,100,1500,950,0x0004) | Out-Null
Start-Sleep -Milliseconds 1200

$c0 = (Get-Process -Id $p.Id).TotalProcessorTime.TotalMilliseconds
[RZ]::SendMessage($main,0x0231,0,0) | Out-Null   # ENTERSIZEMOVE
for ($i=0; $i -lt 60; $i++) {
  $w = 1300 + $i*8; $h = 820 + $i*6
  [RZ]::SetWindowPos($main,[IntPtr]::Zero,100,100,$w,$h,0x0004) | Out-Null
  Start-Sleep -Milliseconds 8
}
$c1 = (Get-Process -Id $p.Id).TotalProcessorTime.TotalMilliseconds
[RZ]::SendMessage($main,0x0232,0,0) | Out-Null   # EXITSIZEMOVE
Start-Sleep -Milliseconds 900

$c2 = (Get-Process -Id $p.Id).TotalProcessorTime.TotalMilliseconds
"resize 60 steps cpu: {0:N0} ms total, {1:N2} ms/step" -f ($c1-$c0), (($c1-$c0)/60)
"after exit settle cpu: {0:N0} ms" -f ($c2-$c1)

$r = New-Object RZ+RECT; [RZ]::GetWindowRect($main,[ref]$r) | Out-Null
$bw=$r.r-$r.l; $bh=$r.b-$r.t
$bmp = New-Object System.Drawing.Bitmap $bw,$bh
$g = [System.Drawing.Graphics]::FromImage($bmp)
$hdc = $g.GetHdc(); [RZ]::PrintWindow($main,$hdc,3) | Out-Null; $g.ReleaseHdc($hdc); $g.Dispose()
$bmp.Save('G:\PyArx\docs\_resize_after.png',[System.Drawing.Imaging.ImageFormat]::Png); $bmp.Dispose()
"shot saved $bw x $bh"
