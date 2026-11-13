import os, sys
sys.path.insert(0, r'G:\PyArx\X64\Release')
os.add_dll_directory(r'G:\PyArx\X64\Release')
import PyMfcXaml as M
print([n for n in dir(M.Window) if 'isible' in n or 'nabl' in n or 'ove' in n or 'hown' in n])
print([n for n in dir(M) if n in ('pump','shutdown','guiload')])
