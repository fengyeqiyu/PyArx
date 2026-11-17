Add-Type -AssemblyName System.Drawing
Add-Type @'
using System; using System.Runtime.InteropServices; using System.Text;
public static class WS {
 [DllImport("user32.dll")] public static extern IntPtr GetWindow(IntPtr h, uint c);
 [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
 [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetClassName(IntPtr h, StringBuilder s, int n);
 [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
 [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr hdc, uint f);
 [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out RECT r);
 [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr h, ref POINT p);
 [DllImport("dwmapi.dll")] public static extern int DwmGetWindowAttribute(IntPtr h, int attr, out RECT r, int sz);
 [StructLayout(LayoutKind.Sequential)] public struct RECT { public int l,t,r,b; }
 [StructLayout(LayoutKind.Sequential)] public struct POINT { public int x,y; }
}
'@

$p = Get-Process UIDesign | Select-Object -First 1
$main = $p.MainWindowHandle
$wr = New-Object WS+RECT; [WS]::GetWindowRect($main,[ref]$wr) | Out-Null
Write-Host ("main {0} rect {1},{2} {3}x{4}" -f $main,$wr.l,$wr.t,($wr.r-$wr.l),($wr.b-$wr.t))

function Walk($h,$d){
  if ($h -eq [IntPtr]::Zero) { return }
  $r = New-Object WS+RECT; [WS]::GetWindowRect($h,[ref]$r) | Out-Null
  $c = New-Object Text.StringBuilder 96; [WS]::GetClassName($h,$c,96) | Out-Null
  $cl = New-Object WS+RECT; [WS]::GetClientRect($h,[ref]$cl) | Out-Null
  Write-Host ("{0}{1} cls={2} vis={3} win={4},{5} {6}x{7} client={8}x{9}" -f ('  '*$d),$h,$c.ToString(),[WS]::IsWindowVisible($h),$r.l,$r.t,($r.r-$r.l),($r.b-$r.t),($cl.r-$cl.l),($cl.b-$cl.t))
  $ch = [WS]::GetWindow($h,5)
  while ($ch -ne [IntPtr]::Zero) { Walk $ch ($d+1); $ch = [WS]::GetWindow($ch,2) }
}
Walk $main 0

$bmp = New-Object System.Drawing.Bitmap ($wr.r-$wr.l),($wr.b-$wr.t)
$g = [System.Drawing.Graphics]::FromImage($bmp)
$hdc = $g.GetHdc(); [WS]::PrintWindow($main,$hdc,3) | Out-Null; $g.ReleaseHdc($hdc)
$g.Dispose()
$bmp.Save('G:\PyArx\docs\_whitebar.png',[System.Drawing.Imaging.ImageFormat]::Png)
$bmp.Dispose()
Write-Host "shot saved"
