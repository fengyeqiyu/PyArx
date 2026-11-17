# 预览器交互验证：点卡片选中 / 滚轮缩放 / 中键平移，各抓一张
param(
  [string]$Exe = "G:\PyArx\X64\Release\Viewer.exe",
  [string]$Xml = "G:\PyArx\X64\Release\power_plan_shell.xml",
  [string]$Prefix = "G:\PyArx\docs\_pp_vi"
)
Add-Type @"
using System;
using System.Runtime.InteropServices;
using System.Drawing;
public class VI {
  [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc cb, IntPtr l);
  public delegate bool EnumProc(IntPtr h, IntPtr l);
  [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr h, ref POINT p);
  [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h, uint msg, IntPtr w, IntPtr l);
  [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr h, IntPtr after, int x, int y, int cx, int cy, uint f);
  [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int cmd);
  [DllImport("user32.dll")] public static extern bool IsIconic(IntPtr h);
  [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr hdc, uint flags);
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
  [DllImport("user32.dll")] public static extern IntPtr GetParent(IntPtr h);
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
  [StructLayout(LayoutKind.Sequential)] public struct POINT { public int X, Y; }
}
public class VPick { public uint pid; public IntPtr best; public int area; }
"@ -ReferencedAssemblies System.Drawing

function Shot([IntPtr]$h, [string]$path) {
  $r = New-Object VI+RECT
  [void][VI]::GetWindowRect($h, [ref]$r)
  $w = $r.Right - $r.Left; $hh = $r.Bottom - $r.Top
  $bmp = New-Object System.Drawing.Bitmap $w, $hh
  $g = [System.Drawing.Graphics]::FromImage($bmp)
  [void][VI]::PrintWindow($h, $g.GetHdc(), 2)
  $g.ReleaseHdc()
  $bmp.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
  "saved $path"
}

$env:PYARX_D2D_RT = 'software'
$p = Start-Process -FilePath $Exe -ArgumentList $Xml -PassThru -WorkingDirectory (Split-Path $Exe)
Start-Sleep -Milliseconds 800
$pick = New-Object VPick
$pick.pid = [uint32]$p.Id
for ($i = 0; $i -lt 40 -and $pick.best -eq [IntPtr]::Zero; $i++) {
  [VI]::EnumWindows({
      param($h, $l)
      if (-not [VI]::IsWindowVisible($h)) { return $true }
      if ([VI]::GetParent($h) -ne [IntPtr]::Zero) { return $true }
      $pid2 = [uint32]0
      [void][VI]::GetWindowThreadProcessId($h, [ref]$pid2)
      if ($pid2 -ne $pick.pid) { return $true }
      $r = New-Object VI+RECT
      [void][VI]::GetWindowRect($h, [ref]$r)
      if (($r.Right - $r.Left) -lt 400) { return $true }
      $a = ($r.Right - $r.Left) * ($r.Bottom - $r.Top)
      if ($a -gt $pick.area) { $pick.area = $a; $pick.best = $h }
      return $true
    }, [IntPtr]::Zero) | Out-Null
  Start-Sleep -Milliseconds 200
}
if ($pick.best -eq [IntPtr]::Zero) { "window not found"; # 5) 点「网格」按钮（视图页显示组第三项）
[void][VI]::PostMessage($pick.best, 0x0201, [IntPtr]1, (LP 366 80))
Start-Sleep -Milliseconds 100
[void][VI]::PostMessage($pick.best, 0x0202, [IntPtr]0, (LP 366 80))
Start-Sleep -Milliseconds 800
Shot $pick.best "$($Prefix)gridoff.png"
# 6) 视图页放大两次后看小地图视口框
[void][VI]::PostMessage($pick.best, 0x0201, [IntPtr]1, (LP 118 80))
Start-Sleep -Milliseconds 80
[void][VI]::PostMessage($pick.best, 0x0202, [IntPtr]0, (LP 118 80))
Start-Sleep -Milliseconds 300
[void][VI]::PostMessage($pick.best, 0x0201, [IntPtr]1, (LP 118 80))
Start-Sleep -Milliseconds 80
[void][VI]::PostMessage($pick.best, 0x0202, [IntPtr]0, (LP 118 80))
Start-Sleep -Milliseconds 300
[void][VI]::PostMessage($pick.best, 0x0201, [IntPtr]1, (LP 118 80))
Start-Sleep -Milliseconds 80
[void][VI]::PostMessage($pick.best, 0x0202, [IntPtr]0, (LP 118 80))
Start-Sleep -Milliseconds 700
Shot $pick.best "$($Prefix)minizoom.png"

# 7) 点小地图右下角：主画布应居中过去
[void][VI]::PostMessage($pick.best, 0x0201, [IntPtr]1, (LP 1290 600))
Start-Sleep -Milliseconds 100
[void][VI]::PostMessage($pick.best, 0x0202, [IntPtr]0, (LP 1290 600))
Start-Sleep -Milliseconds 800
Shot $pick.best "$($Prefix)minijump.png"
# 9) 小地图上按下并拖动：主画布跟随居中
[void][VI]::PostMessage($pick.best, 0x0201, [IntPtr]1, (LP 1030 480))
Start-Sleep -Milliseconds 120
[void][VI]::PostMessage($pick.best, 0x0200, [IntPtr]0, (LP 1200 590))
Start-Sleep -Milliseconds 60
[void][VI]::PostMessage($pick.best, 0x0200, [IntPtr]0, (LP 1260 600))
Start-Sleep -Milliseconds 60
[void][VI]::PostMessage($pick.best, 0x0202, [IntPtr]0, (LP 1260 600))
Start-Sleep -Milliseconds 700
Shot $pick.best "$($Prefix)minidrag_move.png"
Stop-Process -Id $p.Id -Force; exit 1 }
if ([VI]::IsIconic($pick.best)) { [void][VI]::ShowWindow($pick.best, 9) }
[void][VI]::SetWindowPos($pick.best, [IntPtr](-1), 20, 20, 0, 0, 0x0001 -bor 0x0004)
[void][VI]::SetForegroundWindow($pick.best)
Start-Sleep -Milliseconds 1800

