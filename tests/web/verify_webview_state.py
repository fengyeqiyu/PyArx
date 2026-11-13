# -*- coding: utf-8 -*-
"""MFCWebView 状态 / 会话验收：新建一个全新会话，逐项验壳层状态是否如实回报。

跑法：
    & "D:\\Program Files\\Python314\\python.exe" G:\\PyArx\\tests\\web\\verify_webview_state.py

说明：CAD 里 PyArxCore 长期占着 %LOCALAPPDATA%\\PyArx\\WebView2Data 这个 WebView2 用户数据
目录，一个目录同时只允许一个浏览器进程，所以独立会话必须换目录（用 PYARX_WEBVIEW_DATA 指定）。

日志：G:\\PyArx\\tests\\web\\_webview_state.log
截图：G:\\PyArx\\tests\\web\\_webview_state_parent.bmp / _webview_state_chrome.bmp
"""
import ctypes
import ctypes.wintypes as wt
import json
import os
import struct
import sys
import time

STANDALONE_DATA = os.path.join(os.environ.get('LOCALAPPDATA', os.environ['TEMP']), 'PyArx', 'WebView2Data-standalone')
os.makedirs(STANDALONE_DATA, exist_ok=True)
os.environ['PYARX_WEBVIEW_DATA'] = STANDALONE_DATA

sys.path.insert(0, r'G:\PyArx\X64\Release')
import PyMfcWebView as wv  # noqa: E402

PAGE = r'G:\PyArx\tests\web\webview_state_probe.html'
LOG = r'G:\PyArx\tests\web\_webview_state.log'
SHOT_PARENT = r'G:\PyArx\tests\web\_webview_state_parent.bmp'
SHOT_CHROME = r'G:\PyArx\tests\web\_webview_state_chrome.bmp'

u32 = ctypes.windll.user32
g32 = ctypes.windll.gdi32

events = []          # 页面回推的所有回执
ready_payload = []   # readyHandler 收到的原文
results = []


def log(msg):
    line = str(msg)
    print(line)
    with open(LOG, 'a', encoding='utf-8') as fh:
        fh.write(line + '\n')


def check(name, ok, detail=''):
    results.append((name, bool(ok)))
    log(('  OK  ' if ok else '  FAIL') + '  %-32s %s' % (name, detail))


def on_command(raw):
    """页面回推：{cmd:'probe.echo', seq:n, data:{...}}"""
    try:
        msg = json.loads(raw)
    except Exception:
        return False
    if msg.get('cmd') == 'probe.echo':
        events.append(msg.get('data'))
        return True
    return False


def last_event(name, since=0):
    for item in reversed(events[since:]):
        if isinstance(item, dict) and item.get('event') == name:
            return item
    return None


def capture(hwnd, path):
    rc = wt.RECT()
    u32.GetWindowRect(hwnd, ctypes.byref(rc))
    w_, h_ = rc.right - rc.left, rc.bottom - rc.top
    if w_ <= 0 or h_ <= 0:
        return (0, 0, 0)
    hdc = u32.GetWindowDC(hwnd)
    mdc = g32.CreateCompatibleDC(hdc)
    bmp = g32.CreateCompatibleBitmap(hdc, w_, h_)
    old = g32.SelectObject(mdc, bmp)
    u32.PrintWindow(hwnd, mdc, 0x2)

    class BMI(ctypes.Structure):
        _fields_ = [('biSize', wt.DWORD), ('biWidth', wt.LONG), ('biHeight', wt.LONG),
                    ('biPlanes', wt.WORD), ('biBitCount', wt.WORD), ('biCompression', wt.DWORD),
                    ('biSizeImage', wt.DWORD), ('biXPelsPerMeter', wt.LONG),
                    ('biYPelsPerMeter', wt.LONG), ('biClrUsed', wt.DWORD), ('biClrImportant', wt.DWORD)]

    bmi = BMI()
    bmi.biSize = ctypes.sizeof(BMI)
    bmi.biWidth, bmi.biHeight = w_, -h_
    bmi.biPlanes, bmi.biBitCount = 1, 32
    stride = w_ * 4
    buf = ctypes.create_string_buffer(stride * h_)
    g32.GetDIBits(mdc, bmp, 0, h_, buf, ctypes.byref(bmi), 0)

    data = buf.raw
    colors = set()
    for i in range(0, len(data) - 4, 4 * 61):
        colors.add(bytes(data[i:i + 3]))
    with open(path, 'wb') as fh:
        fh.write(b'BM')
        fh.write(struct.pack('<IHHI', 14 + 40 + len(data), 0, 0, 54))
        fh.write(struct.pack('<IiiHHIIiiII', 40, w_, -h_, 1, 32, 0, len(data), 2835, 2835, 0, 0))
        fh.write(data)
    g32.SelectObject(mdc, old)
    g32.DeleteObject(bmp)
    g32.DeleteDC(mdc)
    u32.ReleaseDC(hwnd, hdc)
    return (w_, h_, len(colors))


