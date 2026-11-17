# -*- coding: utf-8 -*-
"""interact —— 交互核心（模块 6）

三件事：
  ViewMapper   屏幕像素 ↔ 图纸 mm（含 y 翻转，和 render 用同一套换算）
  Snapper      捕捉：端点 / 中点 / 圆心 / 象限点 / 最近点，按像素容差找
  Command*     命令状态机：点了第几个点、下一步要什么、预览画什么、怎么结束

设计约定：
  · 命令只负责"攒够点 → 造实体"，不碰界面；提示文字通过 prompt 交给上层显示
  · 每个命令都可以 cancel()，ESC / 右键都走它
  · 预览返回一个临时实体（或 None），由上层决定怎么画 —— 命令不直接改文档
"""
import math

import ent
import geom

DEFAULT_TOL_PX = 9.0


# ---------------------------------------------------------------- 坐标换算
class ViewMapper(object):
    def __init__(self, view, unit):
        self.set_view(view, 1.0, 0, 0)
        self.u = float(unit)

    def set_view(self, view, zoom, ox, oy):
        self.x0, self.y0, self.x1, self.y1 = [float(v) for v in view]
        self.zoom = float(zoom) or 1.0
        self.ox = float(ox)
        self.oy = float(oy)

    def screen_to_world(self, sx, sy):
        vx = (float(sx) - self.ox) / self.zoom
        vy = (float(sy) - self.oy) / self.zoom
        return (self.x0 + vx / self.u, self.y1 - vy / self.u)

    def world_to_screen(self, p):
        vx = (p[0] - self.x0) * self.u
        vy = (self.y1 - p[1]) * self.u
        return (vx * self.zoom + self.ox, vy * self.zoom + self.oy)

    def world_per_px(self):
        """1 像素等于多少毫米 —— 命中与捕捉的容差都按它换。"""
        return 1.0 / (self.zoom * self.u)


# ---------------------------------------------------------------- 捕捉
class Snap(object):
    """一次捕捉结果：点子 + 类型标签 + 参照实体。"""

    def __init__(self, point, kind, entity=None):
        self.point = point
        self.kind = kind
        self.entity = entity

    def __repr__(self):
        return "<Snap %s (%.2f, %.2f)>" % (self.kind, self.point[0], self.point[1])


class Snapper(object):
    """AutoCAD 的对象捕捉：端点 > 中点 > 圆心 > 象限 > 最近点（优先级从高到低）。"""

    ORDER = {"endpoint": 0, "center": 1, "midpoint": 2, "quadrant": 3, "nearest": 4}

    def __init__(self, doc, enabled=True):
        self.doc = doc
        self.enabled = enabled
        self.kinds = ("endpoint", "midpoint", "center", "quadrant", "nearest")

    def candidates(self, e):
        pts = []
        kind = e.kind
        if kind == "line":
            pts += [(e.start, "endpoint"), (e.end, "endpoint"),
                    (((e.x1 + e.x2) / 2.0, (e.y1 + e.y2) / 2.0), "midpoint")]
        elif kind == "circle":
            pts.append((e.center, "center"))
            for a in (0, 90, 180, 270):
                pts.append((geom.polar(e.center, a, e.r), "quadrant"))
        elif kind == "arc":
            pts.append((e.center, "center"))
            for a in (e.a1, e.a2):
                pts.append((geom.polar(e.center, a, e.r), "endpoint"))
        elif kind in ("rect", "polyline"):
            ps = e.points()
            for p in ps[:-1] if e.is_closed() else ps:
                pts.append((p, "endpoint"))
            for a, b in zip(ps, ps[1:]):
                pts.append((((a[0] + b[0]) / 2.0, (a[1] + b[1]) / 2.0), "midpoint"))
        elif kind == "text":
            pts.append(((e.x, e.y), "endpoint"))
        return pts

    def snap(self, p, tol_world):
        if not self.enabled:
            return None
        best = None
        best_d = tol_world
        for e in self.doc.visible_entities():
            for pt, kind in self.candidates(e):
                d = geom.dist_pt_pt(p, pt)
                if d <= best_d:
                    if best is not None and d == best_d:
                        if self.ORDER[kind] >= self.ORDER[best.kind]:
                            continue
                    best = Snap(pt, kind, e)
                    best_d = d
        # 都没命中几何特征就退到"最近点"：把光标投到最近的实体上
        if best is None and "nearest" in self.kinds:
            for e in self.doc.visible_entities():
                if e.hit(p, tol_world):
                    n = self._nearest_on(e, p)
                    if n is not None:
                        best = Snap(n, "nearest", e)
                        break
        return best

    def _nearest_on(self, e, p):
        kind = e.kind
        if kind == "line":
            return _proj_seg(p, e.start, e.end)
        if kind == "circle":
            d = geom.dist_pt_pt(p, e.center)
            return geom.polar(e.center, geom.angle_of(e.center, p), e.r) if d > 1e-9 else None
        if kind in ("rect", "polyline"):
            ps = e.points()
            best, bd = None, float("inf")
            for a, b in zip(ps, ps[1:]):
                q = _proj_seg(p, a, b)
                dd = geom.dist_pt_pt(p, q)
                if dd < bd:
                    best, bd = q, dd
            return best
        return None