function LP($x, $y) { return [IntPtr](((($y) -shl 16) -bor (($x) -band 0xFFFF))); }
function C2S($x, $y) {
  $pt = New-Object VI+POINT
  $pt.X = $x; $pt.Y = $y
  [void][VI]::ClientToScreen($pick.best, [ref]$pt)
  return [IntPtr](((($pt.Y) -shl 16) -bor (($pt.X) -band 0xFFFF)))
}

# 1) 点卡片（画布内第一台设备附近）选中
[void][VI]::PostMessage($pick.best, 0x0201, [IntPtr]1, (LP 542 332))
Start-Sleep -Milliseconds 120
[void][VI]::PostMessage($pick.best, 0x0202, [IntPtr]0, (LP 542 332))
Start-Sleep -Milliseconds 600
Shot $pick.best "$($Prefix)select.png"

# 2) 滚轮放大 3 格
for ($i = 0; $i -lt 3; $i++) {
  [void][VI]::PostMessage($pick.best, 0x020A, [IntPtr](120 -shl 16), (LP 500 500))
  Start-Sleep -Milliseconds 150
}
Start-Sleep -Milliseconds 600
Shot $pick.best "$($Prefix)zoom.png"

# 3) 点 Ribbon「视图」页
[void][VI]::PostMessage($pick.best, 0x0201, [IntPtr]1, (LP 112 40))
Start-Sleep -Milliseconds 100
[void][VI]::PostMessage($pick.best, 0x0202, [IntPtr]0, (LP 112 40))
Start-Sleep -Milliseconds 700
Shot $pick.best "$($Prefix)tabview.png"

# 4) 点视图页第一个按钮（适应窗口）
[void][VI]::PostMessage($pick.best, 0x0201, [IntPtr]1, (LP 70 100))
Start-Sleep -Milliseconds 100
[void][VI]::PostMessage($pick.best, 0x0202, [IntPtr]0, (LP 70 100))
Start-Sleep -Milliseconds 800
Shot $pick.best "$($Prefix)fit.png"

# 5) 点「网格」按钮（视图页显示组第三项）
[void][VI]::PostMessage($pick.best, 0x0201, [IntPtr]1, (LP 366 80))
Start-Sleep -Milliseconds 100
[void][VI]::PostMessage($pick.best, 0x0202, [IntPtr]0, (LP 366 80))
Start-Sleep -Milliseconds 800
Shot $pick.best "$($Prefix)gridoff.png"
# 6) 视图页放大两次后看小地图视口框
[void][VI]::PostMessage($pick.best, 0x0201, [IntPtr]1, (LP 118 80))
Start-Sleep -Milliseconds 80
[void][VI]::PostMessage($pick.best, 0x0202, [IntPtr]0, (LP 118 80))
Start-Sleep -Milliseconds 300
[void][VI]::PostMessage($pick.best, 0x0201, [IntPtr]1, (LP 118 80))
Start-Sleep -Milliseconds 80
[void][VI]::PostMessage($pick.best, 0x0202, [IntPtr]0, (LP 118 80))
Start-Sleep -Milliseconds 300
[void][VI]::PostMessage($pick.best, 0x0201, [IntPtr]1, (LP 118 80))
Start-Sleep -Milliseconds 80
[void][VI]::PostMessage($pick.best, 0x0202, [IntPtr]0, (LP 118 80))
Start-Sleep -Milliseconds 700
Shot $pick.best "$($Prefix)minizoom.png"

# 7) 点小地图右下角：主画布应居中过去
[void][VI]::PostMessage($pick.best, 0x0201, [IntPtr]1, (LP 1290 600))
Start-Sleep -Milliseconds 100
[void][VI]::PostMessage($pick.best, 0x0202, [IntPtr]0, (LP 1290 600))
Start-Sleep -Milliseconds 800
Shot $pick.best "$($Prefix)minijump.png"
# 8) 小地图上按下并拖动：主画布连续跟随
[void][VI]::PostMessage($pick.best, 0x0201, [IntPtr]1, (LP 1060 520))
Start-Sleep -Milliseconds 120
[void][VI]::PostMessage($pick.best, 0x0200, [IntPtr]0, (LP 1150 560))
Start-Sleep -Milliseconds 60
[void][VI]::PostMessage($pick.best, 0x0200, [IntPtr]0, (LP 1250 595))
Start-Sleep -Milliseconds 60
[void][VI]::PostMessage($pick.best, 0x0202, [IntPtr]0, (LP 1250 595))
Start-Sleep -Milliseconds 700
Shot $pick.best "$($Prefix)minidrag.png"

# 9) 点「小地图」按钮：整块迷你画布隐藏
[void][VI]::PostMessage($pick.best, 0x0201, [IntPtr]1, (LP 490 80))
Start-Sleep -Milliseconds 100
[void][VI]::PostMessage($pick.best, 0x0202, [IntPtr]0, (LP 490 80))
Start-Sleep -Milliseconds 700
Shot $pick.best "$($Prefix)minioff.png"

Stop-Process -Id $p.Id -Force