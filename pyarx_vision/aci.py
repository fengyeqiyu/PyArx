# -*- utf-8 -*-
"""ACI(AutoCAD Color Index) 与 RGB / SVG 颜色互换。

实测结论（2026-11-14，CAD 2024 + PyArx）：
  - `o.colorRGB()` 对索引色实体恒返回 (0,0,0)，只对真彩色(TrueColor)有效；
  - `PyDbColor` 设 setColorIndex 后取 red/green/blue 也恒 0。
  即 CAD 不提供 ACI→RGB 换算，必须自带标准色表。

ACI 结构：
  0        ByBlock   无自身颜色（跟随所在块）
  1~9      固定基本色
  10~249   24 组色相 × 每组 10 档明度（h 每 15°递增，明度 5 档 × 纯/半饱和）
  250~255  灰阶，51 → 255
  256      ByLayer   无自身颜色（跟随图层）
"""
import colorsys

# 1~9 固定色（7 在 AutoCAD 是"白/黑"，取决于背景；这里给出光栅白底下应有的黑）
_BASIC = {
    0: (0, 0, 0),          # ByBlock，按黑处理
    1: (255, 0, 0),        # 红
    2: (255, 255, 0),      # 黄
    3: (0, 255, 0),        # 绿
    4: (0, 255, 255),      # 青
    5: (0, 0, 255),        # 蓝
    6: (255, 0, 255),      # 品红
    7: (0, 0, 0),          # 白/黑 → 白底用黑
    8: (128, 128, 128),    # 灰
    9: (192, 192, 192),    # 浅灰
    256: (0, 0, 0),        # ByLayer
}

_LEVELS = (255, 204, 153, 127, 76)      # 每组 5 档明度
_GRAYS = (51, 91, 132, 173, 214, 255)   # 250~255


def aci_to_rgb(i):
    """ACI 索引色 → (r,g,b)。256/0 返回黑，语义上的 ByLayer/ByBlock 需外部先解析。"""
    try:
        i = int(i)
    except Exception:
        return (0, 0, 0)
    if i in _BASIC:
        return _BASIC[i]
    if 250 <= i <= 255:
        v = _GRAYS[i - 250]
        return (v, v, v)
    if 10 <= i <= 249:
        k = (i - 10) // 10          # 0..23  → 色相 0..345°
        j = (i - 10) % 10           # 0..9   → 明度档 + 纯/半饱和
        lv = _LEVELS[j // 2] / 255.0
        s = 1.0 if j % 2 == 0 else 0.5
        r, g, b = colorsys.hsv_to_rgb(k * 15 / 360.0, s, lv)
        return (round(r * 255), round(g * 255), round(b * 255))
    return (0, 0, 0)


def aci_hex(i):
    r, g, b = aci_to_rgb(i)
    return "#%02x%02x%02x" % (r, g, b)


def is_gray(i):
    """8/9/250~255：灰阶。建筑底图多写在这几个色号上。"""
    try:
        i = int(i)
    except Exception:
        return False
    return i in (8, 9) or 250 <= i <= 255


# 反向：RGB → 最接近的 ACI。用加权距离（人眼对绿最敏感），排除 0/256。
_W = (2.0, 4.0, 3.0)


def rgb_to_aci(r, g, b, allow_gray=True):
    best, bd = 7, 1e18
    for i in range(1, 256):
        if not allow_gray and is_gray(i):
            continue
        c = aci_to_rgb(i)
        d = (_W[0] * (c[0] - r) ** 2 + _W[1] * (c[1] - g) ** 2 + _W[2] * (c[2] - b) ** 2)
        if d < bd:
            bd, best = d, i
    return best


def hex_to_rgb(h):
    h = (h or "").strip().lstrip("#")
    if len(h) == 3:
        h = "".join(ch * 2 for ch in h)
    return (int(h[0:2], 16), int(h[2:4], 16), int(h[4:6], 16))


def hex_to_aci(h):
    return rgb_to_aci(*hex_to_rgb(h))
