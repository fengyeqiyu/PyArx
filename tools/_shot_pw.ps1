param([string]$Out = "G:\PyArx\docs\_shot.png", [string]$Title = "*CAD*", [string]$Proc = "python")
Add-Type -AssemblyName System.Drawing
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public struct RECT { public int Left, Top, Right, Bottom; }
public class W {
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr hdc, int flags);
}
'@
$p = Get-Process $Proc -ErrorAction SilentlyContinue | Where-Object { $_.MainWindowTitle -like $Title } | Select-Object -First 1
if (-not $p) { Write-Output "no window: $Title"; exit 1 }
$h = $p.MainWindowHandle
$r = New-Object RECT
[void][W]::GetWindowRect($h, [ref]$r)
$w = $r.Right - $r.Left; $ht = $r.Bottom - $r.Top
if ($w -le 0 -or $ht -le 0) { Write-Output "bad rect"; exit 1 }
$bmp = New-Object System.Drawing.Bitmap $w, $ht
$g = [System.Drawing.Graphics]::FromImage($bmp)
$hdc = $g.GetHdc()
[void][W]::PrintWindow($h, $hdc, 2)
$g.ReleaseHdc($hdc); $g.Dispose()
$bmp.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
$bmp.Dispose()
Write-Output ("[{0}] {1}x{2} -> {3}" -f $p.MainWindowTitle, $w, $ht, $Out)
