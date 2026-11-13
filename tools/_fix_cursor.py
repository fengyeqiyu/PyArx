import io
p = 'G:/PyArx/PyMfcXaml/PyMfcProcs.cpp'
s = io.open(p, encoding='utf-8-sig').read()

# 1) 画布内不再区分拖动：统一手形（用户规格：画布上手形，画布外普通箭头）
old_drag1 = """        ::SetCursor(::LoadCursorW(nullptr, IDC_SIZEALL));
        handled = true;
        return 0;
    }
    case WM_MOUSEMOVE:"""
new_drag1 = """        ::SetCursor(::LoadCursorW(nullptr, IDC_HAND));
        handled = true;
        return 0;
    }
    case WM_MOUSEMOVE:"""
assert s.count(old_drag1) == 1, s.count(old_drag1)
s = s.replace(old_drag1, new_drag1)

old_drag2 = """                st.oy = st.downOy + dy;
                ::SetCursor(::LoadCursorW(nullptr, IDC_SIZEALL));"""
new_drag2 = """                st.oy = st.downOy + dy;
                ::SetCursor(::LoadCursorW(nullptr, IDC_HAND));"""
assert s.count(old_drag2) == 1
s = s.replace(old_drag2, new_drag2)

# 2) 悬浮时登记 MOUSELEAVE，离开画布把光标复位（宿主不会替子窗口复位，会一直挂手形）
old_hover = """        ::SetCursor(::LoadCursorW(nullptr, IDC_HAND));
        handled = true;
        return 0;
    }
    case WM_LBUTTONUP:"""
new_hover = """        // 登记 WM_MOUSELEAVE：鼠标离开画布时必须自己复位成箭头，
        // 否则最后设的手形会一直挂着（宿主不会替子窗口复位光标）。
        TRACKMOUSEEVENT tme = {};
        tme.cbSize = sizeof(tme);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = hwnd;
        ::TrackMouseEvent(&tme);
        ::SetCursor(::LoadCursorW(nullptr, IDC_HAND));
        handled = true;
        return 0;
    }
    case WM_MOUSELEAVE:
        ::SetCursor(::LoadCursorW(nullptr, IDC_ARROW));
        handled = true;
        return 0;
    case WM_LBUTTONUP:"""
assert s.count(old_hover) == 1
s = s.replace(old_hover, new_hover)

# 3) WM_SETCURSOR 统一手形
old_sc = """    case WM_SETCURSOR:
        // 画布内悬浮显示手形（和网页 canvas 的 cursor:grab 一致），拖动中给四向箭头
        ::SetCursor(::LoadCursorW(nullptr, st.drag ? IDC_SIZEALL : IDC_HAND));"""
new_sc = """    case WM_SETCURSOR:
        // 画布内始终手形（和网页 canvas 的 cursor:grab 一致）；拖动中也保持手形，
        // 离开画布由 WM_MOUSELEAVE 复位成箭头。
        ::SetCursor(::LoadCursorW(nullptr, IDC_HAND));"""
assert s.count(old_sc) == 1
s = s.replace(old_sc, new_sc)

io.open(p, 'w', encoding='utf-8-sig', newline='').write(s)
print('画布光标：拖动/悬浮统一手形 + WM_MOUSELEAVE 复位箭头')
print('残留 SIZEALL =', s.count('IDC_SIZEALL'))
