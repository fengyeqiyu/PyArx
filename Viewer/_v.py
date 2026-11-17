# -*- coding: utf-8 -*-
s = open(r"G:\配电规划设计\web\101\scene.html", encoding="utf-8", errors="replace").read()
print("len", len(s))
print("atx_filled", s.count('"atx":[['))
print("atx_blocks", s.count('"atx"'))
i = s.find('"atx":[[')
print("sample", s[i:i + 210] if i >= 0 else "none")
