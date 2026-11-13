import os, sys
sys.path.insert(0, r'G:\PyArx\X64\Release')
os.add_dll_directory(r'G:\PyArx\X64\Release')
import PyMfcXaml as M
for n in ('addString','addChoice','setComboItems','setChoiceItems','addListItem','setText','setTableCell','getTableCell','getTableSelectedRow','setTableSelectedRow','clearTableRows','setComboSelected','getComboSelected','getChoiceSelected','setEditReadOnly','setReadOnly','setSelection'):
    f = getattr(M.Window, n, None)
    print('---', n, 'MISSING' if f is None else '')
    if f is not None: print((f.__doc__ or '').strip()[:200])
