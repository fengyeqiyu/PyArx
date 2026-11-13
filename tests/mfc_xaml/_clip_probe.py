import os, sys
sys.path.insert(0, r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
os.add_dll_directory(r'G:\PyArx\X64\Release')
import pp_app
t = pp_app.clipboardText()
print('剪贴板长度 =', len(t))
print('前 60 字符 =', t[:60])
