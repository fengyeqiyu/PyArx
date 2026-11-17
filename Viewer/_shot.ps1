Add-Type -AssemblyName System.Windows.Forms, System.Drawing
Get-Process -Name PyArxViewer -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Seconds 2
Start-Process 'G:\PyArx\viewer\PyArxViewer.exe'
Start-Sleep -Seconds 14
$r = [System.Windows.Forms.SystemInformation]::VirtualScreen
$bmp = New-Object System.Drawing.Bitmap $r.Width, $r.Height
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.CopyFromScreen($r.Left, $r.Top, 0, 0, $bmp.Size)
$bmp.Save('C:\temp\shot_axis.png', [System.Drawing.Imaging.ImageFormat]::Png)
$g.Dispose(); $bmp.Dispose()
Write-Output ('shot ' + $r.Width + 'x' + $r.Height)
