# -*- coding: utf-8 -*-
"""history —— 撤销 / 重做栈（纯 Python，不碰界面）

三类已覆盖操作：
  ADD    添加图元
  REMOVE 删除图元
  MODIFY 几何修改（夹点拖动 / 移动 / 复制 / 旋转 / 标注偏移等）

每条记录持有图元的深拷贝快照；撤销与重做都是"把文档里的图元换成快照"，
不依赖宿主提供事务 API。
"""


class Op(object):
    def __init__(self, kind, entity, before=None, after=None):
        self.kind = kind            # ADD / REMOVE / MODIFY
        self.entity = entity
        self.before = before
        self.after = after


class UndoStack(object):
    def __init__(self, doc):
        self.doc = doc
        self.undo = []
        self.redo = []

    # ------------------------------------------------ 记录
    def push_add(self, entity):
        self.undo.append(Op("ADD", entity))
        self.redo.clear()

    def push_remove(self, entity):
        self.undo.append(Op("REMOVE", entity, before=entity.snapshot()))
        self.redo.clear()

    def push_modify(self, entity, before):
        self.undo.append(Op("MODIFY", entity, before=before.snapshot(),
                            after=entity.snapshot()))

    def push_block(self, name, block):
        """块表记录新增：撤销即删块定义，重做即按快照还原。"""
        import copy
        self.undo.append(Op("BLOCK_DEFINE", None,
                            before={"name": name, "block": copy.deepcopy(block)}))
        self.redo.clear()

    def clear(self):
        self.undo = []
        self.redo = []

    # ------------------------------------------------ 执行
    def can_undo(self):
        return bool(self.undo)

    def can_redo(self):
        return bool(self.redo)

    def undo_one(self):
        if not self.undo:
            return None
        op = self.undo.pop()
        if op.kind == "ADD":
            self.doc.remove(op.entity)
        elif op.kind == "REMOVE":
            self.doc.restore(op.before)
        elif op.kind == "BLOCK_DEFINE":
            self.doc.blocks.remove(op.before["name"])
        else:
            self._restore(op.entity, op.before)
        self.redo.append(op)
        return op

    def redo_one(self):
        if not self.redo:
            return None
        op = self.redo.pop()
        if op.kind == "ADD":
            self.doc.restore(op.entity)
        elif op.kind == "REMOVE":
            self.doc.remove(op.entity)
        elif op.kind == "BLOCK_DEFINE":
            import copy
            blk = op.before["block"]
            self.doc.blocks.define(blk.name, copy.deepcopy(blk.entities), blk.base)
        else:
            self._restore(op.entity, op.after)
        self.undo.append(op)
        return op

    def _restore(self, entity, snapshot):
        cur = self.doc.by_id(entity.id)
        if cur is None:
            self.doc.restore(snapshot)
            return
        for attr in ("x1", "y1", "x2", "y2", "cx", "cy", "r", "a1", "a2",
                     "pts", "closed", "x", "y", "text", "size", "style", "rot",
                     "p1", "p2", "off", "name", "sx", "sy"):
            if hasattr(snapshot, attr):
                setattr(cur, attr, getattr(snapshot, attr))
        for attr in ("layer", "color", "width", "linetype"):
            if hasattr(snapshot, attr):
                setattr(cur, attr, getattr(snapshot, attr))
