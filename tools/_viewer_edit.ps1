# 预览器「节点编辑」侧板验证：打开 → 选行 → 改负载 → 应用 → 关闭
param(
  [string]$Exe = "G:\PyArx\X64\Release\Viewer.exe",
  [string]$Xml = "G:\PyArx\X64\Release\power_plan_shell.xml"
)
Add-Type @"
using System;
using System.Runtime.InteropServices;
using System.Drawing;
public class ED {
  [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc cb, IntPtr l);
  public delegate bool EnumProc(IntPtr h, IntPtr l);
  [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h, uint msg, IntPtr w, IntPtr l);
  [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int cmd);
  [DllImport("user32.dll")] public static extern bool IsIconic(IntPtr h);
  [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr hdc, uint flags);
  [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
  [DllImport("user32.dll")] public static extern IntPtr GetParent(IntPtr h);
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
}
public class EDPick { public uint pid; public IntPtr best; public int area; }
"@ -ReferencedAssemblies System.Drawing

function LP([int]$x, [int]$y) { return [IntPtr]([long]($x -band 0xFFFF) -bor ([long]($y -band 0xFFFF) -shl 16)) }

function Shot([IntPtr]$h, [string]$path) {
  $r = New-Object ED+RECT
  [void][ED]::GetWindowRect($h, [ref]$r)
  $w = $r.Right - $r.Left; $hh = $r.Bottom - $r.Top
  $bmp = New-Object System.Drawing.Bitmap $w, $hh
  $g = [System.Drawing.Graphics]::FromImage($bmp)
  [void][ED]::PrintWindow($h, $g.GetHdc(), 2)
  $g.ReleaseHdc()
  $bmp.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
  "saved $path"
}

function Click([IntPtr]$h, [int]$x, [int]$y) {
  [void][ED]::PostMessage($h, 0x0201, [IntPtr]1, (LP $x $y))
  Start-Sleep -Milliseconds 90
  [void][ED]::PostMessage($h, 0x0202, [IntPtr]0, (LP $x $y))
  Start-Sleep -Milliseconds 500
}

$env:PYARX_D2D_RT = 'software'
$p = Start-Process -FilePath $Exe -ArgumentList $Xml -PassThru -WorkingDirectory (Split-Path $Exe)
Start-Sleep -Milliseconds 1000
$pick = New-Object EDPick
$pick.pid = [uint32]$p.Id
for ($i = 0; $i -lt 40 -and $pick.best -eq [IntPtr]::Zero; $i++) {
  [ED]::EnumWindows({
      param($h, $l)
      if (-not [ED]::IsWindowVisible($h)) { return $true }
      if ([ED]::GetParent($h) -ne [IntPtr]::Zero) { return $true }
      $pid2 = [uint32]0
      [void][ED]::GetWindowThreadProcessId($h, [ref]$pid2)
      if ($pid2 -ne $pick.pid) { return $true }
      $r = New-Object ED+RECT
      [void][ED]::GetWindowRect($h, [ref]$r)
      if (($r.Right - $r.Left) -lt 400) { return $true }
      $a = ($r.Right - $r.Left) * ($r.Bottom - $r.Top)
      if ($a -gt $pick.area) { $pick.area = $a; $pick.best = $h }
      return $true
    }, [IntPtr]::Zero) | Out-Null
  Start-Sleep -Milliseconds 200
}
if ($pick.best -eq [IntPtr]::Zero) { throw "window not found" }
if ([ED]::IsIconic($pick.best)) { [void][ED]::ShowWindow($pick.best, 9); Start-Sleep -Milliseconds 300 }

# 1) 打开节点编辑侧板
Click $pick.best 500 88
Shot $pick.best "G:\PyArx\docs\_pp_ed_open.png"
# 2) 点表格里的 P1 行 → 表单与画布同步选中
Click $pick.best 900 300
Shot $pick.best "G:\PyArx\docs\_pp_ed_sel.png"
# 3) 负载 +1 两次，再应用
Click $pick.best 870 771
Click $pick.best 870 771
Shot $pick.best "G:\PyArx\docs\_pp_ed_load.png"
Click $pick.best 966 771
Shot $pick.best "G:\PyArx\docs\_pp_ed_apply.png"
# 4) 关闭侧板，主画布恢复 978
Click $pick.best 1062 771
Shot $pick.best "G:\PyArx\docs\_pp_ed_close.png"

Stop-Process -Id $p.Id -Force
