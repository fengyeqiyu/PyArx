import io
P = r'G:\PyArx\tests\mfc_xaml\pp_app.py'
t = io.open(P, encoding='utf-8-sig').read()

old = """    def _panel_ids(self):
        return ([ED_PANEL, L_EDIT_TITLE, L_EDIT_COUNT, B_ADDROW, B_APPLY, B_CLOSE,
                 L_EDIT_FOOT, TBL] + [E_F0 + i for i in range(7)] +
                [B_LOADROW, B_APPLYROW, B_DELROW])"""
new = """    def _panel_ids(self):
        \"\"\"侧板常显件（不含行编辑条）。\"\"\"
        return [ED_PANEL, L_EDIT_TITLE, L_EDIT_COUNT, B_ADDROW, B_APPLY, B_CLOSE,
                L_EDIT_FOOT, TBL]

    def _row_edit_ids(self):
        return [E_F0 + i for i in range(7)] + [B_LOADROW, B_APPLYROW, B_DELROW]"""
assert t.count(old) == 1
t = t.replace(old, new)

old2 = """        for i in self._panel_ids():
            self.win.setVisible(i, True)
        self.win.raiseMany(self._panel_ids())
        self._row_edit_close()
        self.win.setText(L_EDIT_COUNT, '共 %d 行' % len(self.nodes))"""
new2 = """        for i in self._panel_ids():
            self.win.setVisible(i, True)
        for i in self._row_edit_ids():
            self.win.setVisible(i, False)
        self.win.raiseMany(self._panel_ids())
        self.win.setText(L_EDIT_COUNT, '共 %d 行' % len(self.nodes))"""
assert t.count(old2) == 1
t = t.replace(old2, new2)

old3 = """    def hide_editor(self):
        self.editing = False
        self._row_edit_open = False
        self._row_edit_close()
        for i in self._panel_ids():
            self.win.setVisible(i, False)"""
new3 = """    def hide_editor(self):
        self.editing = False
        self._row_edit_open = False
        for i in self._panel_ids() + self._row_edit_ids():
            self.win.setVisible(i, False)"""
assert t.count(old3) == 1
t = t.replace(old3, new3)

old4 = """        self._layout()
        ids = [E_F0 + i for i in range(7)] + [B_LOADROW, B_APPLYROW, B_DELROW]
        for i in ids:
            w.setVisible(i, True)
        w.raiseMany(ids)"""
new4 = """        self._layout()
        ids = self._row_edit_ids()
        for i in ids:
            w.setVisible(i, True)
        w.raiseMany(ids)"""
assert t.count(old4) == 1
t = t.replace(old4, new4)

old5 = """    def _row_edit_close(self):
        w = self.win
        if not getattr(self, '_row_edit_open', False):
            return
        self._row_edit_open = False
        for i in [E_F0 + i for i in range(7)] + [B_LOADROW, B_APPLYROW, B_DELROW]:
            w.setVisible(i, False)
        w.setText(L_EDIT_COUNT, '共 %d 行' % len(self.nodes))"""
new5 = """    def _row_edit_close(self):
        w = self.win
        was_open = getattr(self, '_row_edit_open', False)
        self._row_edit_open = False
        for i in self._row_edit_ids():
            w.setVisible(i, False)
        if was_open:
            w.setText(L_EDIT_COUNT, '共 %d 行' % len(self.nodes))"""
assert t.count(old5) == 1
t = t.replace(old5, new5)
io.open(P, 'w', encoding='utf-8-sig', newline='').write(t)
print('编辑条显隐修正')
