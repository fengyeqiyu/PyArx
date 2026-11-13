import io
P = r'G:\PyArx\PyMfcXaml\PyMfcXaml.cpp'
t = io.open(P, encoding='utf-8-sig').read()
print('WM_PAINT 出现次数:', t.count('if (msg == WM_PAINT)'))
print('BeginPaint(hwnd, &ps) 次数:', t.count('HDC hdc = ::BeginPaint(hwnd, &ps);'))
i = t.find('if (msg == WM_PAINT)')
print(repr(t[i-6:i+90]))
