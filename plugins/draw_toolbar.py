# -*- coding: utf-8 -*-
"""复合控件：竖向绘图工具条（DrawToolBar）。

XML 声明结构与尺寸，本文件只做三件事：
  1. M.xml_layout 取每个图标单元的位置；
  2. addStaticBitmap + setImage 装 plugins/icons/<name>.png；
  3. onClick / onControlEvent(mousemove) 收起事件，派发给 Python 与 CAD 两条通道。

三套图标：icons/（常态）、icons/hi/（悬停，亮白）、icons/on/（选中，蓝底）。
`python G:\\PyArx\\tools\\make_toolbar_icons.py` 一次生成三套。

用法：
    import draw_toolbar
    bar = draw_toolbar.ToolBar(win, 8, 8,
                               on_cmd=lambda cmd: print('Python 通道', cmd),
                               cad_cmd=lambda cmd: print('CAD 通道', cmd))
    bar.hover('I_line')      # 程序化演示悬停态
    bar.set_active('I_line') # 程序化设置选中态
    bar.cmd                  # 当前选中的命令，无则 ''
"""
import os
import xml.etree.ElementTree as ET

import PyMfcXml as M

HERE = os.path.dirname(os.path.abspath(__file__))
XML = os.path.join(HERE, 'draw_toolbar.xml')
ICON_DIR = os.path.join(HERE, 'icons')
W, H = 44, 764
ID_BASE = 8000          # 工具条占 8000..8099


def read_items():
    """XML 顺序 → [(单元名, cmd, 高度)]，分隔单元 cmd 为空。"""
    root = ET.parse(XML).getroot()
    out = []
    for el in root.iter():
        name = el.get('Name')
        if not name:
            continue
        if not name.startswith('I_') and not name.startswith('Sep'):
            continue
        out.append((name, (el.get('Cmd') or '').strip(), int(el.get('Height') or 32)))
    return out


class ToolBar(object):
    def __init__(self, app, x, y, height=None,
                 on_cmd=None, cad_cmd=None, hot=True):
        self.app = app
        self.x, self.y = x, y
        self.height = height or H
        self.items = read_items()
        self.rects = M.xml_layout(XML, W, self.height)
        self.ids = {}
        self.cmds = {}
        self.names = {}          # cid -> 单元名
        self.on_cmd = on_cmd
        self.cad_cmd = cad_cmd
        self.hot = hot
        self.active = ''         # 当前选中 cmd
        self._active_id = None
        self._hot_id = None
        self._build()

    # ---------- 构建 ----------
    def _build(self):
        idx = 0
        for name, cmd, h in self.items:
            r = self.rects.get(name)
            if not r:
                continue
            cid = ID_BASE + idx
            idx += 1
            self.app.add_static_bitmap(cid, self.x + r[0], self.y + r[1], r[2], r[3])
            self.ids[name] = cid
            self.names[cid] = name
            self.cmds[cid] = cmd
            if name.startswith('I_'):
                self._paint(cid, '')
            self.app.on_click(cid, lambda *a, c=cid: self._clicked(c))
            if self.hot:
                try:
                    self.app.on_control_event(
                        cid, 'mousemove', lambda *a, c=cid: self._hover(c))
                except Exception:
                    self.hot = False

    def _paint(self, cid, variant):
        name = self.names.get(cid)
        if not name or not name.startswith('I_'):
            return
        folder = os.path.join(ICON_DIR, variant) if variant else ICON_DIR
        png = os.path.join(folder, name[2:] + '.png')
        if os.path.exists(png):
            try:
                self.app.set_image(cid, png)
            except Exception:
                pass

    # ---------- 事件 ----------
    def _clicked(self, cid):
        cmd = self.cmds.get(cid, '')
        if not cmd:
            return
        self.set_active(cid)
        if self.on_cmd:
            self.on_cmd(cmd)
        if self.cad_cmd:
            self.cad_cmd(cmd)

    def _hover(self, cid):
        if cid == self._hot_id:
            return
        prev, self._hot_id = self._hot_id, cid
        if prev is not None and prev != self._active_id:
            self._paint(prev, '')
        if cid != self._active_id:
            self._paint(cid, 'hi')

    # ---------- 对外 ----------
    @property
    def cmd(self):
        return self.active

    def set_active(self, cid):
        """cid 或单元名均可。"""
        if isinstance(cid, str):
            cid = self.ids.get(cid)
        if cid is None or cid == self._active_id:
            return
        if self._active_id:
            self._paint(self._active_id, '')
        self._active_id = cid
        self.active = self.cmds.get(cid, '')
        self._paint(cid, 'on')

    def hover(self, name):
        """程序化触发悬停态（演示/自测用），name 传单元名如 'I_line'。"""
        cid = self.ids.get(name)
        if cid:
            self._hover(cid)

    def button(self, name):
        return self.ids.get(name)

    def move(self, x, y):
        for name, cid in self.ids.items():
            r = self.rects.get(name)
            if r:
                self.app.set_pos(cid, x + r[0], y + r[1])
        self.x, self.y = x, y