def _proj_seg(p, a, b):
    dx, dy = b[0] - a[0], b[1] - a[1]
    if abs(dx) < 1e-12 and abs(dy) < 1e-12:
        return a
    t = ((p[0] - a[0]) * dx + (p[1] - a[1]) * dy) / float(dx * dx + dy * dy)
    t = max(0.0, min(1.0, t))
    return (a[0] + t * dx, a[1] + t * dy)


# ---------------------------------------------------------------- 命令状态机
class Command(object):
    name = ""
    alias = ()
    max_points = 2

    def __init__(self):
        self.points = []
        self.done = False
        self.args = ""

    @property
    def prompt(self):
        return "指定点"

    def begin(self, ctx):
        self.points = []
        self.done = False

    def accept(self, ctx, p):
        """收一个点。返回 (是否继续, 要提交的实体 or None)。"""
        self.points.append(p)
        if len(self.points) >= self.max_points:
            return False, self.make(ctx)
        return True, None

    def make(self, ctx):
        return None

    def preview(self, ctx, p):
        return None

    def cancel(self, ctx):
        self.done = True


class LineCmd(Command):
    """连续画线：点一次起点，之后每点一次连一段，回车/ESC 结束。"""
    name = "LINE"
    alias = ("L", "LINE", "直线")

    def __init__(self):
        Command.__init__(self)
        self.max_points = 2

    @property
    def prompt(self):
        if not self.points:
            return "指定第一个点"
        return "指定下一点 [Enter 结束]"

    def accept(self, ctx, p):
        if not self.points:
            self.points.append(p)
            return True, None
        a = self.points[-1]
        if geom.dist_pt_pt(a, p) < 1e-9:
            return True, None          # 原地双击，忽略
        self.points.append(p)
        e = ent.Line(a[0], a[1], p[0], p[1], layer=ctx.layer, color=ctx.color)
        self.points = [p]              # 连续画：上一个终点就是下一个起点
        return True, e

    def preview(self, ctx, p):
        if not self.points:
            return None
        a = self.points[-1]
        return ent.Line(a[0], a[1], p[0], p[1], layer=ctx.layer, color=ctx.color)


class CircleCmd(Command):
    name = "CIRCLE"
    alias = ("C", "CIRCLE", "圆")
    max_points = 2

    @property
    def prompt(self):
        return "指定圆心" if not self.points else "指定半径点"

    def accept(self, ctx, p):
        self.points.append(p)
        if len(self.points) == 2:
            c, q = self.points
            r = geom.dist_pt_pt(c, q)
            if r < 1e-9:
                self.points = [c]
                return True, None
            return False, ent.Circle(c[0], c[1], r, layer=ctx.layer, color=ctx.color)
        return True, None

    def preview(self, ctx, p):
        if not self.points:
            return None
        c = self.points[0]
        r = geom.dist_pt_pt(c, p)
        return ent.Circle(c[0], c[1], r, layer=ctx.layer, color=ctx.color)


