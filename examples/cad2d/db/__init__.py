# -*- coding: utf-8 -*-
"""db —— 符号表（模块 4）

CAD 里"实体画什么"由实体自己说，"长什么样"由符号表定：
  LayerTable      图层：颜色、线宽、线型、开关、锁定、冻结
  LinetypeTable   线型：实线 / 虚线 / 点划线（元素是"画长、空长"交替）
  TextStyleTable  文字样式：字体、字高、宽度因子、倾斜
  BlockTable      块：一组实体的命名集合，可带基点
文档持有一份 SymbolTables，实体只存引用（图层名 / 块名 / 样式名）。
"""
import copy


# ---------------------------------------------------------------- 线型
class Linetype(object):
    def __init__(self, name="Continuous", elements=None, description=""):
        self.name = name
        self.elements = list(elements or [])   # [画长, 空长, 画长, 空长, ...] 单位 mm
        self.description = description

    @property
    def continuous(self):
        return not self.elements


# ---------------------------------------------------------------- 图层
class Layer(object):
    def __init__(self, name="0", color=7, width=1.0, linetype="Continuous",
                 visible=True, locked=False, frozen=False):
        self.name = name
        self.color = color          # ACI 索引色
        self.width = width          # 线宽 mm，0 = 最细
        self.linetype = linetype
        self.visible = visible
        self.locked = locked
        self.frozen = frozen

    def clone(self):
        return copy.copy(self)


class LayerTable(object):
    def __init__(self):
        self._items = {}
        self.add(Layer("0", 7, 1.0))

    def add(self, layer):
        self._items[layer.name] = layer
        return layer

    def get(self, name):
        return self._items.get(name)

    def ensure(self, name):
        """图层不存在就建一个默认的（画图时经常遇到）。"""
        lay = self._items.get(name)
        if lay is None:
            lay = self.add(Layer(name))
        return lay

    def remove(self, name):
        self._items.pop(name, None)

    def names(self):
        return list(self._items.keys())

    def all(self):
        return list(self._items.values())

    def __contains__(self, name):
        return name in self._items

    def __getitem__(self, name):
        return self.ensure(name)

    def __setitem__(self, name, layer):
        self.add(layer)

    def __iter__(self):
        return iter(self._items.values())


# ---------------------------------------------------------------- 文字样式
class TextStyle(object):
    def __init__(self, name="Standard", font="Microsoft YaHei", height=0.0,
                 width_factor=1.0, oblique=0.0):
        self.name = name
        self.font = font
        self.height = height            # 0 = 由文字实体自带字高
        self.width_factor = width_factor
        self.oblique = oblique


class TextStyleTable(object):
    def __init__(self):
        self._items = {}
        self.add(TextStyle("Standard"))

    add = LayerTable.add
    get = LayerTable.get
    ensure = LayerTable.ensure
    remove = LayerTable.remove
    names = LayerTable.names
    all = LayerTable.all
    __contains__ = LayerTable.__contains__
    __iter__ = LayerTable.__iter__


class LinetypeTable(object):
    def __init__(self):
        self._items = {}
        self.add(Linetype("Continuous"))           # 实线
        self.add(Linetype("Dashed", [12.0, 6.0]))
        self.add(Linetype("Center", [24.0, 6.0, 6.0, 6.0]))   # 点划线
        self.add(Linetype("Hidden", [6.0, 3.0]))

    add = LayerTable.add
    get = LayerTable.get
    ensure = LayerTable.ensure
    remove = LayerTable.remove
    names = LayerTable.names
    all = LayerTable.all
    __contains__ = LayerTable.__contains__
    __iter__ = LayerTable.__iter__


# ---------------------------------------------------------------- 块
class Block(object):
    def __init__(self, name, entities=None, base=(0.0, 0.0)):
        self.name = name
        self.entities = list(entities or [])      # 块内实体，坐标相对基点
        self.base = base


class BlockTable(object):
    def __init__(self):
        self._items = {}

    def define(self, name, entities, base=(0.0, 0.0)):
        blk = Block(name, entities, base)
        self._items[name] = blk
        return blk

    def get(self, name):
        return self._items.get(name)

    def remove(self, name):
        self._items.pop(name, None)

    def names(self):
        return list(self._items.keys())

    def all(self):
        return list(self._items.values())

    def __contains__(self, name):
        return name in self._items


# ---------------------------------------------------------------- 汇总
class SymbolTables(object):
    def __init__(self):
        self.layers = LayerTable()
        self.linetypes = LinetypeTable()
        self.textstyles = TextStyleTable()
        self.blocks = BlockTable()
        self.current_layer = "0"

    def layer(self, name):
        return self.layers.ensure(name)

    def linetype(self, name):
        return self.linetypes.ensure(name) or self.linetypes.get("Continuous")
