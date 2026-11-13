# 抓 Viewer 预览窗口（按 PID 找窗口，PrintWindow 存 png）
param([string]$Xml = "G:\PyArx\UIDesign\samples\15_qml_baseline.xml", [string]$Out = "G:\PyArx\docs\_viewer_flat.png")
$env:PYARX_D2D_RT = "software"
$p = Start-Process -FilePath "G:\PyArx\X64\Release\Viewer.exe" -ArgumentList "`"$Xml`"" -PassThru
Start-Sleep -Milliseconds 1600

Add-Type -Namespace W -Name N -MemberDefinition @'
[DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr hdc, uint f);
[DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
[DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc cb, IntPtr l);
[DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
[DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
public delegate bool EnumProc(IntPtr h, IntPtr l);
public struct RECT { public int L, T, R, B; }
'@
$found = [IntPtr]::Zero
$cb = [W.N+EnumProc]{
    param($h, $l)
    $pid2 = 0
    [void][W.N]::GetWindowThreadProcessId($h, [ref]$pid2)
    if ($pid2 -eq $p.Id -and [W.N]::IsWindowVisible($h)) {
        $r = New-Object W.N+RECT
        [void][W.N]::GetWindowRect($h, [ref]$r)
        if (($r.R - $r.L) -gt 100) { $script:found = $h; return $false }
    }
    return $true
}
[void][W.N]::EnumWindows($cb, [IntPtr]::Zero)

if ($found -eq [IntPtr]::Zero) { Write-Output "no window"; exit 1 }
$r = New-Object W.N+RECT
[void][W.N]::GetWindowRect($found, [ref]$r)
$w = $r.R - $r.L; $h = $r.B - $r.T
Add-Type -AssemblyName System.Drawing
$bmp = New-Object System.Drawing.Bitmap($w, $h)
$g = [System.Drawing.Graphics]::FromImage($bmp)
$hdc = $g.GetHdc()
[void][W.N]::PrintWindow($found, $hdc, 2)
$g.ReleaseHdc($hdc); $g.Dispose()
$bmp.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
$bmp.Dispose()
Write-Output "saved $Out ${w}x${h}"
Stop-Process -Id $p.Id -Force
