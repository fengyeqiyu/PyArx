import io
p = 'G:/PyArx/PyMfcXaml/PyMfcHost.cpp'
s = io.open(p, encoding='utf-8-sig').read()

old_start = s.index('// 拖动回调传给 Python 的横坐标用**屏幕坐标**')
old_end = s.index('LRESULT CALLBACK PyMfcHost::PyMfcDragBarProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)')
helper = '''// 分割条把"拖动"整个当成**控件自己的状态机**：
//   down 记下自己的左边界 + 光标屏幕位置；move 只按屏幕位移推出"我现在应该站哪"；
//   对外只报一个数——**分隔条左边界在父客户区里的 x**（绝对位置）。
// 这样上层不必知道坐标系、不必记按下时的基准、更不必做差分：
// 每次回调给的都是"当前手势要求的最终位置"，重排多少次都不会累积误差。
// 之前把控件的客户坐标（或屏幕坐标）交给上层去差分，本质上就是把控件的内部状态
// 丢给了使用者，拖动中控件移位就会漂——这是控件自身的逻辑缺陷，不是调用方的问题。
struct DragBarDrag
{
    bool  active = false;
    POINT downScreen = { 0, 0 };   // 按下时的光标屏幕位置
    int   downLeft = 0;            // 按下时控件左边界（父客户坐标）
    int   parentW = 0;             // 按下时父客户区宽度（拖动中不变）
    int   barW = 0;                // 控件自身宽度
};

static std::map<HWND, DragBarDrag>& dragBarState()
{
    static std::map<HWND, DragBarDrag> s_map;
    return s_map;
}

// 手势 → 目标左边界（父客户坐标）。钳在父客户区里，控件永远拖不出窗口。
static int DragBarTargetLeft(HWND hwnd, LPARAM lParam, const DragBarDrag& st)
{
    POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
    ::ClientToScreen(hwnd, &pt);
    int left = st.downLeft + (pt.x - st.downScreen.x);
    const int maxLeft = st.parentW - st.barW;
    if (left < 0)
        left = 0;
    if (left > maxLeft)
        left = maxLeft;
    return left;
}

'''
s = s[:old_start] + helper + s[old_end:]

old = """    case WM_LBUTTONDOWN:
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
new = """    case WM_LBUTTONDOWN:
    {
        ::SetCapture(hwnd);
        DragBarDrag st;
        st.active = true;
        st.downScreen.x = GET_X_LPARAM(lParam);
        st.downScreen.y = GET_Y_LPARAM(lParam);
        ::ClientToScreen(hwnd, &st.downScreen);
        RECT wr;
        ::GetWindowRect(hwnd, &wr);
        st.barW = wr.right - wr.left;
        HWND parent = ::GetParent(hwnd);
        POINT po = { 0, 0 };
        if (parent != nullptr)
            ::ClientToScreen(parent, &po);
        RECT prc = { 0, 0, 0, 0 };
        if (parent != nullptr)
            ::GetClientRect(parent, &prc);
        st.downLeft = wr.left - po.x;
        st.parentW = prc.right - prc.left;
        dragBarState()[hwnd] = st;
        if (host != nullptr)
            host->fireDrag(id, "down", st.downLeft);
        return 0;
    }
    case WM_MOUSEMOVE:
    {
        if (::GetCapture() != hwnd || host == nullptr)
            return 0;
        auto it = dragBarState().find(hwnd);
        if (it == dragBarState().end() || !it->second.active)
            return 0;
        host->fireDrag(id, "move", DragBarTargetLeft(hwnd, lParam, it->second));
        return 0;
    }
    case WM_LBUTTONUP:
    {
        int left = 0;
        auto it = dragBarState().find(hwnd);
        if (it != dragBarState().end() && it->second.active)
        {
            left = DragBarTargetLeft(hwnd, lParam, it->second);
            it->second.active = false;
        }
        if (host != nullptr)
            host->fireDrag(id, "up", left);"""

assert s.count(old) == 1
s = s.replace(old, new)
io.open(p, 'w', encoding='utf-8-sig', newline='').write(s)
print('DragBar 现在是自解算的控件：对外只报"左边界在父客户区的 x"')
