# -*- coding: utf-8 -*-
"""裁剪截图左半部分（平面图轴号区）并缩放。"""
from PIL import Image

im = Image.open(r"C:\temp\shot_max.png")
# 取窗口区域，左侧 45% 宽
w, h = im.size
crop = im.crop((0, 0, int(w * 0.55), h))
cw = 1400
crop = crop.resize((cw, int(crop.height * cw / crop.width)), Image.LANCZOS)
crop.convert("RGB").save(r"C:\temp\shot_axis2.png", quality=86)
print("ok", im.size, crop.size)
