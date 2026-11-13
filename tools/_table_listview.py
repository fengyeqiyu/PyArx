import io, re
P = r'G:\PyArx\tests\mfc_xaml\pp_app.py'
t = io.open(P, encoding='utf-8-sig').read()


def span(name):
    i = t.index('\n    def %s(' % name) + 1
    j = len(t)
    for pat in ('\n    def ', '\n\nclass ', '\n\n# ---', '\n\ndef '):
        k = t.find(pat, i + 1)
        if k != -1:
            j = min(j, k + 1)
    return i, j


def repl(name, code):
    global t
    i, j = span(name)
    if code:
        t = t[:i] + code + t[j:]
    else:
        t = t[:i] + t[j:]


NEW_BUILD_EDITOR = '''    def _build_editor(self):
        """HTML 的 .editor 侧板。
        表格本体就是一个原生 ListView：自带列头、垂直滚动条、滚轮和整块绘制，
        不再用"每格一个 EDIT"去拼表格的样子。要改某一行时双击该行，
        弹出下面这条行编辑条（7 个输入 + 确定/删除/取消），确认后写回对应单元格。
        """
        w = self.win
        w.addWhiteRect(ED_PANEL, ED_X, ED_Y, ED_W, ED_H)
        px = ED_X + ED_PAD
        self._font_ids += [(L_EDIT_TITLE, 13, False), (L_EDIT_COUNT, 13, False),
                           (L_EDIT_FOOT, 12, False)]
        w.addLabel(L_EDIT_TITLE, '节点数据', px, ED_Y + 12, 120, 20)
        w.addLabel(L_EDIT_COUNT, '', px + 78, ED_Y + 12, 100, 20)
        bx = ED_X + ED_W - ED_PAD - 72
        w.addButton(B_CLOSE, '关闭', bx, ED_Y + 8, 72, 26)
        w.addButton(B_APPLY, '应用并重绘', bx - 100, ED_Y + 8, 96, 26)
        w.addButton(B_ADDROW, '＋ 新增节点', bx - 196, ED_Y + 8, 92, 26)
        self._font_ids += [(B_CLOSE, 13, False), (B_APPLY, 13, False), (B_ADDROW, 13, False)]

        # 表格：原生 ListView（列头 = HTML thead，行超出自动出滚动条）
        w.addListCtrl(TBL, px, ED_Y + ED_HEAD_H + 2, ED_W - 2 * ED_PAD, 300)
        w.setControlFont(TBL, 'Microsoft YaHei', 12, False)
        for i, name in enumerate(COLS):
            w.addColumn(TBL, name, COLW[i])

        # 行编辑条（双击行才出现）
        for i in range(7):
            w.addEdit(E_F0 + i, '', px, ED_Y, 1, 1)
            self._font_ids.append((E_F0 + i, 12, False))
        w.addButton(B_LOADROW, '确定', px, ED_Y, 72, 26)
        w.addButton(B_APPLYROW, '删除', px + 76, ED_Y, 72, 26)
        w.addButton(B_DELROW, '取消', px + 152, ED_Y, 72, 26)
        self._font_ids += [(B_LOADROW, 12, False), (B_APPLYROW, 12, False), (B_DELROW, 12, False)]

        w.addLabel(L_EDIT_FOOT, '上游填现有编号即挂到该节点下；留空为起点。双击某行改值，确定后点「应用并重绘」。',
                   px, ED_Y + ED_H - 30, ED_W - 2 * ED_PAD, 20)

        w.onClick(B_ADDROW, self.on_add_row)
        w.onClick(B_APPLY, self.on_apply_all)
        w.onClick(B_CLOSE, self.on_close_editor)
        w.onDoubleClick(TBL, self.on_grid_dbl)
        w.onClick(B_LOADROW, self.on_row_ok)
        w.onClick(B_APPLYROW, self.on_row_del)
        w.onClick(B_DELROW, self.on_row_cancel)

'''
repl('_build_editor', NEW_BUILD_EDITOR)

NEW_PANEL_IDS = '''    def _panel_ids(self):
        return ([ED_PANEL, L_EDIT_TITLE, L_EDIT_COUNT, B_ADDROW, B_APPLY, B_CLOSE,
                 L_EDIT_FOOT, TBL] + [E_F0 + i for i in range(7)] +
                [B_LOADROW, B_APPLYROW, B_DELROW])

'''
repl('_panel_ids', NEW_PANEL_IDS)

NEW_SHOW = '''    def show_editor(self):
        """HTML 的 .editor：贴右 640 宽。画布让出这块宽度，其余布局由 _layout 统一算。"""
        self.editing = True
        self._row_edit_open = False
        self.fill_table()
        self._layout()
        for i in self._panel_ids():
            self.win.setVisible(i, True)
        self.win.raiseMany(self._panel_ids())
        self._row_edit_close()
        self.win.setText(L_EDIT_COUNT, '共 %d 行' % len(self.nodes))

'''
repl('show_editor', NEW_SHOW)

NEW_HIDE = '''    def hide_editor(self):
        self.editing = False
        self._row_edit_open = False
        self._row_edit_close()
        for i in self._panel_ids():
            self.win.setVisible(i, False)
        self._layout()
        self.win.bringToFront(CV)
        self.win.bringToFront(MI_PANEL)
        self.win.bringToFront(MINI)
        self.win.bringToFront(ST_PANEL)
        self.win.bringToFront(L_STATUS)
        self.position_detail()
        if self.cur_sel:
            self.show_detail()

'''
repl('hide_editor', NEW_HIDE)

for dead in ('_grid_geom', '_grid_body_rect', '_grid_bind_slots', '_grid_flush_slots',
             'on_edit_wheel', 'on_grid_delete_slot', '_grid_panel_ids',
             'on_load_row', 'on_apply_row'):
    repl(dead, '')

NEW_FILL = '''    def fill_table(self):
        """把 nodes 全量灌进原生 ListView。滚动、行高、列头都由控件自己管。"""
        w = self.win
        w.clearTableRows(TBL)
        for node in self.nodes:
            vals = [self._grid_text(node, k) for k in FIELDS] + [u'\\u2715']
            w.addTableRow(TBL, u'\\t'.join(vals))
        self._grid_rows = len(self.nodes)

'''
repl('fill_table', NEW_FILL)

NEW_COLLECT = '''    def collect_grid(self):
        """从 ListView 逐格读回全部行；返回 (nodes, 错误文案)。"""
        w = self.win
        out = []
        for i in range(w.getTableRowCount(TBL)):
            vals = [(w.getTableCell(TBL, i, j) or '').strip() for j in range(7)]
            code = vals[0]
            if not code:
                return None, '第 %d 行：编号不能为空' % (i + 1)
            for k, lab in ((4, '负载'), (6, '长度')):
                try:
                    f = float(vals[4] or 0) if k == 4 else float(vals[6] or 0)
                except ValueError:
                    return None, '第 %d 行：%s必须是不小于 0 的数字' % (i + 1, lab)
                if f < 0:
                    return None, '第 %d 行：%s必须是不小于 0 的数字' % (i + 1, lab)
            if vals[2] == code:
                return None, '第 %d 行：上游不能是节点自己' % (i + 1)
            out.append(dict(code=code, name=vals[1], upstream=vals[2], cable=vals[3],
                            loadKw=float(vals[4] or 0), voltage=vals[5] or '380',
                            length=float(vals[6] or 0)))
        return out, ''

'''
repl('collect_grid', NEW_COLLECT)

NEW_ADD = '''    def on_add_row(self):
        """HTML edAdd：新增一行（编号 N1/N2… 递增，上游留空为起点），并直接打开编辑条。"""
        codes = set(n['code'] for n in self.nodes)
        i = 1
        while ('N%d' % i) in codes:
            i += 1
        code = 'N%d' % i
        self.nodes.append(dict(code=code, name='', upstream='', cable='',
                               loadKw=0.0, voltage='380', length=0.0))
        self.cur_sel = code
        self.fill_table()
        self.rebuild(fit=False)
        self.win.selectPlan(CV, code)
        self.fill_detail(code)
        r = len(self.nodes) - 1
        self.win.setTableSelectedRow(TBL, r)
        self._open_row_edit(r)

'''
repl('on_add_row', NEW_ADD)

NEW_DEL = '''    def on_del_row(self):
        r = self.win.getTableSelectedRow(TBL)
        if r < 0 or r >= len(self.nodes):
            self.win.setText(L_EDIT_COUNT, '先在表格里点一行')
            return
        self.on_grid_delete(self.nodes[r]['code'])

'''
repl('on_del_row', NEW_DEL)

NEW_APPLY_ALL = '''    def on_apply_all(self):
        """HTML edApply：收回表格里每一行的值，校验后重绘。"""
        nodes, err = self.collect_grid()
        if err:
            self.win.setText(L_EDIT_COUNT, err)
            self.win.setText(L_STATUS, err)
            return
        self.nodes = nodes
        self._row_edit_close()
        self.rebuild(fit=True)
        self.fill_table()
        self.win.setText(L_EDIT_COUNT, '已按当前数据重绘')

'''
repl('on_apply_all', NEW_APPLY_ALL)

io.open(P, 'w', encoding='utf-8-sig', newline='').write(t)
print('表格改 ListView 完成')
