// PyMfcHost.Interop.cpp —— 拆分自 PyMfcHost.cpp（按职责分组；接口声明仍在 PyMfcHost.h）。
#include "PyMfcHostInternal.h"

void PyMfcHost::firePlanDoubleClick(int id, const std::wstring& /*code*/)
{
        auto it = m_dblclick.find(id);
        if (it != m_dblclick.end())
            invokePython(it->second);
    }

void PyMfcHost::fireToolBarItem(int id, int index)
{
        auto it = m_click.find(id);
        if (it == m_click.end() || !it->second)
            return;
        py::gil_scoped_acquire gil;
        try
        {
            it->second(py::int_(index));
        }
        catch (py::error_already_set& e)
        {
            debugLogf(L"[toolbar] Python 回调抛异常：%hs", e.what());
            e.restore();
            PyErr_Print();
        }
        catch (...)
        {
            debugLogf(L"[toolbar] Python 回调抛未知异常");
        }
    }

void PyMfcHost::fireCanvasSelect(int id)
{
        auto it = m_cvSelCb.find(static_cast<UINT>(id));
        if (it != m_cvSelCb.end())
            invokePython(it->second);
    }

void PyMfcHost::onCanvasSelect(int id, py::object cb)
{ m_cvSelCb[static_cast<UINT>(id)] = std::move(cb); }

void PyMfcHost::onClick(int id, py::object callback)
{ m_click[id] = std::move(callback); }

void PyMfcHost::onControlEvent(int id, const std::wstring& ev, py::object callback)
{
    m_ctlEvents[std::to_wstring(id) + L":" + ev] = std::move(callback);
    m_anyCtlEvent = true;
    // 已经建好的控件要补挂子类化（事件登记晚于控件创建是常态）
    HWND h = findByIdDeep(m_hWnd, id);
    if (h == nullptr)
    {
        // 控件不一定在顶层窗口直接子级，枚举整棵树找
        struct Ctx { int id; HWND out; } ctx = { id, nullptr };
        ::EnumChildWindows(m_hWnd, [](HWND c, LPARAM p) -> BOOL {
            Ctx* c2 = reinterpret_cast<Ctx*>(p);
            if (static_cast<int>(::GetDlgCtrlID(c)) == c2->id) { c2->out = c; return FALSE; }
            return TRUE;
        }, reinterpret_cast<LPARAM>(&ctx));
        h = ctx.out;
    }
    if (h != nullptr && ::GetPropW(h, kEvtHostProp) == nullptr)
    {
        WNDPROC cur = reinterpret_cast<WNDPROC>(
            ::SetWindowLongPtrW(h, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&PyMfcEventSubProc)));
        ::SetPropW(h, kEvtPrevProcProp, reinterpret_cast<HANDLE>(cur));
        ::SetPropW(h, kEvtHostProp, reinterpret_cast<HANDLE>(this));
    }
}

void PyMfcHost::fireControlEvent(int id, const std::wstring& ev)
{
    if (m_ctlEvents.empty())
        return;
    auto it = m_ctlEvents.find(std::to_wstring(id) + L":" + ev);
    if (it == m_ctlEvents.end())
        return;
    invokePython(it->second);
}

void PyMfcHost::onGridAction(py::object cb)
{ m_gridAction = std::move(cb); }

void PyMfcHost::fireGridAction(int ctlId, const char* action, int row, int col, const std::wstring& text)
{
        // py::object 的拷贝会 inc_ref，必须在持有 GIL 时做：消息可能从 C++ 消息泵
        // （已释放 GIL）进来，先取 GIL 再碰 py::object，否则 pybind11 报
        // "inc_ref called while the GIL is not held"。
        py::gil_scoped_acquire gil;
        py::object fn;
        auto it = m_gridActionFor.find(ctlId);
        if (it != m_gridActionFor.end())
            fn = it->second;            // 先给按控件登记的回调，避免多表串台
        if (!fn && m_gridAction)
            fn = m_gridAction;          // 没按控件登记才退回全局
        if (!fn)
            return;
        try
        {
            fn(std::string(action), row, col, text);
        }
        catch (const py::error_already_set& e)
        {
            debugLogf(L"[grid] 回调异常: %hs", e.what());
        }
    }

void PyMfcHost::onGridActionFor(int id, py::object callback)
{ m_gridActionFor[id] = std::move(callback); }

std::wstring PyMfcHost::getXmlValue(int id, const std::wstring& prop) const
{
        const std::wstring k = prop;
        if (k == L"text")
            return getText(id);
        if (k == L"checked")
            return getCheck(id) != 0 ? L"true" : L"false";
        if (k == L"index" || k == L"curSel")
            return std::to_wstring(getCurSel(id));
        if (k == L"value")
            return std::to_wstring(getPos(id));
        if (k == L"visible")
            return getVisible(id) ? L"true" : L"false";
        if (k == L"enabled")
            return getEnabled(id) ? L"true" : L"false";
        if (k == L"readOnly")
            return getReadOnly(id) ? L"true" : L"false";
        if (k == L"rowCount")
            return std::to_wstring(getTableRowCount(id));
        auto it = m_ap.find(id);
        if (it != m_ap.end())
            return xmlapp::GetProperty(it->second, k);   // back/fore/border/opacity/font 等外观
        return std::wstring();
    }

