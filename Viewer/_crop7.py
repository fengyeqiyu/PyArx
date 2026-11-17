# -*- coding: utf-8 -*-
"""裁剪平面图视图（左半），看轴号。"""
from PIL import Image

im = Image.open(r"C:\temp\shot_plan3.png")
w, h = im.size
crop = im.crop((0, 40, int(w * 0.55), h - 30))
cw = 1350
crop = crop.resize((cw, int(crop.height * cw / crop.width)), Image.LANCZOS)
crop.convert("RGB").save(r"C:\temp\shot_plan3_s.png", quality=88)
print("ok", im.size, crop.size)
