# 生成 Ribbon 视图页的三个小图标（透明底白色线条，64x64 4x 超采样）
from PIL import Image, ImageDraw

K = 4
S = 64 * K

def new():
    img = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    return img, ImageDraw.Draw(img)

def save(img, name):
    img = img.resize((64, 64), Image.LANCZOS)
    img.save(r"G:\PyArx\UIDesign\res\%s" % name, "PNG")
    print(name, "ok")

W = (235, 238, 243, 255)
T = 7 * K   # 线宽

# 全屏：四角 L 形箭头向外
img, d = new()
m = 10 * K
L = 16 * K
for sx in (0, 1):
    for sy in (0, 1):
        x0 = m if sx == 0 else S - m
        y0 = m if sy == 0 else S - m
        d.rectangle([min(x0, x0 + (L if sx == 0 else -L)), y0 - T // 2,
                     max(x0, x0 + (L if sx == 0 else -L)), y0 + T // 2], fill=W)
        d.rectangle([x0 - T // 2, min(y0, y0 + (L if sy == 0 else -L)),
                     x0 + T // 2, max(y0, y0 + (L if sy == 0 else -L))], fill=W)
save(img, "fullscreen.png")

# 适应窗口：四角箭头向内 + 中心方框
img, d = new()
m = 8 * K
L = 14 * K
for sx in (0, 1):
    for sy in (0, 1):
        x0 = m if sx == 0 else S - m
        y0 = m if sy == 0 else S - m
        d.rectangle([min(x0, x0 + (L if sx == 0 else -L)), y0 - T // 2,
                     max(x0, x0 + (L if sx == 0 else -L)), y0 + T // 2], fill=W)
        d.rectangle([x0 - T // 2, min(y0, y0 + (L if sy == 0 else -L)),
                     x0 + T // 2, max(y0, y0 + (L if sy == 0 else -L))], fill=W)
box = 22 * K
d.rectangle([(S - box) // 2, (S - box) // 2, (S + box) // 2, (S + box) // 2],
            outline=W, width=T // 2)
save(img, "zoomfit.png")

# 100%：圆环 + 右侧箭头（重置缩放）
img, d = new()
cx, cy, r = S // 2, S // 2, 20 * K
d.ellipse([cx - r, cy - r, cx + r, cy + r], outline=W, width=T)
# 右上角箭头（顺时针指示）
ax, ay = cx + r - 2 * K, cy - r + 2 * K
d.polygon([(ax, ay - 9 * K), (ax + 9 * K, ay), (ax, ay + 9 * K)], fill=W)
save(img, "zoomreset.png")
