# -*- coding: utf-8 -*-
"""app.py —— PyArx CAD 二维绘图应用入口

  cd G:\\PyArx\\X64\\Release
  .\\python.exe G:\\PyArx\\examples\\cad2d\\app.py

分层（对应七个模块）：
  1 UI 核心      cad_shell.xml（外观全 XML）+ 本文件 App（装配与刷新）
  2 几何核心     geom/        纯数学
  3 渲染核心     render/      实体 → SVG
  4 符号表       db/          图层 / 线型 / 文字样式 / 块
  5 实体类       ent/         Line/Circle/Arc/Rect/Polyline/Text/Insert
  6 交互核心     interact/    坐标换算 / 捕捉 / 命令状态机
  7 命令行       cmdline/     命令名、别名、坐标输入
  底层           MfcXmlCore + PyMfcXml（窗口、控件、结构化画布）

测试（在 examples\\cad2d 下执行，PYTHONPATH 指向 X64\\Release）：
  python tests\\test_ui_shell.py   界面层：控件齐备 / 工具条 14 个按钮真实点击 / 胶囊 / 特性行编辑
  python app.py --batch           图元流程 17 用例（画/改/选/存盘读盘/DIM/HATCH/TEXT）
  python app.py --selftest        端到端走查（命令→画布→提交→重画→存盘）
"""
import ctypes
import json
import os
import sys
import math
import time

HERE = os.path.dirname(os.path.abspath(__file__))
if HERE not in sys.path:
    sys.path.insert(0, HERE)

import PyMfcXml as p          # noqa: E402
import cmdline                # noqa: E402
import doc                    # noqa: E402
import ent                    # noqa: E402
import geom                   # noqa: E402
import history                # noqa: E402
import interact               # noqa: E402
import render                 # noqa: E402

XML = os.path.join(HERE, "cad_shell.xml")
SHOT = os.path.join(HERE, "cad_shot.png")
DRAWING = os.path.join(HERE, "cad_drawing.json")


def demo_doc():
    """示例图：一堵墙 + 内墙 + 门洞弧 + 圆 + 矩形 + 文字，验证几何/颜色/缩放。"""
    d = doc.Document(12000.0, 8000.0)
    d.layers["WALL"] = doc.Layer("WALL", 7, 3.0)
    d.layers["DOOR"] = doc.Layer("DOOR", 5, 1.5)
    d.layers["MARK"] = doc.Layer("MARK", 7, 1.0)

    d.add(doc.Polyline([(1000, 1000), (10000, 1000), (10000, 6500), (1000, 6500)],
                       closed=True, layer="WALL"))
    d.add(doc.Line(1000, 4000, 6000, 4000, layer="WALL"))
    d.add(doc.Line(6000, 1000, 6000, 4000, layer="WALL"))
    d.add(doc.Arc(6000, 4000, 900.0, 0.0, 90.0, layer="DOOR"))
    d.add(doc.Circle(3000, 2500, 800.0, layer="MARK"))
    d.add(doc.Rect(7600, 4600, 9200, 6000, layer="MARK"))
    d.add(doc.Text(1400, 6800, "PyArx CAD — 二维绘图演示", 420.0, layer="MARK"))
    return d


