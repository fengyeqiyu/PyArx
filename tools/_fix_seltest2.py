import io
P = r'G:\PyArx\tests\mfc_xaml\pp_select_test.py'
s = io.open(P, encoding='utf-8').read()
s = s.replace("l['bez']", "l['bezier']")
io.open(P,'w',encoding='utf-8').write(s)
print('link 键名修正')
