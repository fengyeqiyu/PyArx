import io
P = r'G:\PyArx\PyMfcXaml\PyMfcXaml.cpp'
t = io.open(P, encoding='utf-8-sig').read()
old = """    PyMfcHost::PlanCanvasState& st = it->second;

    switch (msg)
    {
    case WM_MOUSEWHEEL:"""
new = """    PyMfcHost::PlanCanvasState& st = it->second;

    if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONUP || msg == WM_MOUSEWHEEL)
        debugLogf(L"[plan-mouse] msg=0x%04X pt=(%d,%d) wp=0x%04X drag=%d moved=%d",
                  msg, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam),
                  static_cast<unsigned>(wParam), st.drag ? 1 : 0, st.moved ? 1 : 0);

    switch (msg)
    {
    case WM_MOUSEWHEEL:"""
assert t.count(old) == 1
t = t.replace(old, new)
io.open(P, 'w', encoding='utf-8-sig', newline='').write(t)
print('plan-mouse 日志已加')
