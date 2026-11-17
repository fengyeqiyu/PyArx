# 录制“编辑器窗口大小调整”：四条边各拖一次，全部模拟消息，不动真实鼠标。
# 输出：docs\_rz_frames\*.png + docs\_rz_meta.txt（每帧窗口位置尺寸），由 python 合成为 GIF。
$ErrorActionPreference = 'Stop'
$out = 'G:\PyArx\docs\_rz_frames'
if (Test-Path -LiteralPath $out) { Remove-Item -LiteralPath $out -Recurse -Force }
New-Item -ItemType Directory -Path $out | Out-Null
$meta = New-Object System.Collections.Generic.List[string]

Add-Type -AssemblyName System.Drawing
Add-Type @'
using System; using System.Runtime.InteropServices;
public static class RZ {
  [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr h, IntPtr a, int x, int y, int cx, int cy, uint f);
  [DllImport("user32.dll")] public static extern IntPtr SendMessage(IntPtr h, uint m, IntPtr w, IntPtr l);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] public static extern bool EnumChildWindows(IntPtr h, EnumProc cb, IntPtr p);
  public delegate bool EnumProc(IntPtr h, IntPtr p);
  [DllImport("user32.dll")] public static extern int GetClassName(IntPtr h, System.Text.StringBuilder s, int n);
  [DllImport("user32.dll")] public static extern IntPtr WindowFromPoint(POINT p);
  [StructLayout(LayoutKind.Sequential)] public struct POINT { public int x,y; }
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int l,t,r,b; }
}
'@

Get-Process UIDesign -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Seconds 1
$proc = Start-Process -FilePath 'G:\PyArx\X64\Release\UIDesign.exe' -ArgumentList 'G:\PyArx\tests\mfc_xml\power_plan.xml' -PassThru
Start-Sleep -Seconds 12
$hwnd = $proc.MainWindowHandle
[RZ]::SetWindowPos($hwnd, [IntPtr]::new(-1), 200, 200, 1400, 900, 0x0010) | Out-Null
Start-Sleep -Milliseconds 1500

# 找满客户区的 pane 子窗口
$pane = [IntPtr]::Zero
$cr = New-Object RZ+RECT
[RZ]::GetClientRect($hwnd, [ref]$cr) | Out-Null
$pt = New-Object RZ+POINT
$pt.x = 210; $pt.y = 620
$pane = [RZ]::WindowFromPoint($pt)

$i = 0
function Shot {
  $script:i++
  $r = New-Object RZ+RECT
  [RZ]::GetWindowRect($hwnd, [ref]$r) | Out-Null
  $w = $r.r - $r.l; $hgt = $r.b - $r.t
  if ($w -le 0 -or $hgt -le 0) { return }
  $bmp = New-Object System.Drawing.Bitmap $w, $hgt
  $g = [System.Drawing.Graphics]::FromImage($bmp)
  $g.CopyFromScreen($r.l, $r.t, 0, 0, (New-Object System.Drawing.Size $w, $hgt))
  $g.Dispose()
  $p = Join-Path $out ('f{0:d3}.png' -f $script:i)
  $bmp.Save($p, [System.Drawing.Imaging.ImageFormat]::Png)
  $bmp.Dispose()
  $script:meta.Add("$p,$($r.l),$($r.t),$w,$hgt")
}

function Pump([double]$sec, [int]$fps = 8) {
  $end = (Get-Date).AddSeconds($sec)
  while ((Get-Date) -lt $end) { Shot; Start-Sleep -Milliseconds ([int](1000 / $fps)) }
}

function Drag([int]$edges, [int]$steps, [int]$dx, [int]$dy) {
  # 直接按步进改窗口矩形：等价于把某条边拖到目标位置，但不经过坐标换算
  $r = New-Object RZ+RECT
  [RZ]::GetWindowRect($hwnd, [ref]$r) | Out-Null
  for ($k = 1; $k -le $steps; $k++) {
    $nl = $r.l; $nt = $r.t; $nr = $r.r; $nb = $r.b
    if ($edges -band 1) { $nl += $dx * $k }
    if ($edges -band 2) { $nr += $dx * $k }
    if ($edges -band 4) { $nt += $dy * $k }
    if ($edges -band 8) { $nb += $dy * $k }
    if (($nr - $nl) -lt 720) { if ($edges -band 1) { $nl = $nr - 720 } else { $nr = $nl + 720 } }
    if (($nb - $nt) -lt 480) { if ($edges -band 4) { $nt = $nb - 480 } else { $nb = $nt + 480 } }
    [RZ]::SetWindowPos($hwnd, [IntPtr]::new(-1), $nl, $nt, ($nr - $nl), ($nb - $nt), 0x0014) | Out-Null
    Pump 0.125 8
  }
  Pump 0.6 8
}

Pump 1.0 8
Drag 2 10 22 0     # 右边框向右拉宽
Drag 8 8 0 20    # 下边框向下拉高
Drag 1 6 -20 0    # 左边框向左拉宽
Drag 4 5 0 -16    # 上边框向上拉高
Pump 0.8 8

[IO.File]::WriteAllLines('G:\PyArx\docs\_rz_meta.txt', $meta, (New-Object Text.UTF8Encoding $false))
"frames=$($meta.Count) pane=$pane"
Get-Process UIDesign -ErrorAction SilentlyContinue | Stop-Process -Force
