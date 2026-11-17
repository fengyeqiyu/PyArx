# -*- coding: utf-8 -*-
"""把 101 图导出 DXF（绑定的 xref 一起出去），再从 DXF 文本里读 _AXISO 块的原始组码。
DXF 是纯文本组码，代理对象的 92/310 段都在，绕开 PyArx 的类包装限制。"""
import os
import shutil
import subprocess
import sys
import tempfile

sys.path.insert(0, r"G:\PyArx\examples\cad2d")
import scan_load  # noqa: E402

DWG = r"J:\变更图纸\DRB018\电照\101#中试车间(扩建)-设施配电平面及系统_t3.dwg"
DXF = r"C:\temp\101_ax.dxf"
os.makedirs(r"C:\temp", exist_ok=True)
for f in (DXF, DXF + ".tmp"):
    if os.path.isfile(f):
        os.remove(f)

work = tempfile.mkdtemp(prefix="dxfout_")
dwg = os.path.join(work, "in.dwg")
shutil.copyfile(DWG, dwg)
scr = os.path.join(work, "run.scr")
lines = [
    '(setvar "SECURELOAD" 0)',
    '(setvar "PROXYGRAPHICS" 1)',
    '(arxload "%s")' % scan_load.CRX.replace("\\", "/"),
    '(command "-XREF" "_B" "*")',
    '(command "_.DXFOUT" "%s" "16")' % DXF.replace("\\", "/"),
]
with open(scr, "w", encoding="utf-8") as f:
    f.write("\n".join(lines) + "\n")

proc = subprocess.run([scan_load.console_path(), "/i", dwg, "/s", scr],
                      capture_output=True, timeout=900, cwd=work)
print("exit", proc.returncode)
print((proc.stdout or b"").decode("gbk", "replace")[-600:])
shutil.rmtree(work, ignore_errors=True)
print("dxf", os.path.getsize(DXF) if os.path.isfile(DXF) else "missing")
