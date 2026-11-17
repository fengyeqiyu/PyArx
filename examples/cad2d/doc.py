# -*- coding: utf-8 -*-
"""doc —— 图纸文档（门面）

把四个核心拼起来：
    db.SymbolTables   图层 / 线型 / 文字样式 / 块表
    ent.*             图元
    render.to_svg     渲成画布能吃的 SVG
    geom.*            命中与框选用的数学
本文件只做"文档级"的事：增删图元、可见性、范围、命中、框选。
要加新能力请加到对应模块，不要往这里堆。

兼容旧名：import doc 后仍可用 doc.Line / doc.Layer / doc.Document。
"""
import db
import ent
import geom
import render
from db import Block, Layer, LayerTable, Linetype, SymbolTables, TextStyle
from ent import Arc, Circle, Insert, Line, Polyline, Rect, Text
from render import ACI, REF_PX, SCREEN_UNIT, aci_color

__all__ = ["Document", "Line", "Circle", "Arc", "Rect", "Polyline", "Text", "Insert",
           "Layer", "Linetype", "TextStyle", "Block", "SymbolTables",
           "ACI", "SCREEN_UNIT", "aci_color"]


class Document(object):
    """图纸 = 符号表 + 图元表。ext(w,h) 是图幅，决定默认 viewBox。"""

    def __init__(self, width=12000.0, height=8000.0):
        self.width = float(width)
        self.height = float(height)
        self.symbols = SymbolTables()
        self.entities = []
        self._next = 1

    @property
    def screen_unit(self):
        """本图纸的 px/世界单位：幅面长边统一折算到 REF_PX。

        12000mm 幅面 → 0.1，与 SCREEN_UNIT 完全一致（原生图纸观感不变）；
        91 单位的英寸图 → 13.1；上百公里的城市坐标图 → 几e-4。
        这样 SVG 尺寸始终在一千多画布单位，不会因为图纸单位不同被画布放大成大块
        或被画成几十万像素宽。
        """
        long_side = max(abs(self.width), abs(self.height), 1e-9)
        return REF_PX / long_side

    # ------------------------------------------------------------ 符号表
    @property
    def layers(self):
        return self.symbols.layers

    @property
    def blocks(self):
        return self.symbols.blocks

    @property
    def textstyles(self):
        return self.symbols.textstyles

    @property
    def linetypes(self):
        return self.symbols.linetypes

    @property
    def current_layer(self):
        return self.symbols.current_layer

    @current_layer.setter
    def current_layer(self, name):
        self.symbols.layers.ensure(name)
        self.symbols.current_layer = name

    # ------------------------------------------------------------ 图元
    def add(self, entity):
        if not entity.layer:
            entity.layer = self.current_layer
        self.symbols.layers.ensure(entity.layer)
        entity.id = self._next
        entity.handle = "%X" % (0x100 + self._next)
        self._next += 1
        self.entities.append(entity)
        return entity

    def add_many(self, entities):
        for e in entities:
            self.add(e)
        return entities

    def remove(self, entity):
        ent_id = entity.id if hasattr(entity, "id") else entity
        self.entities = [e for e in self.entities if e.id != ent_id]

    def remove_many(self, entities):
        ids = set(e.id if hasattr(e, "id") else e for e in entities)
        self.entities = [e for e in self.entities if e.id not in ids]

    def clear(self):
        self.entities = []
        self._next = 1

    # ------------------------------------------------------------ 持久化
    def to_dict(self):
        layers = []
        for lay in self.layers:
            layers.append({"name": lay.name, "color": lay.color, "width": lay.width,
                           "linetype": lay.linetype, "visible": lay.visible,
                           "locked": lay.locked, "frozen": lay.frozen})
        blocks = []
        for b in self.blocks.all():
            blocks.append({"name": b.name, "base": list(b.base),
                           "entities": [e.to_dict() for e in b.entities]})
        return {"width": self.width, "height": self.height,
                "current_layer": self.symbols.current_layer,
                "layers": layers, "blocks": blocks,
                "entities": [e.to_dict() for e in self.entities]}

    def load_dict(self, data):
        self.clear()
        self.width = float(data.get("width", 12000.0))
        self.height = float(data.get("height", 8000.0))
        for ld in data.get("layers", []):
            lay = db.Layer(ld["name"], color=ld.get("color", 7),
                           width=ld.get("width", 1.0),
                           linetype=ld.get("linetype", "Continuous"),
                           visible=ld.get("visible", True),
                           locked=ld.get("locked", False),
                           frozen=ld.get("frozen", False))
            self.layers.add(lay)
        self.symbols.current_layer = data.get("current_layer", "0")
        for bd in data.get("blocks", []):
            ents = []
            for ed in bd.get("entities", []):
                cls = ent.FACTORY.get(ed.get("kind"))
                if cls is not None:
                    ents.append(cls.from_dict(ed))
            self.blocks.define(bd["name"], ents, tuple(bd.get("base", [0.0, 0.0])))
        for ed in data.get("entities", []):
            kind = ed.get("kind")
            cls = ent.FACTORY.get(kind)
            if cls is None:
                continue
            self.add(cls.from_dict(ed))
        return self

    def by_id(self, ent_id):
        for e in self.entities:
            if e.id == ent_id:
                return e
        return None

    def restore(self, entity):
        """撤销/重做恢复：保留快照里的 id/句柄，不再重新分配。"""
        if self.by_id(entity.id) is None:
            self.entities.append(entity)
        return entity

    def count(self):
        return len(self.entities)

    # ------------------------------------------------------------ 可见性
    def layer_of(self, entity):
        return self.symbols.layers.ensure(entity.layer)

    def is_visible(self, entity):
        lay = self.layer_of(entity)
        return lay.visible and not lay.frozen

    def is_selectable(self, entity):
        lay = self.layer_of(entity)
        return lay.visible and not lay.frozen and not lay.locked

    def visible_entities(self):
        return [e for e in self.entities if self.is_visible(e)]

    # ------------------------------------------------------------ 范围
    def extents(self, only_visible=True):
        pool = self.visible_entities() if only_visible else self.entities
        boxes = []
        for e in pool:
            b = e.bbox()
            if b[2] - b[0] <= 0.0 and b[3] - b[1] <= 0.0:
                continue          # 零尺寸图元（点、零长线）不参与撑范围
            boxes.append(b)
        if not boxes:
            return (0.0, 0.0, self.width, self.height)
        full = boxes[0]
        for b in boxes[1:]:
            full = geom.bbox_union(full, b)
        # 抗离群：外部图纸常有一两条游离线（老的配电点位、示意参考线）把范围撑到
        # 十几倍，看图时整张图被压成一个点。按图元中心取 1%~99% 圈主体，超出
        # 2.5 倍才改用主体范围，正常图纸不受影响。
        core = self._core_extents(boxes)
        if core is not None:
            fw, fh = full[2] - full[0], full[3] - full[1]
            cw, ch = core[2] - core[0], core[3] - core[1]
            if (cw > 0.0 and fw > cw * 2.5) or (ch > 0.0 and fh > ch * 2.5):
                return core
        return full

    def _core_extents(self, boxes):
        """主体范围：中心落在 1%~99% 分位窗口内的图元并集。样本太少就放弃。"""
        if len(boxes) < 32:
            return None

        def pick(vals, p):
            i = int(p * (len(vals) - 1))
            return vals[max(0, min(len(vals) - 1, i))]

        cxs = sorted((b[0] + b[2]) * 0.5 for b in boxes)
        cys = sorted((b[1] + b[3]) * 0.5 for b in boxes)
        lo_x, hi_x = pick(cxs, 0.01), pick(cxs, 0.99)
        lo_y, hi_y = pick(cys, 0.01), pick(cys, 0.99)
        core = None
        for b in boxes:
            mx, my = (b[0] + b[2]) * 0.5, (b[1] + b[3]) * 0.5
            if lo_x <= mx <= hi_x and lo_y <= my <= hi_y:
                core = b if core is None else geom.bbox_union(core, b)
        return core

    def extents_bbox(self):
        return self.extents()

    # ------------------------------------------------------------ 命中 / 框选
    def hit(self, p, tol_world, pick_locked=False):
        """点选：从最后画的往前找，先命中先返回（AutoCAD 语义）。"""
        for e in reversed(self.entities):
            if not (pick_locked or self.is_selectable(e)):
                continue
            if e.hit(p, tol_world):
                return e
        return None

    def hit_many(self, p, tol_world):
        out = []
        for e in reversed(self.entities):
            if self.is_selectable(e) and e.hit(p, tol_world):
                out.append(e)
        return out

    def window_select(self, p1, p2, crossing=True):
        """窗口选择。crossing=True 时框住或相交都算（默认，和 AutoCAD 拖右→左一致）。"""
        box = geom.rect_norm(p1[0], p1[1], p2[0], p2[1])
        out = []
        for e in self.entities:
            if not self.is_selectable(e):
                continue
            eb = e.bbox()
            if crossing:
                if geom.bbox_intersect(eb, box):
                    out.append(e)
            else:
                if geom.bbox_contains(box, eb):
                    out.append(e)
        return out

    # ------------------------------------------------------------ 渲染
    def view(self):
        """画布 viewBox：图元范围为主，幅面只在图元本就落在幅面内时兜底。"""
        x0, y0, x1, y1 = self.extents()
        # 幅面兜底只对"从原点起画、且没超出幅面多少"的图纸成立；外部导入的图纸
        # 常带负坐标或城市坐标，套幅面会把整图压成一个点、甚至放大出空白。
        if x0 >= 0.0 and y0 >= 0.0 and x1 <= self.width * 1.5 and y1 <= self.height * 1.5:
            return (0.0, 0.0, max(x1, self.width), max(y1, self.height))
        return (x0, y0, x1, y1)

    def to_svg(self, view=None):
        return render.to_svg(self.symbols, self.visible_entities(), view or self.view(),
                             unit=self.screen_unit)
