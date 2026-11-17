Add-Type -AssemblyName System.Drawing
Add-Type @'
using System; using System.Text; using System.Runtime.InteropServices;
public static class CHK {
 [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
 [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out RECT r);
 [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr h, IntPtr a, int x, int y, int cx, int cy, uint f);
 [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
 [DllImport("user32.dll")] public static extern IntPtr GetDC(IntPtr h);
 [DllImport("user32.dll")] public static extern int ReleaseDC(IntPtr h, IntPtr dc);
 [DllImport("gdi32.dll")] public static extern int GetPixel(IntPtr dc, int x, int y);
 [DllImport("user32.dll")] public static extern bool EnumChildWindows(IntPtr h, EnumProc cb, IntPtr p);
 [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetClassName(IntPtr h, StringBuilder s, int n);
 [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
 [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr hdc, uint f);
 [DllImport("user32.dll")] public static extern bool IsIconic(IntPtr h);
 public delegate bool EnumProc(IntPtr h, IntPtr p);
 [StructLayout(LayoutKind.Sequential)] public struct RECT { public int l,t,r,b; }
}
'@

$p = Get-Process UIDesign | Select-Object -First 1
$main = $p.MainWindowHandle
[CHK]::SetWindowPos($main,[IntPtr]::Zero,180,140,1360,860,0x0004) | Out-Null
Start-Sleep -Milliseconds 900

# 1) 窗口/客户区一致性 + 滑块状态
$wr = New-Object CHK+RECT; [CHK]::GetWindowRect($main,[ref]$wr) | Out-Null
$cr = New-Object CHK+RECT; [CHK]::GetClientRect($main,[ref]$cr) | Out-Null
"window  = $($wr.r-$wr.l)x$($wr.b-$wr.t) at $($wr.l),$($wr.t)"
"client  = $($cr.r)x$($cr.b)  (miss right=$((($wr.r-$wr.l)-$cr.r)) bottom=$((($wr.b-$wr.t)-$cr.b)))"

$sliders = New-Object System.Collections.ArrayList
$cb = [CHK+EnumProc]{ param($h,$x)
  $sb = New-Object System.Text.StringBuilder 128
  [CHK]::GetClassName($h,$sb,128) | Out-Null
  $cn = $sb.ToString()
  if ($cn.Contains('Slider')) {
    $r = New-Object CHK+RECT; [CHK]::GetWindowRect($h,[ref]$r) | Out-Null
    [void]$script:sliders.Add("slider $($r.r-$r.l)x$($r.b-$r.t) at $($r.l),$($r.t) vis=$([CHK]::IsWindowVisible($h))")
  }
  return $true }
[CHK]::EnumChildWindows($main,$cb,[IntPtr]::Zero) | Out-Null
if ($sliders.Count -eq 0) { "sliders : none" } else { $sliders -join "; " }

# 2) 外观证据（窗口自身渲染）
$bmp = New-Object System.Drawing.Bitmap ($wr.r-$wr.l),($wr.b-$wr.t)
$g = [System.Drawing.Graphics]::FromImage($bmp)
$hdc = $g.GetHdc(); [CHK]::PrintWindow($main,$hdc,3) | Out-Null; $g.ReleaseHdc($hdc); $g.Dispose()
$bmp.Save('G:\PyArx\docs\_check_ui.png',[System.Drawing.Imaging.ImageFormat]::Png); $bmp.Dispose()

# 3) 缩放 40 步 + 松手
$c0 = (Get-Process -Id $p.Id).TotalProcessorTime.TotalMilliseconds
for ($i=0; $i -lt 40; $i++) {
  [CHK]::SetWindowPos($main,[IntPtr]::Zero,180,140,(1300+$i*10),(820+$i*7),0x0004) | Out-Null
  Start-Sleep -Milliseconds 8
}
$c1 = (Get-Process -Id $p.Id).TotalProcessorTime.TotalMilliseconds
"resize 40 steps cpu = {0:N0} ms ({1:N2} ms/step)" -f ($c1-$c0), (($c1-$c0)/40)

# 4) 空闲 5 秒
Start-Sleep -Milliseconds 1200
$c2 = (Get-Process -Id $p.Id).TotalProcessorTime.TotalMilliseconds
Start-Sleep -Seconds 5
$p.Refresh()
$c3 = $p.TotalProcessorTime.TotalMilliseconds
"idle 5s cpu = {0:N0} ms" -f ($c3-$c2)

# 5) 失焦后边缘像素（保持设计器可见，另起小窗激活）
$np = Start-Process notepad -PassThru
Start-Sleep -Milliseconds 800
[CHK]::SetWindowPos($np.MainWindowHandle,[IntPtr]::Zero,1900,1180,560,180,0x0004) | Out-Null
[CHK]::SetForegroundWindow($np.MainWindowHandle) | Out-Null
Start-Sleep -Milliseconds 900
$wr2 = New-Object CHK+RECT; [CHK]::GetWindowRect($main,[ref]$wr2) | Out-Null
$dc = [CHK]::GetDC([IntPtr]::Zero)
$midY = [int](($wr2.t+$wr2.b)/2); $midX = [int](($wr2.l+$wr2.r)/2)
$rightPix = (1..6 | ForEach-Object { "#{0:X6}" -f ([CHK]::GetPixel($dc,$wr2.r-$_,$midY) -band 0xFFFFFF) }) -join ' '
$bottomPix = (1..6 | ForEach-Object { "#{0:X6}" -f ([CHK]::GetPixel($dc,$midX,$wr2.b-$_) -band 0xFFFFFF) }) -join ' '
$leftPix = (1..6 | ForEach-Object { "#{0:X6}" -f ([CHK]::GetPixel($dc,$wr2.l+$_,$midY) -band 0xFFFFFF) }) -join ' '
$topPix = (1..6 | ForEach-Object { "#{0:X6}" -f ([CHK]::GetPixel($dc,$midX,$wr2.t+$_) -band 0xFFFFFF) }) -join ' '
"deactivated edges: L[$leftPix] R[$rightPix] T[$topPix] B[$bottomPix]"
[CHK]::ReleaseDC([IntPtr]::Zero,$dc) | Out-Null
$np | Stop-Process -Force
[CHK]::SetForegroundWindow($main) | Out-Null
"checks done"