class RectCmd(Command):
    name = "RECT"
    alias = ("REC", "RECT", "RECTANG", "矩形")
    max_points = 2

    @property
    def prompt(self):
        return "指定第一个角点" if not self.points else "指定对角点"

    def accept(self, ctx, p):
        self.points.append(p)
        if len(self.points) == 2:
            a, b = self.points
            return False, ent.Rect(a[0], a[1], b[0], b[1],
                                   layer=ctx.layer, color=ctx.color)
        return True, None

    def preview(self, ctx, p):
        if not self.points:
            return None
        a = self.points[0]
        return ent.Rect(a[0], a[1], p[0], p[1], layer=ctx.layer, color=ctx.color)


class ArcCmd(Command):
    """三点圆弧：起点 → 圆弧上一点 → 终点。"""
    name = "ARC"
    alias = ("A", "ARC", "圆弧")
    max_points = 3

    @property
    def prompt(self):
        n = len(self.points)
        return ("指定起点", "指定弧上一点", "指定端点")[min(n, 2)]

    def accept(self, ctx, p):
        self.points.append(p)
        if len(self.points) < 3:
            return True, None
        e = _arc_from_three(self.points, ctx)
        return False, e

    def preview(self, ctx, p):
        if len(self.points) < 2:
            return None
        return _arc_from_three(self.points + [p], ctx)


def _arc_from_three(pts, ctx):
    """三点定圆：外接圆心 + 起止角，方向按 p1→p3 逆时针补正。"""
    (x1, y1), (x2, y2), (x3, y3) = pts
    d = 2.0 * (x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2))
    if abs(d) < 1e-9:
        return None
    ux = ((x1 * x1 + y1 * y1) * (y2 - y3) + (x2 * x2 + y2 * y2) * (y3 - y1)
          + (x3 * x3 + y3 * y3) * (y1 - y2)) / d
    uy = ((x1 * x1 + y1 * y1) * (x3 - x2) + (x2 * x2 + y2 * y2) * (x1 - x3)
          + (x3 * x3 + y3 * y3) * (x2 - x1)) / d
    r = geom.dist_pt_pt((ux, uy), (x1, y1))
    a1 = geom.angle_of((ux, uy), (x1, y1))
    a2 = geom.angle_of((ux, uy), (x3, y3))
    # 中间点不在 a1→a2 逆时针区间里，就把首尾掉个个
    a3 = geom.angle_of((ux, uy), (x2, y2))
    if geom.norm_deg(a3 - a1) > geom.arc_sweep(a1, a2):
        a1, a2 = a2, a1
    return ent.Arc(ux, uy, r, a1, a2, layer=ctx.layer, color=ctx.color)


class PlineCmd(Command):
    """多段线：一直攒点，回车/双击时把攒下的点连成一条 Polyline。"""
    name = "PLINE"
    alias = ("PL", "PLINE", "多段线")

    def begin(self, ctx):
        Command.begin(self, ctx)
        self.pts_all = []

    @property
    def prompt(self):
        return "指定起点" if not self.pts_all else "指定下一点 [Enter 结束]"

    def accept(self, ctx, p):
        if self.pts_all and geom.dist_pt_pt(self.pts_all[-1], p) < 1e-9:
            return True, None
        self.pts_all.append(p)
        self.points = [p]
        return True, None

    def preview(self, ctx, p):
        if not self.pts_all:
            return None
        a = self.pts_all[-1]
        return ent.Line(a[0], a[1], p[0], p[1], layer=ctx.layer, color=ctx.color)

    def finish(self, ctx):
        if len(self.pts_all) < 2:
            return None
        pl = ent.Polyline(self.pts_all, closed=False, layer=ctx.layer, color=ctx.color)
        self.pts_all = []
        return pl


class EraseCmd(Command):
    name = "ERASE"
    alias = ("E", "ERASE", "DEL", "删除")
    max_points = 1

    @property
    def prompt(self):
        return "选择要删除的对象"

    def accept(self, ctx, p):
        tol = ctx.tol_world()
        sel = list(ctx.selection)
        if not sel:
            e = self.doc_ctx.hit(p, tol)
            sel = [e] if e is not None else []
        self.done = True
        return False, ("erase", sel) if sel else None

    def bind(self, doc):
        self.doc_ctx = doc


