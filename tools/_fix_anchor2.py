import io
P = r'G:\PyArx\tools\_move_grid_members.py'
s = io.open(P, encoding='utf-8').read()
old = '''anchor = "    void setWheelRegion(int x, int y, int w, int h, int step)"'''
new = '''anchor = "    void setWheelRegion(int x, int y, int w, int h, int step)\\n    {"'''
assert s.count(old) == 1
s = s.replace(old, new)
old2 = '''t = t.replace(anchor, block.rstrip('\\n') + "\\n\\n" + anchor, 1)'''
new2 = '''t = t.replace(anchor, block.rstrip('\\n') + "\\n\\n" + anchor, 1)'''
io.open(P, 'w', encoding='utf-8').write(s)
print('ok')
