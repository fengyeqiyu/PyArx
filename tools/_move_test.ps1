Add-Type -AssemblyName System.Drawing
Add-Type @'
using System; using System.Runtime.InteropServices; using System.Text;
public static class MV {
 [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
 [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr h, IntPtr a, int x, int y, int cx, int cy, uint f);
 [DllImport("user32.dll")] public static extern IntPtr SendMessage(IntPtr h, uint m, IntPtr w, IntPtr l);
 [DllImport("user32.dll")] public static extern IntPtr GetWindow(IntPtr h, uint c);
 [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetClassName(IntPtr h, StringBuilder s, int n);
 [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr hdc, uint f);
 [StructLayout(LayoutKind.Sequential)] public struct RECT { public int l,t,r,b; }
}
'@

$p = Get-Process UIDesign | Select-Object -First 1
$main = $p.MainWindowHandle
[MV]::SetWindowPos($main,[IntPtr]::Zero,120,120,1400,900,0x0004) | Out-Null
Start-Sleep -Milliseconds 1200

function Shot([string]$path) {
  $r = New-Object MV+RECT; [MV]::GetWindowRect($main,[ref]$r) | Out-Null
  $bw = $r.r-$r.l; $bh = $r.b-$r.t
  $bmp = New-Object System.Drawing.Bitmap $bw,$bh
  $g = [System.Drawing.Graphics]::FromImage($bmp)
  $hdc = $g.GetHdc(); [MV]::PrintWindow($main,$hdc,3) | Out-Null; $g.ReleaseHdc($hdc); $g.Dispose()
  $bmp.Save($path,[System.Drawing.Imaging.ImageFormat]::Png); $bmp.Dispose()
}

Shot 'G:\PyArx\docs\_move_before.png'

# 模拟：进入移动循环 -> 结束（不改变尺寸，只有位置）——检验预览态收尾
[MV]::SendMessage($main,0x0231,0,0) | Out-Null   # WM_ENTERSIZEMOVE
Start-Sleep -Milliseconds 300
[MV]::SetWindowPos($main,[IntPtr]::Zero,220,200,1400,900,0x0004) | Out-Null
Start-Sleep -Milliseconds 300
[MV]::SendMessage($main,0x0232,0,0) | Out-Null   # WM_EXITSIZEMOVE
Start-Sleep -Milliseconds 800
Shot 'G:\PyArx\docs\_move_after.png'
Write-Host "done"