class _ModifyCmd(Command):
    """修改类命令：作用于当前选择集，提交前抓快照供撤销。"""

    max_points = 2

    def begin(self, ctx):
        Command.begin(self, ctx)
        self.sel = []

    def _ensure_sel(self, ctx):
        if not self.sel:
            self.sel = list(ctx.selection)
        return self.sel

    def _before(self):
        return [e.snapshot() for e in self.sel]


class MoveCmd(_ModifyCmd):
    name = "MOVE"
    alias = ("M", "MOVE", "移动")

    @property
    def prompt(self):
        return "指定基点" if not self.points else "指定目标点"

    def accept(self, ctx, p):
        self._ensure_sel(ctx)
        self.points.append(p)
        if len(self.points) < 2:
            return True, None
        base, target = self.points
        before = self._before()
        dx, dy = target[0] - base[0], target[1] - base[1]
        for e in self.sel:
            e.move_by(dx, dy)
        return False, ("modify", {"kind": "move", "entities": self.sel, "before": before})


class CopyCmd(_ModifyCmd):
    name = "COPY"
    alias = ("CO", "COPY", "复制")

    @property
    def prompt(self):
        return "指定基点" if not self.points else "指定目标点"

    def accept(self, ctx, p):
        self._ensure_sel(ctx)
        self.points.append(p)
        if len(self.points) < 2:
            return True, None
        base, target = self.points
        dx, dy = target[0] - base[0], target[1] - base[1]
        import copy
        new = []
        for e in self.sel:
            c = copy.deepcopy(e)
            c.move_by(dx, dy)
            new.append(c)
        return False, ("add_many", new)


class RotateCmd(_ModifyCmd):
    name = "ROTATE"
    alias = ("RO", "ROTATE", "旋转")

    @property
    def prompt(self):
        return "指定基点" if not self.points else "指定旋转角（第二点方向）"

    def accept(self, ctx, p):
        self._ensure_sel(ctx)
        self.points.append(p)
        if len(self.points) < 2:
            return True, None
        base, second = self.points
        ang = geom.angle_of(base, second)
        before = self._before()
        for e in self.sel:
            e.rotate_by(base, ang)
        return False, ("modify", {"kind": "rotate", "entities": self.sel, "before": before})


class ScaleCmd(_ModifyCmd):
    name = "SCALE"
    alias = ("SC", "SCALE", "缩放")
    max_points = 3

    @property
    def prompt(self):
        return ("指定基点", "指定参照长度点", "指定新长度点")[min(len(self.points), 2)]

    def accept(self, ctx, p):
        self._ensure_sel(ctx)
        self.points.append(p)
        if len(self.points) < 3:
            return True, None
        base, ref, new = self.points
        d_ref = geom.dist_pt_pt(base, ref)
        d_new = geom.dist_pt_pt(base, new)
        k = d_new / d_ref if d_ref > 1e-9 else 1.0
        before = self._before()
        for e in self.sel:
            e.scale_by(base, k)
        return False, ("modify", {"kind": "scale", "entities": self.sel, "before": before})


class SelectCmd(Command):
    """点选/框选：只改选择集，不改文档。"""
    name = "SELECT"
    alias = ("S", "SELECT", "选择")
    max_points = 1

    @property
    def prompt(self):
        return "选择对象"

    def accept(self, ctx, p):
        tol = ctx.tol_world()
        e = self.doc_ctx.hit(p, tol)
        self.done = True
        if e is None:
            return False, ("clear", None)
        return False, ("select", e)

    def bind(self, doc):
        self.doc_ctx = doc


