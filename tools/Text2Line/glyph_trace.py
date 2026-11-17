"""glyph_trace.py —— 用 GDI 把字形栅格化，再做轮廓追踪，输出与"渲染墨迹"完全一致的闭合轮廓。

为什么不用 GetGlyphOutline：
    它返回的是字体原始轮廓（各笔画彼此重叠），笔画相交处的边界会互相穿插，
    炸开后看起来"缺笔画"。栅格化 + 边界追踪得到的是墨迹并集边界，和屏幕所见一致。

坐标约定：基线 y=0，y 向上为正，单位 = 1 em（em 由 em_px 决定），x 从 0 开始。
"""

import ctypes
from ctypes import wintypes as wt

gdi32 = ctypes.WinDLL("gdi32", use_last_error=True)
user32 = ctypes.WinDLL("user32", use_last_error=True)

SRCCOPY = 0x00CC0020
BI_RGB = 0
DIB_RGB_COLORS = 0
TA_BASELINE = 0x0008
TA_LEFT = 0x0000
TA_NOUPDATECP = 0x0000
DEFAULT_CHARSET = 1
OUT_DEFAULT_PRECIS = 0
CLIP_DEFAULT_PRECIS = 0
NONANTIALIASED_QUALITY = 3
DEFAULT_PITCH = 0
FW_NORMAL = 400


class BITMAPINFOHEADER(ctypes.Structure):
    _fields_ = [
        ("biSize", wt.DWORD), ("biWidth", wt.LONG), ("biHeight", wt.LONG),
        ("biPlanes", wt.WORD), ("biBitCount", wt.WORD), ("biCompression", wt.DWORD),
        ("biSizeImage", wt.DWORD), ("biXPelsPerMeter", wt.LONG), ("biYPelsPerMeter", wt.LONG),
        ("biClrUsed", wt.DWORD), ("biClrImportant", wt.DWORD),
    ]


class BITMAPINFO(ctypes.Structure):
    _fields_ = [("bmiHeader", BITMAPINFOHEADER), ("bmiColors", wt.DWORD * 3)]


class TEXTMETRICW(ctypes.Structure):
    _fields_ = [
        ("tmHeight", wt.LONG), ("tmAscent", wt.LONG), ("tmDescent", wt.LONG),
        ("tmInternalLeading", wt.LONG), ("tmExternalLeading", wt.LONG),
        ("tmAveCharWidth", wt.LONG), ("tmMaxCharWidth", wt.LONG), ("tmWeight", wt.LONG),
        ("tmOverhang", wt.LONG), ("tmDigitizedAspectX", wt.LONG), ("tmDigitizedAspectY", wt.LONG),
        ("tmFirstChar", wt.WCHAR), ("tmLastChar", wt.WCHAR), ("tmDefaultChar", wt.WCHAR),
        ("tmBreakChar", wt.WCHAR), ("tmItalic", wt.BYTE), ("tmUnderlined", wt.BYTE),
        ("tmStruckOut", wt.BYTE), ("tmPitchAndFamily", wt.BYTE), ("tmCharSet", wt.BYTE),
    ]


gdi32.CreateCompatibleDC.restype = wt.HDC
gdi32.CreateCompatibleDC.argtypes = [wt.HDC]
gdi32.CreateDIBSection.restype = wt.HBITMAP
gdi32.CreateDIBSection.argtypes = [wt.HDC, ctypes.POINTER(BITMAPINFO), wt.DWORD,
                                   ctypes.POINTER(ctypes.c_void_p), wt.HANDLE, wt.DWORD]
gdi32.CreateFontW.restype = wt.HFONT
gdi32.CreateFontW.argtypes = [wt.LONG, wt.LONG, wt.LONG, wt.LONG, wt.LONG, wt.DWORD,
                              wt.DWORD, wt.DWORD, wt.DWORD, wt.DWORD, wt.DWORD, wt.DWORD,
                              wt.DWORD, wt.LPCWSTR]
