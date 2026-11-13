# -*- coding: utf-8 -*-
"""P0 绘制原语验收：渐变 / 圆角渐变 / 多边形填充 / 路径描边 / 柔和阴影 / 真 opacity 图层 / 旋转。

取样口径：本自绘 UI 的内容不进窗口表面，GDI BitBlt 抓不到。
这里对每个 QCanvas 子窗口 SendMessage(WM_PRINTCLIENT, memDC, PRF_CLIENT)，
把宿主同一条绘制路径画进内存 DIB，再逐点 GetPixel —— 拿到的是真实像素。

用例来源：_cmdmatrix.xaml —— 一个 QCanvas 只放一条命令，出问题能一眼定位到具体原语。
（早期把 7 条命令塞进同一个 QCanvas 的写法会取回全黑：无参命令（AlphaEnd / RotEnd）
  曾被参数拆分逻辑整条跳过，图层/变换关不掉，整帧作废。已修。）
"""
import ctypes
import os
from ctypes import wintypes

import PyMfcXaml as p

HERE = os.path.dirname(os.path.abspath(__file__))
u = ctypes.windll.user32
g = ctypes.windll.gdi32

WM_PRINTCLIENT = 0x0318
PRF_CLIENT = 0x00000004
BG = (0x23, 0x2A, 0x33)          # QCanvas 的 Clear 底色
CW, CH = 220, 110                # _cmdmatrix.xaml 里每个画布的大小

PASS, FAIL = [], []


def record(name, ok, detail=''):
    (PASS if ok else FAIL).append(name)
    print(('ok    ' if ok else 'FAIL  ') + name + ('  ' + detail if detail else ''), flush=True)


class BMIH(ctypes.Structure):
    _fields_ = [('biSize', wintypes.DWORD), ('biWidth', ctypes.c_long), ('biHeight', ctypes.c_long),
                ('biPlanes', wintypes.WORD), ('biBitCount', wintypes.WORD),
                ('biCompression', wintypes.DWORD), ('biSizeImage', wintypes.DWORD),
                ('biXPelsPerMeter', ctypes.c_long), ('biYPelsPerMeter', ctypes.c_long),
                ('biClrUsed', wintypes.DWORD), ('biClrImportant', wintypes.DWORD)]


class BMI(ctypes.Structure):
    _fields_ = [('bmiHeader', BMIH), ('bmiColors', wintypes.DWORD * 3)]


u.EnumChildWindows.argtypes = [ctypes.c_void_p, ctypes.c_void_p, wintypes.LPARAM]
u.GetDlgCtrlID.argtypes = [ctypes.c_void_p]
u.GetDlgCtrlID.restype = ctypes.c_int
u.SendMessageW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM]
u.SendMessageW.restype = wintypes.LPARAM
g.CreateCompatibleDC.argtypes = [ctypes.c_void_p]
g.CreateCompatibleDC.restype = ctypes.c_void_p
g.CreateDIBSection.argtypes = [ctypes.c_void_p, ctypes.c_void_p, wintypes.DWORD,
                               ctypes.c_void_p, ctypes.c_void_p, wintypes.DWORD]
g.CreateDIBSection.restype = ctypes.c_void_p
g.SelectObject.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
g.SelectObject.restype = ctypes.c_void_p
g.GetPixel.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_int]
g.GetPixel.restype = wintypes.DWORD
g.DeleteObject.argtypes = [ctypes.c_void_p]
g.DeleteObject.restype = wintypes.BOOL
g.DeleteDC.argtypes = [ctypes.c_void_p]
g.DeleteDC.restype = wintypes.BOOL


def kids(top):
    out = []
    EP = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)

    def cb(h, _):
        out.append((h, u.GetDlgCtrlID(h)))
        return True
    u.EnumChildWindows(top, EP(cb), 0)
    return out


def grab(hwnd, w, h):
    """把子窗口画进内存 DIB，返回 (采样函数, 释放函数)。"""
    bi = BMI()
    bi.bmiHeader.biSize = ctypes.sizeof(BMIH)
    bi.bmiHeader.biWidth = w
    bi.bmiHeader.biHeight = -h          # 负高度 = 自顶向下
    bi.bmiHeader.biPlanes = 1
    bi.bmiHeader.biBitCount = 32
    bi.bmiHeader.biCompression = 0
    bits = ctypes.c_void_p()
    bmp = g.CreateDIBSection(0, ctypes.byref(bi), 0, ctypes.byref(bits), 0, 0)
    mdc = g.CreateCompatibleDC(0)
    g.SelectObject(mdc, bmp)
    u.SendMessageW(hwnd, WM_PRINTCLIENT, mdc, PRF_CLIENT)

    def px(x, y):
        v = g.GetPixel(mdc, x, y)
        return (v & 0xFF, (v >> 8) & 0xFF, (v >> 16) & 0xFF)   # COLORREF 低字节是红

    def close():
        g.DeleteObject(bmp)
        g.DeleteDC(mdc)
    return px, close


def near(a, b, tol=14):
    return all(abs(x - y) <= tol for x, y in zip(a, b))


names = {}
w = p.guiload(os.path.join(HERE, '_cmdmatrix.xaml'), names=names)
w.show()
p.pump(700)

hwnds = dict((i, h) for h, i in kids(w.hwnd))
record('7 个 QCanvas 都建出了真实子窗口',
       all(names[k] in hwnds for k in ('c1', 'c2', 'c3', 'c4', 'c5', 'c6', 'c7')),
       'ids=%s' % sorted(names.items()))

samplers = {}