class BlockCmd(Command):
    """定义块：选一组对象 → 指定基点，写进块表记录（不改原图）。"""
    name = "BLOCK"
    alias = ("B", "BLOCK", "成块", "定义块")
    max_points = 2

    def begin(self, ctx):
        Command.begin(self, ctx)
        self.sel = []

    @property
    def prompt(self):
        return "选择要成块的对象" if not self.points else "指定基点"

    def accept(self, ctx, p):
        if not self.sel:
            self.sel = list(ctx.selection)
            if not self.sel:
                tol = ctx.tol_world()
                e = ctx.doc.hit(p, tol)
                self.sel = [e] if e is not None else []
        self.points.append(p)
        if len(self.points) < 2:
            return True, None
        base = self.points[1]
        if not self.sel:
            self.done = True
            return False, None
        return False, ("block", {"entities": self.sel, "base": base,
                                 "name": (self.args or "").strip()})


class InsertCmd(Command):
    """插入块参照：落点摆放块；参数 `INSERT [块名] [比例] [角度]`，默认最近块、1:1、不旋转。"""
    name = "INSERT"
    alias = ("I", "INSERT", "插入块")
    max_points = 1

    @property
    def prompt(self):
        return "指定块插入点"

    def accept(self, ctx, p):
        blocks = ctx.doc.blocks
        names = blocks.names()
        if not names:
            self.done = True
            return False, None

        parts = (self.args or "").split()
        name = names[-1]
        rest = parts
        if parts and parts[0] in blocks:
            name = parts[0]
            rest = parts[1:]

        def _num(s, default):
            try:
                return float(s)
            except (TypeError, ValueError):
                return default

        scale = _num(rest[0], 1.0) if rest else 1.0
        rot = _num(rest[1], 0.0) if len(rest) > 1 else 0.0
        return False, ent.Insert(name, p[0], p[1], sx=scale, sy=scale, rot=rot,
                                 layer=ctx.layer, color=ctx.color)


class DimCmd(Command):
    """线性标注：第一原点 → 第二原点 → 尺寸线位置（第三点定垂直偏移）。"""
    name = "DIM"
    alias = ("D", "DIM", "标注")
    max_points = 3

    @property
    def prompt(self):
        return ("指定第一原点", "指定第二原点", "指定尺寸线位置")[min(len(self.points), 2)]

    def accept(self, ctx, p):
        self.points.append(p)
        if len(self.points) < 3:
            return True, None
        p1, p2, p3 = self.points
        dx, dy = p2[0] - p1[0], p2[1] - p1[1]
        ln = geom.length((dx, dy)) or 1.0
        nx, ny = -dy / ln, dx / ln
        off = (p3[0] - p1[0]) * nx + (p3[1] - p1[1]) * ny
        return False, ent.Dimension(p1, p2, off=off,
                                     layer=ctx.layer, color=ctx.color)


class HatchCmd(Command):
    """区域填充：点选闭合边界（矩形 / 闭合多段线），生成剖面线填充。"""
    name = "HATCH"
    alias = ("H", "HATCH", "填充")
    max_points = 1

    @property
    def prompt(self):
        return "选择要填充的闭合区域"

    def accept(self, ctx, p):
        tol = ctx.tol_world()
        e = self.doc_ctx.hit(p, tol)
        if e is None:
            self.done = True
            return False, None
        if e.kind == "rect":
            pts = e.corners
        elif e.kind == "polyline" and e.is_closed():
            pts = list(e.pts)
        else:
            return True, None     # 非闭合边界，继续点选
        self.done = True
        return False, ent.Hatch(pts, layer=ctx.layer, color=ctx.color)

    def bind(self, doc):
        self.doc_ctx = doc


COMMANDS = {}


def _register(cls):
    for a in cls.alias:
        COMMANDS[a.upper()] = cls
    COMMANDS[cls.name] = cls
    return cls


for _c in (LineCmd, CircleCmd, RectCmd, ArcCmd, PlineCmd, EraseCmd, SelectCmd,
            MoveCmd, CopyCmd, RotateCmd, ScaleCmd, BlockCmd, InsertCmd, DimCmd,
            HatchCmd):
    _register(_c)


def create(name):
    cls = COMMANDS.get(str(name).upper())
    if cls is None:
        return None
    return cls()


