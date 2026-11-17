Add-Type -AssemblyName System.Drawing
Add-Type @'
using System; using System.Runtime.InteropServices;
public static class BL2 {
 [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
 [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr h, IntPtr a, int x, int y, int cx, int cy, uint f);
 [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
 [DllImport("user32.dll")] public static extern IntPtr SendMessage(IntPtr h, uint m, IntPtr w, IntPtr l);
 [StructLayout(LayoutKind.Sequential)] public struct RECT { public int l,t,r,b; }
}
'@
$d = Get-Process UIDesign | Select-Object -First 1
$main = $d.MainWindowHandle
[BL2]::SetWindowPos($main,[IntPtr]::Zero,200,150,1200,780,0x0004) | Out-Null
Start-Sleep -Milliseconds 600

# 模拟一次完整缩放（进入循环 -> 改尺寸 -> 退出）
[BL2]::SendMessage($main,0x0231,0,0) | Out-Null
for ($i=0; $i -lt 12; $i++) {
  [BL2]::SetWindowPos($main,[IntPtr]::Zero,200,150,(1200+$i*16),(780+$i*10),0x0004) | Out-Null
  Start-Sleep -Milliseconds 10
}
[BL2]::SendMessage($main,0x0232,0,0) | Out-Null
Start-Sleep -Milliseconds 300
# 把鼠标移开窗口边缘再收尾（模拟用户松手后鼠标离开）
[BL2]::SetWindowPos($main,[IntPtr]::Zero,200,150,1400,900,0x0004) | Out-Null
Start-Sleep -Milliseconds 700

$np = Start-Process notepad -PassThru
Start-Sleep -Milliseconds 800
[BL2]::SetWindowPos($np.MainWindowHandle,[IntPtr]::Zero,1750,950,600,300,0x0004) | Out-Null
[BL2]::SetForegroundWindow($np.MainWindowHandle) | Out-Null
Start-Sleep -Milliseconds 900

$r = New-Object BL2+RECT; [BL2]::GetWindowRect($main,[ref]$r) | Out-Null
$bw=$r.r-$r.l; $bh=$r.b-$r.t
$bmp = New-Object System.Drawing.Bitmap $bw,$bh
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.CopyFromScreen($r.l,$r.t,0,0,(New-Object System.Drawing.Size $bw,$bh))
$g.Dispose()
$bmp.Save('G:\PyArx\docs\_blur_after_resize.png',[System.Drawing.Imaging.ImageFormat]::Png)
$bmp.Dispose()
$np | Stop-Process -Force
"shot $bw x $bh"