class App(object):
    # 界面实体全在 cad_shell.xml（原生控件），这里只负责装配与刷新：
    # 按钮 Name → Python 回调（_bind_buttons）、数据 → 控件文字（_refresh_*）。

    def __init__(self, initial_doc=None):
        self.names = {}
        with open(XML, "r", encoding="utf-8-sig") as f:
            shell = f.read()
        # @@RES@@ → examples\cad2d\res 的绝对路径：外壳只写文件名，换机器不用改 XML
        shell = shell.replace("@@RES@@", os.path.join(HERE, "res"))
        self.win = p.guiload_text(shell, context=globals(), names=self.names)
        self.win.show()
        p.pump(200)
        # 首帧的控件位置还是布局前的兜底坐标，重排一次让控件落到 XML 声明的
        # 位置，并整窗重画 —— 否则旧的绘制位置会在左边缘留下一条残字。
        try:
            self.win.reflow_xml()
        except Exception:
            pass
        p.pump(120)

        self.doc = initial_doc if initial_doc is not None else demo_doc()
        self.mapper = interact.ViewMapper(self.doc.view(), self.doc.screen_unit)
        self.cmds = interact.CommandManager(self.doc, self.mapper)
        self.cmds.on_prompt = self._prompt
        self.cmds.on_commit = self._commit
        self.selection = []
        self.hist = history.UndoStack(self.doc)
        self.grip = None              # (entity, index) 正在拖动的夹点
        self.grip_before = None
        self.rubber = None            # 窗口选择起点（世界坐标）
        self.cursor_world = None      # CAD 十字光标世界坐标
        self._mbtn = False            # 中键按住平移中：自绘十字隐藏，用爪型指针
        self._last_draw = 0.0
        self.grid_on = False
        self._base_dirty = True       # 底图场景是否需要重灌（图元/图层变化时置位）
        self.point_mode = True        # 默认左键选择/定点，中键平移（AutoCAD 习惯）
        self.tab = "DRAW"
        self.status = {"coord": "X 0.0  Y 0.0", "zoom": "100%"}
        self._last_status = 0.0

        # 右栏信息行：数据在 Python，控件是外壳里预置的 <XLabel> 槽位
        # 形态 [(组名, [(键, 值[, 字段]), ...]), ...]（与 _show_props 一致）
        self.prop_rows = [("常规", [("颜色", "—"), ("图层", "—"),
                                  ("线型", "—"), ("线宽", "—")]),
                          ("几何图形", [("类型", "—")])]
        self.view_rows = [("缩放", "100%"), ("光标", "0.0, 0.0"), ("图元", "0")]
        self.prop_field = None

        self._bind_buttons()
        self._refresh_props()
        self._refresh_view()
        self._refresh_status()
        self._hint("就绪")
        cv = self.names["cvMain"]
        self.win.on_canvas_event(cv, self._on_canvas)
        self.redraw(fit=True)
        self._refresh_layers()

    # ---------------------------------------------------------- 画布事件
    def _on_canvas(self, phase, x, y, flags):
        world = self.mapper.screen_to_world(x, y)
        self._cursor(world)
        if phase == "move":
            self._mbtn = bool(int(flags) & 0x10)   # MK_MBUTTON：中键按住平移中
            self._sync_view()      # C++ 侧中键/左键拖拽平移后，拉回 st.ox/oy 供命中换算
            if self.grip is not None:
                self._grip_drag(world)
                return True
            if self.rubber is not None:
                self.rubber = (self.rubber[0], world)
                self.render()
                return True
            self._preview(world)
            if self.cmds.active is None:
                now = time.time()
                if now - self._last_draw > 0.03:
                    self._last_draw = now
                    self.render()      # 十字线跟随光标
            return False
        if phase == "wheel":
            # 松开给宿主处理缩放；缩放后视图变了，换算基准要跟上
            self._sync_view()
            return False
        if phase == "key":
            # ESC(27) 取消当前绘制命令，业务在 Python 命令状态机里收口
            if int(flags) == 27:
                self.cmds.cancel()
                self.rubber = None
                self.render()
            return False
        if phase == "down":
            if self.cmds.active is None:
                hit = self._grip_hit(world)
                if hit is not None:
                    self.grip = hit
                    self.grip_before = self.grip[0].snapshot()
                    return True
                if self.point_mode:
                    if self._select_at(world):
                        return True
                    # 没命中图元：起窗口选择橡皮框
                    self.rubber = (world, world)
                    return True
                return False          # 否则左键仍是平移
            self.cmds.selection = list(self.selection)
            return self.cmds.click(world)
        if phase == "up":
            self._mbtn = False
            if self.grip is not None:
                self._grip_release()
                return True
            if self.rubber is not None:
                self._finish_rubber(world)
                return True
            return False
        if phase == "dblclick":
            return self.cmds.finish_poly()
        return False

    def _cursor(self, world):
        self.cursor_world = world
        self.view_rows[1] = ("光标", "%.1f, %.1f" % (world[0], world[1]))
        self.status["coord"] = "X %.2f  Y %.2f" % (world[0], world[1])
        now = time.time()
        if now - self._last_status > 0.05:
            self._last_status = now
            self._refresh_view()
            self._refresh_status()

    # ---------------------------------------------------------- 预览与重绘
    def _preview(self, world):
        if self.cmds.active is None:
            return
        prev, snap = self.cmds.move(world)
        now = time.time()
        if now - self._last_draw < 0.04:
            return
        self._last_draw = now
        self.render(preview=prev)
        hint = (" 捕捉:%s" % snap.kind) if snap is not None else ""
        self._hint("左键定点" + hint)

    def render(self, preview=None):
        """画一帧：底图只在图元真的变了才重灌，其余每帧只换叠加层 + 视图。"""
        cv = self.names["cvMain"]
        u = self.doc.screen_unit
        step = 0.0
        if self.grid_on:
            step = 24.0 / (self.mapper.zoom * u)
            step = max(50.0, min(5000.0, step))
        self._ensure_base()
        ovl = render.to_overlay_xml(
            self.doc.symbols, self.doc.view(), unit=u, zoom=self.mapper.zoom,
            absolute=True, grid=self.grid_on, grid_step=step,
            selection=self.selection, rubber=self.rubber, preview=preview,
            cursor=None if self._mbtn else self.cursor_world)
        self.win.load_dwg_overlay_xml(cv, ovl)
        self.win.set_dwg_view(cv, *self._to_canvas())

    def _ensure_base(self):
        """整图场景只喂一次：坐标取世界绝对值，之后平移缩放全由画布自己算。

        画布吃的是 <XScene> 声明式场景（XLine/XArc/XPolyline/XText…），不再是
        拼好的 SVG 文本：省掉「Python 拼串 → C++ 当 SVG 再解析一遍」这条往返。
        """
        if not self._base_dirty:
            return
        self.win.load_dwg_xml(self.names["cvMain"], render.to_xml(
            self.doc.symbols, self.doc.visible_entities(), self.doc.view(),
            unit=self.doc.screen_unit, absolute=True, cull=False))
        self._base_dirty = False

    # --- 视图换算：场景是"世界绝对坐标"，mapper 仍以视图左下角为原点 ---
    def _to_canvas(self):
        return self._to_canvas_z(self.mapper.zoom, self.mapper.ox, self.mapper.oy)

    def _to_mapper(self, z, ox, oy):
        view = self.doc.view()
        u = self.doc.screen_unit
        return (z, ox + view[0] * u * z, oy - view[3] * u * z)

    def redraw(self, fit=False):
        self._base_dirty = True          # 图元/图层变了，底图要重灌
        if fit:
            self.fit()
        self.render()
        self.view_rows[2] = ("图元", str(self.doc.count()))
        self._zoom_label()
        self._refresh_view()
        self._refresh_status()

    def fit(self):
        cv = self.names["cvMain"]
        try:
            cw, ch = self.win.get_size(cv)
        except Exception:
            cw, ch = 820, 592
        x0, y0, x1, y1 = self.doc.view()
        u = self.doc.screen_unit
        w = max(1.0, (x1 - x0) * u)
        h = max(1.0, (y1 - y0) * u)
        z = min((cw - 40) / w, (ch - 40) / h)
        self.win.set_dwg_view(cv, *self._to_canvas_z(z, 20.0, 20.0))
        self._sync_view()

    def _to_canvas_z(self, z, ox, oy):
        view = self.doc.view()
        u = self.doc.screen_unit
        return (z, int(round(ox - view[0] * u * z)), int(round(oy + view[3] * u * z)))

    def _sync_view(self):
        try:
            z, ox, oy = self.win.get_dwg_view(self.names["cvMain"])
            z, ox, oy = self._to_mapper(z, ox, oy)
            self.mapper.set_view(self.doc.view(), z, ox, oy)
        except Exception:
            pass
        self._zoom_label()

    def _zoom_label(self):
        z = "%.0f%%" % (self.mapper.zoom * 100.0)
        self.status["zoom"] = z
        self.view_rows[0] = ("缩放", z)

    # ---------------------------------------------------------- 命令
    def _prompt(self, text):
        if text:
            self._hint(text)
        print("[cad2d]", text)

    def _commit(self, payload):
        if isinstance(payload, tuple):
            kind, data = payload
            if kind == "erase" and data:
                ids = {e.id for e in data}
                for e in data:
                    self.hist.push_remove(e)
                self.doc.remove_many(data)
                self.selection = [e for e in self.selection if e.id not in ids]
                self.redraw()
                self._show_props(self.selection[0] if self.selection else None)
                self.info("已删除 %d 个图元" % len(data))
            elif kind == "select":
                self.selection = [data] if data is not None else []
                self._show_props(data)
            elif kind == "clear":
                self.selection = []
                self._show_props(None)
            elif kind == "modify":
                entities = data["entities"]
                before = data["before"]
                for e, b in zip(entities, before):
                    self.hist.push_modify(e, b)
                self.selection = list(entities)
                self._show_props(entities[0] if entities else None)
                self.redraw()
                self.info("已修改 %d 个图元" % len(entities))
            elif kind == "add_many":
                for e in data:
                    self.doc.add(e)
                    self.hist.push_add(e)
                self.selection = list(data)
                self._show_props(data[0] if data else None)
                self.redraw()
                self._refresh_layers()
                self.info("已复制 %d 个图元" % len(data))
            elif kind == "block":
                # 块表记录：深拷贝一组对象 + 基点入块表，原图保留不动
                copies = [e.snapshot() for e in data["entities"]]
                name = (data.get("name") or "").strip()
                if not name:
                    name = "块%d" % (len(self.doc.blocks.names()) + 1)
                self.doc.blocks.define(name, copies, data["base"])
                self.hist.push_block(name, self.doc.blocks.get(name))
                self.info("已定义块 %s（%d 个图元）" % (name, len(copies)))
            return
        if payload is None:
            return
        self.doc.add(payload)
        self.hist.push_add(payload)
        self.selection = [payload]
        self._show_props(payload)
        self.redraw()
        self._refresh_layers()
        self.info("已创建 %s" % payload.kind)

    def info(self, text):
        self._hint(text)
        print("[cad2d]", text)

    def _run_input(self):
        n = self.names
        line = self.win.get_text(n["txtCmd"]) if "txtCmd" in n else ""
        self.win.set_text(n["txtCmd"], "")
        if line.strip():
            self.run_text(line.strip())

    def run_cmd(self, name, args=None):
        if not self.cmds.start(name, args):
            self.info("未知命令：%s" % name)
            return
        self.info("命令 %s：%s" % (name, self.cmds.active.prompt))
        self._hint("左键定点")

    def run_text(self, line):
        """命令行输入：解析后派发。"""
        res = cmdline.parse(line)
        if res is None:
            self.info("未知命令：%s" % line)
            return
        if res.action == "CMD":
            self.run_cmd(res.command, res.args)
        elif res.action == "ZOOM_E":
            self.fit()
        elif res.action == "ZOOM_IN":
            self._zoom_step(1.25)
        elif res.action == "ZOOM_OUT":
            self._zoom_step(1 / 1.25)
        elif res.action == "UNDO":
            self._undo()
        elif res.action == "REDO":
            self._redo()
        elif res.action == "OPEN":
            self.load_json(res.args or DRAWING)
        elif res.action == "SAVE":
            self.save()
        elif res.action == "LAYER":
            self.info("图层：" + ", ".join(self.doc.layers.names()))
        elif res.action == "HELP":
            for ln in cmdline.help_text().splitlines():
                print("[cad2d]", ln)
        elif res.action == "POINT":
            self.info("坐标输入待接命令"% ())
        else:
            self.info("命令 %s 待接入" % res.action)

    # ---------------------------------------------------------- 界面刷新
    # 界面全部是外壳里的原生控件：这里只把数据写进控件（setText / setVisible /
    # setXmlValue），不再拼 SVG、不再维护坐标命中表 —— 点击由控件自身 onClick 回传。
    _PROP_SLOTS = 4          # 常规行槽（颜色/图层/线型/线宽）
    _GEO_SLOTS = 8           # 几何行槽（line 最多 8 行）

    def _hint(self, text):
        """命令提示行（原 cvHint 自绘窄条）。"""
        if "lblHint" in self.names:
            self.win.set_text(self.names["lblHint"], text or "")

    def _set_row(self, prefix, idx, label, value):
        """写一行键值：key 槽 + value 槽，槽位在外壳 XML 里预置。"""
        kid = self.names.get("%s%dk" % (prefix, idx))
        vid = self.names.get("%s%dv" % (prefix, idx))
        if kid is not None:
            self.win.set_text(kid, label or "")
        if vid is not None:
            self.win.set_text(vid, "" if value is None else str(value))

    def _refresh_props(self):
        """把 prop_rows（[(组名, [(键, 值[, 字段]), ...]), ...]）摊进固定行槽。"""
        general, geo = [], []
        for gname, grows in self.prop_rows:
            if "几何" in gname:
                geo.extend(grows)
            else:
                general.extend(grows)
        self._row_fields = {}
        for i in range(self._PROP_SLOTS):
            row = general[i] if i < len(general) else ("", "")
            self._row_fields[("pr", i)] = row[2] if len(row) > 2 else None
            self._set_row("pr", i, row[0], row[1])
        for i in range(self._GEO_SLOTS):
            row = geo[i] if i < len(geo) else ("", "")
            self._row_fields[("ge", i)] = row[2] if len(row) > 2 else None
            self._set_row("ge", i, row[0], row[1])

    def _refresh_view(self):
        """右栏"视图"三行：缩放 / 光标 / 图元数。"""
        for i, row in enumerate(self.view_rows[:3]):
            self._set_row("vw", i, row[0], row[1])

    def _accent(self, name, on):
        """状态栏胶囊的开启态：文字走强调色，不做整块高亮（与深色主题一致）。"""
        try:
            # setXmlValue 的第一个参数是控件名（外壳 XML 里的 Name），不是 id
            self.win.set_xml_value(name, "ForeColor", "#FF4C9AFF" if on else "#FFE6EDF3")
        except Exception:
            pass

    def _refresh_status(self):
        """状态栏：左三格（模型 / 栅格 / 左键模式）+ 右三格（坐标 / 图元 / 缩放）。"""
        n = self.names
        if "stlCoord" in n:
            self.win.set_text(n["stlCoord"], self.status["coord"])
        if "stlCount" in n:
            self.win.set_text(n["stlCount"], "图元 %d" % self.doc.count())
        if "stlZoom" in n:
            self.win.set_text(n["stlZoom"], self.status["zoom"])
        if "btnGrid" in n:
            self.win.set_text(n["btnGrid"], "栅格 " + ("开" if self.grid_on else "关"))
            self._accent("btnGrid", self.grid_on)
        if "btnMode" in n:
            self.win.set_text(n["btnMode"], "左键 " + ("定点" if self.point_mode else "平移"))
            self._accent("btnMode", self.point_mode)

    def _on_row(self, prefix, idx):
        """点特性行的键名 → 该字段进入编辑（下拉或输入框）。"""
        field = (getattr(self, "_row_fields", None) or {}).get((prefix, idx))
        if field:
            self._edit_prop(field)

    def _on_ribbon(self, name):
        """原生 XRibbon 命中：回传按钮 Name，直接派发命令。"""
        if name:
            self._fire(name)

    def _fire(self, act):
        """功能区/工具条共用的动作派发。"""
        if act == "FIT":
            self.fit()
        elif act in ("ZOOM_IN", "ZOOM_OUT"):
            self._zoom("in" if act == "ZOOM_IN" else "out")
        elif act == "SAVE":
            self.save()
        elif act == "OPEN":
            self.load_json()
        elif act == "EXPORT":
            self.export_svg()
        elif act == "LAYER_NEW":
            self._new_layer()
        elif act == "GRID":
            self._toggle_grid()
        elif act == "PANMODE":
            self._toggle_point()
        elif act in ("UNDO",):
            self._undo()
        elif act in ("REDO",):
            self._redo()
        elif act in ("MOVE", "COPY", "ROTATE", "SCALE"):
            self.run_cmd(act)
        else:
            self.run_cmd(act)

    def _bind_buttons(self):
        w, n = self.win, self.names
        # 左工具条：外壳里 14 个 XIconButton，按钮 Name 就是命令名
        for key in ("LINE", "PLINE", "CIRCLE", "ARC", "RECT", "HATCH", "TEXT", "DIM",
                    "MOVE", "COPY", "ROTATE", "ERASE", "SELECT", "FIT"):
            cid = n.get("tb" + key)
            if cid is not None:
                w.on_click(cid, lambda _id=None, k=key: self._fire(k))
        # 命令行：底层还没有键盘事件，先用"执行"按钮把输入框内容取出来派发
        if "btnRun" in n:
            w.on_click(n["btnRun"], lambda _id=None: self._run_input())
        # 功能区：原生 XRibbon 命中后把按钮 Name 直接回传 Python 派发
        if "cvRibbon" in n:
            w.on_click(n["cvRibbon"], self._on_ribbon)
        # 状态栏两个可点胶囊
        if "btnGrid" in n:
            w.on_click(n["btnGrid"], lambda _id=None: self._toggle_grid())
        if "btnMode" in n:
            w.on_click(n["btnMode"], lambda _id=None: self._toggle_point())
        # 特性面板编辑区：应用按钮 + 下拉即时写回
        if "propApply" in n:
            w.on_click(n["propApply"], lambda _id=None: self._apply_prop())
        if "propCombo" in n:
            w.on_select(n["propCombo"], lambda _id=None: self._apply_prop())
        if "propCombo" in n and "propEdit" in n:
            w.set_visible(n["propCombo"], False)
            w.set_visible(n["propEdit"], False)
            self._set_prop_visible(False, False)
        # 特性行：点键名进入编辑（颜色/图层/线型走下拉，几何值走输入框）
        for prefix, count in (("pr", self._PROP_SLOTS), ("ge", self._GEO_SLOTS)):
            for i in range(count):
                cid = n.get("%s%dk" % (prefix, i))
                if cid is not None:
                    w.on_click(cid, lambda _id=None, p=prefix, ix=i: self._on_row(p, ix))

    def _status(self):
        """兼容旧调用点：状态栏现在由原生控件刷新。"""
        self._refresh_status()

    def _new_doc(self):
        self.doc.clear()
        self.selection = []
        self.hist.clear()
        self.grip = None
        self.rubber = None
        self._show_props(None)
        self.redraw(fit=True)
        self._refresh_layers()
        self.info("新建图纸")

    def _toggle_grid(self):
        self.grid_on = not self.grid_on
        self.render()
        self._status()
        self.info("栅格 " + ("开" if self.grid_on else "关"))

    def _toggle_point(self):
        self.point_mode = not self.point_mode
        self._status()
        self.info("左键：定点/选择" if self.point_mode else "左键：平移")

    def _select_at(self, world):
        e = self.doc.hit(world, self.mapper.world_per_px() * 10)
        self.selection = [e] if e is not None else []
        self._show_props(e)
        self.redraw()
        if e is None:
            self.info("未命中图元")
        else:
            self.info("已选择 %s" % type(e).__name__)
        return e is not None

    # ---------------------------------------------------------- 夹点 / 框选
    def _grip_hit(self, world):
        tol = self.mapper.world_per_px() * 10
        for e in self.selection:
            for i, g in enumerate(e.grips()):
                if geom.dist_pt_pt(world, g) <= tol:
                    return (e, i)
        return None

    def _grip_drag(self, world):
        e, idx = self.grip
        e.set_grip(idx, world)
        now = time.time()
        if now - self._last_draw < 0.04:
            return
        self._last_draw = now
        self.render()

    def _grip_release(self):
        e = self.grip[0]
        self.hist.push_modify(e, self.grip_before)
        self.grip = None
        self.grip_before = None
        self._show_props(e)
        self.redraw()
        self.info("夹点修改完成")

    def _finish_rubber(self, world):
        p1, p2 = self.rubber
        self.rubber = None
        if geom.dist_pt_pt(p1, p2) < self.mapper.world_per_px() * 4:
            self.selection = []
            self._show_props(None)
            self.redraw()
            return
        crossing = world[0] < p1[0]      # 右→左为交叉选择（AutoCAD 语义）
        self.selection = self.doc.window_select(p1, p2, crossing=crossing)
        self._show_props(self.selection[0] if self.selection else None)
        self.redraw()
        self.info("框选 %d 个图元" % len(self.selection))

    def _zoom(self, act):
        if act == "fit":
            self.fit()
            return
        z = self.mapper.zoom
        z = z * 1.25 if act == "in" else (z / 1.25 if act == "out" else 1.0)
        self.win.set_dwg_view(self.names["cvMain"], *self._to_canvas_z(z, self.mapper.ox, self.mapper.oy))
        self._sync_view()
        self.render()

    def _zoom_step(self, k):
        self._zoom("in" if k > 1 else "out")

    def _new_layer(self):
        i = len(self.doc.layers.names())
        name = "图层%d" % i
        self.doc.layers.ensure(name)
        self.doc.current_layer = name
        self._refresh_layers()
        self.info("新建图层 %s" % name)

    def _refresh_layers(self):
        if "tblLayers" not in self.names:
            return
        rows = []
        for lay in self.doc.layers:
            rows.append("\t".join([
                lay.name, render.aci_color(lay.color), "%.1f" % lay.width,
                "是" if lay.visible else "否", "是" if lay.locked else "否"]))
        # 底层契约：行间 \n、格间 \t（见 docs/_API_CHANGES.md 第 5 条）
        self.win.set_table_rows(self.names["tblLayers"], "\n".join(rows))

    def set_selected_layer(self, name):
        self.doc.layers.ensure(name)
        for e in self.selection:
            e.layer = name
        self.redraw()
        self._refresh_layers()
        self._show_props(self.selection[0] if self.selection else None)

    def set_selected_color(self, idx):
        for e in self.selection:
            e.color = int(idx)
        self.redraw()
        self._show_props(self.selection[0] if self.selection else None)

    # ---------------------------------------------------------- 撤销 / 重做
    def _undo(self):
        op = self.hist.undo_one()
        if op is None:
            self.info("无可撤销")
            return
        self.selection = [e for e in self.selection if self.doc.by_id(e.id) is not None]
        self.redraw()
        self._refresh_layers()
        self.info("已撤销")

    def _redo(self):
        op = self.hist.redo_one()
        if op is None:
            self.info("无可重做")
            return
        self.selection = [e for e in self.selection if self.doc.by_id(e.id) is not None]
        self.redraw()
        self._refresh_layers()
        self.info("已重做")

    _KIND_NAME = {"line": "直线", "circle": "圆", "arc": "圆弧", "rect": "矩形",
                  "polyline": "多段线", "text": "文字", "insert": "块参照"}

    def _fmt(self, v):
        return "%.3f" % v

    def _geo_rows(self, e):
        k = e.kind
        if k == "line":
            return [("起点 X", self._fmt(e.x1), "x1"), ("起点 Y", self._fmt(e.y1), "y1"),
                    ("终点 X", self._fmt(e.x2), "x2"), ("终点 Y", self._fmt(e.y2), "y2"),
                    ("增量 X", self._fmt(e.x2 - e.x1), None),
                    ("增量 Y", self._fmt(e.y2 - e.y1), None),
                    ("长度", self._fmt(geom.dist_pt_pt(e.start, e.end)), None),
                    ("角度", "%.2f" % geom.angle_of(e.start, e.end), None)]
        if k == "circle":
            return [("圆心 X", self._fmt(e.cx), "cx"), ("圆心 Y", self._fmt(e.cy), "cy"),
                    ("半径", self._fmt(e.r), "r"), ("直径", self._fmt(2 * e.r), None),
                    ("周长", self._fmt(2 * math.pi * e.r), None)]
        if k == "arc":
            return [("圆心 X", self._fmt(e.cx), "cx"), ("圆心 Y", self._fmt(e.cy), "cy"),
                    ("半径", self._fmt(e.r), "r"), ("起始角", "%.2f" % e.a1, "a1"),
                    ("终止角", "%.2f" % e.a2, "a2"), ("夹角", "%.2f" % e.sweep, None),
                    ("弧长", self._fmt(e.r * e.sweep * math.pi / 180.0), None)]
        if k == "rect":
            return [("角点 X1", self._fmt(e.x1), "x1"), ("角点 Y1", self._fmt(e.y1), "y1"),
                    ("角点 X2", self._fmt(e.x2), "x2"), ("角点 Y2", self._fmt(e.y2), "y2"),
                    ("宽度", self._fmt(e.bbox()[2] - e.bbox()[0]), None),
                    ("高度", self._fmt(e.bbox()[3] - e.bbox()[1]), None),
                    ("周长", self._fmt(2 * ((e.bbox()[2] - e.bbox()[0]) +
                                            (e.bbox()[3] - e.bbox()[1]))), None)]
        if k == "polyline":
            pts = e.pts
            per = 0.0
            for i in range(len(pts) - 1):
                per += geom.dist_pt_pt(pts[i], pts[i + 1])
            if e.is_closed() and len(pts) > 2:
                per += geom.dist_pt_pt(pts[-1], pts[0])
            return [("顶点数", str(len(pts)), None),
                    ("闭合", "是" if e.is_closed() else "否", None),
                    ("周长", self._fmt(per), None)]
        if k == "text":
            return [("位置 X", self._fmt(e.x), "x"), ("位置 Y", self._fmt(e.y), "y"),
                    ("内容", e.text, "text"), ("字高", self._fmt(e.size), "size"),
                    ("旋转", "%.2f" % e.rot, "rot"), ("样式", e.style, None)]
        if k == "insert":
            return [("块名", e.name, None), ("插入点 X", self._fmt(e.x), "x"),
                    ("插入点 Y", self._fmt(e.y), "y"), ("X 比例", self._fmt(e.sx), "sx"),
                    ("Y 比例", self._fmt(e.sy), "sy"), ("旋转", "%.2f" % e.rot, "rot")]
        if k == "hatch":
            return [("边界顶点", str(len(e.pts)), None),
                    ("角度", "%.2f" % e.angle, None),
                    ("间距", self._fmt(e.gap), None)]
        if k == "dimension":
            return [("第一原点 X", self._fmt(e.p1[0]), None),
                    ("第一原点 Y", self._fmt(e.p1[1]), None),
                    ("第二原点 X", self._fmt(e.p2[0]), None),
                    ("第二原点 Y", self._fmt(e.p2[1]), None),
                    ("长度", self._fmt(e.length), None),
                    ("偏移", self._fmt(e.off), None)]
        return []

    # 颜色下拉：0=ByLayer，1..7=ACI 常用色
    _COLOR_NAMES = ["ByLayer", "1 红", "2 黄", "3 绿", "4 青", "5 蓝", "6 洋红", "7 白"]

    def _show_props(self, entity):
        if entity is None:
            self.prop_rows = [("常规", [("颜色", "—", None), ("图层", "—", None),
                                        ("线型", "—", None), ("线宽", "—", None)]),
                              ("几何图形", [("类型", "—", None)])]
        else:
            color = ("ACI %d" % entity.color) if entity.color else "ByLayer"
            general = [("颜色", color, "color"), ("图层", entity.layer, "layer"),
                       ("线型", entity.linetype or "Continuous", "linetype"),
                       ("线宽", self._fmt(entity.width) if entity.width else "ByLayer", "width")]
            geo = [("类型", self._KIND_NAME.get(entity.kind, entity.kind), None)]
            geo += self._geo_rows(entity)
            self.prop_rows = [("常规", general), ("几何图形", geo)]
        self._refresh_props()
        if entity is None:
            self.prop_field = None
            self._set_prop_visible(False, False)

    # ------------------------------------------------ 面板可编辑
    def _set_prop_visible(self, combo, edit):
        if "propCombo" in self.names:
            self.win.set_visible(self.names["propCombo"], combo)
        if "propEdit" in self.names:
            self.win.set_visible(self.names["propEdit"], edit)
        # 没进入编辑时不摆"应用"按钮：它在 SVG 时代是个常驻空块，看着像掉了控件
        if "propApply" in self.names:
            self.win.set_visible(self.names["propApply"], bool(combo or edit))

    def _fill_combo(self, items, sel_index=-1):
        cid = self.names["propCombo"]
        self.win.clear_strings(cid)
        for it in items:
            self.win.add_string(cid, it)
        if sel_index >= 0:
            self.win.set_cur_sel(cid, sel_index)

    def _edit_prop(self, field):
        e = self.selection[0] if self.selection else None
        if e is None:
            return
        self.prop_field = field
        if field == "layer":
            names = self.doc.layers.names()
            idx = names.index(e.layer) if e.layer in names else -1
            self._fill_combo(names, idx)
            self._set_prop_visible(True, False)
        elif field == "color":
            self._fill_combo(self._COLOR_NAMES, max(0, e.color or 0))
            self._set_prop_visible(True, False)
        elif field == "linetype":
            names = self.doc.linetypes.names()
            cur = e.linetype or "Continuous"
            idx = names.index(cur) if cur in names else -1
            self._fill_combo(names, idx)
            self._set_prop_visible(True, False)
        else:
            raw = getattr(e, field, "")
            if "propEdit" in self.names:
                self.win.set_text(self.names["propEdit"], "" if raw is None else str(raw))
            self._set_prop_visible(False, True)

    def _apply_prop(self):
        e = self.selection[0] if self.selection else None
        if e is None or not self.prop_field:
            return
        f = self.prop_field
        before = e.snapshot()
        if f == "layer":
            cid = self.names["propCombo"]
            idx = self.win.get_cur_sel(cid)
            if idx >= 0:
                e.layer = self.win.get_string(cid, idx)
        elif f == "color":
            cid = self.names["propCombo"]
            e.color = max(0, self.win.get_cur_sel(cid))
        elif f == "linetype":
            cid = self.names["propCombo"]
            idx = self.win.get_cur_sel(cid)
            if idx >= 0:
                e.linetype = self.win.get_string(cid, idx)
        else:
            raw = self.win.get_text(self.names["propEdit"])
            if f == "text":
                e.text = raw
            else:
                try:
                    setattr(e, f, float(raw))
                except ValueError:
                    self.info("数值无效：%s" % raw)
                    return
        self.hist.push_modify(e, before)
        self.redraw()
        self._refresh_layers()
        self._show_props(e)
        self.info("已修改属性 %s" % f)

    def save(self):
        self.save_json()
        self._bring_front()
        self.win.save_png(SHOT)
        self._unfront()
        self.info("已保存图纸 %s 并截图" % os.path.basename(DRAWING))

    def save_json(self, path=DRAWING):
        with open(path, "w", encoding="utf-8") as f:
            json.dump(self.doc.to_dict(), f, ensure_ascii=False, indent=1)
        self.info("已存盘 %s" % os.path.basename(path))

    def load_json(self, path=DRAWING):
        with open(path, "r", encoding="utf-8") as f:
            data = json.load(f)
        self.doc.load_dict(data)
        self.selection = []
        self.hist.clear()
        self.redraw(fit=True)
        self._refresh_layers()
        self.info("已读入 %s：图元 %d" % (os.path.basename(path), self.doc.count()))

    def export_svg(self):
        path = os.path.join(HERE, "cad_export.svg")
        with open(path, "w", encoding="utf-8") as f:
            f.write(self.doc.to_svg())
        self.info("已导出 %s" % os.path.basename(path))

    def _bring_front(self):
        try:
            hw = int(self.win.hwnd)
            user32 = ctypes.windll.user32
            user32.ShowWindow(hw, 9)                # SW_RESTORE
            # 后台进程被前台锁拦住时，AttachThreadInput 到前台线程后再抢前台
            cur = ctypes.windll.kernel32.GetCurrentThreadId()
            fg = user32.GetForegroundWindow()
            fgt = user32.GetWindowThreadProcessId(fg, None)
            if fgt and fgt != cur:
                user32.AttachThreadInput(fgt, cur, True)
            user32.SetWindowPos(hw, -1, 0, 0, 0, 0, 0x043)  # TOPMOST|NOMOVE|NOSIZE|SHOW
            user32.BringWindowToTop(hw)
            user32.SetForegroundWindow(hw)
            if fgt and fgt != cur:
                user32.AttachThreadInput(fgt, cur, False)
            p.pump(200)
        except Exception:
            pass

    def _unfront(self):
        try:
            hw = int(self.win.hwnd)
            ctypes.windll.user32.SetWindowPos(hw, -2, 0, 0, 0, 0, 0x043)  # NOTOPMOST
            p.pump(100)
        except Exception:
            pass

    def run(self):
        while p.pump(30):
            pass


