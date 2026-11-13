import io
P = r'G:\PyArx\tests\mfc_xaml\pp_app.py'
s = io.open(P, encoding='utf-8').read()

# 1) 常量
old = """ED_X, ED_Y, ED_W, ED_H = CW - 640, CV_Y, 640, CH - CV_Y"""
new = """ED_X, ED_Y, ED_W, ED_H = CW - 640, CV_Y, 640, CH - CV_Y
SPLIT = 1232                     # 画布与侧板之间的可拖动分割条（6px）
SPLIT_W = 6"""
assert s.count(old) == 1
s = s.replace(old, new)

# 2) 状态位
old = """        self._ce = None                       # 原地单元格编辑：(行, 列, 编辑框 id)"""
new = """        self._ce = None                       # 原地单元格编辑：(行, 列, 编辑框 id)
        self._ed_w = ED_W                     # 侧板当前宽度（拖动分割条可改，360..900）"""
assert s.count(old) == 1
s = s.replace(old, new)

# 3) 布局：画布让出 侧板 + 分割条
old = """        ed_w = ED_W if self.editing else 0
        cv_w = max(320, cw - CV_X - 8 - ed_w)"""
new = """        ed_w = (self._ed_w + SPLIT_W) if self.editing else 0
        cv_w = max(320, cw - CV_X - 8 - ed_w)"""
assert s.count(old) == 1
s = s.replace(old, new)

# 4) 侧板布局全部改用可变宽度
old = """        ed_x = max(CV_X, cw - ED_W)
        ed_y = CV_Y
        ed_h = max(240, ch - CV_Y - 8)
        self._ed_rc = (ed_x, ed_y, ED_W, ed_h)
        px = ed_x + ED_PAD
        inner = ED_W - 2 * ED_PAD"""
new = """        edw = max(360, min(900, self._ed_w))
        ed_x = max(CV_X + 320 + SPLIT_W, cw - edw)
        ed_y = CV_Y
        ed_h = max(240, ch - CV_Y - 8)
        self._ed_rc = (ed_x, ed_y, edw, ed_h)
        px = ed_x + ED_PAD
        inner = edw - 2 * ED_PAD"""
assert s.count(old) == 1
s = s.replace(old, new)

old = """        items = [
            (ED_PANEL, ed_x, ed_y, ED_W, ed_h, True),
            (L_EDIT_TITLE, px, ed_y + 12, 120, 20, True),
            (L_EDIT_COUNT, px + 78, ed_y + 12, 160, 20, True),
            (B_CLOSE, ed_x + ED_W - ED_PAD - 72, ed_y + 8, 72, 26, True),
            (B_APPLY, ed_x + ED_W - ED_PAD - 172, ed_y + 8, 96, 26, True),
            (B_ADDROW, ed_x + ED_W - ED_PAD - 268, ed_y + 8, 92, 26, True),"""
new = """        items = [
            (ED_PANEL, ed_x, ed_y, edw, ed_h, True),
            (SPLIT, ed_x - SPLIT_W, ed_y, SPLIT_W, ed_h, True),
            (L_EDIT_TITLE, px, ed_y + 12, 120, 20, True),
            (L_EDIT_COUNT, px + 78, ed_y + 12, 160, 20, True),
            (B_CLOSE, ed_x + edw - ED_PAD - 72, ed_y + 8, 72, 26, True),
            (B_APPLY, ed_x + edw - ED_PAD - 172, ed_y + 8, 96, 26, True),
            (B_ADDROW, ed_x + edw - ED_PAD - 268, ed_y + 8, 92, 26, True),"""
assert s.count(old) == 1
s = s.replace(old, new)

# 5) 编辑时画布可用宽度
old = """        used = ED_W if self.editing else 0"""
new = """        used = (self._ed_w + SPLIT_W) if self.editing else 0"""
assert s.count(old) == 1
s = s.replace(old, new)

# 6) 创建分割条 + 绑定拖动
old = """        w.onBlur(E_F0 + i, self._cell_blur)"""
new = """        w.onBlur(E_F0 + i, self._cell_blur)"""
assert s.count(old) == 1

old = """        w.addWhiteRect(ED_PANEL, ED_X, ED_Y, ED_W, ED_H)"""
new = """        w.addWhiteRect(ED_PANEL, ED_X, ED_Y, ED_W, ED_H)
        # 画布与侧板之间：6px 可拖动分割条（拖动改侧板宽度，和 VS/VSCode 的分栏一样）
        w.addWhiteRect(SPLIT, ED_X - SPLIT_W, ED_Y, SPLIT_W, ED_H)
        if hasattr(w, 'onDrag'):
            w.onDrag(SPLIT, self.on_split_drag)"""
assert s.count(old) == 1
s = s.replace(old, new)

# 7) 常显件加分割条
old = """        return [ED_PANEL, L_EDIT_TITLE, L_EDIT_COUNT, B_ADDROW, B_APPLY, B_CLOSE,
                L_EDIT_FOOT, TBL]"""
new = """        return [SPLIT, ED_PANEL, L_EDIT_TITLE, L_EDIT_COUNT, B_ADDROW, B_APPLY, B_CLOSE,
                L_EDIT_FOOT, TBL]"""
assert s.count(old) == 1
s = s.replace(old, new)

# 8) 拖动回调
old = """    def on_resize(self, cw, ch):"""
new = """    def on_split_drag(self, phase, x):
        \"\"\"分割条拖动：x 是分割条内的横坐标（左移为负），按位移改侧板宽度后整窗重排。\"\"\"
        if phase == 'down':
            self._split_from = (self._ed_w, x)
            return
        if phase == 'move' and getattr(self, '_split_from', None):
            base, x0 = self._split_from
            new_w = int(base - (x - x0))          # 向左拖 → 侧板变宽
            new_w = max(360, min(900, new_w))
            if new_w != self._ed_w:
                self._ed_w = new_w
                self._layout()
            return
        self._split_from = None

    def on_resize(self, cw, ch):"""
assert s.count(old) == 1
s = s.replace(old, new)

io.open(P, 'w', encoding='utf-8').write(s)
print('Python：分割条已接入（SPLIT=1232，360..900 可拖）')
