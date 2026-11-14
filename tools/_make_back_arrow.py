# 文件页返回箭头（透明底浅灰，32x32，4x 超采样）
from PIL import Image, ImageDraw

K = 4
S = 32 * K
COL = (205, 212, 222, 255)
LW = 3 * K

img = Image.new("RGBA", (S, S), (0, 0, 0, 0))
d = ImageDraw.Draw(img)

x0, x1 = 7 * K, 26 * K
cy = S // 2
d.line([(x0, cy), (x1, cy)], fill=COL, width=LW)

tip = 8 * K
d.line([(x0, cy), (x0 + tip, cy - tip)], fill=COL, width=LW, joint="curve")
d.line([(x0, cy), (x0 + tip, cy + tip)], fill=COL, width=LW, joint="curve")

img = img.resize((32, 32), Image.LANCZOS)
img.save(r"G:\PyArx\UIDesign\res\back.png", "PNG")
print("back.png ok")
