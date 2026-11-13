# -*- coding: utf-8 -*-
"""
pymfc_palette_circle.py — 嵌入 CAD 主窗口的非侧栏窗体，点击按钮绘制圆。

设计：窗体 + “画圆”按钮。点击按钮 → 回调在主线程用 PyDbTrans 画圆。
窗体用 WS_EX_NOACTIVATE + SW_SHOWNOACTIVATE：可见但不抢 CAD 焦点，
CAD 保持活动视图，绘制后图形即时刷新（无需切回 CAD）。

架构解耦：
  PyMfcXaml.create_embedded 只接收父窗口 HWND（纯 MFC，不碰 CAD API）；
  父 HWND 由 PyArxCore.Ap.PyApp.mainWindow 提供（CAD 应用门面）。

运行方式（CAD 内）：
  命令: PYRUNFILE   (选择本文件)   → 只创建窗体
  然后点击窗体上的“画圆”按钮，观察 CAD 绘图区即时出现新圆。
"""
import PyMfcXaml
import PyArxCore
from PyArxCore.Db import PyDbCircle, PyDbTrans
from PyArxCore.Ge import AcGePoint3d, AcGeVector3d

LOG = r"G:\PyArx\log\pymfc_embed_circle.log"
XAML = r"G:\PyArx\examples\pymfc_palette_circle.xaml"


def _log(msg):
    try:
        with open(LOG, "a", encoding="utf-8") as f:
            f.write(msg + "\n")
    except Exception:
        pass


_state = {"index": 0}


def draw_circle(radius=10.0):
    """画一个圆：圆心随点击次数递增偏移，颜色轮换。"""
    idx = _state["index"]
    _state["index"] += 1
    color_index = (idx % 255) + 1
    offset = idx * 8.0

    with PyDbTrans() as trans:
        circle = PyDbCircle(
            AcGePoint3d(offset, offset, 0.0),
            AcGeVector3d(0.0, 0.0, 1.0),
            radius,
        )
        circle.setColorIndex(color_index)
        trans.addEntity(circle)
        trans.flushScreen()


    PyArxCore.Ap.PyApp.setFocus()

    _log("[circle] done idx=%d color=%d center=(%.1f,%.1f)"
         % (idx, color_index, offset, offset))
    print("已绘制圆：颜色=%d 圆心=(%.1f, %.1f)" % (color_index, offset, offset))
    return (color_index, offset)


def on_draw_click(_event=None):
    # 直接在 UI 回调画圆，draw_circle 内部 commit 后 setFocus 回 CAD 触发重绘
    draw_circle()


def build_embedded_form():
    """创建嵌入 CAD 主窗口的非侧栏窗体：标题 + “画圆”按钮。"""
    parent_hwnd = PyArxCore.Ap.PyApp.mainWindow
    _log("[build] parent_hwnd=0x%X" % (parent_hwnd if parent_hwnd else 0))
    if not parent_hwnd:
        print("无法取得 CAD 主窗口句柄")
        return None

    w = PyMfcXaml.create_embedded(parent_hwnd, "PyArx 画圆演示", 300, 160)
    PyMfcXaml.guiload(XAML, target=w, context=globals())
    _log("[build] embedded form shown")
    return w


win = build_embedded_form()
_log("[done] 嵌入窗体已创建，等待点击按钮画圆")
