Add-Type @'
using System; using System.Text; using System.Runtime.InteropServices;
public static class PRB {
 [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
 [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr h, IntPtr a, int x, int y, int cx, int cy, uint f);
 [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
 [DllImport("user32.dll")] public static extern IntPtr WindowFromPoint(POINT p);
 [DllImport("user32.dll")] public static extern IntPtr GetParent(IntPtr h);
 [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetClassName(IntPtr h, StringBuilder s, int n);
 [DllImport("user32.dll")] public static extern IntPtr GetDC(IntPtr h);
 [DllImport("user32.dll")] public static extern int ReleaseDC(IntPtr h, IntPtr dc);
 [DllImport("gdi32.dll")] public static extern int GetPixel(IntPtr dc, int x, int y);
 [DllImport("user32.dll")] public static extern IntPtr ChildWindowFromPointEx(IntPtr h, POINT p, uint f);
 [StructLayout(LayoutKind.Sequential)] public struct RECT { public int l,t,r,b; }
 [StructLayout(LayoutKind.Sequential)] public struct POINT { public int x,y; }
}
'@
$p = Get-Process UIDesign | Select-Object -First 1
$main = $p.MainWindowHandle
$r = New-Object PRB+RECT; [PRB]::GetWindowRect($main,[ref]$r) | Out-Null
"window $($r.l),$($r.t) - $($r.r),$($r.b)"

function Probe([int]$x,[int]$y,[string]$tag) {
  $pt = New-Object PRB+POINT; $pt.x=$x; $pt.y=$y
  $h = [PRB]::WindowFromPoint($pt)
  $sb = New-Object System.Text.StringBuilder 160
  [PRB]::GetClassName($h,$sb,160) | Out-Null
  $dc = [PRB]::GetDC([IntPtr]::Zero)
  $px = [PRB]::GetPixel($dc,$x,$y)
  [PRB]::ReleaseDC([IntPtr]::Zero,$dc) | Out-Null
  "$tag ($x,$y) hwnd=$h class=[$($sb.ToString())] pixel=#{0:X6}" -f ($px -band 0xFFFFFF)
}

# 先激活（正常状态）
[PRB]::SetForegroundWindow($main) | Out-Null; Start-Sleep -Milliseconds 600
Probe ($r.r-2) ([int](($r.t+$r.b)/2)) 'active  right'
Probe ([int](($r.l+$r.r)/2)) ($r.b-2) 'active  bottom'

# 失焦：起 notepad 放右下角
$np = Start-Process notepad -PassThru; Start-Sleep -Milliseconds 700
[PRB]::SetWindowPos($np.MainWindowHandle,[IntPtr]::Zero,1900,1180,560,180,0x0004) | Out-Null
[PRB]::SetForegroundWindow($np.MainWindowHandle) | Out-Null; Start-Sleep -Milliseconds 900
Probe ($r.r-2) ([int](($r.t+$r.b)/2)) 'inactive right'
Probe ($r.r-5) ([int](($r.t+$r.b)/2)) 'inactive right-5'
Probe ([int](($r.l+$r.r)/2)) ($r.b-2) 'inactive bottom'
Probe ([int](($r.l+$r.r)/2)) ($r.t+2) 'inactive top'
Probe ($r.l+2) ([int](($r.t+$r.b)/2)) 'inactive left'
$np | Stop-Process -Force