gdi32.SelectObject.restype = wt.HGDIOBJ
gdi32.SelectObject.argtypes = [wt.HDC, wt.HGDIOBJ]
gdi32.DeleteObject.argtypes = [wt.HGDIOBJ]
gdi32.DeleteDC.argtypes = [wt.HDC]
gdi32.SetBkColor.argtypes = [wt.HDC, wt.COLORREF]
gdi32.SetTextColor.argtypes = [wt.HDC, wt.COLORREF]
gdi32.SetBkMode.argtypes = [wt.HDC, ctypes.c_int]
gdi32.SetTextAlign.argtypes = [wt.HDC, wt.DWORD]
gdi32.TextOutW.argtypes = [wt.HDC, ctypes.c_int, ctypes.c_int, wt.LPCWSTR, ctypes.c_int]
gdi32.GetTextMetricsW.argtypes = [wt.HDC, ctypes.POINTER(TEXTMETRICW)]
gdi32.GetCharABCWidthsW.argtypes = [wt.HDC, wt.UINT, wt.UINT, ctypes.c_void_p]


def glyph_mask(face, ch, em_px=768, canvas=1280, pad=None):
    """把一个字符渲染成 1 位掩膜。返回 (mask bytearray, W, H, baseline_y, pen_x)。"""
    pad = em_px // 2 if pad is None else pad
    W = canvas
    H = canvas
    hdc = gdi32.CreateCompatibleDC(None)
    bmi = BITMAPINFO()
    bmi.bmiHeader.biSize = ctypes.sizeof(BITMAPINFOHEADER)
    bmi.bmiHeader.biWidth = W
    bmi.bmiHeader.biHeight = -H          # top-down
    bmi.bmiHeader.biPlanes = 1
    bmi.bmiHeader.biBitCount = 32
    bmi.bmiHeader.biCompression = BI_RGB
    bits = ctypes.c_void_p()
    hbmp = gdi32.CreateDIBSection(hdc, ctypes.byref(bmi), DIB_RGB_COLORS, ctypes.byref(bits), None, 0)
    ctypes.memset(bits, 0xFF, W * H * 4)   # 新 DIB 是黑的，先铺白底
    old = gdi32.SelectObject(hdc, hbmp)

    hfont = gdi32.CreateFontW(-em_px, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
                              OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY,
                              DEFAULT_PITCH, face)
    oldf = gdi32.SelectObject(hdc, hfont)

    tm = TEXTMETRICW()
    gdi32.GetTextMetricsW(hdc, ctypes.byref(tm))
    baseline = pad + tm.tmAscent

    gdi32.SetBkColor(hdc, 0x00FFFFFF)
    gdi32.SetTextColor(hdc, 0x00000000)
    gdi32.SetBkMode(hdc, 2)               # TRANSPARENT? keep OPAQUE=1 -> use 1
    gdi32.SetBkMode(hdc, 1)
    gdi32.SetTextAlign(hdc, TA_BASELINE | TA_LEFT | TA_NOUPDATECP)
    gdi32.TextOutW(hdc, pad, baseline, ch, 1)

    raw = ctypes.string_at(bits, W * H * 4)
    mask = bytearray(W * H)
    for i in range(W * H):
        # 背景白，字形黑：只要不是白就算墨迹
        if raw[i * 4] < 200 or raw[i * 4 + 1] < 200 or raw[i * 4 + 2] < 200:
            mask[i] = 1

    # 字符前进量（em 单位）
    abc = (ctypes.c_int * 3)()
    a = ord(ch) if ord(ch) < 0x10000 else 0
    gdi32.GetCharABCWidthsW(hdc, a, a, ctypes.cast(abc, ctypes.c_void_p))
    adv = (abc[0] + abc[1] + abc[2]) / float(em_px)

    gdi32.SelectObject(hdc, oldf)
    gdi32.SelectObject(hdc, old)
    gdi32.DeleteObject(hfont)
    gdi32.DeleteObject(hbmp)
    gdi32.DeleteDC(hdc)

    return mask, W, H, baseline, pad, adv


DIR_INDEX = {(1, 0): 0, (0, 1): 1, (-1, 0): 2, (0, -1): 3}