def find_chrome_child(hwnd):
    found = []

    @ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)
    def cb(child, _):
        cls = ctypes.create_unicode_buffer(256)
        u32.GetClassNameW(child, cls, 256)
        if cls.value.startswith('Chrome_WidgetWin'):
            found.append(child)
        return True

    u32.EnumChildWindows(hwnd, cb, 0)
    return found[0] if found else 0


def ask(win, cmd, args=None, wait_ms=500):
    """发一条命令并等回执"""
    if args:
        win.eval("window.ask(%r, %s)" % (cmd, json.dumps(args)))
    else:
        win.eval("window.ask(%r)" % cmd)
    win.runFor(wait_ms)


open(LOG, 'w', encoding='utf-8').close()
log('=== MFCWebView 状态 / 会话验收 %s ===' % time.strftime('%Y-%m-%d %H:%M:%S'))
log('独立 user data 目录：%s' % STANDALONE_DATA)

win = wv.WebWindow(900, 640, 'PyArx 状态会话验证')
win.setCommandHandler(on_command)
win.setReadyHandler(lambda raw: ready_payload.append(raw))
log('新会话窗口已建：hwnd=%s' % win.hwnd())

win.loadFile(PAGE)
win.show()
win.center()

t0 = time.time()
for _ in range(80):
    win.runFor(100)
    if win.isReady():
        break
check('WebView2 初始化完成', win.isReady(), '耗时 %.1fs, isReady=%s' % (time.time() - t0, win.isReady()))
check('窗口可见', bool(u32.IsWindowVisible(win.hwnd())), 'IsWindowVisible=%s' % u32.IsWindowVisible(win.hwnd()))

# 1) 就绪事件
win.runFor(700)
ready_evt = last_event('shell.ready')
check('收到 shell.ready 事件', bool(ready_evt), json.dumps(ready_evt, ensure_ascii=False) if ready_evt else '无事件')
check('Python 侧 readyHandler 被回调', bool(ready_payload), str(ready_payload[:1]))
if ready_evt:
    for key in ('dpi', 'scale', 'theme', 'width', 'height', 'state'):
        check('shell.ready 含 %s' % key, key in ready_evt, 'value=%r' % ready_evt.get(key))
    check('dpi 报真实 DPI（≥96）', float(ready_evt.get('dpi', 0)) >= 96, 'dpi=%r' % ready_evt.get('dpi'))
    check('scale 与 dpi 自洽', abs(float(ready_evt.get('scale', 0)) - float(ready_evt.get('dpi', 96)) / 96.0) < 0.02,
          'scale=%r dpi=%r' % (ready_evt.get('scale'), ready_evt.get('dpi')))

# 2) 主动索取 shell.info，比对客户区
n0 = len(events)
ask(win, 'shell.info')
info = last_event('shell.info', n0)
check('shell.info 有回执', bool(info), json.dumps(info, ensure_ascii=False) if info else '无回执')

rc = wt.RECT()
u32.GetClientRect(win.hwnd(), ctypes.byref(rc))
cw, ch = rc.right - rc.left, rc.bottom - rc.top
if info:
    check('客户区宽高与 GetClientRect 一致',
          abs(int(info.get('width', -1)) - cw) <= 2 and abs(int(info.get('height', -1)) - ch) <= 2,
          '上报=%sx%s 实测=%sx%s' % (info.get('width'), info.get('height'), cw, ch))
    check('state 初值为 normal', info.get('state') == 'normal', 'state=%r' % info.get('state'))

