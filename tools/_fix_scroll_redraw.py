import io
# 1) 重绘改异步，避免同步整窗刷新
P = r'G:\PyArx\PyMfcXaml\PyMfcXaml.cpp'
t = io.open(P, encoding='utf-8-sig').read()
old = "        ::RedrawWindow(m_hWnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);"
new = "        // 异步标脏：让系统在消息循环里合并成一次重画，同步 UPDATE 反而更容易看到白闪\n        ::RedrawWindow(m_hWnd, nullptr, nullptr, RDW_INVALIDATE);"
assert t.count(old) == 1
t = t.replace(old, new)
io.open(P, 'w', encoding='utf-8-sig', newline='').write(t)

# 2) 测试脚本截图前把窗口提到前台
P2 = r'G:\PyArx\tests\mfc_xaml\pp_scroll_test.py'
s = io.open(P2, encoding='utf-8-sig').read()
old_fn = '''def wheel(hwnd, cx, cy, up):'''
new_fn = '''def front(hwnd):
    """savePng 抓的是屏幕，窗口被别的程序压住就会抓到别人，截图前先提到前台。"""
    user32.SetForegroundWindow.argtypes = [wt.HWND]
    user32.SetForegroundWindow(hwnd)
    user32.SetWindowPos.argtypes = [wt.HWND, wt.HWND, ctypes.c_int, ctypes.c_int,
                                    ctypes.c_int, ctypes.c_int, wt.UINT]
    user32.SetWindowPos(hwnd, None, 60, 40, 0, 0, 0x0001 | 0x0004)   # NOSIZE|NOZORDER
    time.sleep(0.35)


def wheel(hwnd, cx, cy, up):'''
assert s.count(old_fn) == 1
s = s.replace(old_fn, new_fn, 1)
s = s.replace("    app.win.savePng(os.path.join(DOCS, '_pp_scroll_mid.png'))",
              "    front(hwnd)\n    app.win.savePng(os.path.join(DOCS, '_pp_scroll_mid.png'))")
s = s.replace("    app.win.savePng(os.path.join(DOCS, '_pp_scroll_end.png'))",
              "    front(hwnd)\n    app.win.savePng(os.path.join(DOCS, '_pp_scroll_end.png'))")
io.open(P2, 'w', encoding='utf-8-sig', newline='').write(s)
print('ok')
