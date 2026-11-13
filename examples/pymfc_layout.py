# -*- coding: utf-8 -*-
# pymfc_layout.py — PyMfcXaml 的简单自动布局：VBox / HBox。
#
# 用法：
#   w = PyMfc.Window("test", 420, 300)
#   box = VBox(w, margin=12, spacing=8)
#   box.addLabel("标题")
#   box.addButton("绘制", onClick=...)
#   box.addEdit("edit")
#   box.addListBox(height=100)
#   w.show()


class VBox:
    def __init__(self, window, margin=12, spacing=8):
        self.window = window
        self.margin = margin
        self.spacing = spacing
        self.items = []
        self.next_id = 2000
        window.onResize(self._on_resize)

    def _on_resize(self, w, h):
        self.relayout(w, h)

    def _new_id(self):
        self.next_id += 1
        return self.next_id

    def addLabel(self, text, height=24):
        cid = self._new_id()
        self.window.addLabel(cid, text, 0, 0, 1, height)
        self.items.append({"id": cid, "height": height, "grow": False})
        return cid

    def addButton(self, text, onClick=None, height=32):
        cid = self._new_id()
        self.window.addButton(cid, text, 0, 0, 1, height)
        if onClick is not None:
            self.window.onClick(cid, onClick)
        self.items.append({"id": cid, "height": height, "grow": False})
        return cid

    def addEdit(self, text="", height=28):
        cid = self._new_id()
        self.window.addEdit(cid, text, 0, 0, 1, height)
        self.items.append({"id": cid, "height": height, "grow": False})
        return cid

    def addListBox(self, height=100, grow=True):
        cid = self._new_id()
        self.window.addListBox(cid, 0, 0, 1, height)
        self.items.append({"id": cid, "height": height, "grow": grow})
        return cid

    def relayout(self, width, height):
        x = self.margin
        y = self.margin
        inner_w = max(width - self.margin * 2, 20)
        inner_h = max(height - self.margin * 2, 20)

        fixed = sum(i["height"] for i in self.items if not i["grow"])
        grow_items = [i for i in self.items if i["grow"]]
        spacing_total = self.spacing * max(len(self.items) - 1, 0)
        remaining = inner_h - fixed - spacing_total
        grow_h = max(remaining // len(grow_items), 40) if grow_items else 0

        for item in self.items:
            item_h = grow_h if item["grow"] else item["height"]
            self.window.move(item["id"], x, y, inner_w, item_h)
            y += item_h + self.spacing

    def add_string_list(self, list_id, values):
        for value in values:
            self.window.addString(list_id, value)
