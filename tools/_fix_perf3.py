import io
p = 'G:/PyArx/tests/mfc_xaml/pp_app.py'
s = io.open(p, encoding='utf-8').read()

# 1) 常量：分割线控件 id
old = "SPLIT_W = 6\n"
new = "SPLIT_W = 6\nL_SPLIT = 1233      # 分割线本身（1px owner-draw 细线，跟着分割条走）\n"
assert s.count(old) == 1
s = s.replace(old, new)

# 2) 建控件：拖动热区不再自带白底黑框，线单独画
old = """        # 画布与侧板之间：6px 可拖动分割条（拖动改侧板宽度，和 VS/VSCode 的分栏一样）
        w.addWhiteRect(SPLIT, ED_X - SPLIT_W, ED_Y, SPLIT_W, ED_H)"""
new = """        # 画布与侧板之间：6px 拖动热区（自身不着色）+ 1px 可见分割线。
        # 热区原来是 addWhiteRect：白底 + WS_BORDER，浅色背景上既看不到"线"，
        # 又能看到 1px 深框压在表格左边，看着就像表格画面越过了分割线。
        w.addPanel(SPLIT, ED_X - SPLIT_W, ED_Y, SPLIT_W, ED_H)
        w.addStaticLine(L_SPLIT, ED_X - 1, ED_Y, 2, ED_H)"""
assert s.count(old) == 1
s = s.replace(old, new)

# 3) 布局：线跟分割条一起走
old = "            (SPLIT, ed_x - SPLIT_W, ed_y, SPLIT_W, ed_h, True),\n"
new = "            (SPLIT, ed_x - SPLIT_W, ed_y, SPLIT_W, ed_h, True),\n            (L_SPLIT, ed_x - 1, ed_y, 2, ed_h, True),\n"
assert s.count(old) == 1
s = s.replace(old, new)

# 4) 拖动期不搬和宽度无关的区块（状态栏/小地图只跟 cw/ch 有关）
s = s.replace("""    def _layout(self):""", """    def _layout(self, drag=False):""")
old = """        st_w = max(240, min(ST_W, cv_w - 8))
        st_y = ch - 10 - ST_H
        items += [(ST_PANEL, ST_X, st_y, st_w, ST_H, True),
                  (L_STATUS, ST_X + 4, st_y + 4, st_w - 8, 18, True)]

        # 小地图：右 + 底 锚定（HTML .mini right 12 / bottom 10）
        mfx = cw - 12 - MI_FRAME[2]
        mfy = ch - 10 - MI_FRAME[3]
        items += [(MI_PANEL, mfx, mfy, MI_FRAME[2], MI_FRAME[3], True),
                  (MINI, mfx + 4, mfy + 4, MI_CANVAS[2], MI_CANVAS[3], True)]
"""
new = """        if not drag:                     # 拖动分割条时 cw/ch 没变，这两块不用每帧搬
            st_w = max(240, min(ST_W, cv_w - 8))
            st_y = ch - 10 - ST_H
            items += [(ST_PANEL, ST_X, st_y, st_w, ST_H, True),
                      (L_STATUS, ST_X + 4, st_y + 4, st_w - 8, 18, True)]

            # 小地图：右 + 底 锚定（HTML .mini right 12 / bottom 10）
            mfx = cw - 12 - MI_FRAME[2]
            mfy = ch - 10 - MI_FRAME[3]
            items += [(MI_PANEL, mfx, mfy, MI_FRAME[2], MI_FRAME[3], True),
                      (MINI, mfx + 4, mfy + 4, MI_CANVAS[2], MI_CANVAS[3], True)]
"""
assert s.count(old) == 1
s = s.replace(old, new)

# 5) 拖动回调：30fps 合并 + 松开时补最后一帧
old = """        if phase not in ('move', 'up'):
            return
        cw, _ = self._client_size()
        lo, hi = self._side_bounds(cw)
        new_w = max(lo, min(hi, cw - int(x) - SPLIT_W))
        if new_w != self._ed_w:
            self._ed_w = new_w
            self._layout()"""
new = """        if phase not in ('move', 'up'):
            return
        # 合并重排：一次拖动能来上百个 move，每个都整窗重排 = 每帧 30~80ms。
        # 压到 ~30fps，松手那一下必须补最后一帧，否则停在旧位置。
        self._split_tip = int(x)
        now = time.perf_counter()
        if phase == 'move' and (now - getattr(self, '_split_t', 0.0)) < 0.033:
            return
        self._split_t = now
        cw, _ = self._client_size()
        lo, hi = self._side_bounds(cw)
        new_w = max(lo, min(hi, cw - int(x) - SPLIT_W))
        if new_w != self._ed_w:
            self._ed_w = new_w
            self._layout(drag=(phase == 'move'))"""
assert s.count(old) == 1
s = s.replace(old, new)

# 6) 详情浮层：显隐 + 位置一次重排，不再 layout 两遍
old = """    def show_detail(self):
        self._detail_visible = True
        self._layout()                 # 先按当前画布算出右上角位置，再显示
        for i in self._detail_ids:
            self.win.setVisible(i, True)
            self.win.bringToFront(i)"""
new = """    def show_detail(self):
        # _layout 里 detail 那几条已经带 show 标志，位置和显隐一次做完；
        # 原来这里 _layout() 之后再 position_detail() 又调用一次 _layout()，
        # 等于每次点节点都整窗重排两遍，这就是"属性弹窗慢"的由来。
        self._detail_visible = True
        self._layout()
        ids = self._detail_ids
        if hasattr(self.win, 'raiseMany'):
            self.win.raiseMany(ids)    # 一次批量置顶，避免 O(n²) 逐个 bringToFront
        else:
            for i in ids:
                self.win.bringToFront(i)"""
assert s.count(old) == 1
s = s.replace(old, new)

old = """        win.setText(DT_TIP, tip)
        self.show_detail()
        self.position_detail()"""
new = """        win.setText(DT_TIP, tip)
        self.show_detail()"""
assert s.count(old) == 1
s = s.replace(old, new)

io.open(p, 'w', encoding='utf-8', newline='').write(s)
print('pp_app: 分割线可见 + 拖动30fps + 详情单次重排')