def main():
    docx = None
    if "--scan" in sys.argv:
        # 直接吃一份 PyArx 扫描结果（scan.json），跳过取数
        i = sys.argv.index("--scan")
        json_path = sys.argv[i + 1] if i + 1 < len(sys.argv) else ""
        import scan_load
        docx = scan_load.load_scan(json_path)
    if "--dwg" in sys.argv:
        i = sys.argv.index("--dwg")
        dwg_path = sys.argv[i + 1] if i + 1 < len(sys.argv) else ""
        import dwg_load
        try:
            docx = dwg_load.load_dwg(dwg_path)
        except Exception as ex:
            # PyArxDwg 只解老版本/未加密 section 的 DWG；新版图纸撞 AcDb:FileDepList
            # 加密时退回 PyArx 无头扫描（accoreconsole + PyArxEngine.crx + scan.py）。
            print("PyArxDwg 解码失败(%s)，改用 PyArx 无头扫描：%s"
                  % (type(ex).__name__, str(ex)[:120]))
            import scan_load
            docx = scan_load.scan_dwg(dwg_path)
    app = App(docx)
    if "--selftest" in sys.argv:
        selftest(app)
        return
    if "--batch" in sys.argv:
        rows = batch_test(app)
        _print_batch(rows)
        app._bring_front()
        app.win.save_png(os.path.join(HERE, "cad2d_batch.png"))
        app._unfront()
        return
    app.info("就绪 · 滚轮缩放 · 中键平移 · 命令 LINE/CIRCLE/RECT/ARC/PLINE/ERASE")
    app.run()


