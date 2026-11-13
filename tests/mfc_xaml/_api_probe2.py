import os, sys
sys.path.insert(0, r'G:\PyArx\X64\Release')
os.add_dll_directory(r'G:\PyArx\X64\Release')
import PyMfcXaml as M
for n in ('addButton','addEdit','addLabel','addCheckBox','addCombo','addListCtrl','addColumn','addTableRow','addPanel','onClick','onChange','onDoubleClick','setText','getText','show','savePng','setPlanOptions','getPlanView'):
    f = getattr(M.Window, n, None)
    print('---', n)
    print((f.__doc__ or '').strip()[:300])
