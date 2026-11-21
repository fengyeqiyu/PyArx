# -*- coding: utf-8 -*-
"""启动 accoreconsole 无头进程执行 arx_dump_all.scr，导出全类型权威表。"""
import os
import subprocess

CONSOLE = r"G:\PyArx\ACAD\accoreconsole.exe"
SRC = r"I:\xwechat_files\wxid_jbg0jgp7km6m21_d2a3\msg\file\2026-10\中试车间扩建-工艺配电及系统2026.10.12_t3.dwg"
SCR = r"G:\PyArx\examples\cad2d\dwg1018\arx_dump_all.scr"
OUT = r"G:\PyArx\examples\cad2d\dwg1018\arx_all.tsv"

if os.path.exists(OUT):
    os.remove(OUT)

p = subprocess.run([CONSOLE, "/i", SRC, "/s", SCR],
                    capture_output=True, timeout=600)
print("returncode=", p.returncode)
print("---- stdout tail ----")
print((p.stdout or b"").decode("utf-8", "ignore")[-1500:])
print("---- stderr tail ----")
print((p.stderr or b"").decode("utf-8", "ignore")[-1500:])
print("out exists=", os.path.exists(OUT),
      "size=", os.path.getsize(OUT) if os.path.exists(OUT) else 0)
