# -*- coding: utf-8 -*-
"""裁剪窗口区域并缩放，便于查看。"""
from PIL import Image

src = r"C:\temp\shot_axis.png"
im = Image.open(src)
box = (0, 0, min(2576, im.width), min(1424, im.height))
im2 = im.crop(box)
w = 1500
im2 = im2.resize((w, int(im2.height * w / im2.width)), Image.LANCZOS)
im2.convert("RGB").save(r"C:\temp\shot_view.png", quality=88)
print("ok", im.size, "->", im2.size)
