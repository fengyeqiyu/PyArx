Add-Type -AssemblyName System.Windows.Forms, System.Drawing
$p = Get-Process -Name PyArxViewer -ErrorAction SilentlyContinue
if ($p) {
    $sig = '[DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int c); [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);'
    $u = Add-Type -MemberDefinition $sig -Name WinApi -Namespace W -PassThru
    $u::ShowWindow($p.MainWindowHandle, 3) | Out-Null
    $u::SetForegroundWindow($p.MainWindowHandle) | Out-Null
    Start-Sleep -Seconds 3
}
$r = [System.Windows.Forms.SystemInformation]::VirtualScreen
$bmp = New-Object System.Drawing.Bitmap $r.Width, $r.Height
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.CopyFromScreen($r.Left, $r.Top, 0, 0, $bmp.Size)
$bmp.Save('C:\temp\shot_max.png', [System.Drawing.Imaging.ImageFormat]::Png)
$g.Dispose(); $bmp.Dispose()
Write-Output 'ok'
