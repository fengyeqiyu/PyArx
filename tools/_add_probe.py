import io
P = r'G:\PyArx\PyMfcXaml\PyMfcXaml.cpp'
t = io.open(P, encoding='utf-8-sig').read()

old = """        bool mini = false;        // 小地图：按客户区 fit 铺满、不画文字、画视口红框
        int miniOf = -1;          // 小地图对应的主画布 id
    };"""
new = """        bool mini = false;        // 小地图：按客户区 fit 铺满、不画文字、画视口红框
        int miniOf = -1;          // 小地图对应的主画布 id
        // 诊断探针：记录最近一次 up 的坐标与命中结果，便于从 Python 侧看清
        // "真实鼠标点到底把什么坐标送来、命中了没有"。
        int upX = 0, upY = 0;
        bool upWasDrag = false;
        std::wstring lastHit;
        int lastMsg = 0;
    };"""
assert t.count(old) == 1
t = t.replace(old, new)

old2 = """        const bool wasDrag = st.drag;
        st.drag = false;
        if (::GetCapture() == hwnd)
            ::ReleaseCapture();
        if (wasDrag && !st.moved)"""
new2 = """        const bool wasDrag = st.drag;
        st.drag = false;
        st.upWasDrag = wasDrag;
        st.lastMsg = WM_LBUTTONUP;
        st.upX = GET_X_LPARAM(lParam);
        st.upY = GET_Y_LPARAM(lParam);
        st.lastHit.clear();
        if (::GetCapture() == hwnd)
            ::ReleaseCapture();
        if (wasDrag && !st.moved)"""
assert t.count(old2) == 1
t = t.replace(old2, new2)

old3 = """            const std::wstring code = host->planHit(id, x, y);
            if (code != st.sel)"""
new3 = """            const std::wstring code = host->planHit(id, x, y);
            st.lastHit = code;
            if (code != st.sel)"""
assert t.count(old3) == 1
t = t.replace(old3, new3)

# 小地图分支也要记坐标（在 return 之前）
old4 = """        st.upWasDrag = wasDrag;
        st.lastMsg = WM_LBUTTONUP;"""
assert t.count(old4) == 1

# 探针 API：放在 getPlanView 旁边
old5 = """    // 命中测试（客户端坐标）：返回卡片编号，空 = 没命中。搜索/外部联动用
    std::wstring hitPlan(int id, int x, int y) { return m_host.planHit(id, x, y); }"""
new5 = """    // 画布鼠标诊断：返回 (lastMsg, upX, upY, wasDrag, moved_now, lastHit)
    py::object planProbe(int id)
    {
        auto it = m_host.m_planCv.find(id);
        if (it == m_host.m_planCv.end())
            return py::none();
        const PyMfcHost::PlanCanvasState& st = it->second;
        return py::make_tuple(st.lastMsg, st.upX, st.upY,
                              st.upWasDrag ? 1 : 0, st.moved ? 1 : 0, st.lastHit);
    }
    // 命中测试（客户端坐标）：返回卡片编号，空 = 没命中。搜索/外部联动用
    std::wstring hitPlan(int id, int x, int y) { return m_host.planHit(id, x, y); }"""
assert t.count(old5) == 1
t = t.replace(old5, new5)

old6 = """        .def("getPlanView", &PyMfcWindow::getPlanView,"""
assert t.count(old6) == 1
line_end = t.find('\n', t.find(old6))
t = (t[:line_end + 1] +
     '        .def("planProbe", &PyMfcWindow::planProbe, py::arg("id"),\n'
     '             "画布鼠标诊断：(lastMsg, upX, upY, wasDrag, moved, lastHit)")\n' + t[line_end + 1:])

io.open(P, 'w', encoding='utf-8-sig', newline='').write(t)
print('探针已加')
