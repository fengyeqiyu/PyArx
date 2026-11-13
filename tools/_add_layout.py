import io
P = r'G:\PyArx\tests\mfc_xaml\pp_app.py'
t = io.open(P, encoding='utf-8-sig').read()

# 1) __init__ 状态字段
old = """        self.editing = False
        self.cur_sel = ''
        self.cv_w, self.cv_h = CV_W, CV_H     # 画布当前尺寸（编辑侧板打开时会变窄）"""
new = """        self.editing = False
        self.cur_sel = ''
        self._row_edit_open = False           # 行编辑条是否展开
        self._row_edit_row = -1               # 正在编辑的行号
        self._detail_visible = False          # 详情浮层是否可见（_layout 要用它决定显隐）
        self.cv_w, self.cv_h = CV_W, CV_H     # 画布当前尺寸（编辑侧板打开时会变窄）"""
assert t.count(old) == 1, 'init'
t = t.replace(old, new)

# 2) _build_ui 注册 resize 回调
old2 = """        w.onClick(MINI, self.on_mini_click)

        self.hide_editor()"""
new2 = """        w.onClick(MINI, self.on_mini_click)
        w.onResize(self.on_resize)    # 窗口尺寸变化 → 按网格重排（见 _layout）

        self.hide_editor()"""
assert t.count(old2) == 1, 'resize bind'
t = t.replace(old2, new2)

# 3) 详情浮层显隐与定位
old3 = """    def hide_detail(self):
        for i in self._detail_ids:
            self.win.setVisible(i, False)

    def show_detail(self):
        for i in self._detail_ids:
            self.win.setVisible(i, True)
            self.win.bringToFront(i)

    def position_detail(self):
        \"\"\"HTML positionDetail：编辑器打开时 detail 左移 640，避免被侧板压住。\"\"\"
        dx = -ED_W if self.editing else 0
        for cid, (x, y, ww, hh) in self._detail_rc.items():
            self.win.move(cid, x + dx, y, ww, hh)"""
new3 = """    def hide_detail(self):
        self._detail_visible = False
        for i in self._detail_ids:
            self.win.setVisible(i, False)

    def show_detail(self):
        self._detail_visible = True
        self._layout()                 # 先按当前画布算出右上角位置，再显示
        for i in self._detail_ids:
            self.win.setVisible(i, True)
            self.win.bringToFront(i)

    def position_detail(self):
        \"\"\"HTML positionDetail：详情浮层固定在画布右上角，位置由 _layout 统一算
        （侧板打开时画布变窄，浮层跟着左移，等价于 HTML 里 right:12px 的吸附）。\"\"\"
        self._layout()"""
assert t.count(old3) == 1, 'detail'
t = t.replace(old3, new3)

# 4) 插入 _layout 与行编辑
anchor = "    def apply_fonts(self):"
NEW = '''    # ---------- 版式（grid 自适应） ----------
    def _client_size(self):
        try:
            cw, ch = self.win.clientSize()
        except Exception:
            cw, ch = CW, CH
        if not cw or not ch or cw < 400 or ch < 300:
            cw, ch = CW, CH           # 首帧窗口还没真正布局出来，先用设计基准
        return int(cw), int(ch)

    def _layout(self):
        """整窗按网格重排：
          行：工具栏（固定 78 高）/ 内容区（*）/ 底边 8
          列：画布（*）/ 侧板（640，仅编辑时占位）
        窗口放大时新增空间全部给画布，和 HTML 的 flex/grid 行为一致。"""
        w = self.win
        cw, ch = self._client_size()
        ed_w = ED_W if self.editing else 0
        cv_w = max(320, cw - CV_X - 8 - ed_w)
        cv_h = max(200, ch - CV_Y - 8)
        self.cv_w, self.cv_h = cv_w, cv_h

        items = [(CV, CV_X, CV_Y, cv_w, cv_h, True)]

        # 状态栏：左 + 底 锚定（HTML .status left 12 / bottom 10）
        st_w = max(240, min(ST_W, cv_w - 8))
        st_y = ch - 10 - ST_H
        items += [(ST_PANEL, ST_X, st_y, st_w, ST_H, True),
                  (L_STATUS, ST_X + 4, st_y + 4, st_w - 8, 18, True)]

        # 小地图：右 + 底 锚定（HTML .mini right 12 / bottom 10）
        mfx = cw - 12 - MI_FRAME[2]
        mfy = ch - 10 - MI_FRAME[3]
        items += [(MI_PANEL, mfx, mfy, MI_FRAME[2], MI_FRAME[3], True),
                  (MINI, mfx + 4, mfy + 4, MI_CANVAS[2], MI_CANVAS[3], True)]

        if self.editing:
            items += self._layout_editor(cw, ch)

        # 详情浮层：画布右上（HTML .detail top 12 right 12）
        if getattr(self, '_detail_rc', None):
            vis = bool(getattr(self, '_detail_visible', False))
            dtx = CV_X + cv_w - 12 - DT_W
            dty = CV_Y + 12
            for cid, (x, y, ww, hh) in self._detail_rc.items():
                items.append((cid, dtx + (x - DT_X), dty + (y - DT_Y), ww, hh, vis))
        if items:
            w.moveMany(items)

    def _layout_editor(self, cw, ch):
        """侧板内部：头部（标题 / 3 按钮）→ 表格（原生 ListView 撑满）→ 行编辑条 → footer。"""
        ed_x = max(CV_X, cw - ED_W)
        ed_y = CV_Y
        ed_h = max(240, ch - CV_Y - 8)
        self._ed_rc = (ed_x, ed_y, ED_W, ed_h)
        px = ed_x + ED_PAD
        inner = ED_W - 2 * ED_PAD
        on = self._row_edit_open
        return [
            (ED_PANEL, ed_x, ed_y, ED_W, ed_h, True),
            (L_EDIT_TITLE, px, ed_y + 12, 120, 20, True),
            (L_EDIT_COUNT, px + 78, ed_y + 12, 100, 20, True),
            (B_CLOSE, ed_x + ED_W - ED_PAD - 72, ed_y + 8, 72, 26, True),
            (B_APPLY, ed_x + ED_W - ED_PAD - 172, ed_y + 8, 96, 26, True),
            (B_ADDROW, ed_x + ED_W - ED_PAD - 268, ed_y + 8, 92, 26, True),
            (L_EDIT_FOOT, px, ed_y + ed_h - 30, inner, 20, True),
        ]

    def _layout_editor_tail(self, ed_x, ed_y, ed_h):
        """表格与行编辑条的位置（依赖 _row_edit_open）。"""
        return []

'''
t = t.replace(anchor, NEW + anchor, 1)
io.open(P, 'w', encoding='utf-8-sig', newline='').write(t)
print('layout 骨架已插入')
