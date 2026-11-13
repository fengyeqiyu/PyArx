import io
P = r'G:\PyArx\tools\_add_grid_keys.py'
s = io.open(P, encoding='utf-8').read()
old = '''anchor = ("static LRESULT PyMfcPlanCanvasMouse(PyMfcHost* host, HWND hwnd, UINT msg,\\n"
          "                                    WPARAM wParam, LPARAM lParam, bool& handled)")'''
new = '''anchor = ("static LRESULT PyMfcPlanCanvasMouse(PyMfcHost* host, HWND hwnd, UINT msg,\\n"
          "                                    WPARAM wParam, LPARAM lParam, bool& handled)\\n{")'''
assert s.count(old) == 1
s = s.replace(old, new)
io.open(P, 'w', encoding='utf-8').write(s)
print('ok')
