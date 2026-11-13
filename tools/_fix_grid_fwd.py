import io
P = r'G:\PyArx\PyMfcXaml\PyMfcXaml.cpp'
t = io.open(P, encoding='utf-8-sig').read()

consts = '''static constexpr const wchar_t* kGridHostProp = L"PyMfcGridHost";
static constexpr const wchar_t* kGridOldProp = L"PyMfcGridOldProc";
'''
assert t.count(consts) >= 1
t = t.replace(consts, '', 1)

decl = """static LRESULT PyMfcPlanCanvasMouse(PyMfcHost* host, HWND hwnd, UINT msg,
                                    WPARAM wParam, LPARAM lParam, bool& handled);"""
assert t.count(decl) == 1
t = t.replace(decl, consts + "\n// 可编辑表格钩子（定义见 PyMfcPlanCanvasMouse 之前）\nstatic LRESULT CALLBACK PyMfcGridProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);\n\n" + decl, 1)

io.open(P, 'w', encoding='utf-8-sig', newline='').write(t)
print('前向声明已加')
