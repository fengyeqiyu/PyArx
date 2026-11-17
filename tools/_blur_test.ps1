Add-Type -AssemblyName System.Drawing
Add-Type @'
using System; using System.Runtime.InteropServices;
public static class BL {
 [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
 [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr h, IntPtr a, int x, int y, int cx, int cy, uint f);
 [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
 [StructLayout(LayoutKind.Sequential)] public struct RECT { public int l,t,r,b; }
}
'@
$d = Get-Process UIDesign | Select-Object -First 1
$main = $d.MainWindowHandle
[BL]::SetWindowPos($main,[IntPtr]::Zero,200,150,1400,900,0x0004) | Out-Null
Start-Sleep -Milliseconds 900

$np = Start-Process notepad -PassThru
Start-Sleep -Milliseconds 900
[BL]::SetWindowPos($np.MainWindowHandle,[IntPtr]::Zero,1750,950,600,300,0x0004) | Out-Null
[BL]::SetForegroundWindow($np.MainWindowHandle) | Out-Null
Start-Sleep -Milliseconds 900

$r = New-Object BL+RECT; [BL]::GetWindowRect($main,[ref]$r) | Out-Null
$bw=$r.r-$r.l; $bh=$r.b-$r.t
$bmp = New-Object System.Drawing.Bitmap $bw,$bh
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.CopyFromScreen($r.l,$r.t,0,0,(New-Object System.Drawing.Size $bw,$bh))
$g.Dispose()
$bmp.Save('G:\PyArx\docs\_blur_edge.png',[System.Drawing.Imaging.ImageFormat]::Png)
$bmp.Dispose()
$np | Stop-Process -Force
"shot $bw x $bh"
