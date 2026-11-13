import io
# --- C++：拖动中显式设四向箭头 ---
P = r'G:\PyArx\PyMfcXaml\PyMfcXaml.cpp'
t = io.open(P, encoding='utf-8-sig').read()
old = """            if (st.moved)
            {
                st.ox = st.downOx + dx;
                st.oy = st.downOy + dy;
                ::InvalidateRect(hwnd, nullptr, FALSE);
            }"""
new = """            if (st.moved)
            {
                st.ox = st.downOx + dx;
                st.oy = st.downOy + dy;
                ::SetCursor(::LoadCursorW(nullptr, IDC_SIZEALL));
                ::InvalidateRect(hwnd, nullptr, FALSE);
            }"""
assert t.count(old) == 1
t = t.replace(old, new)
io.open(P, 'w', encoding='utf-8-sig', newline='').write(t)

# --- pp_grid_test：F2 改走原地编辑 ---
G = r'G:\PyArx\tests\mfc_xaml\pp_grid_test.py'
s = io.open(G, encoding='utf-8').read()
old = """ok('F2 打开行编辑条', app._row_edit_open and app._row_edit_row >= 0,
   'row=%s open=%s' % (app._row_edit_row, app._row_edit_open))
r0 = app._row_edit_row
old_code = app.win.getText(pp_app.E_F0)
app.win.setText(pp_app.E_F0, 'X1-TEST')
app.on_row_ok()"""
new = """ok('F2 进入原地编辑', bool(app._ce) and app._ce[0] == 3, repr(app._ce))
r0 = app._ce[0] if app._ce else 3
eid0 = app._ce[2] if app._ce else pp_app.E_F0
old_code = app.win.getText(eid0)
app.win.setText(eid0, 'X1-TEST')
app._cell_end(commit=True)"""
assert s.count(old) == 1
s = s.replace(old, new)

old = """app.win.setText(pp_app.E_F0, old_code)
app._open_row_edit(r0)
app.on_row_ok()
M.pump(60)"""
new = """app._cell_begin(r0, 0)
app.win.setText(pp_app.E_F0, old_code)
app._cell_end(commit=True)
M.pump(60)"""
assert s.count(old) == 1
s = s.replace(old, new)
io.open(G, 'w', encoding='utf-8').write(s)

# --- pp_event_test：F2 断言改成原地编辑 ---
E = r'G:\PyArx\tests\mfc_xaml\pp_event_test.py'
s = io.open(E, encoding='utf-8').read()
old = """chk('发事件 F2 -> 行编辑条展开', bool(app._row_edit_open), 'row=%s' % row)
app.on_row_cancel()
M.pump(100)
chk('行编辑条可收起', not app._row_edit_open, '')"""
new = """chk('发事件 F2 -> 原地编辑开启', bool(app._ce), 'row=%s ce=%s' % (row, app._ce))
app._cell_end(commit=False)
M.pump(100)
chk('原地编辑可取消', app._ce is None, repr(app._ce))"""
assert s.count(old) == 1
s = s.replace(old, new)
io.open(E, 'w', encoding='utf-8').write(s)
print('C++ 拖动光标 + 两个测试套件改为原地编辑语义')
