# -*- coding: utf-8 -*-
"""裁剪截图中央区域放大，便于看轴号文字。"""
from PIL import Image

im = Image.open(r"C:\temp\shot_v4.png")
w, h = im.size
# 中央区域
crop = im.crop((int(w * 0.1), int(h * 0.08), int(w * 0.62), int(h * 0.92)))
cw = 1400
crop = crop.resize((cw, int(crop.height * cw / crop.width)), Image.LANCZOS)
crop.convert("RGB").save(r"C:\temp\shot_v4s.png", quality=88)
print("ok", im.size, crop.size)
