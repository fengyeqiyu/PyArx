import os, sys
sys.path.insert(0, r'G:\PyArx\X64\Release'); os.add_dll_directory(r'G:\PyArx\X64\Release')
import PyMfcXaml as M
X = r'G:\PyArx\tests\mfc_xaml\power_plan.xaml'
for over in (None, {'Root/cols/3': '900'}):
    r = M.xamlLayout(X, 1324, 841, over) if over else M.xamlLayout(X, 1324, 841)
    print('overrides=%s' % over)
    for k in sorted(r):
        print('   %-6s %s' % (k, r[k]))
sys.stdout.flush()
os._exit(0)