def selftest(app):
    """端到端自检：走完 命令→画布定点→提交→重画→截图 这一整条链。"""
    before = app.doc.count()
    app.run_cmd("LINE")
    assert app.cmds.active is not None, "命令没起来"
    hit = app._on_canvas("down", 300, 300, 0)
    assert hit is True, "命令激活时左键必须被吃掉（否则会变平移）"
    app._on_canvas("down", 500, 420, 0)
    after = app.doc.count()
    assert after >= before + 1, "画线没提交：%d → %d" % (before, after)

    app.run_cmd("CIRCLE")
    app._on_canvas("down", 620, 500, 0)
    app._on_canvas("down", 700, 500, 0)
    assert app.doc.count() >= after + 1, "圆没提交"

    app.run_cmd("RECT")
    app._on_canvas("down", 200, 150, 0)
    app._on_canvas("down", 320, 240, 0)

    # 无命令时左键要交还给宿主（保留平移）
    app.cmds.cancel()
    app.point_mode = False
    assert app._on_canvas("down", 400, 400, 0) is False, "无命令时必须放行给平移"
    app.point_mode = True

    # 命令行入口：输入 → 执行按钮 → 派发
    if "txtCmd" in app.names:
        app.win.set_text(app.names["txtCmd"], "ZE")
        app._run_input()
        app.win.set_text(app.names["txtCmd"], "L")
        app._run_input()
        assert app.cmds.active is not None, "命令行没能起命令"
        app.cmds.cancel()
        app.win.set_text(app.names["txtCmd"], "多段线")
        app._run_input()
        assert app.cmds.active is not None, "中文命令名没认出来"
        app.cmds.cancel()

    # 工具条：14 个原生 XIconButton 的派发口（真实点击见 tests/test_ui_shell.py）
    app.cmds.cancel()
    app._fire("LINE")
    assert app.cmds.active is not None, "工具条动作派发没起 LINE"
    app.cmds.cancel()

    # 功能区：原生 XRibbon 命中后回传按钮 Name，直接派发 ERASE 不应崩
    app.cmds.cancel()
    app._on_ribbon("ERASE")
    app.cmds.cancel()

    # 块：命令行指定块名成块 → 带比例/角度插入块参照 → 撤销/重做块定义 → 存盘往返保留
    app.cmds.cancel()
    app.selection = [e for e in app.doc.entities if e.kind == "line"][:1]
    assert app.selection, "没有可选直线"
    app.run_cmd("BLOCK", "门")
    app._on_canvas("down", 300, 300, 0)    # 选择（已带选择集）
    app._on_canvas("down", 200, 200, 0)     # 基点
    assert "门" in app.doc.blocks.names(), "指定块名没写入块表"
    app.run_cmd("INSERT", "门 0.5 30")
    app._on_canvas("down", 600, 300, 0)
    ins = [e for e in app.doc.entities if e.kind == "insert"]
    assert ins, "块参照没插入"
    assert abs(ins[0].sx - 0.5) < 1e-9 and abs(ins[0].rot - 30.0) < 1e-9, "比例/角度没带上"

    # 撤销：先撤块参照，再撤块定义；重做还原
    app._undo()
    app._undo()
    assert not app.doc.blocks.names(), "撤销后块定义没移除"
    app._redo()
    app._redo()
    assert "门" in app.doc.blocks.names(), "重做后块定义没还原"

    d = app.doc.to_dict()
    app.doc.load_dict(d)
    assert app.doc.blocks.names(), "存盘往返块表丢失"

    # 左键模式：定点模式吃掉点击（可选中），平移模式交还宿主
    app.point_mode = True
    app.cmds.cancel()
    app._on_canvas("down", 400, 400, 0)
    app.point_mode = False
    assert app._on_canvas("down", 400, 400, 0) is False, "平移模式下必须放行"
    app.point_mode = True

    # 选中一条直线，特性面板出几何明细（供截图核验）
    line = next((e for e in app.doc.entities if e.kind == "line"), None)
    if line is not None:
        app.selection = [line]
        app._show_props(line)

    # 特性面板可编辑：改图层下拉 → 写回 → 撤销还原
    if line is not None:
        names = app.doc.layers.names()
        door = next((n for n in names if n != line.layer), None)
        if door is not None:
            app._edit_prop("layer")
            app.win.set_cur_sel(app.names["propCombo"], names.index(door))
            app._apply_prop()
            assert line.layer == door, "面板改图层没写回"
            app._undo()
            assert line.layer != door, "面板改层撤销未还原"

    # 几何数值：输入框改起点 X → 写回 → 撤销
    app._edit_prop("x1")
    app.win.set_text(app.names["propEdit"], "1234.5")
    app._apply_prop()
    assert abs(line.x1 - 1234.5) < 1e-6, "面板几何数值没写回"
    app._undo()
    assert abs(line.x1 - 1000.0) < 1e-6, "几何数值撤销未还原"

    # 颜色下拉：选色 → 写回 → 撤销
    app._edit_prop("color")
    app.win.set_cur_sel(app.names["propCombo"], 3)
    app._apply_prop()
    assert line.color == 3, "面板颜色没写回"
    app._undo()
    assert line.color == 0, "颜色撤销未还原"

    # 线型下拉：选 Dashed → 写回 → 撤销
    app._edit_prop("linetype")
    app.win.set_cur_sel(app.names["propCombo"], 1)
    app._apply_prop()
    assert line.linetype == "Dashed", "面板线型没写回"
    app._undo()

    # 线宽输入框：2.5mm → 写回 → 撤销
    app._edit_prop("width")
    app.win.set_text(app.names["propEdit"], "2.5")
    app._apply_prop()
    assert abs(line.width - 2.5) < 1e-6, "面板线宽没写回"
    app._undo()

    app.redraw()
    p.pump(300)
    app.save()
    print("[selftest] OK 图元 %d · 缩放 %.0f%%" % (app.doc.count(), app.mapper.zoom * 100))
    p.pump(200)


