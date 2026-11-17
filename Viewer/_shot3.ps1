Add-Type -AssemblyName System.Windows.Forms, System.Drawing
$sig = @'
[DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int c);
[DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
[DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
[DllImport("user32.dll")] public static extern void mouse_event(uint f, uint x, uint y, uint d, IntPtr e);
'@
$u = Add-Type -MemberDefinition $sig -Name Api2 -Namespace W2 -PassThru
$p = Get-Process -Name PyArxViewer -ErrorAction SilentlyContinue | Select-Object -First 1
if ($p) {
    $u::ShowWindow($p.MainWindowHandle, 3) | Out-Null
    $u::SetForegroundWindow($p.MainWindowHandle) | Out-Null
}
Start-Sleep -Seconds 2
$u::SetCursorPos(2104, 1315) | Out-Null
Start-Sleep -Milliseconds 200
$u::mouse_event(2, 0, 0, 0, [IntPtr]::Zero)
Start-Sleep -Milliseconds 100
$u::mouse_event(4, 0, 0, 0, [IntPtr]::Zero)
Start-Sleep -Seconds 4
$r = [System.Windows.Forms.SystemInformation]::VirtualScreen
$bmp = New-Object System.Drawing.Bitmap $r.Width, $r.Height
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.CopyFromScreen($r.Left, $r.Top, 0, 0, $bmp.Size)
$bmp.Save('C:\temp\shot_v4.png', [System.Drawing.Imaging.ImageFormat]::Png)
$g.Dispose(); $bmp.Dispose()
Write-Output 'clicked+shot'
