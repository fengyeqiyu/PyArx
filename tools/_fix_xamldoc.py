import io
P = r'G:\PyArx\PyMfcXaml\PyMfcXaml.cpp'
t = io.open(P, encoding='utf-8-sig').read()
bad = '''          "读 XAML 按客户区尺寸算布局，返回 {x:Name: (x, y, w, h)}；
"
          "  overrides 可运行时改行列尺寸，键 'Root/cols/3'、值 '640'");'''
good = '''          "读 XAML 按客户区尺寸算布局，返回 {x:Name: (x, y, w, h)}；overrides 可运行时改行列尺寸");'''
assert t.count(bad) == 1
t = t.replace(bad, good)
io.open(P, 'w', encoding='utf-8-sig', newline='').write(t)
print('docstring 修好')
