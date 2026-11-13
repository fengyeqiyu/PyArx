import io
def sub_once(path, old, new):
    s = io.open(path, encoding='utf-8-sig').read()
    if s.count(old) < 1:
        print('FAIL', path); return False
    s = s.replace(old, new, 1)
    io.open(path, 'w', encoding='utf-8-sig', newline='').write(s)
    print('OK', path); return True

sub_once('G:/PyArx/PyMfcXaml/PyMfcHost.cpp', "void PyMfcHost::attachXaml(", """void PyMfcHost::setSurface(int id, int x, int y, int w, int h, int fill)
{
        SurfaceItem& it = m_surfaces[id];
        it.rc = RECT{ x, y, x + w, y + h };
        it.fill = static_cast<COLORREF>(fill & 0x00FFFFFF);
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
            const int sx = py::cast<int>(sq[1]);
            const int sy = py::cast<int>(sq[2]);
            it->second.rc = RECT{ sx, sy, sx + py::cast<int>(sq[3]), sy + py::cast<int>(sq[4]) };
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

void PyMfcHost::attachXaml(""")

sub_once('G:/PyArx/PyMfcXaml/PyMfcProcs.cpp', """    return TRUE;
}

// ---------------------------------------------------------------------------
// 全控件自绘""", """    // 父表面装饰矩形（白卡/状态栏底/小地图框/详情卡/分割条）：画在窗口底色之上、
    // 子 HWND 之下，z 序由这里统一保证。
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
// 全控件自绘""")
