import io
P = r'G:\PyArx\PyMfcXaml\PyMfcXaml.cpp'
t = io.open(P, encoding='utf-8-sig').read()
old = "static // 可拖动分割条：一个 6px 的 owner-draw STATIC"
new = "// 可拖动分割条：一个 6px 的 owner-draw STATIC"
assert t.count(old) == 1
t = t.replace(old, new)
io.open(P, 'w', encoding='utf-8-sig', newline='').write(t)
print('去掉重复 static')
