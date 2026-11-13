$ErrorActionPreference = 'Stop'
$exe = 'C:\Program Files\Autodesk\AutoCAD 2024\acad.exe'

Add-Type -AssemblyName System.Drawing, System.Windows.Forms

Add-Type @'
using System;
using System.Text;
using System.Runtime.InteropServices;
public class WinScan {
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc cb, IntPtr p);
    public delegate bool EnumProc(IntPtr h, IntPtr p);
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
    [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetClassName(IntPtr h, StringBuilder s, int n);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L, T, R, B; }

    public static IntPtr FindMain(uint pid) {
        IntPtr found = IntPtr.Zero;
        EnumWindows(delegate(IntPtr h, IntPtr p) {
            uint wpid; GetWindowThreadProcessId(h, out wpid);
            if (wpid != pid || !IsWindowVisible(h)) return true;
            StringBuilder sb = new StringBuilder(256);
            GetClassName(h, sb, 256);
            string cls = sb.ToString();
            if (cls.StartsWith("AfxMDIFrame") || cls.StartsWith("AfxFrameOrView")) {
                RECT r; GetWindowRect(h, out r);
                if ((r.R - r.L) > 600 && (r.B - r.T) > 400) { found = h; return false; }
            }
            return true;
        }, IntPtr.Zero);
        return found;
    }
}
'@

$proc = Get-Process acad -ErrorAction SilentlyContinue
if (-not $proc) {
    Write-Host '[driver] 启动 CAD'
    Start-Process -FilePath $exe -ArgumentList '/nologo'
} else { Write-Host '[driver] CAD 已在运行' }

$app = $null
for ($i = 0; $i -lt 90; $i++) {
    Start-Sleep -Seconds 2
    try { $app = [Runtime.InteropServices.Marshal]::GetActiveObject('AutoCAD.Application'); if ($app -and $app.ActiveDocument) { break } } catch { }
}
if (-not $app) { throw 'CAD COM 未就绪' }
Write-Host "[driver] COM 就绪（$i 轮）"
Start-Sleep -Seconds 3

$doc = $app.ActiveDocument
$doc.SendCommand("(arxload `"G:/PyArx/X64/Release/PyArxEngine.arx`")" + "`n")
Start-Sleep -Seconds 4
$doc.SendCommand("PYRUNFILE G:/PyArx/tests/qt_spike/pyarx_qml_demo.py" + "`n")
Write-Host '[driver] 已下发 demo'

Start-Sleep -Seconds 6
$doc.SendCommand("PYRUNFILE G:/PyArx/tests/qt_spike/pyarx_qml_probe.py" + "`n")
Write-Host '[driver] 已下发 probe'
Start-Sleep -Seconds 6

$pid2 = (Get-Process acad | Sort-Object Id | Select-Object -First 1).Id
Write-Host "[driver] acad pid=$pid2"
$hwnd = [WinScan]::FindMain($pid2)
Write-Host "[driver] 主窗 hwnd=$hwnd"

if ($hwnd -ne [IntPtr]::Zero) {
    $r = New-Object WinScan+RECT
    [WinScan]::GetWindowRect($hwnd, [ref]$r) | Out-Null
    $w = $r.R - $r.L; $ht = $r.B - $r.T
    $bmp = New-Object System.Drawing.Bitmap -ArgumentList ([int]$w), ([int]$ht)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.CopyFromScreen($r.L, $r.T, 0, 0, $bmp.Size)
    $bmp.Save('G:\PyArx\docs\_pyarx_qml_module.png', [System.Drawing.Imaging.ImageFormat]::Png)
    $g.Dispose(); $bmp.Dispose()
    Write-Host "[driver] 截图 $w x $ht"
}
$app.Quit()
Start-Sleep -Seconds 5
Get-Process acad -ErrorAction SilentlyContinue | Stop-Process -Force
