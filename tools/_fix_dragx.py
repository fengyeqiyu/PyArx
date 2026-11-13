import io
p = 'G:/PyArx/PyMfcXaml/PyMfcHost.cpp'
s = io.open(p, encoding='utf-8-sig').read()
old = """    case WM_LBUTTONDOWN:
        ::SetCapture(hwnd);
        if (host != nullptr)
            host->fireDrag(id, "down", GET_X_LPARAM(lParam));
        return 0;
    case WM_MOUSEMOVE:
        if (::GetCapture() == hwnd && host != nullptr)
            host->fireDrag(id, "move", GET_X_LPARAM(lParam));
        return 0;
    case WM_LBUTTONUP:
        if (host != nullptr)
            host->fireDrag(id, "up", GET_X_LPARAM(lParam));"""
new = """    case WM_LBUTTONDOWN:
        ::SetCapture(hwnd);
        if (host != nullptr)
            host->fireDrag(id, "down", DragScreenX(hwnd, lParam));
        return 0;
    case WM_MOUSEMOVE:
        if (::GetCapture() == hwnd && host != nullptr)
            host->fireDrag(id, "move", DragScreenX(hwnd, lParam));
        return 0;
    case WM_LBUTTONUP:
        if (host != nullptr)
            host->fireDrag(id, "up", DragScreenX(hwnd, lParam));"""
assert old in s
s = s.replace(old, new)

anchor = """LRESULT CALLBACK PyMfcHost::PyMfcDragBarProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{"""
helper = """// 拖动回调传给 Python 的横坐标用**屏幕坐标**，不用控件客户坐标：
// 拖动过程中分割条自己会移位（每帧重排），客户坐标的零点跟着走，
// 同一个屏幕点在不同帧会映射出不同的 x，回调按 x 差分就永远对不齐。
// 屏幕坐标在整个拖动过程中是不变的参照系。
static int DragScreenX(HWND hwnd, LPARAM lParam)
{
    POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
    ::ClientToScreen(hwnd, &pt);
    return pt.x;
}

LRESULT CALLBACK PyMfcHost::PyMfcDragBarProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{"""
assert anchor in s
s = s.replace(anchor, helper, 1)
io.open(p, 'w', encoding='utf-8-sig', newline='').write(s)
print('拖动回调改为屏幕坐标')
