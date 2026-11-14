# 退出图标：门框 + 向右穿出的箭头（透明底浅灰，32x32，4x 超采样）
from PIL import Image, ImageDraw

K = 4
S = 32 * K
COL = (205, 212, 222, 255)
LW = 3 * K

img = Image.new("RGBA", (S, S), (0, 0, 0, 0))
d = ImageDraw.Draw(img)

# 门（左侧竖线 + 顶部与底部短横）
x_left = 8 * K
d.line([(x_left, 7 * K), (x_left, 25 * K)], fill=COL, width=LW)
d.line([(x_left, 7 * K), (x_left + 8 * K, 7 * K)], fill=COL, width=LW)
d.line([(x_left, 25 * K), (x_left + 8 * K, 25 * K)], fill=COL, width=LW)

# 穿出的横线 + 箭头
ay = S // 2
d.line([(12 * K, ay), (25 * K, ay)], fill=COL, width=LW)
tip = 7 * K
d.line([(25 * K, ay), (25 * K - tip, ay - tip)], fill=COL, width=LW, joint="curve")
d.line([(25 * K, ay), (25 * K - tip, ay + tip)], fill=COL, width=LW, joint="curve")

img = img.resize((32, 32), Image.LANCZOS)
img.save(r"G:\PyArx\UIDesign\res\exit.png", "PNG")
print("exit.png ok")
