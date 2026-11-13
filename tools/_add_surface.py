import io

def patch(path, pairs):
    s = io.open(path, encoding='utf-8-sig').read()
    for i, (old, new) in enumerate(pairs):
        n = s.count(old)
        if n != 1:
            print('ANCHOR FAIL %s #%d count=%d' % (path, i, n)); return False
        s = s.replace(old, new, 1)
    io.open(path, 'w', encoding='utf-8-sig', newline='').write(s)
    print('OK', path); return True

ok = patch('G:/PyArx/PyMfcXaml/PyMfcHost.h', [(
"""    void moveMany(py::list items);""",
"""    void moveMany(py::list items);

    // 父表面绘制的装饰矩形：没有自己的 HWND，由父窗口在 OnEraseBkgnd 里铺底色，
    // z 序由父表面统一保证（分割条不会被表格之类的兄弟窗口盖掉），也省掉一次独立 WM_PAINT。
    struct SurfaceItem
    {
        RECT rc = { 0, 0, 0, 0 };
        COLORREF fill = RGB(255, 255, 255);
        COLORREF border = RGB(0, 0, 0);
        bool hasBorder = false;
        bool visible = true;
    };
    std::map<int, SurfaceItem> m_surfaces;
    std::vector<int> m_surfaceOrder;
    void setSurface(int id, int x, int y, int w, int h, int fill, int border, bool hasBorder);
    void moveSurfaces(py::list items);
    py::tuple getSurfaceRect(int id);""")])

if ok:
    ok = patch('G:/PyArx/PyMfcXaml/PyMfcHost.cpp', [(
"""void PyMfcHost::attachXaml(""",
"""void PyMfcHost::setSurface(int id, int x, int y, int w, int h, int fill, int border, bool hasBorder)
{
        SurfaceItem& it = m_surfaces[id];
        it.rc = RECT{ x, y, x + w, y + h };
        it.fill = static_cast<COLORREF>(fill & 0x00FFFFFF);
        it.border = static_cast<COLORREF>(border & 0x00FFFFFF);
        it.hasBorder = hasBorder;
        it.visible = true;
        if (std::find(m_surfaceOrder.begin(), m_surfaceOrder.end(), id) == m_surfaceOrder.end())
            m_surfaceOrder.push_back(id);
        if (m_hWnd != nullptr)
            ::InvalidateRect(m_hWnd, &it.rc, FALSE);
    }

void PyMfcHost::moveSurfaces(py::list items)
{
        for (py::handle h : items)
        {
            py::sequence sq = py::reinterpret_borrow<py::sequence>(h);
            if (sq.size() < 6)
                continue;
            const int id = py::cast<int>(sq[0]);
            auto it = m_surfaces.find(id);
            if (it == m_surfaces.end())
                continue;
            RECT before = it->second.rc;
            it->second.rc = RECT{ py::cast<int>(sq[1]), py::cast<int>(sq[2]),
                                  py::cast<int>(sq[1]) + py::cast<int>(sq[3]),
                                  py::cast<int>(sq[2]) + py::cast<int>(sq[4]) };
            it->second.visible = py::cast<bool>(sq[5]);
            if (m_hWnd != nullptr)
            {
                RECT both = before;
                ::UnionRect(&both, &before, &it->second.rc);
                ::InvalidateRect(m_hWnd, &both, FALSE);
            }
        }
    }

py::tuple PyMfcHost::getSurfaceRect(int id)
{
        auto it = m_surfaces.find(id);
        if (it == m_surfaces.end())
            return py::make_tuple(-1, -1, -1, -1);
        const RECT& rc = it->second.rc;
        return py::make_tuple(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
    }

void PyMfcHost::attachXaml(""")])

if ok:
    ok = patch('G:/PyArx/PyMfcXaml/PyMfcProcs.cpp', [(
"""    return TRUE;
}

// ---------------------------------------------------------------------------
// 全控件自绘""",
"""    // 父表面装饰矩形（白卡 / 状态栏底 / 小地图框 / 详情卡 / 分割条）：
    // 画在窗口底色之上、子 HWND 之下，z 序由这里统一保证。
    for (int sid : m_surfaceOrder)
    {
        auto sit = m_surfaces.find(sid);
        if (sit == m_surfaces.end() || !sit->second.visible)
            continue;
        CRect sclip;
        sclip.IntersectRect(sit->second.rc, rc);
        if (sclip.IsRectEmpty())
            continue;
        pDC->FillSolidRect(sclip, sit->second.fill);
    }
    return TRUE;
}

// ---------------------------------------------------------------------------
// 全控件自绘""")])
