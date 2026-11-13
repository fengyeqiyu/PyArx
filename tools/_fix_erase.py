import io
P = r'G:\PyArx\PyMfcXaml\PyMfcXaml.cpp'
t = io.open(P, encoding='utf-8-sig').read()
old = """    if (msg == WM_ERASEBKGND)
    {
        // 不能直接 return 1（声明"已擦除"却没擦）：控件按 XAML 圆角绘制，
        // 圆角外的四个角落会漏出未擦除的脏像素，白底页上就是一圈深色描边。
        // 这里用父窗口按声明给出的背景刷真正填满整个客户区。
        HDC hdc = reinterpret_cast<HDC>(wParam);"""
new = """    if (msg == WM_ERASEBKGND)
    {
        // 原生控件（SysListView32 等）自己擦背景，且带 LVS_EX_DOUBLEBUFFER 双缓冲。
        // 若这里用父背景刷整块填满，每次选中/滚动都先铺一遍白底再逐行画回去，
        // 屏幕上就是"选中一行时整块闪一下、旧行还留着"的拖影 —— 交回原生过程。
        wchar_t eraseCls[64] = {};
        ::GetClassNameW(hwnd, eraseCls, 64);
        if (isNativePaintClass(eraseCls))
        {
            WNDPROC oldProc = reinterpret_cast<WNDPROC>(::GetPropW(hwnd, kOldProcProp));
            if (oldProc != nullptr)
                return ::CallWindowProcW(oldProc, hwnd, msg, wParam, lParam);
            return ::DefWindowProcW(hwnd, msg, wParam, lParam);
        }
        // 不能直接 return 1（声明"已擦除"却没擦）：控件按 XAML 圆角绘制，
        // 圆角外的四个角落会漏出未擦除的脏像素，白底页上就是一圈深色描边。
        // 这里用父窗口按声明给出的背景刷真正填满整个客户区。
        HDC hdc = reinterpret_cast<HDC>(wParam);"""
assert t.count(old) == 1
t = t.replace(old, new)
io.open(P, 'w', encoding='utf-8-sig', newline='').write(t)
print('原生控件背景擦除交回原过程')
