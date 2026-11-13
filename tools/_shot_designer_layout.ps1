Add-Type -AssemblyName System.Drawing
Add-Type @'
using System;
using System.Runtime.InteropServices;
public class WinCap {
    public delegate bool EnumProc(IntPtr hWnd, IntPtr lParam);
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc cb, IntPtr lParam);
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint pid);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)] public static extern int GetWindowText(IntPtr hWnd, System.Text.StringBuilder sb, int n);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT r);
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr hWnd, IntPtr hdc, uint flags);
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
}
'@

$exe = 'G:\PyArx\X64\Release\UIDesign.exe'
$doc = 'G:\PyArx\UIDesign\designer_layout.xml'
$out = 'G:\PyArx\docs\_uid_self2.png'

$p = Start-Process -FilePath $exe -ArgumentList $doc -PassThru
Start-Sleep -Seconds 5

$found = [IntPtr]::Zero
$cb = [WinCap+EnumProc]{
    param($h, $l)
    $procId = 0
    [WinCap]::GetWindowThreadProcessId($h, [ref]$procId) | Out-Null
    if ($procId -eq $p.Id) {
        $sb = New-Object System.Text.StringBuilder 256
        [WinCap]::GetWindowText($h, $sb, 256) | Out-Null
        if ($sb.ToString() -like '*PyMfc UI Designer*') { $script:found = $h; return $false }
    }
    return $true
}
[WinCap]::EnumWindows($cb, [IntPtr]::Zero) | Out-Null

if ($found -ne [IntPtr]::Zero) {
    $r = New-Object WinCap+RECT
    [WinCap]::GetWindowRect($found, [ref]$r) | Out-Null
    $w = $r.Right - $r.Left
    $hgt = $r.Bottom - $r.Top
    Write-Output ("win {0}x{1}" -f $w, $hgt)
    $bmp = New-Object System.Drawing.Bitmap $w, $hgt
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $hdc = $g.GetHdc()
    [WinCap]::PrintWindow($found, $hdc, 2) | Out-Null
    $g.ReleaseHdc($hdc)
    $g.Dispose()
    $bmp.Save($out, [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()
    Write-Output 'saved'
} else {
    Write-Output 'window not found'
}
Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue
