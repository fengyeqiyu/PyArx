import io
P = r'G:\PyArx\tests\mfc_xaml\pp_app.py'
t = io.open(P, encoding='utf-8-sig').read()
old = """        tbl_y = ed_y + ED_HEAD_H + 2
        tbl_h = max(120, strip_y - 8 - tbl_y)"""
new = """        tbl_y = ed_y + ED_HEAD_H + 2
        # 编辑条没展开时表格一直铺到 footer 上方；展开时给它让出两行
        tbl_bottom = (strip_y - 8) if on else (foot_y - 10)
        tbl_h = max(120, tbl_bottom - tbl_y)"""
assert t.count(old) == 1
t = t.replace(old, new)
io.open(P, 'w', encoding='utf-8-sig', newline='').write(t)
print('表格高度自适应')
