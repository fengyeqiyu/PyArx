# -*- coding: utf-8 -*-
"""裁剪菜单区域查看。"""
from PIL import Image

im = Image.open(r"C:\temp\shot_menu.png")
crop = im.crop((0, 0, 900, 700))
crop.convert("RGB").save(r"C:\temp\shot_menu_s.png", quality=90)
print("ok", im.size, crop.size)
