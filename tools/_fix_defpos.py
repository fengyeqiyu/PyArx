import io
# --- C++ ---
P = r'G:\PyArx\PyMfcXaml\PyMfcXaml.cpp'
t = io.open(P, encoding='utf-8-sig').read()
bad = '''        .def("getPlanSelection", &PyMfcWindow::getPlanSelection,
        .def("saveControlPng", &PyMfcWindow::saveControlPng,
             "把控件按当前外观导出成 PNG（截图取证）")
             "当前选中的卡片编号（未选中返回空串）")'''
good = '''        .def("getPlanSelection", &PyMfcWindow::getPlanSelection,
             "当前选中的卡片编号（未选中返回空串）")
        .def("saveControlPng", &PyMfcWindow::saveControlPng,
             "把控件按当前外观导出成 PNG（截图取证）")'''
assert t.count(bad) == 1
t = t.replace(bad, good)
io.open(P, 'w', encoding='utf-8-sig', newline='').write(t)
print('C++ def 位置修正')

# --- Python ---
P2 = r'G:\PyArx\tests\mfc_xaml\pp_app.py'
s = io.open(P2, encoding='utf-8-sig').read()
badpy = '''        w.onClick(CV, self.on_canvas_click)
        # 单击：C++ 画布做完命中（卡片或连线）后回调，Python 取回选中编号弹详情
w.onClick(CV, self.on_canvas_click)
w.onDoubleClick(CV, self.on_canvas_dblclick)'''
goodpy = '''        # 单击：C++ 画布做完命中（卡片或连线）后回调，Python 取回选中编号弹详情
        w.onClick(CV, self.on_canvas_click)
        w.onDoubleClick(CV, self.on_canvas_dblclick)'''
assert s.count(badpy) == 1
s = s.replace(badpy, goodpy)
io.open(P2, 'w', encoding='utf-8-sig', newline='').write(s)
print('Python 缩进修正')
