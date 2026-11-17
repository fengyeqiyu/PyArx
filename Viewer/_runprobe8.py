# -*- coding: utf-8 -*-
"""跑 _probe8.py（块参照属性诊断）。"""
import json
import os
import shutil
import subprocess
import sys
import tempfile

sys.path.insert(0, r"G:\PyArx\examples\cad2d")
import scan_load  # noqa: E402

DWG = r"J:\变更图纸\DRB018\电照\101#中试车间(扩建)-设施配电平面及系统_t3.dwg"
PROBE = r"G:\PyArx\viewer\_probe8.py"
OUT = r"C:\temp\probe8.json"
if os.path.isfile(OUT):
    os.remove(OUT)

work = tempfile.mkdtemp(prefix="prb8_")
dwg = os.path.join(work, "in.dwg")
shutil.copyfile(DWG, dwg)
scr = os.path.join(work, "run.scr")
with open(scr, "w", encoding="utf-8") as f:
    f.write("\n".join([
        '(setvar "SECURELOAD" 0)',
        '(arxload "%s")' % scan_load.CRX.replace("\\", "/"),
        '(command "PYRUNFILE" "%s")' % PROBE.replace("\\", "/"),
    ]) + "\n")

proc = subprocess.run([scan_load.console_path(), "/i", dwg, "/s", scr],
                      capture_output=True, timeout=900, cwd=work)
print("exit", proc.returncode)
shutil.rmtree(work, ignore_errors=True)

if os.path.isfile(OUT):
    d = json.load(open(OUT, encoding="utf-8"))
    print("err:", d["err"][-400:] if d["err"] else "(none)")
    for r in d["rows"]:
        print("ROW", r)
else:
    print("no out")
