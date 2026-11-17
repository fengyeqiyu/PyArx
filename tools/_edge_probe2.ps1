Add-Type @'
using System; using System.Text; using System.Runtime.InteropServices;
public static class PRC {
 [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
 [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr h, IntPtr a, int x, int y, int cx, int cy, uint f);
 [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
 [DllImport("user32.dll")] public static extern IntPtr WindowFromPoint(POINT p);
 [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetClassName(IntPtr h, StringBuilder s, int n);
 [DllImport("user32.dll")] public static extern IntPtr GetDC(IntPtr h);
 [DllImport("user32.dll")] public static extern int ReleaseDC(IntPtr h, IntPtr dc);
 [DllImport("gdi32.dll")] public static extern int GetPixel(IntPtr dc, int x, int y);
 [StructLayout(LayoutKind.Sequential)] public struct RECT { public int l,t,r,b; }
 [StructLayout(LayoutKind.Sequential)] public struct POINT { public int x,y; }
}
'@
$p = Get-Process UIDesign | Select-Object -First 1
$main = $p.MainWindowHandle
[PRC]::SetWindowPos($main,[IntPtr]::Zero,180,140,1360,860,0x0004) | Out-Null
Start-Sleep -Milliseconds 900

function Probe([int]$x,[int]$y,[string]$tag) {
  $pt = New-Object PRC+POINT; $pt.x=$x; $pt.y=$y
  $h = [PRC]::WindowFromPoint($pt)
  $sb = New-Object System.Text.StringBuilder 160
  [PRC]::GetClassName($h,$sb,160) | Out-Null
  $dc = [PRC]::GetDC([IntPtr]::Zero)
  $px = [PRC]::GetPixel($dc,$x,$y)
  [PRC]::ReleaseDC([IntPtr]::Zero,$dc) | Out-Null
  "$tag ($x,$y) class=[$($sb.ToString())] pixel=#{0:X6}" -f ($px -band 0xFFFFFF)
}

$r = New-Object PRC+RECT; [PRC]::GetWindowRect($main,[ref]$r) | Out-Null
"rect $($r.l),$($r.t)-$($r.r),$($r.b)"
$np = Start-Process notepad -PassThru; Start-Sleep -Milliseconds 700
[PRC]::SetWindowPos($np.MainWindowHandle,[IntPtr]::Zero,1900,1180,560,180,0x0004) | Out-Null
[PRC]::SetForegroundWindow($np.MainWindowHandle) | Out-Null; Start-Sleep -Milliseconds 1200
$my = [int](($r.t+$r.b)/2); $mx = [int](($r.l+$r.r)/2)
1..6 | ForEach-Object { Probe ($r.r-$_) $my "R$_" }
1..3 | ForEach-Object { Probe $mx ($r.b-$_) "B$_" }
1..3 | ForEach-Object { Probe $mx ($r.t+$_) "T$_" }
1..3 | ForEach-Object { Probe ($r.l+$_) $my "L$_" }
$np | Stop-Process -Force
