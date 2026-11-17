# -*- coding: utf-8 -*-
"""跑 scan.py（accoreconsole + PyArxEngine + PYRUNFILE），检查 _AXISO 的 atx 是否带值。"""
import json
import os
import shutil
import subprocess
import sys
import tempfile

sys.path.insert(0, r"G:\PyArx\examples\cad2d")
import scan_load  # noqa: E402

DWG = r"J:\变更图纸\DRB018\电照\101#中试车间(扩建)-设施配电平面及系统_t3.dwg"
OUT = r"C:\temp\scan.json"
if os.path.isfile(OUT):
    os.remove(OUT)

work = tempfile.mkdtemp(prefix="scn_")
dwg = os.path.join(work, "in.dwg")
shutil.copyfile(DWG, dwg)
scr = os.path.join(work, "run.scr")
with open(scr, "w", encoding="utf-8") as f:
    f.write("\n".join([
        '(setvar "SECURELOAD" 0)',
        '(arxload "%s")' % scan_load.CRX.replace("\\", "/"),
        '(command "-XREF" "_B" "*")',
        '(command "PYRUNFILE" "G:/PyArx/pyarx_vision/scan.py")',
    ]) + "\n")

proc = subprocess.run([scan_load.console_path(), "/i", dwg, "/s", scr],
                      capture_output=True, timeout=1800, cwd=work)
txt = (proc.stdout or b"").decode("gbk", "replace")
print("exit", proc.returncode)
for ln in txt.splitlines():
    if "scan ok" in ln or "PYRUN" in ln and "rc=" in ln:
        print(ln.strip()[:120])
shutil.rmtree(work, ignore_errors=True)

if not os.path.isfile(OUT):
    print("scan.json missing")
    raise SystemExit(1)
d = json.load(open(OUT, encoding="utf-8"))
bf = d.get("block_defs") or {}
for k, v in bf.items():
    if "AXISO" in k.upper():
        sg = v.get("gi") or []
        print("def", k, "subs", len(sg))
        for s in sg[:6]:
            print("  p=", s.get("p"), "s=", s.get("s"), "atx=", str(s.get("atx"))[:110])
        break
