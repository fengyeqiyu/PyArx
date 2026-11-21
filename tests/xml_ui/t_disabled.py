# 第47轮：CSS :disabled —— 禁用控件整块降对比（文字已灰，本轮把背板也压淡）
# HTML `<button disabled>` 浏览器自动给的是"底变淡 + 字变灰"；XML 早先只灰了字，底仍是原色。
import os, sys, ctypes
from collections import Counter
from ctypes import wintypes as W
BIN = os.environ["PYARX_BIN"]; sys.path.insert(0, BIN); os.add_dll_directory(BIN)
os.environ.setdefault("PYARX_D2D_RT", "software")
import PyMfcXml as M
from PIL import Image

fails = []
def chk(name, cond, got=""):
    print(("PASS  " if cond else "FAIL  ") + name + ("  " + str(got) if got else ""))
    if not cond: fails.append(name)

u = ctypes.WinDLL("user32", use_last_error=True)
u.EnumChildWindows.argtypes = [W.HWND, ctypes.WINFUNCTYPE(W.BOOL, W.HWND, W.LPARAM), W.LPARAM]
u.GetWindowTextW.argtypes = [W.HWND, ctypes.c_wchar_p, ctypes.c_int]
u.IsWindowEnabled.argtypes = [W.HWND]

TPL = ('<XWindow Width="240" Height="70">'
       '<button Name="b" Left="12" Top="12" Width="180" Height="36" Text="Stop" '
       'style="background:#2F353B;color:#FFFFFF" {EN}/></XWindow>')

def shot(tag, extra):
    n = {}
    w = M.guiload_text(TPL.replace("{EN}", extra), context=globals(), names=n)
    M.pump(250)
    p = "G:/PyArx/log/dis_%s.png" % tag
    w.save_png(p)
    hw = []
    @ctypes.WINFUNCTYPE(W.BOOL, W.HWND, W.LPARAM)
    def cb(h, lp):
        buf = ctypes.create_unicode_buffer(64)
        u.GetWindowTextW(h, buf, 64)
        if buf.value == "Stop":
            hw.append(h)
        return True
    u.EnumChildWindows(W.HWND(w.hwnd), cb, 0)
    enabled = bool(u.IsWindowEnabled(W.HWND(hw[0]))) if hw else None
    r = w.get_rect(n["b"])
    w.close()
    im = Image.open(p).convert("RGB")
    return count_btn(im, r), enabled

def count_btn(im, r):
    """只统计按钮矩形内的像素。

    savePng 是整窗截图，客户区坐标到图像坐标有一段固定偏移（标题栏高度），
    所以先在几个候选偏移里挑"矩形内非页面底色占比最高"的那个，
    避免把标题栏的白字算成按钮文字。
    """
    bg = Counter(im.getdata()).most_common(1)[0][0]
    best = None
    for dy in (0, 31, -31, 20, 40):
        y0 = r[1] + dy
        hit = tot = 0
        for y in range(max(0, y0), min(im.size[1], y0 + r[3])):
            for x in range(r[0], min(im.size[0], r[0] + r[2])):
                tot += 1
                if im.getpixel((x, y)) != bg:
                    hit += 1
        score = hit / tot if tot else 0
        if best is None or score > best[0]:
            best = (score, dy)
    dy = best[1] if best else 0
    bright = gray = 0
    for y in range(max(0, r[1] + dy), min(im.size[1], r[1] + dy + r[3])):
        for x in range(r[0], min(im.size[0], r[0] + r[2])):
            px = im.getpixel((x, y))
            if px[0] > 200 and px[1] > 200 and px[2] > 200:
                bright += 1
            elif 120 <= px[0] <= 165 and abs(px[0] - px[1]) < 12 and abs(px[1] - px[2]) < 12:
                gray += 1
    return bright, gray

on, on_en = shot("on", "")
off, off_en = shot("off", 'disabled="true"')
(on, on_gray), (off, off_gray) = on, off
chk("启用态窗口可用", on_en is True, on_en)
chk("disabled 属性真的禁用窗口", off_en is False, off_en)
chk("启用态按钮内有亮白文字像素", on > 20, (on, on_gray))
chk("禁用态按钮内文字转灰（灰像素增多、亮像素减少）",
    off_gray > on_gray + 10 and off < on, (on, on_gray, off, off_gray))
print("t_disabled: PASS=%d FAIL=%d" % (4 - len(fails), len(fails)))
