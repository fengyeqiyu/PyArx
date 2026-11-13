// PyMfcWrap.cpp —— PyMfcTopWnd / PyMfcWindow / PyMfcPanel 的实现。

#include "PyMfcWrap.h"

BEGIN_MESSAGE_MAP(PyMfcTopWnd, CWnd)
    ON_WM_SIZE()
END_MESSAGE_MAP()

void PyMfcTopWnd::OnSize(UINT nType, int cx, int cy)
{
    CWnd::OnSize(nType, cx, cy);
    if (host != nullptr && host->GetSafeHwnd() != nullptr)
        host->MoveWindow(0, 0, cx, cy, TRUE);
    if (resize)
        resize(cx, cy);
}




// ---------------------------------------------------------------------------
// 进程级登记表：原生对象归 C++ 所有，Python 只持引用
// ---------------------------------------------------------------------------

void PyMfcTopWnd::PostNcDestroy()
{}


PyMfcWindow::~PyMfcWindow()
{
        // 由 PyMfcStore 持有到进程结束，见顶部生命周期原则
        debugLogf(L"[store] ~PyMfcWindow hwnd=0x%llX",
                  reinterpret_cast<unsigned long long>(m_wnd.GetSafeHwnd()));
    }

void PyMfcWindow::setWindowIcon(const std::wstring& path)
{ m_iconPath = path; }


void PyMfcWindow::applyWindowIcon(HWND hwnd)
{
        if (hwnd == nullptr || m_iconPath.empty())
            return;
        if (m_icon == nullptr)
            m_icon = xamlIconFromFile(m_iconPath);
        if (m_icon == nullptr)
            return;
        ::SendMessageW(hwnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(m_icon));
        ::SendMessageW(hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(m_icon));
    }

bool PyMfcWindow::create(const std::wstring& title, int width, int height)
{
        AFX_MANAGE_STATE(AfxGetAppModuleState());
        ensureNativeClass();
        HWND hwnd = ::CreateWindowExW(
            // WS_EX_COMPOSITED：整窗底部先画到离屏位图再一次性提交，
            // 布局重排/多控件同时刷新时不再看到"一块块补上"的闪烁。
            WS_EX_APPWINDOW | WS_EX_COMPOSITED, L"PyMfcNativeWindow", title.c_str(),
            // WS_CLIPCHILDREN 必须有：宿主整窗自绘，缺了它每次重画都会把子控件那块
            // 一起刷成背景色，页内容一移动就闪一下（切页时最明显）。
            WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
            100, 100, width, height,
            nullptr, nullptr, AfxGetInstanceHandle(), nullptr);
        if (hwnd == nullptr)
            return false;
        m_wnd.SubclassWindow(hwnd);
        // 滚轮拦截挂在 MFC 子类化之后：old proc 就是 AfxWndProc，没命中会原样转回
        installWheelHost(hwnd, &m_host);
        m_wnd.host = &m_host;
        m_wnd.resize = [this](int w, int h) { notifyResize(w, h); };
        applyWindowIcon(hwnd);
        const BOOL darkTitle = TRUE;
        ::DwmSetWindowAttribute(hwnd, 20, &darkTitle, sizeof(darkTitle));
        ::SetWindowTheme(hwnd, L"DarkMode_Explorer", nullptr);
        if (!m_host.create(hwnd, CRect(0, 0, width, height)))
            return false;
        CRect client;
        m_wnd.GetClientRect(&client);
        m_host.MoveWindow(client);
        return true;
    }

HWND PyMfcWindow::hwnd() const
{ return m_wnd.GetSafeHwnd(); }


PyMfcHost* PyMfcWindow::Host()
{ return &m_host; }


