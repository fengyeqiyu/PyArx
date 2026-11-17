param([string]$Out = "G:\PyArx\docs\_cad_scr.png")
Add-Type -AssemblyName System.Drawing
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public struct RECT { public int Left, Top, Right, Bottom; }
public class W {
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
}
'@
$p = Get-Process python -ErrorAction SilentlyContinue | Where-Object { $_.MainWindowTitle -like '*CAD*' } | Select-Object -First 1
if (-not $p) { Write-Output "no window"; exit 1 }
$r = New-Object RECT
[void][W]::GetWindowRect($p.MainWindowHandle, [ref]$r)
$w = $r.Right - $r.Left; $h = $r.Bottom - $r.Top
if ($w -le 0 -or $h -le 0) { Write-Output "bad rect"; exit 1 }
$bmp = New-Object System.Drawing.Bitmap $w, $h
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.CopyFromScreen($r.Left, $r.Top, 0, 0, (New-Object System.Drawing.Size $w, $h))
$bmp.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
$g.Dispose(); $bmp.Dispose()
Write-Output ("rect {0},{1} {2}x{3} -> {4}" -f $r.Left, $r.Top, $w, $h, $Out)
