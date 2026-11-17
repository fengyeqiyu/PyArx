"""录制"编辑器窗口大小调整"GIF：四条边各拖一次，全程模拟消息，不动真实鼠标。"""
import ctypes, subprocess, time
from ctypes import wintypes as wt
from PIL import Image, ImageDraw, ImageGrab

u = ctypes.WinDLL("user32", use_last_error=True)
u.SetWindowPos.argtypes = [wt.HWND, wt.HWND, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_uint]
u.SendMessageW.argtypes = [wt.HWND, ctypes.c_uint, wt.WPARAM, wt.LPARAM]
u.SendMessageW.restype = ctypes.c_ssize_t
u.GetWindowRect.argtypes = [wt.HWND, ctypes.POINTER(wt.RECT)]
u.GetClientRect.argtypes = [wt.HWND, ctypes.POINTER(wt.RECT)]
u.EnumChildWindows.argtypes = [wt.HWND, ctypes.c_void_p, wt.LPARAM]
u.IsWindowVisible.argtypes = [wt.HWND]

EXE = r"G:\PyArx\X64\Release\UIDesign.exe"
XML = r"G:\PyArx\tests\mfc_xml\power_plan.xml"
OUT = r"G:\PyArx\docs\_uid_resize.gif"
WX, WY, WW, WH = 200, 200, 1400, 900
BASE = (60, 100)          # 合成画布左上角（屏幕坐标）
CANVAS = (1820, 1200)     # 合成画布尺寸：够放窗口向四个方向长大

subprocess.run(["taskkill", "/F", "/IM", "UIDesign.exe"], capture_output=True)
time.sleep(1)
proc = subprocess.Popen([EXE, XML], cwd=r"G:\PyArx")
time.sleep(12)

def find_main():
    res = []
    CB = ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)
    def cb(h, _):
        pid = wt.DWORD()
        u.GetWindowThreadProcessId(h, ctypes.byref(pid))
        if pid.value == proc.pid and u.IsWindowVisible(h):
            r = wt.RECT(); u.GetWindowRect(h, ctypes.byref(r))
            if r.right - r.left > 600:
                res.append(h)
        return True
    u.EnumWindows(CB(cb), 0)
    return res[0]

hwnd = find_main()
u.SetWindowPos(hwnd, -1, WX, WY, WW, WH, 0x0010)   # HWND_TOPMOST：录制时不被别的窗口盖住
time.sleep(1.2)

# 找满客户区的 pane 子窗口（画布/工具栏都是 pane 自己画的，消息要发给它）
client = wt.RECT(); u.GetClientRect(hwnd, ctypes.byref(client))
pane = [0]
CB2 = ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)
def cb2(h, _):
    r = wt.RECT(); u.GetWindowRect(h, ctypes.byref(r))
    if r.right - r.left >= client.right and r.bottom - r.top >= client.bottom:
        pane[0] = h
        return False
    return True
u.EnumChildWindows(hwnd, CB2(cb2), 0)
assert pane[0], "pane not found"

frames = []
def shot():
    r = wt.RECT(); u.GetWindowRect(hwnd, ctypes.byref(r))
    img = ImageGrab.grab(bbox=(r.left, r.top, r.right, r.bottom)).convert("RGB")
    canvas = Image.new("RGB", CANVAS, (18, 20, 24))
    canvas.paste(img, (r.left - BASE[0], r.top - BASE[1]))
    frames.append(canvas)

def pump(sec, fps=8):
    end = time.time() + sec
    while time.time() < end:
        shot(); time.sleep(1 / fps)

def drag(edge, fx, fy, steps, step_dx, step_dy, hold=0.055):
    """edge=1左 2右 4上 8下；fx/fy 为 pane 客户区起点（百分比 0~1）"""
    w = wt.RECT(); u.GetClientRect(pane[0], ctypes.byref(w))
    x = int(w.right * fx); y = int(w.bottom * fy)
    lp = x | (y << 16)
    u.SendMessageW(pane[0], 0x0201, 1, lp)
    for i in range(1, steps + 1):
        nx, ny = x + i * step_dx, y + i * step_dy
        u.SendMessageW(pane[0], 0x0200, 1, (nx & 0xFFFF) | ((ny & 0xFFFF) << 16))
        pump(hold, fps=8)
    u.SendMessageW(pane[0], 0x0202, 0, lp)
    pump(0.55)

pump(1.1)

# 1) 右边框向右拉宽
drag(2, 0.995, 0.5, 10, 22, 0)
# 2) 下边框向下拉高
drag(8, 0.5, 0.99, 8, 0, 20)
# 3) 左边框向左拉宽
drag(1, 0.005, 0.5, 6, -20, 0)
# 4) 上边框向上拉高
drag(4, 0.5, 0.005, 5, 0, -16)

pump(0.9)

if frames:
    nw = 760
    for i, f in enumerate(frames):
        f = f.resize((nw, int(f.height * nw / f.width)), Image.LANCZOS)
        d = ImageDraw.Draw(f)
        n = len(frames)
        d.rectangle([0, f.height - 5, f.width, f.height], fill=(24, 26, 30))
        d.rectangle([0, f.height - 5, int(f.width * (i + 1) / max(1, n)), f.height], fill=(120, 170, 255))
        frames[i] = f.convert("P", palette=Image.ADAPTIVE, colors=96, dither=Image.NONE)
    frames[0].save(OUT, save_all=True, append_images=frames[1:], duration=125, loop=0,
                   optimize=True, disposal=2)
print("frames=%d size=%dx%d" % (len(frames), frames[0].width, frames[0].height))
subprocess.run(["taskkill", "/F", "/IM", "UIDesign.exe"], capture_output=True)
