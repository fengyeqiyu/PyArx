import io
d = r'G:\PyArx\PyMfcXaml'
p = d + r'\PyMfcCommon.h'
s = io.open(p, encoding='utf-8').read()
s = s.replace('static LRESULT CALLBACK PyMfcGridProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);',
              'LRESULT CALLBACK PyMfcGridProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);')
s = s.replace('static LRESULT PyMfcPlanCanvasMouse(PyMfcHost* host, HWND hwnd, UINT msg,',
              'LRESULT PyMfcPlanCanvasMouse(PyMfcHost* host, HWND hwnd, UINT msg,')
io.open(p, 'w', encoding='utf-8', newline='').write(s)

p = d + r'\PyMfcProcs.cpp'
s = io.open(p, encoding='utf-8-sig').read()
s = s.replace('static LRESULT CALLBACK PyMfcGridProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)\n',
              'LRESULT CALLBACK PyMfcGridProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)\n')
s = s.replace('static LRESULT PyMfcPlanCanvasMouse(', 'LRESULT PyMfcPlanCanvasMouse(')
io.open(p, 'w', encoding='utf-8-sig', newline='').write(s)
print('已改为外部链接')
