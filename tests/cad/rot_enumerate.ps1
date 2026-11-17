# Enumerate ROT only; no CreateObject, no window broadcast.
Add-Type @'
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Runtime.InteropServices.ComTypes;

public static class RotProbe {
  [DllImport("ole32.dll")] static extern int GetRunningObjectTable(int reserved, out IRunningObjectTable pprot);
  [DllImport("ole32.dll")] static extern int CreateBindCtx(int reserved, out IBindCtx pctx);
  [DllImport("user32.dll")] static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);

  public class Item { public uint Pid; public IntPtr Hwnd; public string Name; public object Obj; }

  public static List<Item> Enum() {
    var outList = new List<Item>();
    IRunningObjectTable rot;
    if (GetRunningObjectTable(0, out rot) != 0) return outList;
    IEnumMoniker en;
    rot.EnumRunning(out en);
    IMoniker[] m = new IMoniker[1];
    CreateBindCtx(0, out var bc);
    IntPtr pctx = Marshal.GetIUnknownForObject(bc);
    for (;;) {
      IntPtr fetched = IntPtr.Zero;
      en.Next(1, m, fetched);
      int n = fetched.ToInt32();
      if (n == 0) break;
      string disp = null;
      try { m[0].GetDisplayName(bc, null, out disp); } catch { }
      object obj = null;
      try { rot.GetObject(m[0], out obj); } catch { }
      uint pid = 0; IntPtr hwnd = IntPtr.Zero;
      if (obj != null) {
        try {
          dynamic d = obj;
          object h = d.HWND;
          hwnd = (IntPtr)(long)h;
          GetWindowThreadProcessId(hwnd, out pid);
        } catch { }
      }
      outList.Add(new Item { Pid = pid, Hwnd = hwnd, Name = disp, Obj = obj });
    }
    Marshal.Release(pctx);
    return outList;
  }
}
'@

[RotProbe]::Enum() | ForEach-Object {
  "{0} hwnd={1} name={2}" -f $_.Pid, $_.Hwnd, $_.Name
}
