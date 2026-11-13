import io
p='G:/PyArx/tests/mfc_xaml/pp_app.py'
s=io.open(p,encoding='utf-8').read()
old="        w.onDrag(SPLIT, self.on_split_drag)"
new="""        w.onDrag(SPLIT, self.on_split_drag)
        # z 序：分割条必须压在表格/编辑框之上。子窗口是谁后建谁在上，
        # 而表格比分割条晚建 -> 表格边缘会盖住分割条，肉眼看就是"表格越过了分割线"。
        # 建完所有控件后统一把分割条提到最前（逐个 bringToFront 是 O(n²)，用 raiseMany）。
        w.raiseMany([SPLIT])"""
assert s.count(old)==1
s=s.replace(old,new,1)
io.open(p,'w',encoding='utf-8',newline='').write(s)
print('split 提到 z 序最前')
