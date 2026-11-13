import io
P = r'G:\PyArx\PyMfcXaml\PyMfcXaml.cpp'
t = io.open(P, encoding='utf-8-sig').read()
n = t.count('pyMfcSkinOldProcs()')
t = t.replace('pyMfcSkinOldProcs()', 'skinOldProcs()')
io.open(P, 'w', encoding='utf-8-sig', newline='').write(t)
print('修正 %d 处 skinOldProcs' % n)