void PyMfcHost::onDrag(int id, py::object cb)
{ m_drag[id] = std::move(cb); }

void PyMfcHost::fireDrag(int id, const char* phase, int x)
{
        auto it = m_drag.find(id);
        if (it == m_drag.end())
            return;
        py::gil_scoped_acquire gil;
        try { it->second(std::string(phase), x); }
        catch (const py::error_already_set& e) { debugLogf(L"[drag] %hs", e.what()); }
    }

void PyMfcHost::onBlur(int id, py::object cb)
{ m_blur[id] = std::move(cb); }

void PyMfcHost::onCanvasEvent(int id, py::object cb)
{ m_canvasEvt[id] = std::move(cb); }

bool PyMfcHost::fireCanvasEvent(int id, const char* phase, int x, int y, int flags)
{
    auto it = m_canvasEvt.find(id);
    if (it == m_canvasEvt.end())
        return false;
    py::gil_scoped_acquire gil;
    try
    {
        py::object r = it->second(std::string(phase), x, y, flags);
        if (r.is_none())
            return false;
        return r.cast<bool>();
    }
    catch (const py::error_already_set& e)
    {
        debugLogf(L"[canvas] %hs", e.what());
        return false;
    }
}

void PyMfcHost::fireBlur(int id)
{
        auto it = m_blur.find(id);
        if (it == m_blur.end())
            return;
        py::gil_scoped_acquire gil;
        try { it->second(); }
        catch (const py::error_already_set& e) { debugLogf(L"[blur] %hs", e.what()); }
    }

py::dict PyMfcHost::perfStats(bool reset)
{
        PyMfcPerfCounters& c = pyMfcPerf();
        py::dict d;
        d["create_ms"] = c.createMs;
        d["create_n"] = c.nCreate;
        d["ap_ms"] = c.apMs;
        d["ap_n"] = c.nAp;
        d["paint_ms"] = c.paintMs;
        d["paint_n"] = c.nPaint;
        d["hostpaint_ms"] = c.hostPaintMs;
        d["hostpaint_n"] = c.nHostPaint;
        d["layout_ms"] = c.layoutMs;
        d["layout_n"] = c.nLayout;
        d["parse_ms"] = c.parseMs;
        d["parse_n"] = c.nParse;
        d["decl_ms"] = c.declMs;
        d["decl_n"] = c.nDecl;
        d["load_ms"] = c.loadMs;
        d["load_n"] = c.nLoad;
        d["show_ms"] = c.showMs;
        d["show_n"] = c.nShow;
        d["elem_ms"] = c.elemMs;
        d["elem_n"] = c.nElem;
        d["apof_ms"] = c.apOfMs;
        d["apof_n"] = c.nApOf;
        d["cwe_ms"] = c.cweMs;
        d["cwe_n"] = c.nCwe;
        d["theme_ms"] = c.themeMs;
        d["theme_n"] = c.nTheme;
        d["font_ms"] = c.fontMs;
        d["font_n"] = c.nFont;
        d["shot_ms"] = c.shotMs;
        d["shot_n"] = c.nShot;
        d["enc_ms"] = c.encMs;
        d["enc_n"] = c.nEnc;
        d["enabled"] = pyMfcPerfEnabled();
        if (reset)
        {
            c = PyMfcPerfCounters();
        }
        return d;
    }

void PyMfcHost::setWheelRegion(int x, int y, int w, int h, int step)
{
        m_wheelOn = (w > 0 && h > 0);
        m_wheelRect.left = x; m_wheelRect.top = y;
        m_wheelRect.right = x + w; m_wheelRect.bottom = y + h;
        m_wheelStep = step > 0 ? step : 1;
    }

void PyMfcHost::onWheel(py::object cb)
{ m_wheelCb = std::move(cb); }

bool PyMfcHost::fireWheelMessage(WPARAM wp, LPARAM lp)
{
        if (!m_wheelOn) return false;
        POINT pt = {};
        pt.x = GET_X_LPARAM(lp);
        pt.y = GET_Y_LPARAM(lp);
        ::ScreenToClient(m_hWnd, &pt);
        if (pt.x < m_wheelRect.left || pt.x > m_wheelRect.right ||
            pt.y < m_wheelRect.top || pt.y > m_wheelRect.bottom)
            return false;
        const int dir = GET_WHEEL_DELTA_WPARAM(wp) > 0 ? -1 : 1;   // 往上滚 = 行号减小
        py::gil_scoped_acquire gil;
        if (!m_wheelCb || m_wheelCb.is_none())
            return false;
        try
        {
            m_wheelCb(dir * m_wheelStep);
        }
        catch (py::error_already_set& e)
        {
            debugLogf(L"[wheel] 回调异常: %hs", e.what());
        }
        return true;
    }

void PyMfcHost::onChange(int id, py::object callback)
{ m_change[id] = std::move(callback); }

void PyMfcHost::onSelect(int id, py::object callback)
{ m_select[id] = std::move(callback); }

void PyMfcHost::onDoubleClick(int id, py::object callback)
{ m_dblclick[id] = std::move(callback); }

void PyMfcHost::onDropDown(int id, py::object callback)
{ m_dropdown[id] = std::move(callback); }

void PyMfcHost::onValue(int id, py::object callback)
{ m_value[id] = std::move(callback); }

void PyMfcHost::onChecked(int id, py::object callback)
{ m_checked[id] = std::move(callback); }
