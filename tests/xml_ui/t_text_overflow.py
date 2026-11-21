# 第53轮：CSS text-overflow:ellipsis —— 单行溢出打点（此前溢出被硬裁，半个字切在边界上）
import os, sys, glob
BIN = os.environ["PYARX_BIN"]; sys.path.insert(0, BIN); os.add_dll_directory(BIN)
os.environ.setdefault("PYARX_D2D_RT", "software")
os.environ["PYARX_MFC_DEBUG"] = "1"
LOG = os.path.join(BIN, "log", "pymfc_shutdown.log")
if os.path.exists(LOG): os.remove(LOG)
import PyMfcXml as M

TEXT = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
fails = []
def chk(name, cond, got=""):
    print(("PASS  " if cond else "FAIL  ") + name + ("  " + str(got) if got else ""))
    if not cond: fails.append(name)

def run(style):
    n = {}
    w = M.guiload_text(
        '<XWindow Width="240" Height="90">'
        '<label Name="t" Left="10" Top="20" Width="70" Height="26" Text="%s" '
        'style="font-size:14px;%s"/>'
        '</XWindow>' % (TEXT, style), context=globals(), names=n)
    M.pump(250)
    v = w.get_xml_value(n["t"], "TextOverflow")
    w.save_png("G:/PyArx/log/ellip_%s.png" % ("on" if style else "off"))
    w.close()
    return v

v_off = run("")
v_on = run("text-overflow:ellipsis")
chk("TextOverflow 契约回读", v_off == "" and v_on == "ellipsis", (v_off, v_on))

# 第 24 轮补：同一份声明必须在两条绘制通路都生效。
# 此前只有 WM_DRAWITEM（PyMfcProcs::OnDrawItem）自己量宽截断，D3D 表面的节点帧
# （PyMfcHost::drawOwnerNode → canvas.text）不认 text-overflow，同一 XML 在两种后端下
# 一个打点一个硬裁。这里直接比两张截图：ellipsis 版必须与 clip 版像素不同。
from PIL import Image, ImageChops  # noqa: E402
_a = Image.open("G:/PyArx/log/ellip_off.png").convert("RGB")
_b = Image.open("G:/PyArx/log/ellip_on.png").convert("RGB")
_diff = ImageChops.difference(_a, _b)
changed = sum(1 for p in _diff.getdata() if sum(p) > 30)
chk("表面节点帧通路同样按 ellipsis 截断（两版像素不同）", changed > 10, changed)

tail = None
if os.path.exists(LOG):
    for line in open(LOG, encoding="utf-8", errors="ignore"):
        if "[ellip]" in line:
            tail = line.strip()
chk("溢出时确实按最长前缀截断并补省略号", tail is not None and 0 < int(tail.rsplit("tail=", 1)[1]) < len(TEXT), tail)
print("t_text_overflow: PASS=%d FAIL=%d" % (3 - len(fails), len(fails)))