# ---------------------------------------------------------------- 命令上下文 & 管理器
class CommandContext(object):
    def __init__(self, doc, mapper, layer="0", color=0):
        self.doc = doc
        self.mapper = mapper
        self.layer = layer
        self.color = color
        self.selection = []

    def tol_world(self, px=DEFAULT_TOL_PX):
        return px * self.mapper.world_per_px()


class CommandManager(object):
    """管"当前在跑哪个命令"，并把画布事件翻译成点。"""

    def __init__(self, doc, mapper):
        self.doc = doc
        self.mapper = mapper
        self.ctx = CommandContext(doc, mapper)
        self.selection = []
        self.snapper = Snapper(doc)
        self.active = None
        self.last_preview = None
        self.last_snap = None
        self.on_prompt = None      # 上层注册：改命令行提示
        self.on_commit = None      # 上层注册：命令产出了实体

    # ---- 命令生命周期 ----
    def start(self, name, args=None):
        cmd = create(name)
        if cmd is None:
            return False
        if hasattr(cmd, "bind"):
            cmd.bind(self.doc)
        self.active = cmd
        cmd.args = (args or "").strip()
        cmd.begin(self.ctx)
        self._prompt()
        return True

    def cancel(self):
        if self.active is not None:
            self.active.cancel(self.ctx)
            self.active = None
        self.last_preview = None
        self.last_snap = None
        self._prompt("命令已取消" if False else "")

    def finish_poly(self):
        """回车/双击：让当前命令收尾（多段线把攒下的点连成一条）。"""
        cmd = self.active
        if cmd is None:
            return False
        self.active = None
        fin = getattr(cmd, "finish", None)
        if fin is not None:
            e = fin(self.ctx)
            if e is not None:
                self._commit(e)
        self.last_preview = None
        self.last_snap = None
        self._prompt()
        return True

    # ---- 事件入口 ----
    def move(self, world_pt):
        """鼠标移动：做捕捉 + 生成预览，返回 (预览实体, 捕捉结果)。"""
        tol = self.ctx.tol_world()
        snap = self.snapper.snap(world_pt, tol)
        self.last_snap = snap
        effective = snap.point if snap is not None else world_pt
        self.last_preview = None
        if self.active is not None:
            self.last_preview = self.active.preview(self.ctx, effective)
        return self.last_preview, snap

    def click(self, world_pt):
        """左键定点。返回 True 表示命令吃掉了这一击（不要当平移）。"""
        if self.active is None:
            return False
        self.ctx.selection = list(self.selection)
        tol = self.ctx.tol_world()
        snap = self.snapper.snap(world_pt, tol)
        effective = snap.point if snap is not None else world_pt
        cont, payload = self.active.accept(self.ctx, effective)
        if payload is not None:
            if isinstance(payload, tuple):
                self._event(payload[0], payload[1])
            else:
                self._commit(payload)
        if not cont:
            cmd = self.active
            self.active = None
            fin = getattr(cmd, "finish", None)
            if fin is not None:
                leftover = fin(self.ctx)
                if leftover is not None:
                    self._commit(leftover)
        self.last_preview = None
        self._prompt()
        return True

    # ---- 回调 -----
    def _commit(self, entity):
        if entity is not None and self.on_commit is not None:
            self.on_commit(entity)

    def _event(self, kind, entity):
        if self.on_commit is not None:
            self.on_commit((kind, entity))

    def _prompt(self, text=None):
        if self.on_prompt is None:
            return
        if text is None:
            text = self.active.prompt if self.active is not None else ""
        self.on_prompt(text)


if __name__ == "__main__":
    d = __import__("doc").Document()
    m = ViewMapper((0, 0, 12000, 8000), 0.1)
    m.set_view((0, 0, 12000, 8000), 1.0, 0, 0)
    p = m.screen_to_world(300, 200)
    assert abs(p[0] - 3000) < 1e-6 and abs(p[1] - 6000) < 1e-6, p
    assert m.world_to_screen(p)[0] == 300
    cm = CommandManager(d, m)
    got = []
    cm.on_commit = got.append
    assert cm.start("LINE")
    cm.click((1000, 1000))
    cm.click((2000, 1500))
    assert got and got[0].kind == "line"
    print("interact 自检通过")