bool PyMfcWindow::savePng(const std::wstring& path)
{
        HWND hwnd = m_wnd.GetSafeHwnd();
        if (hwnd == nullptr)
            return false;

        ::SetForegroundWindow(hwnd);
        ::RedrawWindow(hwnd, nullptr, nullptr,
                       RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_UPDATENOW);
        // 泵 600ms 的消息：D2D 出帧、光标/动画就位，否则抓到的是半张空壳
        MSG msg = {};
        for (int i = 0; i < 60; ++i)
        {
            while (::PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
            {
                ::TranslateMessage(&msg);
                ::DispatchMessageW(&msg);
            }
            ::Sleep(10);
        }

        RECT rc = {};
        ::GetWindowRect(hwnd, &rc);
        const int w = rc.right - rc.left;
        const int h = rc.bottom - rc.top;
        if (w <= 0 || h <= 0)
            return false;

        HDC screen = ::GetDC(nullptr);
        if (screen == nullptr)
            return false;

        BITMAPINFO bi = {};
        bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bi.bmiHeader.biWidth = w;
        bi.bmiHeader.biHeight = -h;   // 顶朝下，和屏幕抓图同向，省一次翻转
        bi.bmiHeader.biPlanes = 1;
        bi.bmiHeader.biBitCount = 32;
        bi.bmiHeader.biCompression = BI_RGB;

        void* bits = nullptr;
        HBITMAP dib = ::CreateDIBSection(screen, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
        HDC mem = ::CreateCompatibleDC(screen);
        bool ok = false;
        if (dib != nullptr && mem != nullptr && bits != nullptr)
        {
            HGDIOBJ old = ::SelectObject(mem, dib);
            ok = ::BitBlt(mem, 0, 0, w, h, screen, rc.left, rc.top,
                          SRCCOPY | CAPTUREBLT) != FALSE;
            ::SelectObject(mem, old);
        }
        if (ok)
            ok = wicWritePng(path, w, h, bits);
        if (mem != nullptr)
            ::DeleteDC(mem);
        if (dib != nullptr)
            ::DeleteObject(dib);
        ::ReleaseDC(nullptr, screen);
        return ok;
    }

bool PyMfcWindow::CreateEmbedded(HWND parent, const std::wstring& title, int width, int height)
{
        AFX_MANAGE_STATE(AfxGetAppModuleState());
        if (parent == nullptr || !::IsWindow(parent))
            return false;
        ensureNativeClass();
        // 定位：优先放到 CAD 客户区中部，避免被边框/状态栏遮挡
        CRect rcParent;
        ::GetClientRect(parent, &rcParent);
        int x = (rcParent.Width() - width) / 2;
        int y = (rcParent.Height() - height) / 3;
        if (x < 0) x = 0;
        if (y < 0) y = 0;
        HWND hwnd = ::CreateWindowExW(
            WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE, L"PyMfcNativeWindow", title.c_str(),
            // WS_CLIPCHILDREN 不能少：宿主是自绘整窗，缺了它父窗口每次重绘都会把
            // 子控件所在矩形整个刷掉，原生控件（进度条/滑块/列表/树/表格/选项卡）
            // 就变成"空底"，看起来像只画了个壳
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME |
                WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
            x, y, width, height,
            parent, nullptr, AfxGetInstanceHandle(), nullptr);
        if (hwnd == nullptr)
            return false;
        m_wnd.SubclassWindow(hwnd);
        m_wnd.host = &m_host;
        m_wnd.resize = [this](int w, int h) { notifyResize(w, h); };
        applyWindowIcon(hwnd);
        if (!m_host.create(hwnd, CRect(0, 0, width, height)))
            return false;
        CRect client;
        m_wnd.GetClientRect(&client);
        m_host.MoveWindow(client);
        // 先不显示：控件树还没建完就把窗口露出来，用户会先看到一只空壳，
        // 再被一个一个控件"填"上；在 CAD 宿主里这段半成品帧还会和宿主重绘叠在一起，
        // 看起来像"窗口不稳定/一闪一闪"。交给 guiload 末尾的 endSilentBuild() 一次性亮相。
        m_showPending = true;
        // 只设置焦点给 CAD owner，不改前台窗口。
        if (::IsWindow(parent))
            ::SetFocus(parent);
        return true;
    }

void PyMfcWindow::setTheme(bool dark)
{ m_host.setTheme(dark); }


void PyMfcWindow::setResizeCallback(py::object cb)
{ m_resize = std::move(cb); }


std::pair<int, int> PyMfcWindow::ClientSize() const
{
        CRect rc;
        m_wnd.GetClientRect(&rc);
        return {rc.Width(), rc.Height()};
    }

void PyMfcWindow::notifyResize(int w, int h)
{
        if (!m_resize)
            return;
        py::gil_scoped_acquire gil;
        try { m_resize(w, h); }
        catch (py::error_already_set& e) { e.restore(); PyErr_Print(); }
        catch (...) { PyErr_Print(); }
    }

void PyMfcWindow::show()
{
        if (!::IsWindow(m_wnd.GetSafeHwnd()))
            return;
        m_showPending = false;   // 显式 show 覆盖"等加载完再显示"
        m_wnd.ShowWindow(SW_SHOW);
        m_wnd.UpdateWindow();
    }

void PyMfcWindow::markSilentBuild()
{ m_showPending = true; }


void PyMfcWindow::endSilentBuild()
{
        if (!m_showPending)
            return;
        m_showPending = false;
        HWND hwnd = m_wnd.GetSafeHwnd();
        if (!::IsWindow(hwnd))
            return;
        ::EnumChildWindows(hwnd, [](HWND child, LPARAM) -> BOOL {
            ::InvalidateRect(child, nullptr, TRUE);
            return TRUE;
        }, 0);
        ::InvalidateRect(hwnd, nullptr, TRUE);
        ::RedrawWindow(hwnd, nullptr, nullptr,
                       RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_UPDATENOW | RDW_ERASE);
        ::ShowWindow(hwnd, SW_SHOWNOACTIVATE);
        ::UpdateWindow(hwnd);
    }

void PyMfcWindow::close()
{
        HWND hwnd = m_wnd.GetSafeHwnd();
        if (hwnd == nullptr || !::IsWindow(hwnd))
            return;
        // 先解绑（恢复原窗口过程，MFC 不再参与销毁），再销毁：
        // 直接在 Python 栈上让 MFC 走 WM_NCDESTROY 会破坏框架状态，导致退出期 AV
        debugLogf(L"[close] unsubclass+destroy hwnd=0x%llX",
                  reinterpret_cast<unsigned long long>(hwnd));
        m_wnd.UnsubclassWindow();
        ::DestroyWindow(hwnd);
    }

void PyMfcWindow::closeAsync()
{
        if (::IsWindow(m_wnd.GetSafeHwnd()))
            ::PostMessageW(m_wnd.GetSafeHwnd(), WM_CLOSE, 0, 0);
    }

void PyMfcWindow::hide()
{
        HWND hwnd = m_wnd.GetSafeHwnd();
        if (!::IsWindow(hwnd))
            return;
        ::ShowWindow(hwnd, SW_HIDE);
        ::SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                       SWP_HIDEWINDOW | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }

void PyMfcWindow::clearCallbacks()
{ m_host.clearCallbacks(); }


void PyMfcWindow::addButton(int id, const std::wstring& text, int x, int y, int w, int h)
{ m_host.addButton(id, text, x, y, w, h); }


void PyMfcWindow::addLabel(int id, const std::wstring& text, int x, int y, int w, int h)
{ m_host.addLabel(id, text, x, y, w, h); }


void PyMfcWindow::addEdit(int id, const std::wstring& text, int x, int y, int w, int h)
{ m_host.addEdit(id, text, x, y, w, h); }


void PyMfcWindow::addListBox(int id, int x, int y, int w, int h)
{ m_host.addListBox(id, x, y, w, h); }


void PyMfcWindow::addCheckBox(int id, const std::wstring& text, int x, int y, int w, int h)
{ m_host.addCheckBox(id, text, x, y, w, h); }


void PyMfcWindow::addRadio(int id, const std::wstring& text, int x, int y, int w, int h)
{ m_host.addRadio(id, text, x, y, w, h); }


void PyMfcWindow::addGroup(int id, const std::wstring& text, int x, int y, int w, int h)
{ m_host.addGroup(id, text, x, y, w, h); }


void PyMfcWindow::addCombo(int id, int x, int y, int w, int h)
{ m_host.addCombo(id, x, y, w, h); }


void PyMfcWindow::addEditAdvanced(int id, const std::wstring& text, bool multiline, bool readOnly,
                         int maxLength, const std::wstring& placeholder, int x, int y, int w, int h)
{
        m_host.addEditAdvanced(id, text, multiline, readOnly, maxLength, placeholder, x, y, w, h);
    }

void PyMfcWindow::addComboAdvanced(int id, bool dropDownList, int x, int y, int w, int h)
{
        m_host.addComboAdvanced(id, dropDownList, x, y, w, h);
    }

void PyMfcWindow::addListBoxAdvanced(int id, bool multipleSelect, int x, int y, int w, int h)
{
        m_host.addListBoxAdvanced(id, multipleSelect, x, y, w, h);
    }

void PyMfcWindow::addProgress(int id, int x, int y, int w, int h)
{ m_host.addProgress(id, x, y, w, h); }


void PyMfcWindow::addSlider(int id, int minValue, int maxValue, int x, int y, int w, int h)
{ m_host.addSlider(id, minValue, maxValue, x, y, w, h); }


void PyMfcWindow::addTab(int id, int x, int y, int w, int h)
{ m_host.addTab(id, x, y, w, h); }


void PyMfcWindow::addTree(int id, int x, int y, int w, int h)
{ m_host.addTree(id, x, y, w, h); }


void PyMfcWindow::addListCtrl(int id, int x, int y, int w, int h)
{ m_host.addListCtrl(id, x, y, w, h); }


void PyMfcWindow::addChoice(int id, int x, int y, int w, int h)
{ m_host.addChoice(id, x, y, w, h); }


void PyMfcWindow::addDatePicker(int id, int x, int y, int w, int h)
{ m_host.addDatePicker(id, x, y, w, h); }


void PyMfcWindow::addPanel(int id, int x, int y, int w, int h)
{ m_host.addPanel(id, x, y, w, h); }


void PyMfcWindow::addWhiteRect(int id, int x, int y, int w, int h)
{ m_host.addWhiteRect(id, x, y, w, h); }


void PyMfcWindow::addShadowBox(int id, int x, int y, int w, int h, int pad, int radius, int dx, int dy, int blur, const std::wstring& shadowColor, const std::wstring& bgColor, const std::wstring& backColor, const std::wstring& borderColor, int borderW)
{ m_host.addShadowBox(id, x, y, w, h, pad, radius, dx, dy, blur, shadowColor, bgColor, backColor, borderColor, borderW); }


void PyMfcWindow::setControlFont(int id, const std::wstring& face, int px, bool bold)
{ m_host.setControlFont(id, face, px, bold); }


void PyMfcWindow::setControlAccent(int id, int r, int g, int b)
{ m_host.setControlAccent(id, r, g, b); }


void PyMfcWindow::setLabelColor(int id, int r, int g, int b)
{ m_host.setLabelColor(id, r, g, b); }


void PyMfcWindow::addSplitter(int id, int x, int y, int w, int h)
{ m_host.addSplitter(id, x, y, w, h); }


void PyMfcWindow::addStaticLine(int id, int x, int y, int w, int h)
{ m_host.addStaticLine(id, x, y, w, h); }


void PyMfcWindow::addHyperlink(int id, const std::wstring& text, int x, int y, int w, int h)
{ m_host.addHyperlink(id, text, x, y, w, h); }


void PyMfcWindow::addRichText(int id, int x, int y, int w, int h)
{ m_host.addRichText(id, x, y, w, h); }


void PyMfcWindow::addToggle(int id, const std::wstring& text, int x, int y, int w, int h)
{ m_host.addToggle(id, text, x, y, w, h); }


void PyMfcWindow::addBitmapButton(int id, const std::wstring& text, int x, int y, int w, int h)
{ m_host.addBitmapButton(id, text, x, y, w, h); }


void PyMfcWindow::addStaticBitmap(int id, int x, int y, int w, int h)
{ m_host.addStaticBitmap(id, x, y, w, h); }


void PyMfcWindow::addGrid(int id, int x, int y, int w, int h)
{ m_host.addGrid(id, x, y, w, h); }


void PyMfcWindow::addScrolledWindow(int id, int x, int y, int w, int h)
{ m_host.addScrolledWindow(id, x, y, w, h); }


void PyMfcWindow::addCalendar(int id, int x, int y, int w, int h)
{ m_host.addCalendar(id, x, y, w, h); }


void PyMfcWindow::addTabItem(int id, const std::wstring& text)
{ m_host.addTabItem(id, text); }


void PyMfcWindow::addTreeNode(int id, const std::wstring& text, int parentIndex)
{ m_host.addTreeNode(id, text, parentIndex); }


void PyMfcWindow::addColumn(int id, const std::wstring& text, int width)
{ m_host.addColumn(id, text, width); }


void PyMfcWindow::addRow(int id, const std::wstring& text)
{ m_host.addRow(id, text); }


void PyMfcWindow::setUrl(int id, const std::wstring& url)
{ m_host.setUrl(id, url); }


std::wstring PyMfcWindow::getUrl(int id) const
{ return m_host.getUrl(id); }


bool PyMfcWindow::setImage(int id, const std::wstring& path)
{ return m_host.setImage(id, path); }


void PyMfcWindow::setRichFormat(int id, int sizePt, bool bold, bool italic, unsigned long rgb)
{ m_host.setRichFormat(id, sizePt, bold, italic, rgb); }


void PyMfcWindow::setScrollRange(int id, int minValue, int maxValue)
{ m_host.setScrollRange(id, minValue, maxValue); }


std::pair<int, int> PyMfcWindow::getScrollRange(int id) const
{ return m_host.getScrollRange(id); }


int PyMfcWindow::getScrollPos(int id) const
{ return m_host.getScrollPos(id); }


std::pair<int, int> PyMfcWindow::getSelection(int id) const
{ return m_host.getSelection(id); }


void PyMfcWindow::setSelection(int id, int from, int to) const
{ m_host.setSelection(id, from, to); }


void PyMfcWindow::onClick(int id, py::object cb)
{ m_host.onClick(id, std::move(cb)); }


void PyMfcWindow::setWheelRegion(int x, int y, int w, int h, int step)
{ m_host.setWheelRegion(x, y, w, h, step); }


void PyMfcWindow::onWheel(py::object cb)
{ m_host.onWheel(std::move(cb)); }


void PyMfcWindow::onChange(int id, py::object cb)
{ m_host.onChange(id, std::move(cb)); }


void PyMfcWindow::onSelect(int id, py::object cb)
{ m_host.onSelect(id, std::move(cb)); }


void PyMfcWindow::onDoubleClick(int id, py::object cb)
{ m_host.onDoubleClick(id, std::move(cb)); }


void PyMfcWindow::onDropDown(int id, py::object cb)
{ m_host.onDropDown(id, std::move(cb)); }


void PyMfcWindow::onValue(int id, py::object cb)
{ m_host.onValue(id, std::move(cb)); }


void PyMfcWindow::onChecked(int id, py::object cb)
{ m_host.onChecked(id, std::move(cb)); }


void PyMfcWindow::setText(int id, const std::wstring& text)
{ m_host.setText(id, text); }


std::wstring PyMfcWindow::getText(int id) const
{ return m_host.getText(id); }


void PyMfcWindow::setVisible(int id, bool visible)
{ m_host.setVisible(id, visible); }


void PyMfcWindow::setEnabled(int id, bool enabled)
{ m_host.setEnabled(id, enabled); }


void PyMfcWindow::moveControl(int id, int x, int y, int w, int h)
{ m_host.moveControl(id, x, y, w, h); }


void PyMfcWindow::moveMany(py::list items)
{ m_host.moveMany(std::move(items)); }


bool PyMfcWindow::setXamlValue(const std::wstring& name, const std::wstring& prop, const std::wstring& value)
{
        return m_host.setXamlProperty(name, prop, value);
    }

std::wstring PyMfcWindow::getXamlValue(int id, const std::wstring& prop)
{ return m_host.getXamlValue(id, prop); }

bool PyMfcWindow::reflowXaml()
{ return m_host.reflowXaml(); }


void PyMfcWindow::addString(int id, const std::wstring& text)
{ m_host.addString(id, text); }


void PyMfcWindow::clearStrings(int id)
{ m_host.clearStrings(id); }


int PyMfcWindow::countStrings(int id) const
{ return m_host.countStrings(id); }


std::wstring PyMfcWindow::getString(int id, int index) const
{ return m_host.getString(id, index); }


void PyMfcWindow::deleteString(int id, int index)
{ m_host.deleteString(id, index); }


int PyMfcWindow::getCurSel(int id) const
{ return m_host.getCurSel(id); }


void PyMfcWindow::setCurSel(int id, int index)
{ m_host.setCurSel(id, index); }


bool PyMfcWindow::saveControlPng(int id, const std::wstring& path)
{ return m_host.saveControlPng(id, path); }


int PyMfcWindow::getCheck(int id) const
{ return m_host.getCheck(id); }


void PyMfcWindow::setCheck(int id, bool checked)
{ m_host.setCheck(id, checked); }


void PyMfcWindow::setRange(int id, int minValue, int maxValue)
{ m_host.setRange(id, minValue, maxValue); }


void PyMfcWindow::setPos(int id, int value)
{ m_host.setPos(id, value); }


int PyMfcWindow::getPos(int id) const
{ return m_host.getPos(id); }


void PyMfcWindow::setReadOnly(int id, bool readOnly)
{ m_host.setReadOnly(id, readOnly); }


bool PyMfcWindow::getReadOnly(int id) const
{ return m_host.getReadOnly(id); }


void PyMfcWindow::setFocusTo(int id)
{ m_host.setFocusTo(id); }


bool PyMfcWindow::getVisible(int id) const
{ return m_host.getVisible(id); }


bool PyMfcWindow::getEnabled(int id) const
{ return m_host.getEnabled(id); }


bool PyMfcWindow::getSwitch(int id) const
{ return m_host.getSwitchValue(id); }


void PyMfcWindow::setSwitch(int id, bool on)
{ m_host.setSwitchValue(id, on); }


int PyMfcWindow::getSpin(int id) const
{ return m_host.getSpinValue(id); }


void PyMfcWindow::setSpin(int id, int v)
{ m_host.setSpinValue(id, v); }


void PyMfcWindow::setSpinRange(int id, int lo, int hi)
{ m_host.setSpinRange(id, lo, hi); }


void PyMfcWindow::setRangeValues(int id, int lo, int hi)
{ m_host.setRangeValues(id, lo, hi); }


int PyMfcWindow::getRangeLow(int id) const
{ return m_host.getRangeLow(id); }


int PyMfcWindow::getRangeHigh(int id) const
{ return m_host.getRangeHigh(id); }


void PyMfcWindow::setPageCount(int id, int count)
{ m_host.setPageCount(id, count); }


int PyMfcWindow::getPageCount(int id) const
{ return m_host.getPageCount(id); }


void PyMfcWindow::setPageIndex(int id, int index)
{ m_host.setPageIndex(id, index); }


int PyMfcWindow::getPageIndex(int id) const
{ return m_host.getPageIndex(id); }


int PyMfcWindow::getDial(int id) const
{ return m_host.getDial(id); }


void PyMfcWindow::setDial(int id, int v)
{ m_host.setDial(id, v); }


int PyMfcWindow::getTumblerCount(int id) const
{ return m_host.getTumblerCount(id); }


void PyMfcWindow::setTumblerCount(int id, int c)
{ m_host.setTumblerCount(id, c); }


int PyMfcWindow::getTumblerValue(int id) const
{ return m_host.getTumblerValue(id); }


void PyMfcWindow::setTumblerValue(int id, int v)
{ m_host.setTumblerValue(id, v); }


void PyMfcWindow::setScrollBarRange(int id, int lo, int hi, int page, bool vertical)
{ m_host.setScrollBarRange(id, lo, hi, page, vertical); }


int PyMfcWindow::getScrollBarValue(int id) const
{ return m_host.getScrollBarValue(id); }


void PyMfcWindow::setScrollBarValue(int id, int v)
{ m_host.setScrollBarValue(id, v); }


void PyMfcWindow::setTableColumns(int id, const std::wstring& s)
{ m_host.setTableColumns(id, s); }


void PyMfcWindow::addTableRow(int id, const std::wstring& s)
{ m_host.addTableRow(id, s); }

void PyMfcWindow::setTableRows(int id, const std::wstring& blob)
{ m_host.setTableRows(id, blob); }


void PyMfcWindow::setColumnWidth(int id, int col, int px)
{ m_host.setColumnWidth(id, col, px); }

void PyMfcWindow::setColumnWidths(int id, const std::vector<int>& widths)
{ m_host.setColumnWidths(id, widths); }


py::object PyMfcWindow::getTableMetrics(int id)
{ return m_host.tableMetrics(id); }


void PyMfcWindow::onBlur(int id, py::object cb)
{ m_host.onBlur(id, cb); }


void PyMfcWindow::onDrag(int id, py::object cb)
{ m_host.onDrag(id, cb); m_host.installDragBar(id); }


py::object PyMfcWindow::cellRect(int id, int row, int col)
{ return m_host.cellRect(id, row, col); }


void PyMfcWindow::onGridAction(py::object cb)
{ m_host.onGridAction(std::move(cb)); }

void PyMfcWindow::onGridActionFor(int id, py::object cb)
{ m_host.onGridActionFor(id, std::move(cb)); }

void PyMfcWindow::onControlEvent(int id, const std::wstring& ev, py::object cb)
{ m_host.onControlEvent(id, ev, std::move(cb)); }


void PyMfcWindow::clearTableRows(int id)
{ m_host.clearTableRows(id); }


int PyMfcWindow::getTableRowCount(int id) const
{ return m_host.getTableRowCount(id); }


std::wstring PyMfcWindow::getTableCell(int id, int r, int c) const
{ return m_host.getTableCell(id, r, c); }


void PyMfcWindow::setTableCell(int id, int r, int c, const std::wstring& s)
{ m_host.setTableCell(id, r, c, s); }


int PyMfcWindow::getTableSelectedRow(int id) const
{ return m_host.getTableSelectedRow(id); }


void PyMfcWindow::setTableSelectedRow(int id, int r)
{ m_host.setTableSelectedRow(id, r); }


void PyMfcWindow::sortTable(int id, int col)
{ m_host.sortTable(id, col); }


void PyMfcWindow::addTable(int id, int x, int y, int w, int h)
{ m_host.addTable(id, x, y, w, h); }

void PyMfcWindow::tableClear(int id)
{ m_host.tableClear(id); }

int PyMfcWindow::tableRowCount(int id) const
{ return m_host.tableRowCount(id); }

void PyMfcWindow::tableSetColumns(int id, const std::vector<int>& widths,
                                   const std::vector<std::wstring>& titles)
{ m_host.tableSetColumns(id, widths, titles); }

void PyMfcWindow::tableSetColTypes(int id, const std::vector<int>& types)
{ m_host.tableSetColTypes(id, types); }

void PyMfcWindow::appendTableRow(int id, const std::vector<std::wstring>& cells)
{ m_host.tableAppendRow(id, cells); }

void PyMfcWindow::tableSetCell(int id, int row, int col, const std::wstring& s)
{ m_host.tableSetCell(id, row, col, s); }

std::wstring PyMfcWindow::tableGetCell(int id, int row, int col) const
{ return m_host.tableGetCell(id, row, col); }

py::object PyMfcWindow::tableCellRect(int id, int row, int col) const
{ return m_host.tableCellRect(id, row, col); }

int PyMfcWindow::tableSelectedRow(int id) const
{ return m_host.tableSelectedRow(id); }

void PyMfcWindow::tableSetSelectedRow(int id, int row)
{ m_host.tableSelectRow(id, row); }

void PyMfcWindow::tableEnsureVisible(int id, int row)
{ m_host.tableEnsureVisible(id, row); }

void PyMfcWindow::tableSetFilter(int id, int col, const std::wstring& s)
{ m_host.tableSetFilter(id, col, s); }

void PyMfcWindow::tableClearFilters(int id)
{ m_host.tableClearFilters(id); }

void PyMfcWindow::tableSort(int id, int col, bool desc)
{ m_host.tableSort(id, col, desc); }

int PyMfcWindow::tableViewRows(int id) const
{ return m_host.tableViewRows(id); }


void PyMfcWindow::addTreeItem(int id, const std::wstring& s, int level, bool kids)
{ m_host.addTreeItem(id, s, level, kids); }


void PyMfcWindow::clearTreeItems(int id)
{ m_host.clearTreeItems(id); }


int PyMfcWindow::getTreeSelectedRow(int id) const
{ return m_host.getTreeSelectedRow(id); }


bool PyMfcWindow::isTreeItemCollapsed(int id, int r) const
{ return m_host.isTreeItemCollapsed(id, r); }


void PyMfcWindow::setTreeItemCollapsed(int id, int r, bool c)
{ m_host.setTreeItemCollapsed(id, r, c); }


void PyMfcWindow::setTreeSelectedRow(int id, int r)
{ m_host.setTreeSelectedRow(id, r); }


void PyMfcWindow::addToolBarItem(int id, const std::wstring& s)
{ m_host.addToolBarItem(id, s); }


void PyMfcWindow::clearToolBarItems(int id)
{ m_host.clearToolBarItems(id); }


int PyMfcWindow::getToolBarSelected(int id) const
{ return m_host.getToolBarSelected(id); }


void PyMfcWindow::setToolBarSelected(int id, int i)
{ m_host.setToolBarSelected(id, i); }


void PyMfcWindow::animate(int id, const std::wstring& prop, double to, int dur, const std::string& ease,
                 int delay, bool loop)
{ m_host.animate(id, prop, to, dur, ease, delay, loop); }


void PyMfcWindow::animateFrom(int id, const std::wstring& prop, double from, double to, int dur,
                     const std::string& ease, int delay, bool loop)
{ m_host.animateFrom(id, prop, from, to, dur, ease, delay, loop); }


void PyMfcWindow::animateColor(int id, int rgb, int dur, const std::string& ease, int delay)
{ m_host.animateColor(id, static_cast<COLORREF>(rgb), dur, ease, delay); }


void PyMfcWindow::stopAnimations(int id)
{ m_host.stopAnimations(id); }


void PyMfcWindow::setBehavior(int id, const std::wstring& prop, int dur, const std::string& ease)
{ m_host.setBehavior(id, prop, dur, ease); }


void PyMfcWindow::showToolTip(int ownerId, const std::wstring& text, int durationMs)
{ m_host.showToolTip(ownerId, text, durationMs); }


void PyMfcWindow::hideToolTip()
{ m_host.hideToolTip(); }


void PyMfcWindow::showMenu(int id, const std::vector<std::wstring>& items, int x, int y)
{ m_host.showMenu(id, items, x, y); }


void PyMfcWindow::hideMenu(int id)
{ m_host.hideMenu(id); }


int PyMfcWindow::getMenuSelected(int id) const
{ return m_host.getMenuSelected(id); }


void PyMfcWindow::setMenuItems(int id, const std::vector<std::wstring>& items)
{ m_host.setMenuItems(id, items); }


void PyMfcWindow::addMenuBarItem(int id, const std::wstring& text)
{ m_host.addMenuBarItem(id, text); }


void PyMfcWindow::clearMenuBarItems(int id)
{ m_host.clearMenuBarItems(id); }


int PyMfcWindow::getMenuBarSelected(int id) const
{ return m_host.getMenuBarSelected(id); }


void PyMfcWindow::setMenuBarSelected(int id, int i)
{ m_host.setMenuBarSelected(id, i); }


void PyMfcWindow::showPopup(int id, const std::wstring& title, const std::vector<std::wstring>& lines,
                   int x, int y, int w, int h)
{ m_host.showPopup(id, title, lines, x, y, w, h); }


void PyMfcWindow::hidePopup(int id)
{ m_host.hidePopup(id); }


bool PyMfcWindow::isPopupVisible(int id) const
{ return m_host.isPopupVisible(id); }


void PyMfcWindow::setCanvasCommands(int id, const std::wstring& cmds)
{ m_host.setCanvasCommands(id, cmds); }


std::wstring PyMfcWindow::getCanvasCommands(int id) const
{ return m_host.getCanvasCommands(id); }


void PyMfcWindow::addPowerCanvas(int id, int x, int y, int w, int h)
{ m_host.addPowerCanvas(id, x, y, w, h); }


void PyMfcWindow::loadPowerPlan(int id, const std::wstring& json)
{ m_host.setPowerCanvasPlan(id, json); }


void PyMfcWindow::loadPowerScene(int id, const std::wstring& json)
{ m_host.setPowerCanvasScene(id, json); }

void PyMfcWindow::copyPowerScene(int toId, int fromId)
{ m_host.copyPowerCanvasScene(toId, fromId); }


void PyMfcWindow::loadPowerSvg(int id, const std::wstring& svg)
{ m_host.setPowerCanvasSvg(id, svg); }


void PyMfcWindow::setPlanView(int id, float zoom, int ox, int oy)
{ m_host.setPowerCanvasView(id, zoom, ox, oy); }


float PyMfcWindow::getPlanZoom(int id) const
{ return m_host.getPowerCanvasZoom(id); }


std::wstring PyMfcWindow::getPlanSelection(int id) const
{ return m_host.getPowerCanvasSelection(id); }


void PyMfcWindow::setPlanOptions(int id, bool showCable, bool mini, int miniOf)
{ m_host.setPowerCanvasOptions(id, showCable, mini, miniOf); }


py::object PyMfcWindow::getPlanView(int id) const
{
        float zoom = 0.0f;
        int ox = 0, oy = 0;
        m_host.getPowerCanvasView(id, zoom, ox, oy);
        return py::make_tuple(zoom, ox, oy);
    }

py::object PyMfcWindow::planProbe(int id)
{
        auto it = m_host.m_planCv.find(id);
        if (it == m_host.m_planCv.end())
            return py::none();
        const PyMfcHost::PlanCanvasState& st = it->second;
        return py::make_tuple(st.lastMsg, st.upX, st.upY,
                              st.upWasDrag ? 1 : 0, st.moved ? 1 : 0, st.lastHit);
    }

std::wstring PyMfcWindow::hitPlan(int id, int x, int y)
{ return m_host.planHit(id, x, y); }


bool PyMfcWindow::selectPlan(int id, const std::wstring& code)
{ return m_host.selectPlanItem(id, code); }


py::object PyMfcWindow::cardRectPlan(int id, const std::wstring& code)
{
        RECT rc = {};
        if (!m_host.planCardRect(id, code, rc))
            return py::none();
        return py::make_tuple(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
    }

void PyMfcWindow::setDetailCard(int id, py::list rect, py::list texts)
{ m_host.setDetailCard(id, std::move(rect), std::move(texts)); }

void PyMfcWindow::clearDetailCard(int id)
{ m_host.clearDetailCard(id); }

void PyMfcWindow::setCanvasEditable(int id, bool on)
{ m_host.setCanvasEditable(id, on); }


bool PyMfcWindow::getCanvasEditable(int id) const
{ return m_host.getCanvasEditable(id); }


int PyMfcWindow::getLevel(int id) const
{ return m_host.getLevel(id); }


void PyMfcWindow::setLevel(int id, int level)
{ m_host.setLevel(id, level); }


void PyMfcWindow::bringToFront(int id)
{ m_host.bringToFront(id); }


void PyMfcWindow::raiseMany(const std::vector<int>& ids)
{ m_host.raiseMany(ids); }

void PyMfcWindow::bringToFrontMany(const std::vector<int>& ids)
{ m_host.bringToFrontMany(ids); }

void PyMfcWindow::repaint(int id)
{ m_host.repaintCanvas(id); }


void PyMfcWindow::sendToBack(int id)
{ m_host.sendToBack(id); }


int PyMfcWindow::getTabIndex(int id) const
{ return m_host.getTabIndex(id); }


void PyMfcWindow::setTabIndex(int id, int n)
{ m_host.setTabIndex(id, n); }


int PyMfcWindow::getCanvasItemCount(int id) const
{ return m_host.canvasItemCount(id); }


std::wstring PyMfcWindow::getCanvasItemOp(int id, int index) const
{ return m_host.canvasItemOp(id, index); }


std::vector<int> PyMfcWindow::getCanvasItemBox(int id, int index) const
{
        int op = 0;
        RECT b = {0, 0, 0, 0};
        m_host.canvasItem(id, index, op, b);
        return std::vector<int>{static_cast<int>(b.left), static_cast<int>(b.top),
                                static_cast<int>(b.right), static_cast<int>(b.bottom)};
    }

std::vector<int> PyMfcWindow::getCanvasSelection(int id) const
{ return m_host.getCanvasSelection(id); }


void PyMfcWindow::setCanvasSelection(int id, const std::vector<int>& idx)
{ m_host.setCanvasSelection(id, idx); }


void PyMfcWindow::clearCanvasSelection(int id)
{ m_host.clearCanvasSelection(id); }


int PyMfcWindow::removeCanvasItems(int id, const std::vector<int>& idx)
{ return m_host.removeCanvasItems(id, idx); }


int PyMfcWindow::removeCanvasSelection(int id)
{ return m_host.removeCanvasSelection(id); }


void PyMfcWindow::onCanvasSelect(int id, py::object cb)
{ m_host.onCanvasSelect(id, std::move(cb)); }


void PyMfcWindow::setLayerOpacity(int id, int alpha)
{ m_host.setLayerOpacity(id, alpha); }


int PyMfcWindow::getLayerOpacity(int id) const
{ return m_host.getLayerOpacity(id); }


void PyMfcWindow::addListItem(int id, const std::wstring& line)
{ m_host.addListItem(id, line); }


void PyMfcWindow::clearListItems(int id)
{ m_host.clearListItems(id); }


int PyMfcWindow::getListItemCount(int id) const
{ return m_host.getListItemCount(id); }


std::wstring PyMfcWindow::getListItem(int id, int index) const
{ return m_host.getListItem(id, index); }


int PyMfcWindow::getListSelected(int id) const
{ return m_host.getListSelected(id); }


void PyMfcWindow::setListSelected(int id, int index)
{ m_host.setListSelected(id, index); }


int PyMfcWindow::getListScroll(int id) const
{ return m_host.getListScroll(id); }


void PyMfcWindow::setListScroll(int id, int pos)
{ m_host.setListScroll(id, pos); }


void PyMfcWindow::addPage(int id, const std::vector<int>& kids)
{ m_host.addPage(id, kids); }


void PyMfcWindow::clearPages(int id)
{ m_host.clearPages(id); }


int PyMfcWindow::getViewCount(int id) const
{ return m_host.viewCount(id); }


int PyMfcWindow::getViewIndex(int id) const
{ return m_host.viewIndex(id); }


void PyMfcWindow::setViewIndex(int id, int index)
{ m_host.viewSetIndex(id, index); }


void PyMfcWindow::animatePage(int id, int index, int durMs)
{ m_host.viewAnimate(id, index, durMs); }


void PyMfcWindow::setFlickContent(int id, const std::vector<int>& kids, int cw, int ch)
{ m_host.setFlickContent(id, kids, cw, ch); }


void PyMfcWindow::clearFlickContent(int id)
{ m_host.clearFlickContent(id); }


void PyMfcWindow::setFlickOffset(int id, int x, int y)
{ m_host.setFlickOffset(id, x, y); }


int PyMfcWindow::getFlickOffsetX(int id) const
{ return m_host.getFlickOffsetX(id); }


int PyMfcWindow::getFlickOffsetY(int id) const
{ return m_host.getFlickOffsetY(id); }


PyMfcPanel::PyMfcPanel(HWND parent)
{
        m_host = new PyMfcHost();
        CRect rc(0, 0, 280, 600);
        if (parent != nullptr && ::IsWindow(parent))
        {
            ::GetClientRect(parent, &rc);
            if (rc.Width() <= 0 || rc.Height() <= 0)
                rc.SetRect(0, 0, 280, 600);
        }
        if (!m_host->create(parent, rc))
        {
            delete m_host;
            m_host = nullptr;
        }
    }

PyMfcPanel::~PyMfcPanel()
{
        if (m_host != nullptr)
        {
            if (::IsWindow(m_host->GetSafeHwnd()))
                m_host->DestroyWindow();
            delete m_host;
            m_host = nullptr;
        }
    }

bool PyMfcPanel::valid() const
{ return m_host != nullptr; }


PyMfcHost* PyMfcPanel::Host() const
{ return m_host; }


HWND PyMfcPanel::hwnd() const
{ return m_host != nullptr ? m_host->GetSafeHwnd() : nullptr; }


void PyMfcPanel::resize()
{
        if (m_host == nullptr || !::IsWindow(m_host->GetSafeHwnd()))
            return;
        HWND parent = ::GetParent(m_host->GetSafeHwnd());
        if (parent == nullptr)
            return;
        CRect rc;
        ::GetClientRect(parent, &rc);
        ::MoveWindow(m_host->GetSafeHwnd(), 0, 0, rc.Width(), rc.Height(), TRUE);
    }

void PyMfcPanel::addLabel(int id, const std::wstring& text, int x, int y, int w, int h)
{ m_host->addLabel(id, text, x, y, w, h); }


void PyMfcPanel::addButton(int id, const std::wstring& text, int x, int y, int w, int h)
{ m_host->addButton(id, text, x, y, w, h); }


void PyMfcPanel::addEdit(int id, const std::wstring& text, int x, int y, int w, int h)
{ m_host->addEdit(id, text, x, y, w, h); }


void PyMfcPanel::addListBox(int id, int x, int y, int w, int h)
{ m_host->addListBox(id, x, y, w, h); }


void PyMfcPanel::addCheckBox(int id, const std::wstring& text, int x, int y, int w, int h)
{ m_host->addCheckBox(id, text, x, y, w, h); }


void PyMfcPanel::addRadio(int id, const std::wstring& text, int x, int y, int w, int h)
{ m_host->addRadio(id, text, x, y, w, h); }


void PyMfcPanel::addGroup(int id, const std::wstring& text, int x, int y, int w, int h)
{ m_host->addGroup(id, text, x, y, w, h); }


void PyMfcPanel::addCombo(int id, int x, int y, int w, int h)
{ m_host->addCombo(id, x, y, w, h); }


void PyMfcPanel::addEditAdvanced(int id, const std::wstring& text, bool multiline, bool readOnly,
                         int maxLength, const std::wstring& placeholder, int x, int y, int w, int h)
{
        m_host->addEditAdvanced(id, text, multiline, readOnly, maxLength, placeholder, x, y, w, h);
    }

void PyMfcPanel::addComboAdvanced(int id, bool dropDownList, int x, int y, int w, int h)
{
        m_host->addComboAdvanced(id, dropDownList, x, y, w, h);
    }

void PyMfcPanel::addListBoxAdvanced(int id, bool multipleSelect, int x, int y, int w, int h)
{
        m_host->addListBoxAdvanced(id, multipleSelect, x, y, w, h);
    }

void PyMfcPanel::addProgress(int id, int x, int y, int w, int h)
{ m_host->addProgress(id, x, y, w, h); }


void PyMfcPanel::addSlider(int id, int minValue, int maxValue, int x, int y, int w, int h)
{ m_host->addSlider(id, minValue, maxValue, x, y, w, h); }


void PyMfcPanel::addTab(int id, int x, int y, int w, int h)
{ m_host->addTab(id, x, y, w, h); }


void PyMfcPanel::addTree(int id, int x, int y, int w, int h)
{ m_host->addTree(id, x, y, w, h); }


void PyMfcPanel::addListCtrl(int id, int x, int y, int w, int h)
{ m_host->addListCtrl(id, x, y, w, h); }


void PyMfcPanel::addChoice(int id, int x, int y, int w, int h)
{ m_host->addChoice(id, x, y, w, h); }


void PyMfcPanel::addDatePicker(int id, int x, int y, int w, int h)
{ m_host->addDatePicker(id, x, y, w, h); }


void PyMfcPanel::addPanel(int id, int x, int y, int w, int h)
{ m_host->addPanel(id, x, y, w, h); }


void PyMfcPanel::addWhiteRect(int id, int x, int y, int w, int h)
{ m_host->addWhiteRect(id, x, y, w, h); }


void PyMfcPanel::addShadowBox(int id, int x, int y, int w, int h, int pad, int radius, int dx, int dy, int blur, const std::wstring& shadowColor, const std::wstring& bgColor, const std::wstring& backColor, const std::wstring& borderColor, int borderW)
{ m_host->addShadowBox(id, x, y, w, h, pad, radius, dx, dy, blur, shadowColor, bgColor, backColor, borderColor, borderW); }


void PyMfcPanel::setControlFont(int id, const std::wstring& face, int px, bool bold)
{ m_host->setControlFont(id, face, px, bold); }


void PyMfcPanel::setControlAccent(int id, int r, int g, int b)
{ m_host->setControlAccent(id, r, g, b); }


void PyMfcPanel::setLabelColor(int id, int r, int g, int b)
{ m_host->setLabelColor(id, r, g, b); }


void PyMfcPanel::addSplitter(int id, int x, int y, int w, int h)
{ m_host->addSplitter(id, x, y, w, h); }


void PyMfcPanel::addStaticLine(int id, int x, int y, int w, int h)
{ m_host->addStaticLine(id, x, y, w, h); }


void PyMfcPanel::addHyperlink(int id, const std::wstring& text, int x, int y, int w, int h)
{ m_host->addHyperlink(id, text, x, y, w, h); }


void PyMfcPanel::addRichText(int id, int x, int y, int w, int h)
{ m_host->addRichText(id, x, y, w, h); }


void PyMfcPanel::addToggle(int id, const std::wstring& text, int x, int y, int w, int h)
{ m_host->addToggle(id, text, x, y, w, h); }


void PyMfcPanel::addBitmapButton(int id, const std::wstring& text, int x, int y, int w, int h)
{ m_host->addBitmapButton(id, text, x, y, w, h); }


void PyMfcPanel::addStaticBitmap(int id, int x, int y, int w, int h)
{ m_host->addStaticBitmap(id, x, y, w, h); }


void PyMfcPanel::addGrid(int id, int x, int y, int w, int h)
{ m_host->addGrid(id, x, y, w, h); }


void PyMfcPanel::addScrolledWindow(int id, int x, int y, int w, int h)
{ m_host->addScrolledWindow(id, x, y, w, h); }


void PyMfcPanel::addCalendar(int id, int x, int y, int w, int h)
{ m_host->addCalendar(id, x, y, w, h); }


void PyMfcPanel::addTabItem(int id, const std::wstring& text)
{ m_host->addTabItem(id, text); }


void PyMfcPanel::addTreeNode(int id, const std::wstring& text, int parentIndex)
{ m_host->addTreeNode(id, text, parentIndex); }


void PyMfcPanel::addColumn(int id, const std::wstring& text, int width)
{ m_host->addColumn(id, text, width); }


void PyMfcPanel::addRow(int id, const std::wstring& text)
{ m_host->addRow(id, text); }


void PyMfcPanel::setUrl(int id, const std::wstring& url)
{ m_host->setUrl(id, url); }


std::wstring PyMfcPanel::getUrl(int id) const
{ return m_host->getUrl(id); }


bool PyMfcPanel::setImage(int id, const std::wstring& path)
{ return m_host->setImage(id, path); }


void PyMfcPanel::setRichFormat(int id, int sizePt, bool bold, bool italic, unsigned long rgb)
{ m_host->setRichFormat(id, sizePt, bold, italic, rgb); }


void PyMfcPanel::setScrollRange(int id, int minValue, int maxValue)
{ m_host->setScrollRange(id, minValue, maxValue); }


std::pair<int, int> PyMfcPanel::getScrollRange(int id) const
{ return m_host->getScrollRange(id); }


int PyMfcPanel::getScrollPos(int id) const
{ return m_host->getScrollPos(id); }


std::pair<int, int> PyMfcPanel::getSelection(int id) const
{ return m_host->getSelection(id); }


void PyMfcPanel::setSelection(int id, int from, int to) const
{ m_host->setSelection(id, from, to); }


void PyMfcPanel::onClick(int id, py::object cb)
{ m_host->onClick(id, std::move(cb)); }


void PyMfcPanel::setWheelRegion(int x, int y, int w, int h, int step)
{ m_host->setWheelRegion(x, y, w, h, step); }


void PyMfcPanel::onWheel(py::object cb)
{ m_host->onWheel(std::move(cb)); }


void PyMfcPanel::onChange(int id, py::object cb)
{ m_host->onChange(id, std::move(cb)); }


void PyMfcPanel::onSelect(int id, py::object cb)
{ m_host->onSelect(id, std::move(cb)); }


void PyMfcPanel::onDoubleClick(int id, py::object cb)
{ m_host->onDoubleClick(id, std::move(cb)); }


void PyMfcPanel::onDropDown(int id, py::object cb)
{ m_host->onDropDown(id, std::move(cb)); }


void PyMfcPanel::onValue(int id, py::object cb)
{ m_host->onValue(id, std::move(cb)); }


void PyMfcPanel::onChecked(int id, py::object cb)
{ m_host->onChecked(id, std::move(cb)); }


void PyMfcPanel::setText(int id, const std::wstring& text)
{ m_host->setText(id, text); }


std::wstring PyMfcPanel::getText(int id) const
{ return m_host->getText(id); }


void PyMfcPanel::move(int id, int x, int y, int w, int h)
{ m_host->moveControl(id, x, y, w, h); }


void PyMfcPanel::moveMany(py::list items)
{ m_host->moveMany(std::move(items)); }


bool PyMfcPanel::setXamlValue(const std::wstring& name, const std::wstring& prop, const std::wstring& value)
{
        return m_host->setXamlProperty(name, prop, value);
    }

std::wstring PyMfcPanel::getXamlValue(int id, const std::wstring& prop)
{ return m_host->getXamlValue(id, prop); }

bool PyMfcPanel::reflowXaml()
{ return m_host->reflowXaml(); }


void PyMfcPanel::setVisible(int id, bool visible)
{ m_host->setVisible(id, visible); }


void PyMfcPanel::setEnabled(int id, bool enabled)
{ m_host->setEnabled(id, enabled); }


void PyMfcPanel::addString(int id, const std::wstring& text)
{ m_host->addString(id, text); }


void PyMfcPanel::clearStrings(int id)
{ m_host->clearStrings(id); }


int PyMfcPanel::countStrings(int id) const
{ return m_host->countStrings(id); }


std::wstring PyMfcPanel::getString(int id, int index) const
{ return m_host->getString(id, index); }


void PyMfcPanel::deleteString(int id, int index)
{ m_host->deleteString(id, index); }


int PyMfcPanel::getCurSel(int id) const
{ return m_host->getCurSel(id); }


void PyMfcPanel::setCurSel(int id, int index)
{ m_host->setCurSel(id, index); }


bool PyMfcPanel::saveControlPng(int id, const std::wstring& path)
{ return m_host->saveControlPng(id, path); }


int PyMfcPanel::getCheck(int id) const
{ return m_host->getCheck(id); }


void PyMfcPanel::setCheck(int id, bool checked)
{ m_host->setCheck(id, checked); }


void PyMfcPanel::setRange(int id, int minValue, int maxValue)
{ m_host->setRange(id, minValue, maxValue); }


void PyMfcPanel::setPos(int id, int value)
{ m_host->setPos(id, value); }


int PyMfcPanel::getPos(int id) const
{ return m_host->getPos(id); }


void PyMfcPanel::setReadOnly(int id, bool readOnly)
{ m_host->setReadOnly(id, readOnly); }


bool PyMfcPanel::getReadOnly(int id) const
{ return m_host->getReadOnly(id); }


void PyMfcPanel::setFocusTo(int id)
{ m_host->setFocusTo(id); }


bool PyMfcPanel::getVisible(int id) const
{ return m_host->getVisible(id); }


bool PyMfcPanel::getEnabled(int id) const
{ return m_host->getEnabled(id); }


bool PyMfcPanel::getSwitch(int id) const
{ return m_host->getSwitchValue(id); }


void PyMfcPanel::setSwitch(int id, bool on)
{ m_host->setSwitchValue(id, on); }


int PyMfcPanel::getSpin(int id) const
{ return m_host->getSpinValue(id); }


void PyMfcPanel::setSpin(int id, int v)
{ m_host->setSpinValue(id, v); }


void PyMfcPanel::setSpinRange(int id, int lo, int hi)
{ m_host->setSpinRange(id, lo, hi); }


void PyMfcPanel::setRangeValues(int id, int lo, int hi)
{ m_host->setRangeValues(id, lo, hi); }


int PyMfcPanel::getRangeLow(int id) const
{ return m_host->getRangeLow(id); }


int PyMfcPanel::getRangeHigh(int id) const
{ return m_host->getRangeHigh(id); }


void PyMfcPanel::setPageCount(int id, int count)
{ m_host->setPageCount(id, count); }


int PyMfcPanel::getPageCount(int id) const
{ return m_host->getPageCount(id); }


void PyMfcPanel::setPageIndex(int id, int index)
{ m_host->setPageIndex(id, index); }


int PyMfcPanel::getPageIndex(int id) const
{ return m_host->getPageIndex(id); }


int PyMfcPanel::getDial(int id) const
{ return m_host->getDial(id); }


void PyMfcPanel::setDial(int id, int v)
{ m_host->setDial(id, v); }


int PyMfcPanel::getTumblerCount(int id) const
{ return m_host->getTumblerCount(id); }


void PyMfcPanel::setTumblerCount(int id, int c)
{ m_host->setTumblerCount(id, c); }


int PyMfcPanel::getTumblerValue(int id) const
{ return m_host->getTumblerValue(id); }


void PyMfcPanel::setTumblerValue(int id, int v)
{ m_host->setTumblerValue(id, v); }


void PyMfcPanel::setScrollBarRange(int id, int lo, int hi, int page, bool vertical)
{ m_host->setScrollBarRange(id, lo, hi, page, vertical); }


int PyMfcPanel::getScrollBarValue(int id) const
{ return m_host->getScrollBarValue(id); }


void PyMfcPanel::setScrollBarValue(int id, int v)
{ m_host->setScrollBarValue(id, v); }


void PyMfcPanel::setTableColumns(int id, const std::wstring& s)
{ m_host->setTableColumns(id, s); }


void PyMfcPanel::addTableRow(int id, const std::wstring& s)
{ m_host->addTableRow(id, s); }

void PyMfcPanel::setTableRows(int id, const std::wstring& blob)
{ m_host->setTableRows(id, blob); }


void PyMfcPanel::setColumnWidth(int id, int col, int px)
{ m_host->setColumnWidth(id, col, px); }

void PyMfcPanel::setColumnWidths(int id, const std::vector<int>& widths)
{ m_host->setColumnWidths(id, widths); }


py::object PyMfcPanel::getTableMetrics(int id)
{ return m_host->tableMetrics(id); }


void PyMfcPanel::onBlur(int id, py::object cb)
{ m_host->onBlur(id, cb); }


void PyMfcPanel::onDrag(int id, py::object cb)
{ m_host->onDrag(id, cb); m_host->installDragBar(id); }


py::object PyMfcPanel::cellRect(int id, int row, int col)
{ return m_host->cellRect(id, row, col); }


void PyMfcPanel::onGridAction(py::object cb)
{ m_host->onGridAction(std::move(cb)); }

void PyMfcPanel::onGridActionFor(int id, py::object cb)
{ m_host->onGridActionFor(id, std::move(cb)); }

void PyMfcPanel::onControlEvent(int id, const std::wstring& ev, py::object cb)
{ m_host->onControlEvent(id, ev, std::move(cb)); }


void PyMfcPanel::clearTableRows(int id)
{ m_host->clearTableRows(id); }


int PyMfcPanel::getTableRowCount(int id) const
{ return m_host->getTableRowCount(id); }


std::wstring PyMfcPanel::getTableCell(int id, int r, int c) const
{ return m_host->getTableCell(id, r, c); }


void PyMfcPanel::setTableCell(int id, int r, int c, const std::wstring& s)
{ m_host->setTableCell(id, r, c, s); }


int PyMfcPanel::getTableSelectedRow(int id) const
{ return m_host->getTableSelectedRow(id); }


void PyMfcPanel::setTableSelectedRow(int id, int r)
{ m_host->setTableSelectedRow(id, r); }


void PyMfcPanel::sortTable(int id, int col)
{ m_host->sortTable(id, col); }


void PyMfcPanel::addTable(int id, int x, int y, int w, int h)
{ m_host->addTable(id, x, y, w, h); }

void PyMfcPanel::tableClear(int id)
{ m_host->tableClear(id); }

int PyMfcPanel::tableRowCount(int id) const
{ return m_host->tableRowCount(id); }

void PyMfcPanel::tableSetColumns(int id, const std::vector<int>& widths,
                                  const std::vector<std::wstring>& titles)
{ m_host->tableSetColumns(id, widths, titles); }

void PyMfcPanel::tableSetColTypes(int id, const std::vector<int>& types)
{ m_host->tableSetColTypes(id, types); }

void PyMfcPanel::appendTableRow(int id, const std::vector<std::wstring>& cells)
{ m_host->tableAppendRow(id, cells); }

void PyMfcPanel::tableSetCell(int id, int row, int col, const std::wstring& s)
{ m_host->tableSetCell(id, row, col, s); }

std::wstring PyMfcPanel::tableGetCell(int id, int row, int col) const
{ return m_host->tableGetCell(id, row, col); }

py::object PyMfcPanel::tableCellRect(int id, int row, int col) const
{ return m_host->tableCellRect(id, row, col); }

int PyMfcPanel::tableSelectedRow(int id) const
{ return m_host->tableSelectedRow(id); }

void PyMfcPanel::tableSetSelectedRow(int id, int row)
{ m_host->tableSelectRow(id, row); }

void PyMfcPanel::tableEnsureVisible(int id, int row)
{ m_host->tableEnsureVisible(id, row); }

void PyMfcPanel::tableSetFilter(int id, int col, const std::wstring& s)
{ m_host->tableSetFilter(id, col, s); }

void PyMfcPanel::tableClearFilters(int id)
{ m_host->tableClearFilters(id); }

void PyMfcPanel::tableSort(int id, int col, bool desc)
{ m_host->tableSort(id, col, desc); }

int PyMfcPanel::tableViewRows(int id) const
{ return m_host->tableViewRows(id); }


void PyMfcPanel::addTreeItem(int id, const std::wstring& s, int level, bool kids)
{ m_host->addTreeItem(id, s, level, kids); }


void PyMfcPanel::clearTreeItems(int id)
{ m_host->clearTreeItems(id); }


int PyMfcPanel::getTreeSelectedRow(int id) const
{ return m_host->getTreeSelectedRow(id); }


bool PyMfcPanel::isTreeItemCollapsed(int id, int r) const
{ return m_host->isTreeItemCollapsed(id, r); }


void PyMfcPanel::setTreeItemCollapsed(int id, int r, bool c)
{ m_host->setTreeItemCollapsed(id, r, c); }


void PyMfcPanel::setTreeSelectedRow(int id, int r)
{ m_host->setTreeSelectedRow(id, r); }


void PyMfcPanel::addToolBarItem(int id, const std::wstring& s)
{ m_host->addToolBarItem(id, s); }


void PyMfcPanel::clearToolBarItems(int id)
{ m_host->clearToolBarItems(id); }


int PyMfcPanel::getToolBarSelected(int id) const
{ return m_host->getToolBarSelected(id); }


void PyMfcPanel::setToolBarSelected(int id, int i)
{ m_host->setToolBarSelected(id, i); }


void PyMfcPanel::animate(int id, const std::wstring& prop, double to, int dur, const std::string& ease,
                 int delay, bool loop)
{ m_host->animate(id, prop, to, dur, ease, delay, loop); }


void PyMfcPanel::animateFrom(int id, const std::wstring& prop, double from, double to, int dur,
                     const std::string& ease, int delay, bool loop)
{ m_host->animateFrom(id, prop, from, to, dur, ease, delay, loop); }


void PyMfcPanel::animateColor(int id, int rgb, int dur, const std::string& ease, int delay)
{ m_host->animateColor(id, static_cast<COLORREF>(rgb), dur, ease, delay); }


void PyMfcPanel::stopAnimations(int id)
{ m_host->stopAnimations(id); }


void PyMfcPanel::setBehavior(int id, const std::wstring& prop, int dur, const std::string& ease)
{ m_host->setBehavior(id, prop, dur, ease); }


void PyMfcPanel::showToolTip(int ownerId, const std::wstring& text, int durationMs)
{ m_host->showToolTip(ownerId, text, durationMs); }


void PyMfcPanel::hideToolTip()
{ m_host->hideToolTip(); }


void PyMfcPanel::showMenu(int id, const std::vector<std::wstring>& items, int x, int y)
{ m_host->showMenu(id, items, x, y); }


void PyMfcPanel::hideMenu(int id)
{ m_host->hideMenu(id); }


int PyMfcPanel::getMenuSelected(int id) const
{ return m_host->getMenuSelected(id); }


void PyMfcPanel::setMenuItems(int id, const std::vector<std::wstring>& items)
{ m_host->setMenuItems(id, items); }


void PyMfcPanel::addMenuBarItem(int id, const std::wstring& text)
{ m_host->addMenuBarItem(id, text); }


void PyMfcPanel::clearMenuBarItems(int id)
{ m_host->clearMenuBarItems(id); }


int PyMfcPanel::getMenuBarSelected(int id) const
{ return m_host->getMenuBarSelected(id); }


void PyMfcPanel::setMenuBarSelected(int id, int i)
{ m_host->setMenuBarSelected(id, i); }


void PyMfcPanel::showPopup(int id, const std::wstring& title, const std::vector<std::wstring>& lines,
                   int x, int y, int w, int h)
{ m_host->showPopup(id, title, lines, x, y, w, h); }


void PyMfcPanel::hidePopup(int id)
{ m_host->hidePopup(id); }


bool PyMfcPanel::isPopupVisible(int id) const
{ return m_host->isPopupVisible(id); }


void PyMfcPanel::setCanvasCommands(int id, const std::wstring& cmds)
{ m_host->setCanvasCommands(id, cmds); }


std::wstring PyMfcPanel::getCanvasCommands(int id) const
{ return m_host->getCanvasCommands(id); }


void PyMfcPanel::addPowerCanvas(int id, int x, int y, int w, int h)
{ m_host->addPowerCanvas(id, x, y, w, h); }


void PyMfcPanel::loadPowerPlan(int id, const std::wstring& json)
{ m_host->setPowerCanvasPlan(id, json); }


void PyMfcPanel::loadPowerScene(int id, const std::wstring& json)
{ m_host->setPowerCanvasScene(id, json); }

void PyMfcPanel::copyPowerScene(int toId, int fromId)
{ m_host->copyPowerCanvasScene(toId, fromId); }


void PyMfcPanel::loadPowerSvg(int id, const std::wstring& svg)
{ m_host->setPowerCanvasSvg(id, svg); }


void PyMfcPanel::setPlanView(int id, float zoom, int ox, int oy)
{ m_host->setPowerCanvasView(id, zoom, ox, oy); }


float PyMfcPanel::getPlanZoom(int id) const
{ return m_host->getPowerCanvasZoom(id); }


std::wstring PyMfcPanel::getPlanSelection(int id) const
{ return m_host->getPowerCanvasSelection(id); }


void PyMfcPanel::setPlanOptions(int id, bool showCable, bool mini, int miniOf)
{ m_host->setPowerCanvasOptions(id, showCable, mini, miniOf); }


py::object PyMfcPanel::getPlanView(int id) const
{
        float zoom = 0.0f;
        int ox = 0, oy = 0;
        m_host->getPowerCanvasView(id, zoom, ox, oy);
        return py::make_tuple(zoom, ox, oy);
    }

std::wstring PyMfcPanel::hitPlan(int id, int x, int y)
{ return m_host->planHit(id, x, y); }


bool PyMfcPanel::selectPlan(int id, const std::wstring& code)
{ return m_host->selectPlanItem(id, code); }

void PyMfcPanel::setDetailCard(int id, py::list rect, py::list texts)
{ m_host->setDetailCard(id, std::move(rect), std::move(texts)); }

void PyMfcPanel::clearDetailCard(int id)
{ m_host->clearDetailCard(id); }

void PyMfcPanel::setCanvasEditable(int id, bool on)
{ m_host->setCanvasEditable(id, on); }


bool PyMfcPanel::getCanvasEditable(int id) const
{ return m_host->getCanvasEditable(id); }


int PyMfcPanel::getLevel(int id) const
{ return m_host->getLevel(id); }


void PyMfcPanel::setLevel(int id, int level)
{ m_host->setLevel(id, level); }


void PyMfcPanel::bringToFront(int id)
{ m_host->bringToFront(id); }


void PyMfcPanel::raiseMany(const std::vector<int>& ids)
{ m_host->raiseMany(ids); }

void PyMfcPanel::bringToFrontMany(const std::vector<int>& ids)
{ m_host->bringToFrontMany(ids); }

void PyMfcPanel::repaint(int id)
{ m_host->repaintCanvas(id); }


void PyMfcPanel::sendToBack(int id)
{ m_host->sendToBack(id); }


int PyMfcPanel::getTabIndex(int id) const
{ return m_host->getTabIndex(id); }


void PyMfcPanel::setTabIndex(int id, int n)
{ m_host->setTabIndex(id, n); }


int PyMfcPanel::getCanvasItemCount(int id) const
{ return m_host->canvasItemCount(id); }


std::wstring PyMfcPanel::getCanvasItemOp(int id, int index) const
{ return m_host->canvasItemOp(id, index); }


std::vector<int> PyMfcPanel::getCanvasItemBox(int id, int index) const
{
        int op = 0;
        RECT b = {0, 0, 0, 0};
        m_host->canvasItem(id, index, op, b);
        return std::vector<int>{static_cast<int>(b.left), static_cast<int>(b.top),
                                static_cast<int>(b.right), static_cast<int>(b.bottom)};
    }

std::vector<int> PyMfcPanel::getCanvasSelection(int id) const
{ return m_host->getCanvasSelection(id); }


void PyMfcPanel::setCanvasSelection(int id, const std::vector<int>& idx)
{ m_host->setCanvasSelection(id, idx); }


void PyMfcPanel::clearCanvasSelection(int id)
{ m_host->clearCanvasSelection(id); }


int PyMfcPanel::removeCanvasItems(int id, const std::vector<int>& idx)
{ return m_host->removeCanvasItems(id, idx); }


int PyMfcPanel::removeCanvasSelection(int id)
{ return m_host->removeCanvasSelection(id); }


void PyMfcPanel::onCanvasSelect(int id, py::object cb)
{ m_host->onCanvasSelect(id, std::move(cb)); }


void PyMfcPanel::setLayerOpacity(int id, int alpha)
{ m_host->setLayerOpacity(id, alpha); }


int PyMfcPanel::getLayerOpacity(int id) const
{ return m_host->getLayerOpacity(id); }


void PyMfcPanel::addListItem(int id, const std::wstring& line)
{ m_host->addListItem(id, line); }


void PyMfcPanel::clearListItems(int id)
{ m_host->clearListItems(id); }


int PyMfcPanel::getListItemCount(int id) const
{ return m_host->getListItemCount(id); }


std::wstring PyMfcPanel::getListItem(int id, int index) const
{ return m_host->getListItem(id, index); }


int PyMfcPanel::getListSelected(int id) const
{ return m_host->getListSelected(id); }


void PyMfcPanel::setListSelected(int id, int index)
{ m_host->setListSelected(id, index); }


int PyMfcPanel::getListScroll(int id) const
{ return m_host->getListScroll(id); }


void PyMfcPanel::setListScroll(int id, int pos)
{ m_host->setListScroll(id, pos); }


void PyMfcPanel::addPage(int id, const std::vector<int>& kids)
{ m_host->addPage(id, kids); }


void PyMfcPanel::clearPages(int id)
{ m_host->clearPages(id); }


int PyMfcPanel::getViewCount(int id) const
{ return m_host->viewCount(id); }


int PyMfcPanel::getViewIndex(int id) const
{ return m_host->viewIndex(id); }


void PyMfcPanel::setViewIndex(int id, int index)
{ m_host->viewSetIndex(id, index); }


void PyMfcPanel::animatePage(int id, int index, int durMs)
{ m_host->viewAnimate(id, index, durMs); }


void PyMfcPanel::setFlickContent(int id, const std::vector<int>& kids, int cw, int ch)
{ m_host->setFlickContent(id, kids, cw, ch); }


void PyMfcPanel::clearFlickContent(int id)
{ m_host->clearFlickContent(id); }


int PyMfcPanel::getFlickOffsetX(int id) const
{ return m_host->getFlickOffsetX(id); }


int PyMfcPanel::getFlickOffsetY(int id) const
{ return m_host->getFlickOffsetY(id); }