def sampler(key):
    if key not in samplers:
        px, close = grab(hwnds[names[key]], CW, CH)
        samplers[key] = (px, close)
    return samplers[key][0]


# 1. 渐变：横向，左端应接近 c1、右端应接近 c2，且两端必须不同
px1 = sampler('c1')
left, right = px1(30, 70), px1(200, 70)
record('Grad 左端接近起始色 #00A8E8', near(left, (0x00, 0xA8, 0xE8), 26), 'px=%s' % (left,))
record('Grad 右端接近结束色 #7A5AF8', near(right, (0x7A, 0x5A, 0xF8), 26), 'px=%s' % (right,))
record('Grad 两端颜色真的不同（不是单色填充）', left != right, '%s vs %s' % (left, right))

# 2. 圆角渐变：纵向
px2 = sampler('c2')
top, bot = px2(110, 25), px2(110, 85)
record('GradRR 纵向渐变上端 #57C84D', near(top, (0x57, 0xC8, 0x4D), 26), 'px=%s' % (top,))
record('GradRR 纵向渐变下端 #0E8A6A', near(bot, (0x0E, 0x8A, 0x6A), 26), 'px=%s' % (bot,))

# 3. 多边形填充：三角形内部橙色，包围盒角落仍是底色
px3 = sampler('c3')
inside, corner = px3(50, 40), px3(110, 85)
record('Fill 多边形内部是 #E8A030', near(inside, (0xE8, 0xA0, 0x30), 26), 'px=%s' % (inside,))
record('Fill 只填多边形、包围盒角落不被填', near(corner, BG, 24), 'corner=%s' % (corner,))

# 4. 路径描边：折线经过顶点处是白色
px4 = sampler('c4')
on_line, away = px4(60, 60), px4(5, 5)
record('Path 折线经过顶点处是白色', near(on_line, (255, 255, 255), 60), 'px=%s' % (on_line,))
record('Path 没画到的角落仍是底色', near(away, BG, 24), 'px=%s' % (away,))

# 5. 柔和阴影：矩形下方外侧应有暗部，左上外侧应保持底色
px5 = sampler('c5')
shadow_px, outside = px5(60, 88), px5(10, 30)
record('Shadow 在矩形外侧投出了暗部', sum(shadow_px) < sum(BG) - 20,
       'shadow=%s bg=%s' % (shadow_px, BG))
record('Shadow 只在矩形一侧（左上外侧仍是底色）', near(outside, BG, 24), 'px=%s' % (outside,))

# 6. 真 opacity 图层：140/255 的蓝块压到底色上，应得到"蓝色与底色混合"的结果
px6 = sampler('c6')
alpha_px = px6(60, 45)
expect = tuple(int(c * 140 / 255 + b * (255 - 140) / 255)
               for c, b in zip((0x00, 0xA8, 0xE8), BG))
record('Alpha 图层里的矩形按 alpha 与底色混合', near(alpha_px, expect, 12),
       'px=%s expect=%s' % (alpha_px, expect))
record('Alpha 图层不是"整块乘 alpha"（底色不该被一起压暗）',
       not near(px6(5, 5), (int(x * 140 / 255) for x in BG), 4),
       'layer 外 px=%s' % (px6(5, 5),))

# 7. 旋转：18° 绕 (110,55)，旋转后的菱形上应出现玫红，原位应已转走
px7 = sampler('c7')
rotated_hit, plain_spot = px7(110, 55), px7(90, 70)
record('Rot 后旋转位置出现 #FF5C7A', near(rotated_hit, (0xFF, 0x5C, 0x7A), 26), 'px=%s' % (rotated_hit,))
record('Rot 后未旋转的原始角度位置已被转走', near(plain_spot, BG, 24), 'px=%s' % (plain_spot,))

for k in list(samplers):
    samplers[k][1]()
samplers.clear()

# 8. 控件级真 opacity：setLayerOpacity 后同一画布的像素必须整体变暗
try:
    qc3 = names['c3']
    w.setLayerOpacity(qc3, 120)
    p.pump(300)
    record('setLayerOpacity 可调用并回读', w.getLayerOpacity(qc3) == 120,
           'alpha=%d' % w.getLayerOpacity(qc3))
    px, close = grab(hwnds[qc3], CW, CH)
    faded = px(50, 40)
    record('控件级 opacity 让画布整体变暗', sum(faded) < sum((0xE8, 0xA0, 0x30)) - 30,
           'faded=%s 原色=(232,160,48)' % (faded,))
    close()
    w.setLayerOpacity(qc3, 255)
    p.pump(300)
    record('setLayerOpacity 复位回 255', w.getLayerOpacity(qc3) == 255)
    px, close = grab(hwnds[qc3], CW, CH)
    record('复位后画布恢复原色', near(px(50, 40), (0xE8, 0xA0, 0x30), 26), 'px=%s' % (px(50, 40),))
    close()
except Exception as e:
    record('setLayerOpacity 接口可用', False, repr(e))

print()
print('合计 %d 项：通过 %d，失败 %d' % (len(PASS) + len(FAIL), len(PASS), len(FAIL)), flush=True)
for f in FAIL:
    print('  失败：' + f, flush=True)

try:
    ok = w.savePng(os.path.join(HERE, 'p0_draw.png'))
    print(('saved ' if ok else 'savePng 返回 False：')
          + os.path.join(HERE, 'p0_draw.png'), flush=True)
except Exception as e:
    print('savePng 不可用：%r' % (e,), flush=True)

p.pump(300)
w.close()
p.pump(200)
p.shutdown()