def _close(a, b, tol=1.0):
    return abs(float(a) - float(b)) <= tol


def _geom_close(d1, d2, tol=1.0):
    for k, v in d1.items():
        if k in ("layer", "color", "width", "linetype", "kind", "closed",
                 "style", "rot", "text", "name", "sx", "sy", "off"):
            if v != d2.get(k):
                return False
        elif isinstance(v, list):
            for p, q in zip(v, d2.get(k, [])):
                if not (_close(p[0], q[0], tol) and _close(p[1], q[1], tol)):
                    return False
        elif isinstance(v, (int, float)):
            if not _close(v, d2.get(k), tol):
                return False
    return True


def batch_test(app):
    """真实宿主会话内的图元增改批测：走画布事件与命令状态机，不 mock。"""
    rows = []

    def rec(case, expected, actual, ok, root=""):
        rows.append({"case": case, "expected": expected, "actual": actual,
                     "pass": bool(ok), "root": "" if ok else root})

    def wc(world, phase="down"):
        sx, sy = app.mapper.world_to_screen(world)
        return app._on_canvas(phase, sx, sy, 0)

    # 清空成干净图纸，视图适配
    app.doc.clear()
    app.selection = []
    app.hist.clear()
    app.redraw(fit=True)
    p.pump(60)

    # 1) 直线
    app.run_cmd("LINE")
    wc((1000, 1000)); wc((3000, 1200))
    e = app.doc.entities[-1]
    ok = (e.kind == "line" and _close(e.x1, 1000) and _close(e.y1, 1000)
          and _close(e.x2, 3000) and _close(e.y2, 1200))
    rec("直线绘制", "line (1000,1000)-(3000,1200)",
        "%s (%.0f,%.0f)-(%.0f,%.0f)" % (e.kind, e.x1, e.y1, e.x2, e.y2), ok,
        "端点与命令提交不符")

    # 2) 圆
    app.run_cmd("CIRCLE")
    wc((5000, 2000)); wc((5800, 2000))
    e = app.doc.entities[-1]
    ok = e.kind == "circle" and _close(e.cx, 5000) and _close(e.cy, 2000) and _close(e.r, 800)
    rec("圆绘制", "circle 圆心(5000,2000) r=800",
        "%s (%.0f,%.0f) r=%.1f" % (e.kind, e.cx, e.cy, e.r), ok,
        "圆心/半径未按两点落位")

    # 3) 弧
    app.run_cmd("ARC")
    wc((7000, 1000)); wc((7500, 1800)); wc((8000, 1000))
    e = app.doc.entities[-1]
    ok = e.kind == "arc" and e.r > 1e-6
    rec("圆弧绘制", "arc 三点成弧，半径>0",
        "%s 圆心(%.0f,%.0f) r=%.1f" % (e.kind, e.cx, e.cy, e.r), ok,
        "三点定圆失败或半径为 0")

    # 4) 矩形
    app.run_cmd("RECT")
    wc((1000, 3000)); wc((2500, 4000))
    e = app.doc.entities[-1]
    b = e.bbox()
    ok = e.kind == "rect" and _close(b[0], 1000) and _close(b[1], 3000) \
        and _close(b[2], 2500) and _close(b[3], 4000)
    rec("矩形绘制", "rect 对角 (1000,3000)-(2500,4000)",
        "%s bbox=(%.0f,%.0f,%.0f,%.0f)" % (e.kind, b[0], b[1], b[2], b[3]), ok,
        "对角点未落位")

    # 5) 多段线
    app.run_cmd("PLINE")
    wc((3000, 3000)); wc((3500, 3500)); wc((4000, 3000))
    app.cmds.finish_poly()
    e = app.doc.entities[-1]
    ok = e.kind == "polyline" and len(e.pts) == 3
    rec("多段线绘制", "polyline 3 个顶点",
        "%s 顶点数=%d" % (e.kind, len(e.pts)), ok,
        "多段线收尾未提交或顶点丢失")

    # 6) 夹点拖动（直线终点夹点）
    line = app.doc.entities[0]
    app._select_at(((line.x1 + line.x2) / 2.0, (line.y1 + line.y2) / 2.0))
    g = line.grips()[2]
    wc(g, "down"); wc((3200, 1600), "move"); wc((3200, 1600), "up")
    ok = _close(line.x2, 3200) and _close(line.y2, 1600)
    rec("夹点拖动", "直线终点移到 (3200,1600)",
        "终点=(%.1f,%.1f)" % (line.x2, line.y2), ok,
        "夹点命中或 set_grip 未改几何")

    # 7) 移动
    base = (line.x1, line.y1)
    target = (base[0] + 500, base[1] + 300)
    app.run_cmd("MOVE")
    wc(base); wc(target)
    ok = _close(line.x1, base[0] + 500) and _close(line.y1, base[1] + 300)
    rec("移动 MOVE", "整体偏移 (+500,+300)",
        "起点=(%.1f,%.1f)" % (line.x1, line.y1), ok,
        "基点→目标点偏移未作用到选择集")

    # 8) 旋转（直线绕起点相对旋转 90°，长度不变）
    start = (line.x1, line.y1)
    old_end = (line.x2, line.y2)
    old_ang = geom.angle_of(start, old_end)
    before_len = geom.dist_pt_pt(start, old_end)
    app.run_cmd("ROTATE")
    wc(start); wc((start[0], start[1] + 500))
    after_len = geom.dist_pt_pt(start, (line.x2, line.y2))
    new_ang = geom.angle_of(start, (line.x2, line.y2))
    delta = geom.norm_deg(new_ang - old_ang)
    ok = abs(delta - 90.0) < 3.0 and _close(after_len, before_len, 2.0)
    rec("旋转 ROTATE", "绕起点相对旋转 90°，长度不变",
        "角度 %.1f→%.1f°（Δ%.1f°）长度 %.1f→%.1f"
        % (old_ang, new_ang, delta, before_len, after_len),
        ok, "旋转角未按基点-第二点方向作用")

    # 9) 缩放（圆 r 800 → 1200）
    circle = [x for x in app.doc.entities if x.kind == "circle"][0]
    app._select_at((circle.cx + circle.r, circle.cy))
    app.run_cmd("SCALE")
    wc(circle.center)
    wc((circle.cx + 800, circle.cy))
    wc((circle.cx + 1200, circle.cy))
    ok = _close(circle.r, 1200, 2.0)
    rec("缩放 SCALE", "圆半径 800 → 1200",
        "r=%.1f" % circle.r, ok, "参照长度比例未作用到半径")

    # 10) 删除
    app._select_at((circle.cx + circle.r, circle.cy))
    before = app.doc.count()
    app.run_cmd("ERASE")
    wc(circle.center)
    gone = all(x.id != circle.id for x in app.doc.entities)
    ok = gone and app.doc.count() == before - 1
    rec("删除 ERASE", "圆被移除，图元数 -1",
        "剩余 %d 个，圆存在=%s" % (app.doc.count(), not gone), ok,
        "选择集未传入 ERASE 或 remove_many 未生效")

    # 11) 改图层
    app.doc.layers.ensure("TESTLAYER")
    app._select_at(((line.x1 + line.x2) / 2.0, (line.y1 + line.y2) / 2.0))
    app.set_selected_layer("TESTLAYER")
    ok = line.layer == "TESTLAYER"
    rec("改图层", "直线图层 = TESTLAYER", "layer=%s" % line.layer, ok,
        "set_selected_layer 未写回实体")

    # 12) 改颜色
    app.set_selected_color(3)
    ok = line.color == 3
    rec("改颜色", "直线颜色索引 = 3", "color=%s" % line.color, ok,
        "set_selected_color 未写回实体")

    # 13) 窗口框选（右下→左上交叉选择）
    n_before = len(app.selection)
    wc((4500, 4500), "down"); wc((500, 2500), "move"); wc((500, 2500), "up")
    ok = len(app.selection) >= 1
    rec("窗口选择", "框内至少选中 1 个图元", "选中 %d 个" % len(app.selection),
        ok, "橡皮框结束后 window_select 未返回图元")

    # 14) 存盘 → 清空 → 读盘往返
    tmp = os.path.join(HERE, "_batch_roundtrip.json")
    saved = [(e.kind, e.layer, e.color, e.to_dict()) for e in app.doc.entities]
    app.save_json(tmp)
    app.doc.clear()
    app.selection = []
    app.hist.clear()
    app.load_json(tmp)
    loaded = [(e.kind, e.layer, e.color, e.to_dict()) for e in app.doc.entities]
    ok = len(saved) == len(loaded)
    detail = "存 %d / 读 %d" % (len(saved), len(loaded))
    if ok:
        for s, l in zip(saved, loaded):
            if s[0] != l[0] or s[1] != l[1] or s[2] != l[2] or not _geom_close(s[3], l[3]):
                ok = False
                detail += "；不一致: %s vs %s" % (s[0], l[0])
                break
    rec("存盘读盘往返", "数量/类型/图层/颜色/几何一致", detail, ok,
        "序列化字段缺失或读盘重建参数漂移")

    # 15) 线性标注：三点生成 Dimension 实体
    app.run_cmd("DIM")
    wc((1000, 1000)); wc((3000, 1200)); wc((2000, 1600))
    dim = next((e for e in app.doc.entities if e.kind == "dimension"), None)
    expect_len = geom.dist_pt_pt((1000, 1000), (3000, 1200))
    ok = dim is not None and _close(dim.length, expect_len, 2.0)
    rec("线性标注 DIM", "三点线性标注实体",
        "dimension=%s，长度=%.1f" % (dim is not None, dim.length if dim else 0),
        ok, "DIM 命令状态机未生成 Dimension")

    # 16) 区域填充：点选矩形边界，生成剖面线 Hatch
    rect = next((x for x in app.doc.entities if x.kind == "rect"), None)
    if rect is not None:
        b = rect.bbox()
        app.run_cmd("HATCH")
        wc(((b[0] + b[2]) / 2.0, b[3]))
        h = next((x for x in app.doc.entities if x.kind == "hatch"), None)
        ok = h is not None and len(h.pts) == 4
        rec("区域填充 HATCH", "闭合边界剖面填充",
            "hatch=%s，顶点=%d" % (h is not None, len(h.pts) if h else 0),
            ok, "HATCH 未生成剖面填充")
    else:
        rec("区域填充 HATCH", "闭合边界剖面填充", "无矩形边界", False,
            "前置矩形实体缺失")

    # 17) 文字：单点 + 命令参数 → Text 实体（工具条/Ribbon 的 TEXT 按钮同一入口）
    app.run_cmd("TEXT", "门厅说明")
    wc((2000, 5000))
    t = next((e for e in app.doc.entities if e.kind == "text" and e.text == "门厅说明"), None)
    ok = t is not None and _close(t.x, 2000) and _close(t.y, 5000)
    rec("文字 TEXT", "单点生成 text，内容取自命令参数",
        "text=%s，位置=(%.0f,%.0f)" % (t.text if t else None, t.x if t else 0,
                                    t.y if t else 0),
        ok, "TEXT 命令未落位或内容丢失")

    try:
        os.remove(tmp)
    except OSError:
        pass
    return rows


def _print_batch(rows):
    passed = sum(1 for r in rows if r["pass"])
    failed = sum(1 for r in rows if not r["pass"] and "未实现" not in r["actual"])
    pending = sum(1 for r in rows if "未实现" in r["actual"])
    print("[batch] 用例 %d · 通过 %d · 失败 %d · 未实现 %d"
          % (len(rows), passed, failed, pending))
    for r in rows:
        flag = "PASS" if r["pass"] else ("TODO" if "未实现" in r["actual"] else "FAIL")
        print("[batch] %s | %s | 预期:%s | 实际:%s | %s"
              % (flag, r["case"], r["expected"], r["actual"], r["root"]))


if __name__ == "__main__":
    main()
