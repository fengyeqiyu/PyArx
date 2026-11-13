import io
p = 'G:/PyArx/tests/mfc_xaml/pp_app.py'
s = io.open(p, encoding='utf-8').read()
start = s.index('    def on_split_drag(self, phase, x):')
end = s.index('    def on_resize(self, cw, ch):')
new = '''    def on_split_drag(self, phase, x):
        """分割条拖动。x 是**分割条左边界在窗口客户区里的目标 x**（绝对位置）。

        这个数由控件自己解算：按下时记下自己的左边界与光标屏幕位置，之后每一帧
        只按屏幕位移推出"我现在该站哪"，并钳在父客户区内。上层拿到的就是手势要求的
        最终位置 —— 不做差分、不记基准，重排多少次都不会累积误差。

        侧板右贴窗口，所以 侧板宽 = 客户宽 - (分割条左边界 + 分割条宽)。
        """
        if phase not in ('move', 'up'):
            return
        cw, _ = self._client_size()
        lo, hi = self._side_bounds(cw)
        new_w = max(lo, min(hi, cw - int(x) - SPLIT_W))
        if new_w != self._ed_w:
            self._ed_w = new_w
            self._layout()

'''
s = s[:start] + new + s[end:]
io.open(p, 'w', encoding='utf-8', newline='').write(s)
print('pp_app.on_split_drag 改成"接收绝对边界位置"')
