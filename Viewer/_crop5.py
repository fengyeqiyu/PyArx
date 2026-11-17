# -*- coding: utf-8 -*-
"""裁剪平面图视图截图。"""
from PIL import Image

im = Image.open(r"C:\temp\shot_plan.png")
w, h = im.size
crop = im.crop((0, 60, int(w * 0.6), h - 40))
cw = 1400
crop = crop.resize((cw, int(crop.height * cw / crop.width)), Image.LANCZOS)
crop.convert("RGB").save(r"C:\temp\shot_plan_s.png", quality=88)
print("ok", im.size, crop.size)
