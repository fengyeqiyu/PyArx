import os, sys
sys.path.insert(0, r'G:\PyArx\X64\Release')
os.add_dll_directory(r'G:\PyArx\X64\Release')
import PyMfcXaml as M
names = [n for n in dir(M.Window)]
print([n for n in names if 'heck' in n or 'alue' in n or 'ombo' in n or 'oggl' in n or 'adio' in n])
