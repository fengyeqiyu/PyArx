# 关闭提示冒烟测试：改脏文档 → 关窗 → 截图确认弹出“是否保存”
# 用法：.\_test_close_prompt.ps1
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class W {
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc cb, IntPtr p);
    [DllImport("user32.dll")] public static extern bool EnumChildWindows(IntPtr h, EnumProc cb, IntPtr p);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetWindowText(IntPtr h, System.Text.StringBuilder s, int n);
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h, uint m, IntPtr w, IntPtr l);
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr hdc, uint flags);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
    public delegate bool EnumProc(IntPtr h, IntPtr p);
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int l, t, r, b; }
    public static IntPtr FindTop(uint pid, string cls) {
        IntPtr found = IntPtr.Zero;
        EnumWindows((h, p) => {
            uint id; GetWindowThreadProcessId(h, out id);
            if (id == pid && IsWindowVisible(h)) {
                var sb = new System.Text.StringBuilder(1024); GetWindowText(h, sb, 1024);
                if (cls == null || sb.ToString().Length > 0) { found = h; return false; }
            }
            return true;
        }, IntPtr.Zero);
        return found;
    }
    public static IntPtr FindChild(IntPtr parent) {
        IntPtr found = IntPtr.Zero;
        EnumChildWindows(parent, (h, p) => { if (IsWindowVisible(h)) { found = h; return false; } return true; }, IntPtr.Zero);
        return found;
    }
    public static IntPtr FindDialog(uint pid) {
        IntPtr found = IntPtr.Zero;
        EnumWindows((h, p) => {
            uint id; GetWindowThreadProcessId(h, out id);
            if (id == pid && IsWindowVisible(h)) {
                var sb = new System.Text.StringBuilder(1024); GetWindowText(h, sb, 1024);
                if (sb.ToString().Contains("XML") || sb.ToString().Contains("保存") || sb.ToString().Contains("修改")) {
                    var r = new RECT(); GetWindowRect(h, out r);
                    if (r.r - r.l < 700 && r.b - r.t < 400) { found = h; return false; }
                }
            }
            return true;
        }, IntPtr.Zero);
        return found;
    }
}
'@

$exe = 'G:\PyArx\X64\Release\UIDesign.exe'
$doc = 'G:\PyArx\UIDesign\samples\01_basic_controls.xml'
$p = Start-Process -FilePath $exe -ArgumentList @($doc) -PassThru
Start-Sleep -Seconds 8

$main = [W]::FindTop([uint32]$p.Id, $null)
"main=0x{0:X}" -f [int64]$main

# 弄脏：往 pane 子窗口投 WM_APP+2（画布就地编辑用的同一条消息）
$pane = [W]::FindChild($main)
"pane=0x{0:X}" -f [int64]$pane
if ($pane -ne [IntPtr]::Zero) { [W]::PostMessage($pane, 0x8000 + 2, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null }
Start-Sleep -Milliseconds 500

# 关窗 → 应该弹“是否保存”
[W]::PostMessage($main, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
Start-Sleep -Milliseconds 1200

$dlg = [W]::FindDialog([uint32]$p.Id)
if ($dlg -eq [IntPtr]::Zero) {
    'no dialog'
} else {
    $r = New-Object W+RECT
    [W]::GetWindowRect($dlg, [ref]$r) | Out-Null
    $w = $r.r - $r.l; $h = $r.b - $r.t
    $bmp = New-Object System.Drawing.Bitmap $w, $h
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $hdc = $g.GetHdc()
    [W]::PrintWindow($dlg, $hdc, 2) | Out-Null
    $g.ReleaseHdc($hdc)
    $g.Dispose()
    $bmp.Save('G:\PyArx\docs\_close_prompt.png', [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()
    "saved G:\PyArx\docs\_close_prompt.png ${w}x${h}"
}
Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue
