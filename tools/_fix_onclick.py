import io
P = r'G:\PyArx\tests\mfc_xaml\pp_app.py'
t = io.open(P, encoding='utf-8-sig').read()
old = "w.onDoubleClick(CV, self.on_canvas_dblclick)"
new = ("# 单击：C++ 画布做完命中（卡片或连线）后回调，Python 取回选中编号弹详情\n"
       "w.onClick(CV, self.on_canvas_click)\n"
       "w.onDoubleClick(CV, self.on_canvas_dblclick)")
assert t.count(old) == 1
t = t.replace(old, new)
io.open(P,'w',encoding='utf-8-sig',newline='').write(t)
print('Python 绑 onClick')
