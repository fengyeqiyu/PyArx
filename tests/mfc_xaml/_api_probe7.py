import os, sys
sys.path.insert(0, r'G:\PyArx\X64\Release')
os.add_dll_directory(r'G:\PyArx\X64\Release')
import PyMfcXaml as M
for n in ('setVisible','getVisible','setEnabled','move'):
    print('---',n); print((getattr(M.Window,n).__doc__ or '').strip()[:180])
