# 关闭提示保存端到端验证：
# 1) 定位 pane view（ControlBar 下全尺寸 Afx 子窗）；2) 发 WM_DOC_CHANGED 置脏；
# 3) PostMessage WM_CLOSE；4) 枚举 #32770 弹窗；5) 点“不保存”(IDNO=7)，窗口应关闭。
Add-Type -AssemblyName System.Drawing
Add-Type @'
using System; using System.Runtime.InteropServices; using System.Text;
public static class CL {
 [DllImport("user32.dll")] public static extern IntPtr SendMessage(IntPtr h, uint m, IntPtr w, IntPtr l);
 [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h, uint m, IntPtr w, IntPtr l);
 [DllImport("user32.dll")] public static extern IntPtr GetWindow(IntPtr h, uint c);
 [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
 [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
 [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
 [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetClassName(IntPtr h, StringBuilder s, int n);
 [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetWindowTextW(IntPtr h, StringBuilder s, int n);
 [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc cb, IntPtr p);
 [DllImport("user32.dll")] public static extern bool EnumChildWindows(IntPtr h, EnumProc cb, IntPtr p);
 public delegate bool EnumProc(IntPtr h, IntPtr p);
 [StructLayout(LayoutKind.Sequential)] public struct RECT { public int l,t,r,b; }
}
'@

$proc = Get-Process UIDesign | Select-Object -First 1
$main = $proc.MainWindowHandle
Write-Host "main=$main pid=$($proc.Id)"

# main -> 最大子窗口（纯宿主的 pane view 直接挂在 shell 下，不再有 ControlBar 停靠栏）
$bar = [IntPtr]::Zero
Write-Host "bar=$bar"

$pane = [IntPtr]::Zero; $best = 0
$script:best = 0; $script:pane = [IntPtr]::Zero
$cb0 = [CL+EnumProc]{
  param($h,$p)
  $r = New-Object CL+RECT; [CL]::GetWindowRect($h,[ref]$r) | Out-Null
  $area = ($r.r-$r.l)*($r.b-$r.t)
  if ($area -gt $script:best) { $script:best = $area; $script:pane = $h }
  return $true
}
[CL]::EnumChildWindows($main,$cb0,[IntPtr]::Zero) | Out-Null
$pane = $script:pane; $best = $script:best
Write-Host "pane=$pane area=$best"

# 置脏（WM_DOC_CHANGED = WM_APP+2 = 0x8002）
[CL]::SendMessage($pane,0x8002,0,0) | Out-Null
Start-Sleep -Milliseconds 500

# 触发关闭（Post，避免被模态框阻塞）
[CL]::PostMessage($main,0x0010,0,0) | Out-Null
Start-Sleep -Milliseconds 1000

# 枚举同进程可见弹窗
$dlg = [IntPtr]::Zero
$cb2 = [CL+EnumProc]{
  param($h,$p)
  $pid2 = 0; [CL]::GetWindowThreadProcessId($h,[ref]$pid2) | Out-Null
  if ($pid2 -ne $proc.Id -or -not [CL]::IsWindowVisible($h)) { return $true }
  $cls = New-Object Text.StringBuilder 64; [CL]::GetClassName($h,$cls,64) | Out-Null
  $txt = New-Object Text.StringBuilder 256; [CL]::GetWindowTextW($h,$txt,256) | Out-Null
  Write-Host ("  win class={0} text={1}" -f $cls.ToString(), $txt.ToString())
  if ($cls.ToString() -eq '#32770') { $script:dlg = $h }
  return $true
}
[CL]::EnumWindows($cb2,[IntPtr]::Zero) | Out-Null

$r = New-Object CL+RECT; [CL]::GetWindowRect($main,[ref]$r) | Out-Null
if (($r.r-$r.l) -gt 0) {
  $bmp = New-Object System.Drawing.Bitmap ($r.r-$r.l),($r.b-$r.t)
  $g = [System.Drawing.Graphics]::FromImage($bmp)
  $g.CopyFromScreen($r.l,$r.t,0,0,(New-Object System.Drawing.Size ($r.r-$r.l),($r.b-$r.t)))
  $g.Dispose()
  $bmp.Save('G:\PyArx\docs\_close_prompt.png',[System.Drawing.Imaging.ImageFormat]::Png)
  $bmp.Dispose()
}

if ($dlg -ne [IntPtr]::Zero) {
  Write-Host "DIALOG FOUND $dlg -> 点 不保存(IDNO=7)"
  [CL]::SendMessage($dlg,0x0111,7,0) | Out-Null
  Start-Sleep -Milliseconds 800
  $alive = Get-Process -Id $proc.Id -ErrorAction SilentlyContinue
  Write-Host ("after IDNO alive={0}" -f [bool]$alive)
} else {
  Write-Host "NO DIALOG"
}