def mask_loops(mask, W, H, prefer="right"):
    """把掩膜转成闭合轮廓（像素坐标）。返回 list[list[(x,y)]]。

    prefer 决定自接触岔口处的转向策略：right = 始终右转（贴墙走），
    可在笔画自交处把一圈边界完整走完，不会串到相邻环。
    """
    edges = {}
    for y in range(H):
        row = y * W
        for x in range(W):
            if not mask[row + x]:
                continue
            if y == 0 or not mask[row - W + x]:
                edges.setdefault((x, y), []).append((x + 1, y))
            if x + 1 >= W or not mask[row + x + 1]:
                edges.setdefault((x + 1, y), []).append((x + 1, y + 1))
            if y + 1 >= H or not mask[row + W + x]:
                edges.setdefault((x + 1, y + 1), []).append((x, y + 1))
            if x == 0 or not mask[row + x - 1]:
                edges.setdefault((x, y + 1), []).append((x, y))

    loops = []
    while edges:
        start = next(iter(edges))
        pts = [start]
        cur = start
        prev_dir = None
        while True:
            lst = edges.get(cur)
            if not lst:
                break
            if len(lst) == 1 or prev_dir is None:
                nxt = lst.pop()
            else:
                idx_in = DIR_INDEX[prev_dir]
                cands = []
                for e in lst:
                    d = (e[0] - cur[0], e[1] - cur[1])
                    turn = (DIR_INDEX[d] - idx_in) % 4
                    cands.append((turn, e))
                cands.sort(key=lambda t: t[0] if prefer == "right" else -t[0])
                turn, nxt = cands[0]
                lst.remove(nxt)
            if not lst:
                del edges[cur]
            pts.append(nxt)
            prev_dir = (nxt[0] - cur[0], nxt[1] - cur[1])
            cur = nxt
            if cur == start:
                break
        if len(pts) >= 4 and pts[-1] == start:
            loops.append(pts[:-1])
        elif len(pts) >= 4:
            loops.append(pts)
    return loops


def rdp(pts, tol):
    if len(pts) < 3:
        return pts
    keep = [False] * len(pts)
    keep[0] = keep[-1] = True
    stack = [(0, len(pts) - 1)]
    while stack:
        i0, i1 = stack.pop()
        x0, y0 = pts[i0]
        x1, y1 = pts[i1]
        dx, dy = x1 - x0, y1 - y0
        norm = (dx * dx + dy * dy) ** 0.5
        best, idx = -1.0, -1
        for i in range(i0 + 1, i1):
            px, py = pts[i]
            if norm < 1e-12:
                d = ((px - x0) ** 2 + (py - y0) ** 2) ** 0.5
            else:
                d = abs(dy * (px - x0) - dx * (py - y0)) / norm
            if d > best:
                best, idx = d, i
        if best > tol and idx > 0:
            keep[idx] = True
            stack.append((i0, idx))
            stack.append((idx, i1))
    return [p for p, k in zip(pts, keep) if k]


def text_contours(face, text, em_px=768, simp_px=1.6):
    """整串文字的墨迹轮廓。坐标：x 从 0 起、基线 y=0、向上为正，单位 em。"""
    out = []
    pen = 0.0
    for ch in text:
        if ch == " ":
            continue
        mask, W, H, baseline, pad, adv = glyph_mask(face, ch, em_px=em_px)
        for loop in mask_loops(mask, W, H):
            pts = rdp(loop + [loop[0]], simp_px)
            if len(pts) < 4:
                continue
            em = [((px - pad) / float(em_px) + pen, (baseline - py) / float(em_px))
                  for px, py in pts[:-1]]
            out.append(em)
        pen += adv
    return out


if __name__ == "__main__":
    import json
    import sys

    face = "SimSun"
    text = "工艺配电(桥架)"
    loops = text_contours(face, text)
    rep = {"face": face, "text": text, "loops": len(loops),
           "pts": sum(len(l) for l in loops),
           "bbox": [min(p[0] for l in loops for p in l), min(p[1] for l in loops for p in l),
                    max(p[0] for l in loops for p in l), max(p[1] for l in loops for p in l)]}
    print(json.dumps(rep, ensure_ascii=False))
    with open(r"G:\PyArx\tools\Text2Line\glyph_trace_out.json", "w", encoding="utf-8") as f:
        json.dump({"loops": loops}, f)
