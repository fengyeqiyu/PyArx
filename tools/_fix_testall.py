import io
P = r'G:\PyArx\tests\mfc_xaml\pp_test_all.py'
t = io.open(P, encoding='utf-8-sig').read()

old = """    chk('侧板打开', app.editing and app._grid_rows == 39,
        '%d 行 / %d 控件 / 画布 %dx%d' % (app._grid_rows, len(app._grid_ids),
                                   app.cv_w, app.cv_h))

    # 14 行内改值 -> 应用
    base = pp_app.GRID_BASE + 2 * 8
    w.setText(base + 1, '行内改名')
    app.on_apply_all()
    M.pump(40)
    chk('行内改值并重绘', (app.nodes[2].get('name') == '行内改名'),
        '%s / 画布 %dx%d' % (app.nodes[2].get('name'), app.view['width'], app.view['height']))

    # 15 校验：空编号
    w.setText(base + 0, '')
    app.on_apply_all()
    chk('校验空编号', '编号不能为空' in (w.getText(pp_app.L_EDIT_COUNT) or ''),
        w.getText(pp_app.L_EDIT_COUNT))
    w.setText(base + 0, app.nodes[2]['code'])

    # 16 校验：上游是自己
    w.setText(base + 2, app.nodes[2]['code'])
    app.on_apply_all()
    chk('校验上游自己', '上游不能是节点自己' in (w.getText(pp_app.L_EDIT_COUNT) or ''),
        w.getText(pp_app.L_EDIT_COUNT))
    w.setText(base + 2, app.nodes[2].get('upstream') or '')"""
new = """    chk('侧板打开', app.editing and app._grid_rows == 39,
        '%d 行 / 表格 %d 列 / 画布 %dx%d' % (app._grid_rows,
                                   7, app.cv_w, app.cv_h))

    # 14 表格单元格改值 -> 应用（表格是原生 ListView，数据源在 Python）
    w.setTableCell(pp_app.TBL, 2, 1, '行内改名')
    app.on_apply_all()
    M.pump(40)
    chk('表格改值并重绘', (app.nodes[2].get('name') == '行内改名'),
        '%s / 画布 %dx%d' % (app.nodes[2].get('name'), app.view['width'], app.view['height']))

    # 15 校验：空编号
    w.setTableCell(pp_app.TBL, 2, 0, '')
    app.on_apply_all()
    chk('校验空编号', '编号不能为空' in (w.getText(pp_app.L_EDIT_COUNT) or ''),
        w.getText(pp_app.L_EDIT_COUNT))
    w.setTableCell(pp_app.TBL, 2, 0, app.nodes[2]['code'])

    # 16 校验：上游是自己
    w.setTableCell(pp_app.TBL, 2, 2, app.nodes[2]['code'])
    app.on_apply_all()
    chk('校验上游自己', '上游不能是节点自己' in (w.getText(pp_app.L_EDIT_COUNT) or ''),
        w.getText(pp_app.L_EDIT_COUNT))
    w.setTableCell(pp_app.TBL, 2, 2, app.nodes[2].get('upstream') or '')"""
assert t.count(old) == 1
t = t.replace(old, new)
io.open(P, 'w', encoding='utf-8-sig', newline='').write(t)
print('测试更新')
