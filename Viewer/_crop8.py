# -*- coding: utf-8 -*-
"""裁剪浏览器截图（含轴网左上区域）。"""
from PIL import Image

im = Image.open(r"C:\temp\shot_br.png")
w, h = im.size
crop = im.crop((0, 0, int(w * 0.62), int(h * 0.95)))
cw = 1400
crop = crop.resize((cw, int(crop.height * cw / crop.width)), Image.LANCZOS)
crop.convert("RGB").save(r"C:\temp\shot_br_s.png", quality=88)
print("ok", im.size, crop.size)
