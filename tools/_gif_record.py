"""录制设计器交互 GIF：中键平移 / 滚轮缩放 / 下边框缩放。
全部用模拟消息驱动，不移动真实鼠标；捕获固定屏幕区域，窗口变化也能看到。
"""
import ctypes, os, subprocess, time
from ctypes import wintypes as wt
from PIL import Image, ImageGrab

user32 = ctypes.WinDLL("user32", use_last_error=True)
user32.SetWindowPos.argtypes = [wt.HWND, wt.HWND, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_uint]
user32.SendMessageW.argtypes = [wt.HWND, ctypes.c_uint, wt.WPARAM, wt.LPARAM]
user32.SendMessageW.restype = ctypes.c_ssize_t
user32.WindowFromPoint.argtypes = [wt.POINT]
user32.WindowFromPoint.restype = wt.HWND
user32.GetWindowRect.argtypes = [wt.HWND, ctypes.POINTER(wt.RECT)]
user32.ScreenToClient.argtypes = [wt.HWND, ctypes.POINTER(wt.POINT)]

EXE = r"G:\PyArx\X64\Release\UIDesign.exe"
XML = r"G:\PyArx\tests\mfc_xml\power_plan.xml"
OUT = r"G:\PyArx\docs\_uid_demo.gif"
WX, WY, WW, WH = 200, 200, 1400, 900
BBOX = (WX - 20, WY - 20, WX + WW + 20, WY + WH + 20)

def main_window():
    out = []
    def cb(h, _):
        pid = wt.DWORD()
        user32.GetWindowThreadProcessId.argtypes = [wt.HWND, ctypes.POINTER(wt.DWORD)]
        user32.GetWindowThreadProcessId(h, ctypes.byref(pid))
        if pid.value == proc.pid and user32.IsWindowVisible(h):
            r = wt.RECT()
            user32.GetWindowRect(h, ctypes.byref(r))
            if r.right - r.left > 600:
                out.append(h)
        return True
    user32.EnumWindows(ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)(cb), 0)
    return out[0] if out else None

subprocess.run(["taskkill", "/F", "/IM", "UIDesign.exe"], capture_output=True)
time.sleep(1)
proc = subprocess.Popen([EXE, XML], cwd=r"G:\PyArx")
time.sleep(12)

hwnd = main_window()
user32.SetWindowPos(hwnd, 0, WX, WY, WW, WH, 0x0004)
time.sleep(1.2)

frames = []
def shot():
    frames.append(ImageGrab.grab(bbox=BBOX).convert("RGB"))

def pump(seconds, fps=10):
    end = time.time() + seconds
    while time.time() < end:
        shot()
        time.sleep(1.0 / fps)

def client_pt(h, x, y):
    p = wt.POINT(x, y)
    user32.ScreenToClient(h, ctypes.byref(p))
    return p.x | (p.y << 16)

# 画布 HWND：点 (560,420) 屏幕坐标落进画布
p = wt.POINT(560, 420)
canvas = user32.WindowFromPoint(p)

pump(1.2)

# 1) 中键平移：向右下拖
mk = 300 | (300 << 16)
user32.SendMessageW(canvas, 0x0207, 0, mk)
for i in range(1, 9):
    user32.SendMessageW(canvas, 0x0200, 0x0010, (300 + i * 25) | ((300 + i * 15) << 16))
    pump(0.1, fps=10)
user32.SendMessageW(canvas, 0x0208, 0, 500 | (420 << 16))
pump(1.0)

# 2) 滚轮缩放：以光标为锚点放大
screen_pt = 700 | (420 << 16)
for k in range(4):
    user32.SendMessageW(canvas, 0x020A, 120 << 16, screen_pt)
    pump(0.3, fps=10)
pump(1.0)

# 3) 下边框缩放：向下拉高
pc = wt.POINT(700, WH - 4)
user32.ScreenToClient(hwnd, ctypes.byref(pc))
down = pc.x | (pc.y << 16)
user32.SendMessageW(hwnd, 0x0201, 1, down)
for i in range(1, 7):
    user32.SendMessageW(hwnd, 0x0200, 1, pc.x | ((pc.y + i * 18) << 16))
    pump(0.12, fps=10)
user32.SendMessageW(hwnd, 0x0202, 0, down)
pump(1.2)

if frames:
    # 缩到 900 宽，GIF 体积可控
    nw = 760
    for i, f in enumerate(frames):
        nh = int(f.height * nw / f.width)
        f = f.resize((nw, nh), Image.LANCZOS)
        # 每帧左下角点一个 2x2 标记：让相邻帧不完全相同，Pillow 就不会把时间轴合并掉
        from PIL import ImageDraw
        d = ImageDraw.Draw(f)
        n = len(frames) if frames else 1
        d.rectangle([0, f.height - 5, f.width, f.height], fill=(24, 26, 30))
        d.rectangle([0, f.height - 5, int(f.width * (i + 1) / max(1, n)), f.height], fill=(120, 170, 255))
        frames[i] = f
    pal = []
    for f in frames:
        pal.append(f.convert("P", palette=Image.ADAPTIVE, colors=96, dither=Image.NONE))
    pal[0].save(OUT, save_all=True, append_images=pal[1:],
                duration=100, loop=0, optimize=True, disposal=2)
print("frames=%d size=%dx%d out=%s" % (len(frames), frames[0].width, frames[0].height, OUT))
subprocess.run(["taskkill", "/F", "/IM", "UIDesign.exe"], capture_output=True)