# 3) 窗口状态：max → restore，回执要跟着变
n1 = len(events)
ask(win, 'window.max', wait_ms=800)
st_max = last_event('window.state', n1)
check('window.max 回执 state=max', bool(st_max) and st_max.get('state') == 'max',
      'payload=%s' % (json.dumps(st_max, ensure_ascii=False) if st_max else '无回执'))
check('最大化后 IsZoomed=1', bool(u32.IsZoomed(win.hwnd())), 'IsZoomed=%s' % u32.IsZoomed(win.hwnd()))

n2 = len(events)
ask(win, 'window.restore', wait_ms=800)
st_norm = last_event('window.state', n2)
check('window.restore 回执 state=normal', bool(st_norm) and st_norm.get('state') == 'normal',
      'payload=%s' % (json.dumps(st_norm, ensure_ascii=False) if st_norm else '无回执'))

# 4) resize 后客户区对齐
n3 = len(events)
ask(win, 'window.resize', {'w': 820, 'h': 600}, wait_ms=800)
u32.GetClientRect(win.hwnd(), ctypes.byref(rc))
cw2, ch2 = rc.right - rc.left, rc.bottom - rc.top
res_evt = last_event('window.resized', n3)
check('window.resize 客户区生效', abs(cw2 - 820) <= 3 and abs(ch2 - 600) <= 3,
      '实测=%sx%s 回执=%s' % (cw2, ch2, json.dumps(res_evt, ensure_ascii=False) if res_evt else '无'))
n4 = len(events)
ask(win, 'shell.info')
info2 = last_event('shell.info', n4)
if info2:
    check('resize 后 shell.info 上报新尺寸',
          abs(int(info2.get('width', -1)) - 820) <= 3 and abs(int(info2.get('height', -1)) - 600) <= 3,
          '上报=%sx%s' % (info2.get('width'), info2.get('height')))

# 5) 会话连续性：连续 5 条命令，回执数量与顺序都要对
n5 = len(events)
for _ in range(5):
    ask(win, 'shell.info', wait_ms=250)
got = [e.get('event') for e in events[n5:] if isinstance(e, dict)]
check('会话连续 5 条命令全部有回执', len(got) == 5, '收到 %d 条: %s' % (len(got), got))
check('连发命令事件类型一致', all(e == 'shell.info' for e in got), 'events=%s' % got)
check('会话内序号连续递增', all(e.get('event') for e in events[n5:]), '共 %d 条回推' % len(events[n5:]))

# 6) 像素取证
u32.ShowWindow(win.hwnd(), 5)
win.runFor(700)
pw, ph, pcolors = capture(win.hwnd(), SHOT_PARENT)
check('窗口 PrintWindow 有内容', pcolors >= 2, 'parent %sx%s unique=%d' % (pw, ph, pcolors))
child = find_chrome_child(win.hwnd())
check('找到 Chrome_WidgetWin 子窗', bool(child), 'hwnd=%s' % child)
if child:
    cw3, ch3, ccolors = capture(child, SHOT_CHROME)
    check('WebView2 子窗有内容（页面真的画出来了）', ccolors >= 3, 'chrome %sx%s unique=%d' % (cw3, ch3, ccolors))

# 7) 会话数据目录
check('独立会话数据目录有内容', os.path.isdir(os.path.join(STANDALONE_DATA, 'EBWebView')),
      os.path.join(STANDALONE_DATA, 'EBWebView'))

# 8) 关闭
n6 = len(events)
ask(win, 'window.close', wait_ms=900)
check('window.close 生效', win.isClosed(), 'isClosed=%s' % win.isClosed())
win.destroy()

passed = sum(1 for _, ok in results if ok)
failed = len(results) - passed
log('状态会话验收：通过 %d，失败 %d' % (passed, failed))
log('WEBVIEW STATE TEST PASSED' if failed == 0 else 'WEBVIEW STATE TEST FAILED')
sys.exit(0 if failed == 0 else 1)
