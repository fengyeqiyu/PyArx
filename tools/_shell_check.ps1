Add-Type -AssemblyName System.Drawing
Add-Type @'
using System; using System.Runtime.InteropServices;
public static class SH1 {
 [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
 [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr h, IntPtr a, int x, int y, int cx, int cy, uint f);
 [DllImport("user32.dll")] public static extern IntPtr SendMessage(IntPtr h, uint m, IntPtr w, IntPtr l);
 [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h, uint m, IntPtr w, IntPtr l);
 [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
 [DllImport("user32.dll")] public static extern bool IsZoomed(IntPtr h);
 [StructLayout(LayoutKind.Sequential)] public struct RECT { public int l,t,r,b; }
}
'@

# 调试固定放副屏 \\.\DISPLAY2（2560,252 / 1920x1200，工作区 y=252..1420），不占主屏
$SCR_X = 2700; $SCR_Y = 330

function Shot([string]$path) {
  # 抓屏前先临时置顶，避免后台时被其它窗口（聊天窗/浏览器）盖住拿到假样张
  [SH1]::SetWindowPos($main,[IntPtr](-1),0,0,0,0,0x0003) | Out-Null   # HWND_TOPMOST + NOSIZE|NOMOVE
  Start-Sleep -Milliseconds 350
  $r = New-Object SH1+RECT; [SH1]::GetWindowRect($main,[ref]$r) | Out-Null
  $bw=$r.r-$r.l; $bh=$r.b-$r.t
  $bmp = New-Object System.Drawing.Bitmap $bw,$bh
  $g = [System.Drawing.Graphics]::FromImage($bmp)
  $g.CopyFromScreen($r.l,$r.t,0,0,(New-Object System.Drawing.Size $bw,$bh)); $g.Dispose()
  $bmp.Save($path,[System.Drawing.Imaging.ImageFormat]::Png); $bmp.Dispose()
  [SH1]::SetWindowPos($main,[IntPtr](-2),0,0,0,0,0x0003) | Out-Null  # HWND_NOTOPMOST
  "$path ${bw}x${bh} at $($r.l),$($r.t)"
}
$p = Get-Process UIDesign | Where-Object { $_.MainWindowTitle -like '*PyArx*' } | Sort-Object StartTime -Descending | Select-Object -First 1
if ($null -eq $p) { throw '找不到 UIDesign 主窗口' }
$main = $p.MainWindowHandle

# 先搬到副屏再做任何测试
[SH1]::SetWindowPos($main,[IntPtr]::Zero,$SCR_X,$SCR_Y,1200,800,0x0004) | Out-Null
Start-Sleep -Milliseconds 500
$r = New-Object SH1+RECT; [SH1]::GetWindowRect($main,[ref]$r) | Out-Null
"start rect $($r.l),$($r.t) $($r.r-$r.l)x$($r.b-$r.t)"

# 缩放 40 步（带 ENTERSIZEMOVE，模拟真实拖边）
[SH1]::SendMessage($main,0x0231,0,0) | Out-Null
$c0 = (Get-Process -Id $p.Id).TotalProcessorTime.TotalMilliseconds
for ($i=0; $i -lt 40; $i++) {
  [SH1]::SetWindowPos($main,[IntPtr]::Zero,$SCR_X,$SCR_Y,(1200+$i*8),(800+$i*6),0x0004) | Out-Null
  Start-Sleep -Milliseconds 8
}
$c1 = (Get-Process -Id $p.Id).TotalProcessorTime.TotalMilliseconds
[SH1]::SendMessage($main,0x0232,0,0) | Out-Null
"resize 40 steps cpu = {0:N0} ms ({1:N2} ms/step)" -f ($c1-$c0), (($c1-$c0)/40)
Start-Sleep -Milliseconds 600
Shot 'G:\PyArx\docs\_shell_resized.png'

# 空闲 5s
$c2 = (Get-Process -Id $p.Id).TotalProcessorTime.TotalMilliseconds
Start-Sleep -Seconds 5
$p.Refresh()
$c3 = $p.TotalProcessorTime.TotalMilliseconds
"idle 5s cpu = {0:N0} ms" -f ($c3-$c2)

# 最大化 / 还原（走系统命令，验证不盖任务栏 + 布局收尾；最大化落在副屏工作区）
[SH1]::SendMessage($main,0x0112,[IntPtr]0xF030,[IntPtr]::Zero) | Out-Null   # SC_MAXIMIZE
Start-Sleep -Milliseconds 900
$rz = New-Object SH1+RECT; [SH1]::GetWindowRect($main,[ref]$rz) | Out-Null
"maximized rect $($rz.l),$($rz.t) $($rz.r-$rz.l)x$($rz.b-$rz.t) zoomed=$([SH1]::IsZoomed($main))"
Shot 'G:\PyArx\docs\_shell_max.png'
[SH1]::SendMessage($main,0x0112,[IntPtr]0xF120,[IntPtr]::Zero) | Out-Null   # SC_RESTORE
Start-Sleep -Milliseconds 800
$rr = New-Object SH1+RECT; [SH1]::GetWindowRect($main,[ref]$rr) | Out-Null
"restored rect $($rr.l),$($rr.t) $($rr.r-$rr.l)x$($rr.b-$rr.t) zoomed=$([SH1]::IsZoomed($main))"
