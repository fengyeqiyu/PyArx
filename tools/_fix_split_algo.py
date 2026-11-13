import io
p = 'G:/PyArx/tests/mfc_xaml/pp_app.py'
s = io.open(p, encoding='utf-8').read()

# 1) 常量区：加上侧板/画布的尺寸约束
old = "SPLIT_W = 6"
new = """SPLIT_W = 6
SPLIT_MIN_W = 360        # 侧板最小宽：7 列表格的最小列宽 + 顶部 3 个按钮排得下
SPLIT_MAX_W = 900        # 侧板最大宽（人的偏好上限）
CV_MIN_W = 320           # 画布最小宽（再小就看不见图了）"""
assert s.count(old) == 1
s = s.replace(old, new)

# 2) 插入 _side_bounds / _side_width（放在 _client_size 之后、_layout 之前）
anchor = """    def _layout(self):
        \"\"\"整窗按网格重排："""
ins = """    def _side_bounds(self, cw=None):
        \"\"\"侧板宽度的合法区间。上限不只由偏好决定，还要给画布留 CV_MIN_W：
        窗口越窄，上限越低；窗口窄到连下限都放不下时退回下限。\"\"\"
        if cw is None:
            cw, _ = self._client_size()
        room = cw - CV_X - 8 - SPLIT_W - CV_MIN_W        # 扣掉画布最小宽后剩下的空间
        lo = SPLIT_MIN_W
        hi = max(lo, min(SPLIT_MAX_W, int(room)))
        return lo, hi

    def _side_width(self, cw=None):
        \"\"\"当前**生效**的侧板宽度 = 用户偏好 _ed_w 在合法区间里钳一次。
        窗口临时变窄只收窄生效宽度、不动偏好，窗口恢复后自动回到偏好宽度。\"\"\"
        lo, hi = self._side_bounds(cw)
        return max(lo, min(hi, int(self._ed_w)))

    def _layout(self):
        \"\"\"整窗按网格重排："""
assert s.count(anchor) == 1
s = s.replace(anchor, ins)

# 3) _layout：画布宽度用生效侧板宽度算
old = """        ed_w = (self._ed_w + SPLIT_W) if self.editing else 0
        cv_w = max(320, cw - CV_X - 8 - ed_w)"""
new = """        ed_w = (self._side_width(cw) + SPLIT_W) if self.editing else 0
        cv_w = max(CV_MIN_W, cw - CV_X - 8 - ed_w)"""
assert s.count(old) == 1
s = s.replace(old, new)

# 4) _layout_editor：侧板宽与左边界同一处推导，保证画布不被挤破
old = """        edw = max(360, min(900, self._ed_w))
        ed_x = max(CV_X + 320 + SPLIT_W, cw - edw)"""
new = """        edw = self._side_width(cw)
        ed_x = cw - edw                                  # 右贴窗口
        if ed_x < CV_X + CV_MIN_W + SPLIT_W:             # 极端窄窗兜底：先保画布最小宽
            ed_x = CV_X + CV_MIN_W + SPLIT_W
            edw = max(240, cw - ed_x)"""
assert s.count(old) == 1
s = s.replace(old, new)

# 5) 拖动算法：绝对目标宽度 + 版式推导的边界
old = '''    def on_split_drag(self, phase, x):
        """分割条拖动：x 是分割条内的横坐标（左移为负），按位移改侧板宽度后整窗重排。"""
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
        self._split_from = None'''
new = '''    def on_split_drag(self, phase, x):
        """分割条拖动。x 是**屏幕横坐标**（C++ 侧 ClientToScreen 换算后传出）。

        为什么不能用控件客户坐标：拖动过程中分割条自己每帧都在移位，客户坐标的
        零点跟着走，同一个屏幕点在相邻两帧算出的 x 不相等，按 x 差分算出来的宽度
        就会漂（实测拖 120px 只走出 94px，clamp 也落不到 360/900）。

        算法本身按**绝对目标宽度**算：按下时记 (base=侧板宽, x0=屏幕 x)，
        之后任何一帧都取 target = base + (x0 - x)；不在上一帧宽度上累加增量，
        增量累加会把重排造成的坐标系漂移滚成误差。
        """
        if phase == 'down':
            self._split_from = (self._side_width(), x)
            return
        if phase == 'move' and getattr(self, '_split_from', None):
            base, x0 = self._split_from
            lo, hi = self._side_bounds()
            target = int(round(base + (x0 - x)))      # 向左拖（x 变小）→ 侧板变宽
            new_w = max(lo, min(hi, target))
            if new_w != self._ed_w:
                self._ed_w = new_w
                self._layout()
            return
        self._split_from = None'''
assert s.count(old) == 1
s = s.replace(old, new)

# 6) view_area 也用生效宽度
old = """        used = (self._ed_w + SPLIT_W) if self.editing else 0"""
new = """        used = (self._side_width() + SPLIT_W) if self.editing else 0"""
assert s.count(old) == 1
s = s.replace(old, new)

io.open(p, 'w', encoding='utf-8', newline='').write(s)
print('pp_app.py 已改：侧板宽度改由版式约束推导，拖动改绝对目标宽度算法')
