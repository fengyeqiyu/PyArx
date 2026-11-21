// PyMfcHost.Layout.cpp —— 拆分自 PyMfcHost.cpp（按职责分组；接口声明仍在 PyMfcHost.h）。
#include "PyMfcHostInternal.h"

void PyMfcHost::setVisible(int id, bool visible)
{
    // 显隐状态必须由宿主自己记一份：布局层（reflowXml/moveMany）会把"元素声明的可见"
    // 重新下发成 SWP_SHOWWINDOW，若只 ShowWindow 一次，下一次重排就把它放回来了 ——
    // 这正是"写了 Visible=False 却还看得见"的真实原因。
    if (visible)
        m_hidden.erase(id);
    else
        m_hidden.insert(id);
    if (auto it = m_nodes.find(id); it != m_nodes.end())
    {
        it->second.visible = visible;
        it->second.dirty = true;
    }
    if (HWND h = findByIdDeep(m_hWnd, id))
        ::ShowWindow(h, visible ? SW_SHOW : SW_HIDE);
    if (m_hWnd != nullptr)
        ::InvalidateRect(m_hWnd, nullptr, FALSE);
}

bool PyMfcHost::isControlHidden(int id) const
{
    return m_hidden.count(id) != 0;
}

void PyMfcHost::setEnabled(int id, bool enabled)
{
        if (HWND h = findByIdDeep(m_hWnd, id))
            ::EnableWindow(h, enabled ? TRUE : FALSE);
    }

void PyMfcHost::moveControl(int id, int x, int y, int w, int h)
{
    updatePaintNodeRect(id, x, y, w, h, !isControlHidden(id));
    // 对外坐标语义一律是「相对宿主客户区」。控件若已被 SetParent 到 flex 容器，
    // GetDlgItem 在宿主下找不到，且父坐标系也不是宿主 —— 必须深查找 + 换算
    HWND hwnd = findByIdDeep(m_hWnd, id);
    if (hwnd != nullptr)
    {
        HWND parent = ::GetParent(hwnd);
        if (parent != nullptr && parent != m_hWnd)
        {
            POINT pt = { x, y };
            ::MapWindowPoints(m_hWnd, parent, &pt, 1);
            x = pt.x;
            y = pt.y;
        }
        // 位置与尺寸都没变就不搬：MoveWindow(bRepaint=TRUE) 会连整棵子树一起白重绘一次。
        // reflowXml 每帧对全树逐节点调用本函数，静态界面下绝大多数节点是空转。
        HWND ref = (parent != nullptr) ? parent : m_hWnd;
        RECT cur = {};
        if (ref != nullptr && ::GetWindowRect(hwnd, &cur))
        {
            POINT pts[2] = { { cur.left, cur.top }, { cur.right, cur.bottom } };
            ::MapWindowPoints(nullptr, ref, reinterpret_cast<POINT*>(pts), 2);
            const int cw = pts[1].x - pts[0].x;
            const int ch = pts[1].y - pts[0].y;
            if (pts[0].x == x && pts[0].y == y && cw == w && ch == h)
            {
                syncOneContainerBackdrop(id, x, y, w, h);
                return;
            }
        }
        ::MoveWindow(hwnd, x, y, w, h, TRUE);
    }
    syncOneContainerBackdrop(id, x, y, w, h);   // 容器落位后登记背景节点（外观推送早于控件创建）
}

void PyMfcHost::moveMany(py::list items)
{
        const int n = static_cast<int>(items.size());
        if (n <= 0 || m_hWnd == nullptr)
            return;
        HDWP dwp = ::BeginDeferWindowPos(n);
        if (dwp == nullptr)
            return;
        std::vector<HWND> moved;
        moved.reserve(static_cast<size_t>(n));
        std::vector<RECT> oldRects;              // 每个控件挪动前的父客户区矩形
        oldRects.reserve(static_cast<size_t>(n));
        for (py::handle h : items)
        {
            py::sequence s = py::reinterpret_borrow<py::sequence>(h);
            if (s.size() < 6)
                continue;
            const int id = py::cast<int>(s[0]);
            HWND c = findByIdDeep(m_hWnd, id);
            if (c == nullptr)
                continue;
            const int ax = py::cast<int>(s[1]);   // 相对宿主客户区（对外语义）
            const int ay = py::cast<int>(s[2]);
            int x = ax;
            int y = ay;
            if (const HWND parent = ::GetParent(c); parent != nullptr && parent != m_hWnd)
            {
                POINT pt = { ax, ay };
                ::MapWindowPoints(m_hWnd, parent, &pt, 1);
                x = pt.x;
                y = pt.y;
            }
            const int w = py::cast<int>(s[3]);
            const int hh = py::cast<int>(s[4]);
            const bool vis = py::cast<bool>(s[5]) && !isControlHidden(id);
            updatePaintNodeRect(id, ax, ay, w, hh, vis);
            // 批量路径也要给容器登记背景节点（布局主要走 moveMany，不是 moveControl）
            syncOneContainerBackdrop(id, ax, ay, w, hh);
            UINT flags = SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW;
            flags |= vis ? SWP_SHOWWINDOW : SWP_HIDEWINDOW;
            RECT before = { 0, 0, 0, 0 };
            ::GetWindowRect(c, &before);
            ::MapWindowPoints(HWND_DESKTOP, m_hWnd, reinterpret_cast<LPPOINT>(&before), 2);
            const bool curVis = ::IsWindowVisible(c) != 0;
            const bool sameRect = before.left == ax && before.top == ay &&
                                  before.right == ax + w && before.bottom == ay + hh;
            if (sameRect && curVis == vis)
                continue;  // 位置与显隐都没变：不进 DeferWindowPos，也不重复失效
            dwp = ::DeferWindowPos(dwp, c, nullptr, x, y, w, hh, flags);
            if (dwp == nullptr)
                return;
            moved.push_back(c);
            oldRects.push_back(before);
        }
        ::EndDeferWindowPos(dwp);
        // 只把"动过的子窗口"和父窗口露出的空白标脏。不用 RDW_ALLCHILDREN，
        // 否则会把 D2D 画布也拉进来整张重画，滚动一下子就卡。
        // erase=FALSE：控件都是整块自绘（画布自己 fillRect 底色、原生控件自己画底），
        // 带 TRUE 会先擦成一窗白再画，肉眼就是"闪一下"。
        for (HWND c : moved)
        {
            ::InvalidateRect(c, nullptr, FALSE);
            // 关键：控件挪走后，旧位置是**父容器**的地盘。父容器自己不被标脏，
            // 主窗又被 WS_CLIPCHILDREN 裁掉这一块 —— 旧内容就留在屏幕上，即"拖影"。
            if (HWND p = ::GetParent(c); p != nullptr && p != m_hWnd)
                ::InvalidateRect(p, nullptr, FALSE);
        }
        // 父窗口只标"动过的并集矩形"，不再整窗 RDW_INVALIDATE：
        // 整窗标脏会把工具栏、状态栏、小地图一起拉来重画，而它们根本没动。
        RECT uni = { 0, 0, 0, 0 };
        bool hasUni = false;
        for (size_t i = 0; i < moved.size(); ++i)
        {
            RECT now = { 0, 0, 0, 0 };
            ::GetWindowRect(moved[i], &now);
            ::MapWindowPoints(HWND_DESKTOP, m_hWnd, reinterpret_cast<LPPOINT>(&now), 2);
            RECT both = oldRects[i];
            ::UnionRect(&both, &oldRects[i], &now);
            if (!hasUni)
            {
                uni = both;
                hasUni = true;
            }
            else
            {
                ::UnionRect(&uni, &uni, &both);
            }
        }
        if (hasUni)
            ::InvalidateRect(m_hWnd, &uni, FALSE);
    }

void PyMfcHost::applyContainerBackdrop(int id, int x, int y, int w, int h,
                                       const xmlapp::XmlAppearance& ap)
{
    // 旧 GDI 路径下绘制节点只登记不出帧，这里也不注册，保持老行为
    if (!m_newSurface)
        return;

    COLORREF bc = 0;
    const bool hasBack = !ap.backColor.empty() && xmlapp::ParseColor(ap.backColor, bc);
    const bool hasBorder = !ap.borderStyle.empty() &&
                           _wcsicmp(ap.borderStyle.c_str(), L"None") != 0;
    // 纯渐变容器（只声明 background:linear-gradient）同样要登记成背板节点，
    // 否则在表面路径会被当成"无背板"摘掉，渐变整面留空。
    const bool hasGradient = !ap.bgImage.empty();
    debugLogf(L"[backdrop] id=%d rect=%d,%d,%d,%d back='%ls' border='%ls' hasBack=%d",
                     id, x, y, w, h, ap.backColor.c_str(), ap.borderStyle.c_str(),
                     hasBack ? 1 : 0);

    auto it = m_nodes.find(id);
    if (!hasBack && !hasBorder && !hasGradient)
    {
        // 没有可见背板（div 默认 transparent）：必须摘掉，否则容器改透明后还留着旧底色
        if (it != m_nodes.end() && it->second.kind == 6)
        {
            m_nodes.erase(it);
            m_nodeOrder.erase(std::remove(m_nodeOrder.begin(), m_nodeOrder.end(), id),
                              m_nodeOrder.end());
            ::InvalidateRect(m_hWnd, nullptr, FALSE);
        }
        return;
    }

    if (it != m_nodes.end())
    {
        if (it->second.kind != 6)
            return;                     // 该 id 是 owner-draw 控件节点，别抢它的绘制
        it->second.rc = RECT{ x, y, x + w, y + h };
        it->second.dirty = true;
    }
    else
    {
        registerPaintNode(id, 6, NodeLayerBackdrop, x, y, w, h);
    }
    // 不能藏容器窗口：容器（含 page 这类整页容器）虽然是空 STATIC，但它本身也承载
    // 自己的 WM_DRAWITEM 自绘（SS_OWNERDRAW），一藏就整页空白。空 STATIC 且无 SS 样式时
    // 它不会刷背景，留着也不盖住父 DC 上的背板。
    ::InvalidateRect(m_hWnd, nullptr, FALSE);
}

void PyMfcHost::attachXml(std::shared_ptr<XmlRuntime> rt, const std::map<std::wstring, int>& ids,
                           py::dict names, py::dict context, int nextId,
                           void* ownerWindow, void* ownerPage)
{
        m_xml = std::move(rt);
        m_xmlIds = ids;
        m_xmlNames = std::move(names);
        m_xmlContext = std::move(context);
        m_xmlNextId = nextId;
        m_xmlOwnerWindow = ownerWindow;
        m_xmlOwnerPage = ownerPage;
    }

void PyMfcHost::removeControlById(int id)
{
        if (m_hWnd != nullptr)
        {
            if (HWND h = findByIdDeep(m_hWnd, id))
            {
                ::SetWindowLongPtrW(h, GWLP_ID, 0);
                ::DestroyWindow(h);
            }
        }
        for (auto it = m_xmlIds.begin(); it != m_xmlIds.end();)
        {
            if (it->second == id)
                it = m_xmlIds.erase(it);
            else
                ++it;
        }
    }

bool PyMfcHost::reflowXml()
{
        if (!m_xml || m_xml->root() == nullptr)
            return false;
        CRect rc;
        ::GetClientRect(m_hWnd, &rc);
        m_xml->applyExpressions();
        if (xmlprop2::EnvFlagOn(L"PYARX_XML_LAYOUT_DBG"))
        {
            FILE* f = nullptr;
            const std::wstring p = xmlprop2::LogDir() + L"\\xml_layout.log";
            if (_wfopen_s(&f, p.c_str(), L"a") == 0 && f != nullptr)
            {
                fputs("--- call PyMfcHost::reflowXml\n", f);
                fclose(f);
            }
        }
        m_xml->layout(0, 0, rc.Width(), rc.Height());

        int moved = 0, texts = 0, skipped = 0;
        // P0「搬运算子单一化」：flex 容器的子项由容器自己搬（XmlFlexPanel::reflowItems，
        // 用「容器原点 + rel」拼绝对），宿主不再用 XmlCore 的 preview 再搬一次。
        // 容器在下面先被 move（父先子后），尺寸一变它自己收到 WM_SIZE 就重排，顺序是对的。
        std::function<void(XmlElement&, bool)> walk = [&](XmlElement& el, bool underPanel) {
            auto hit = m_xmlIds.find(el.name);
            if (!el.name.empty() && hit != m_xmlIds.end())
            {
                // 容器只搬它自己的 flex 项（m_items）；写了 left/top（绝对定位）的子项
                // 由 XmlCore 的布局落位，仍归宿主搬 —— 否则这类子窗口永远停在创建时
                // 的 0×0 / 错位状态（div 里 `<label style="left:10;width:160">` 就是这种）。
                const bool freeChild = el.hasX || el.hasY ||
                                       !el.get(L"Right").empty() || !el.get(L"Bottom").empty();
                if (underPanel && !freeChild)
                {
                    ++skipped;
                }
                else
                {
                    moveControl(hit->second, el.previewX, el.previewY, el.previewW, el.previewH);
                    ++moved;
                }
                // 只有"算出来的文字"才回写控件：普通 Text 是用户输入初值，不能覆盖
                const std::wstring* rawText = nullptr;
                for (const auto& kv : el.attrs)
                    if (_wcsicmp(kv.first.c_str(), L"Text") == 0)
                        rawText = &kv.second;
                if (rawText != nullptr && xmlIsExpression(*rawText))
                {
                    if (HWND h = ::GetDlgItem(m_hWnd, hit->second))
                    {
                        ::SetWindowTextW(h, el.get(L"Text").c_str());
                        ++texts;
                    }
                }
            }
            // 容器标签有 HTML 归一化后的 XFlexPanel，也有沿用老写法的 FlexPanel / FlexBox
            const bool selfPanel = _wcsicmp(el.tag.c_str(), L"XFlexPanel") == 0 ||
                                   _wcsicmp(el.tag.c_str(), L"FlexPanel") == 0 ||
                                   _wcsicmp(el.tag.c_str(), L"FlexBox") == 0 ||
                                   _wcsicmp(el.tag.c_str(), L"XFlexBox") == 0;
            for (auto& c : el.children)
                walk(c, selfPanel);
        };
        walk(*m_xml->root(), false);
        syncContainerBackdrops();   // 布局完再把"容器背景"登记成绘制节点（此时窗口已建好）
        m_backdropSyncDirty = false;
        ::InvalidateRect(m_hWnd, nullptr, TRUE);
        (void)texts;
        return moved > 0;
    }

void PyMfcHost::syncOneContainerBackdrop(int id, int x, int y, int w, int h)
{
    // 容器（div/面板）是空 STATIC、宿主不做 owner-draw，Background 只能由绘制节点画。
    // 外观推送发生在控件创建之前（那时 GetDlgItem 还是空），所以挂在布局落位处：
    // 控件已存在，rect 用布局算出的最终值（比 GetWindowRect 更准，moveMany 还没挪）。
    if (m_hWnd == nullptr)
        return;
    auto apIt = m_ap.find(id);
    if (apIt == m_ap.end() || apIt->second.empty())
        return;
    HWND hw = findByIdDeep(m_hWnd, id);
    if (hw == nullptr)
        return;
    wchar_t cls[64] = {};
    ::GetClassNameW(hw, cls, 64);
    const int textLen = static_cast<int>(::GetWindowTextLengthW(hw));
    // 容器身份以 addPanel 登记为准：XML 老写法的面板 STATIC 会带 Text，
    // 按"空 STATIC 才是容器"判会把 XML 面板整批漏登记，表现为整页空白。
    if (m_containerIds.count(id) == 0)
    {
        debugLogf(L"[bd-one] id=%d skip cls='%ls' textLen=%d", id, cls, textLen);
        return;
    }
    auto ni = m_nodes.find(id);
    if (ni != m_nodes.end() && ni->second.kind != 6)
    {
        debugLogf(L"[bd-one] id=%d skip node-kind=%d", id, ni->second.kind);
        return;                     // owner-draw 控件节点，不是容器
    }
    // ① 宿主 DC 通道（GDI 路径唯一可见的一条）：按 id 覆盖同一格，移动只更新不累积
    m_containerBackdrops[id] = std::make_pair(CRect(x, y, x + w, y + h), apIt->second);
    debugLogf(L"[bd-one] id=%d stored rect=%d,%d,%d,%d bg='%ls' total=%d vis=%d parent=%d",
              id, x, y, w, h, apIt->second.backColor.c_str(),
              static_cast<int>(m_containerBackdrops.size()),
              ::IsWindowVisible(hw) ? 1 : 0,
              (::GetParent(hw) == m_hWnd) ? 1 : 0);
    ::InvalidateRect(m_hWnd, nullptr, FALSE);
    // ② 绘制节点通道（D2D 表面路径用）
    applyContainerBackdrop(id, x, y, w, h, apIt->second);
}

void PyMfcHost::syncContainerBackdrops()
{
        // 容器（div/面板）是空 STATIC，自己不会 WM_PAINT；它的 Background 只有在
        // 绘制节点管线里才看得见。外观推送早于控件创建，所以在布局收尾时统一补登记。
        // 非表面（纯 GDI）路径同样需要这份容器背板：本工程 flex_demo / cad2d 都走 GDI，
        // 只有 D2D 表面路径才用得上节点管线，所以这里不能按 m_newSurface 提前返回。
        // 容器（div）必须沉到 z 序底部：CreateWindow 默认把新窗口插到最顶，而容器是整页大窗口，
        // 留在最上面就会把所有叶子控件盖住 —— 表现为"整页只剩底色、控件全都不见"。
        // 按 id（≈ XML 文档顺序）逆序逐个沉底：父容器沉得最深，内层容器依次在其上，
        // 与 HTML 的"先父后子、子在父之上"绘制顺序一致。叶子控件不受影响，仍在容器之上。
        // 容器一律沉底：**不能**只沉"有背景色、登记了背板"的那批。
        // 无 Background 的容器（HTML 的透明 div、XML 里没写背景的 <page>）同样是一整页大小的
        // STATIC，留在最上面就把所有叶子盖住 —— 这就是"XML 面板缩放后整页空白"的真因。
        // 按 id（≈ XML 文档顺序）**正序**逐个沉底：先沉的是父容器（落得最深），后沉的子容器
        // 依次压在父容器之上，与 HTML 的"先父后子、子在父之上"绘制顺序一致。
        // （逆序会把父容器沉到子容器上面，父容器那层底色就糊住了卡片背景。）
        for (auto it = m_containerIds.begin(); it != m_containerIds.end(); ++it)
        {
            if (HWND h = ::GetDlgItem(m_hWnd, *it))
                ::SetWindowPos(h, HWND_BOTTOM, 0, 0, 0, 0,
                               SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        }
        debugLogf(L"[backdrop-sync] enter ap-count=%d surface=%d", static_cast<int>(m_ap.size()),
                  m_newSurface ? 1 : 0);
        for (const auto& kv : m_ap)
        {
            HWND h = ::GetDlgItem(m_hWnd, kv.first);
            if (h == nullptr)
            {
                continue;
            }
            wchar_t cls[64] = {};
            ::GetClassNameW(h, cls, 64);
            if (m_containerIds.count(kv.first) == 0)
            {
                continue;               // 不是 addPanel 建的容器（叶子控件自己有 CTLCOLOR 刷背景）
            }
            int x = 0, y = 0, w = 0, hh = 0;
            getControlRect(kv.first, x, y, w, hh);
            debugLogf(L"[backdrop-sync] id=%d rect=%d,%d,%d,%d bg='%ls'", kv.first,
                      x, y, w, hh, kv.second.backColor.c_str());
            syncOneContainerBackdrop(kv.first, x, y, w, hh);
        }
    }

bool PyMfcHost::setXmlProperty(const std::wstring& name, const std::wstring& prop, const std::wstring& value)
{
        if (!m_xml)
            return false;
        if (!m_xml->setProperty(name, prop, value))
            return false;
        const bool isModelProp = _wcsicmp(prop.c_str(), L"Items") == 0 ||
                                 _wcsicmp(prop.c_str(), L"items") == 0 ||
                                 _wcsicmp(prop.c_str(), L"model") == 0 ||
                                 _wcsicmp(prop.c_str(), L"Model") == 0;
        // ItemView 的行是"普通控件"：只改元素树上的 Model 不会长出新行，
        // 必须按新行数销毁旧行控件、重建行控件（与加载时同一条建控件通路）。
        // delegate 同理：Delegate="模板键" 一换，整行模板跟着换（QML 换 Component）。
        const bool isItemViewProp = isModelProp || _wcsicmp(prop.c_str(), L"delegate") == 0;
        if (isItemViewProp &&
            _wcsicmp(m_xml->kindOfElement(name).c_str(), L"itemview") == 0)
        {
            extern bool PyMfcRebuildItemView(PyMfcHost*, const std::wstring&);
            PyMfcRebuildItemView(this, name);
            return reflowXml();
        }
        // QML 语义：改 model / items 要立刻重建列表项（走与加载时同一条字符串表通路）
        if (isModelProp)
        {
            auto it = m_xmlIds.find(name);
            if (it != m_xmlIds.end())
            {
                clearStrings(it->second);
                std::wstring cur;
                auto flush = [&]() {
                    if (cur.empty())
                        return;
                    std::wstring item;
                    for (wchar_t ch : cur)
                        item.push_back(ch == L'\t' ? L' ' : ch);   // 单元格分隔在列表里显示成空格
                    addString(it->second, item);
                    cur.clear();
                };
                for (wchar_t ch : value)
                {
                    if (ch == L'|' || ch == L'\n' || ch == L'\r')
                        flush();
                    else
                        cur.push_back(ch);
                }
                flush();
            }
        }
        return reflowXml();
    }

void PyMfcHost::ensureOverlay(int id, int kind)
{
        if (findByIdDeep(m_hWnd, id) == nullptr)
            addSkinBox(id, kind, 0, 0, 10, 10);
        if (HWND h = findByIdDeep(m_hWnd, id))
            ::ShowWindow(h, SW_HIDE);
    }

void PyMfcHost::hideOverlay(int id)
{
        if (HWND h = findByIdDeep(m_hWnd, id))
            ::ShowWindow(h, SW_HIDE);
    }

bool PyMfcHost::isOverlayVisible(int id) const
{
        HWND h = findByIdDeep(m_hWnd, id);
        return h != nullptr && ::IsWindowVisible(h) != FALSE;
    }

void PyMfcHost::placeOverlay(int id, int x, int y, int w, int hgt)
{
        HWND h = findByIdDeep(m_hWnd, id);
        if (h == nullptr)
            return;
        RECT crc = {};
        ::GetClientRect(m_hWnd, &crc);
        if (x < 0) x = 0;
        if (y < 0) y = 0;
        if (x + w > crc.right) x = crc.right - w;
        if (y + hgt > crc.bottom) y = crc.bottom - hgt;
        if (x < 0) x = 0;
        if (y < 0) y = 0;
        ::SetWindowPos(h, HWND_TOP, x, y, w, hgt, SWP_SHOWWINDOW | SWP_NOACTIVATE);
        ::InvalidateRect(h, nullptr, TRUE);
    }

int PyMfcHost::levelOf(int id) const
{
        auto i = m_zLevel.find(id);
        return i != m_zLevel.end() ? i->second : 0;
    }

int PyMfcHost::tabIndexOf(int id) const
{
        auto i = m_tabIndex.find(id);
        return i != m_tabIndex.end() ? i->second : -1;
    }

void PyMfcHost::noteLevel(int id, int level)
{
        if (level == 0)
            m_zLevel.erase(id);
        else
            m_zLevel[id] = level;
        // 自绘节点不参与 HWND 排序（很多控件根本没有窗口），它的 z-index 必须落在节点上，
        // 否则 applyZOrder 白算一趟、屏幕上纹丝不动。层内次序变了就要重画一帧。
        auto it = m_nodes.find(id);
        if (it != m_nodes.end() && it->second.zLevel != level)
        {
            it->second.zLevel = level;
            it->second.dirty = true;
            if (m_hWnd != nullptr)
                ::InvalidateRect(m_hWnd, nullptr, FALSE);
        }
    }

void PyMfcHost::noteTabIndex(int id, int n)
{
        if (n < 0)
            m_tabIndex.erase(id);
        else
            m_tabIndex[id] = n;
    }

void PyMfcHost::raiseMany(const std::vector<int>& ids)
{
        int base = maxLevel();
        int n = 0;
        for (int id : ids) noteLevel(id, base + (++n));
        applyZOrder();
    }

void PyMfcHost::setLevel(int id, int level)
{
        noteLevel(id, level);
        applyZOrder();
    }

int PyMfcHost::getLevel(int id) const
{ return levelOf(id); }

int PyMfcHost::maxLevel() const
{
        int mx = 0;
        for (const auto& kv : m_zLevel)
            if (kv.second > mx) mx = kv.second;
        return mx;
    }

int PyMfcHost::minLevel() const
{
        int mn = 0;
        for (const auto& kv : m_zLevel)
            if (kv.second < mn) mn = kv.second;
        return mn;
    }

void PyMfcHost::bringToFront(int id)
{ setLevel(id, maxLevel() + 1); }

void PyMfcHost::bringToFrontMany(const std::vector<int>& ids)
{
    // 逐个 bringToFront：每次都要遍历子窗口重排 + RedrawWindow(全部子窗口)，
    // 10221 节点刷一次图这 5 次置顶要 120 ms；这里合并成一次重排
    int base = maxLevel();
    int n = 0;
    for (int id : ids) noteLevel(id, base + (++n));
    applyZOrder();
}

void PyMfcHost::sendToBack(int id)
{ setLevel(id, minLevel() - 1); }

void PyMfcHost::setTabIndex(int id, int n)
{
        noteTabIndex(id, n);
        applyTabProps();
    }

int PyMfcHost::getTabIndex(int id) const
{ return tabIndexOf(id); }

void PyMfcHost::noteCreated(int id)
{
        m_createSeq[id] = ++m_seqNext;
        if (HWND h = (m_hWnd != nullptr ? findByIdDeep(m_hWnd, id) : nullptr))
            ::SetPropW(h, L"PyMfcSeq",
                       reinterpret_cast<HANDLE>(static_cast<std::intptr_t>(m_seqNext)));
    }

void PyMfcHost::applyTabProps()
{
        for (HWND h = ::GetWindow(m_hWnd, GW_CHILD); h != nullptr;
             h = ::GetWindow(h, GW_HWNDNEXT))
        {
            auto ti = m_tabIndex.find(::GetDlgCtrlID(h));
            if (ti != m_tabIndex.end())
                ::SetPropW(h, L"PyMfcTabIndex",
                           reinterpret_cast<HANDLE>(static_cast<std::intptr_t>(ti->second)));
            else
                ::RemovePropW(h, L"PyMfcTabIndex");
        }
    }

void PyMfcHost::applyZOrder()
{
        struct ZItem
        {
            int level = 0;
            int seq = 0;
            HWND h = nullptr;
        };
        std::vector<ZItem> items;
        int seq = 0;
        for (HWND h = ::GetWindow(m_hWnd, GW_CHILD); h != nullptr;
             h = ::GetWindow(h, GW_HWNDNEXT))
        {
            ZItem z;
            z.level = levelOf(::GetDlgCtrlID(h));
            z.seq = ++seq;
            z.h = h;
            items.push_back(z);
        }
        const std::vector<HWND> before = [&]() {
            std::vector<HWND> v;
            v.reserve(items.size());
            for (const ZItem& z : items) v.push_back(z.h);
            return v;
        }();
        std::sort(items.begin(), items.end(), [](const ZItem& a, const ZItem& b) {
            if (a.level != b.level)
                return a.level < b.level;
            return a.seq < b.seq;
        });
        // 次序没变就别重排、更别整窗重画（重复 rebuild 时这里是纯浪费）。
        // 方向必须对齐：GetWindow(GW_CHILD) 给的是**顶→底**，而 items 排完是**底→顶**
        // （升序 level，最后 SetWindowPos 的那个在最上）。此前直接按下标比，等于把两串
        // 反着比 —— z-index 一旦把某块排到"和创建顺序相同的位置"，这里就判 same 提前返回，
        // 看起来像"z-index 写了不管用"。按逆序比才是在比同一个东西。
        bool same = (items.size() == before.size());
        for (size_t i = 0; same && i < items.size(); ++i)
            if (items[items.size() - 1 - i].h != before[i]) same = false;
        if (same)
            return;
        debugLogf(L"[zorder] reorder n=%d", static_cast<int>(items.size()));
        for (const ZItem& z : items)
            ::SetWindowPos(z.h, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        // 换了叠放次序必须整片重画：两块重叠时，谁从底下翻上来，
        // 原来压着它的那块留下的像素不会自己消失，看上去就像"层次没变"。
        ::RedrawWindow(m_hWnd, nullptr, nullptr, RDW_INVALIDATE | RDW_ALLCHILDREN);
    }

void PyMfcHost::applyLevelsAndTabs()
{
        applyTabProps();
        applyZOrder();
    }

void PyMfcHost::setLayerOpacity(int id, int alpha)
{
        if (alpha < 0) alpha = 0;
        if (alpha > 255) alpha = 255;
        if (alpha >= 255)
            m_layerAlpha.erase(id);
        else
            m_layerAlpha[id] = alpha;
        HWND h = findByIdDeep(m_hWnd, id);
        ::InvalidateRect(h != nullptr ? h : m_hWnd, nullptr, TRUE);
    }

int PyMfcHost::getLayerOpacity(int id) const
{
        auto it = m_layerAlpha.find(id);
        return it != m_layerAlpha.end() ? it->second : 255;
    }

void PyMfcHost::repairSiblingsOf(HWND moved, const RECT& oldRc, const RECT& newRc)
{
        RECT u = {
            oldRc.left < newRc.left ? oldRc.left : newRc.left,
            oldRc.top < newRc.top ? oldRc.top : newRc.top,
            oldRc.right > newRc.right ? oldRc.right : newRc.right,
            oldRc.bottom > newRc.bottom ? oldRc.bottom : newRc.bottom,
        };
        struct Ctx
        {
            HWND self;
            HWND moved;
            RECT r;
        } ctx = {m_hWnd, moved, u};
        ::EnumChildWindows(m_hWnd, [](HWND c, LPARAM lp) -> BOOL {
            Ctx* p = reinterpret_cast<Ctx*>(lp);
            if (c == p->moved || ::GetParent(c) != p->self)
                return TRUE;
            RECT cr = {};
            ::GetWindowRect(c, &cr);
            POINT a = {cr.left, cr.top};
            POINT b = {cr.right, cr.bottom};
            ::MapWindowPoints(HWND_DESKTOP, p->self, &a, 1);
            ::MapWindowPoints(HWND_DESKTOP, p->self, &b, 1);
            RECT rr = {a.x, a.y, b.x, b.y};
            RECT in = {};
            if (::IntersectRect(&in, &rr, &p->r))
                ::RedrawWindow(c, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
            return TRUE;
        }, reinterpret_cast<LPARAM>(&ctx));
    }

bool PyMfcHost::scrollOverflowContainer(int id, int dy)
{
        if (dy == 0 || m_scrollContainers.count(id) == 0)
            return false;
        HWND hw = findByIdDeep(m_hWnd, id);
        if (hw == nullptr)
            return false;
        RECT cr = {};
        ::GetClientRect(hw, &cr);
        const int viewH = cr.bottom - cr.top;
        if (viewH <= 0)
            return false;
        // 内容高 = 子窗口相对客户区的最大 bottom（已含当前偏移）
        int contentBottom = 0;
        {
            struct Ctx { HWND self; int maxBottom; const std::set<int>* fixed; } ctx = {hw, 0, &m_fixedChildren};
            ::EnumChildWindows(hw, [](HWND c, LPARAM lp) -> BOOL {
                Ctx* p = reinterpret_cast<Ctx*>(lp);
                if (::GetParent(c) != p->self)
                    return TRUE;
                if (p->fixed != nullptr && p->fixed->count(::GetDlgCtrlID(c)) != 0)
                    return TRUE;        // fixed 子项不算内容高
                RECT r = {};
                ::GetWindowRect(c, &r);
                POINT a = {r.left, r.top};
                POINT b = {r.right, r.bottom};
                ::MapWindowPoints(HWND_DESKTOP, p->self, &a, 1);
                ::MapWindowPoints(HWND_DESKTOP, p->self, &b, 1);
                if (b.y > p->maxBottom)
                    p->maxBottom = b.y;
                return TRUE;
            }, reinterpret_cast<LPARAM>(&ctx));
            contentBottom = ctx.maxBottom;
        }
        const int cur = m_scrollContainerY.count(id) ? m_scrollContainerY[id] : 0;
        // contentBottom 是"已平移后"的 bottom，真实内容高要加回当前偏移
        const int maxOff = (std::max)(0, contentBottom + cur - viewH);
        int want = cur + dy;
        if (want < 0)
            want = 0;
        if (want > maxOff)
            want = maxOff;
        if (want == cur)
            return false;
        const int delta = want - cur;
        m_scrollContainerY[id] = want;
        // 子窗口整体平移（相对坐标不变，只是相当于把内容往上/下推）
        {
            struct Ctx
            {
                HWND self;
                int dy;
                int offset;          // 滚动后的总偏移（sticky 用绝对值重算，不能用增量）
                int prevOffset;      // 滚动前的总偏移（sticky 基准惰性捕获用）
                const std::set<int>* fixed;
                std::map<int, std::pair<int, int>>* sticky;
            } ctx2 = {hw, -delta, want, cur, &m_fixedChildren, &m_stickyChildren};
            ::EnumChildWindows(hw, [](HWND c, LPARAM lp) -> BOOL {
                Ctx* p = reinterpret_cast<Ctx*>(lp);
                if (::GetParent(c) != p->self)
                    return TRUE;
                const int cid = ::GetDlgCtrlID(c);
                if (p->fixed != nullptr && p->fixed->count(cid) != 0)
                    return TRUE;        // CSS position:fixed —— 容器滚动时原地不动
                RECT r = {};
                ::GetWindowRect(c, &r);
                POINT a = {r.left, r.top};
                ::MapWindowPoints(HWND_DESKTOP, p->self, &a, 1);
                int ty = a.y + p->dy;
                // CSS position:sticky —— 按"基准位置 - 滚动量"算绝对位置，
                // 一旦越过吸顶线（top）就钉在 top 上，往回滚再跟着内容走。
                if (p->sticky != nullptr)
                {
                    const auto it = p->sticky->find(cid);
                    if (it != p->sticky->end())
                    {
                        int base = it->second.second;
                        if (base == INT_MIN)
                        {
                            // 首次滚动：此刻控件还在"上一偏移"的位置上，反推出未滚动的基准
                            base = a.y + p->prevOffset;
                            it->second.second = base;
                        }
                        ty = base - p->offset;
                        if (ty < it->second.first)
                            ty = it->second.first;
                        if (ty < 0)
                            ty = 0;
                    }
                }
                ::SetWindowPos(c, nullptr, a.x, ty, r.right - r.left, r.bottom - r.top,
                               SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSIZE);
                return TRUE;
            }, reinterpret_cast<LPARAM>(&ctx2));
        }
        ::InvalidateRect(hw, nullptr, TRUE);
        return true;
}

void PyMfcHost::registerStickyChild(int id, int topPx)
{
        if (id < 1000 || id > 65000)
            return;
        m_stickyChildren[id] = std::make_pair(topPx, INT_MIN);
}

bool PyMfcHost::scrollOverflowFrom(HWND hwnd, int notches)
{
        if (notches == 0)
            return false;
        // 从光标/子控件窗口往上找最近的"可滚容器"
        HWND w = hwnd;
        while (w != nullptr && w != m_hWnd)
        {
            const int id = ::GetDlgCtrlID(w);
            if (m_scrollContainers.count(id) != 0)
                return scrollOverflowContainer(id, -notches * 40);
            w = ::GetParent(w);
        }
        return false;
}

void PyMfcHost::setControlLevel(int id, int level)
{ setLevel(id, level); }

int PyMfcHost::getControlLevel(int id) const
{ return getLevel(id); }

void PyMfcHost::setControlTabIndex(int id, int index)
{ setTabIndex(id, index); }

int PyMfcHost::getControlTabIndex(int id) const
{ return getTabIndex(id); }

void PyMfcHost::setControlAppearance(int id, const xmlapp::XmlAppearance& ap)
{
        PyMfcPerfScope _perf(&pyMfcPerf().apMs, &pyMfcPerf().nAp);
        debugLogf(L"[ap] id=%d bg='%ls' fg='%ls' op=%d empty=%d ov='%ls' outl=%d ts='%ls'", id, ap.backColor.c_str(),
                  ap.foreColor.c_str(), ap.opacity, ap.empty() ? 1 : 0, ap.overflow.c_str(), ap.outlineWidth,
                  ap.textShadow.c_str());
        // 幂等短路：外观逐字段相同且主题代次没变 → 底色/字色/圆角/etc 全都没区别，
        // 不必删刷子重建、不必丢容器背板位图缓存、也不必整控件失效重绘。
        // 属性是逐条下发的（一个控件写 K 个外观属性 = K 次调用），没有这道短路就是
        // K 次刷子重建 + K 次全控件 InvalidateRect。
        {
            // 必须确认控件已存在：外观可能先于控件创建下发（那时 findByIdDeep 返回空，
            // 一次应用都没落地），此时不能短路，否则后续同一份外观永远刷不上。
            HWND hProbe = findByIdDeep(m_hWnd, id);
            auto prev = m_ap.find(id);
            if (hProbe != nullptr && prev != m_ap.end() && prev->second.sameAs(ap))
            {
                auto g = m_apGen.find(id);
                if (g != m_apGen.end() && g->second == m_themeGen)
                    return;
            }
        }
        // 外观是**分批**下发的（同一控件会来多次 setControlAppearance，每批只带一部分键）。
        // 直接覆盖会把前一批已经落地的声明抹掉（实测 text-shadow 就是这么丢的：后一批只带
        // Foreground，合并后整块阴影声明消失）。这里按"有值才覆盖"续存几个后加字段。
        xmlapp::XmlAppearance merged = ap;
        {
            auto prev = m_ap.find(id);
            if (prev != m_ap.end())
            {
                const xmlapp::XmlAppearance& p = prev->second;
                if (merged.textShadow.empty()) merged.textShadow = p.textShadow;
                if (merged.overflow.empty()) merged.overflow = p.overflow;
                if (merged.outlineWidth < 0) merged.outlineWidth = p.outlineWidth;
                if (merged.outlineColor.empty()) merged.outlineColor = p.outlineColor;
                if (merged.outlineStyle.empty()) merged.outlineStyle = p.outlineStyle;
                if (merged.outlineOffset == 0) merged.outlineOffset = p.outlineOffset;
                if (merged.pointerEvents.empty()) merged.pointerEvents = p.pointerEvents;
                if (merged.transitionMs < 0) merged.transitionMs = p.transitionMs;
                if (merged.transitionEase.empty()) merged.transitionEase = p.transitionEase;
            }
        }
        m_ap[id] = merged;
        m_apGen[id] = m_themeGen;
        // CSS transition：时长/缓动落到宿主表，hover 状态翻转时据此起颜色插值
        if (merged.transitionMs >= 0)
            m_cssTransitionMs[id] = merged.transitionMs;
        if (!merged.transitionEase.empty())
            m_cssTransitionEase[id] = merged.transitionEase;
        // CSS pointer-events 通过外观通道下发时也要落地（与 style 直读是同一条表）
        if (!merged.pointerEvents.empty())
            setControlPointerEvents(id, merged.pointerEvents);
        m_backdropSyncDirty = true;   // 外观变了，容器背板要在下一帧重新登记
        auto it = m_apBrush.find(id);
        if (it != m_apBrush.end())
        {
            if (it->second) ::DeleteObject(it->second);
            m_apBrush.erase(it);
        }
        releaseContainerPatternBrush(id);
        COLORREF bg = m_bg;
        if (!ap.backColor.empty())
        {
            COLORREF c = 0;
            if (xmlapp::ParseColor(ap.backColor, c))
                bg = xmlapp::Blend(c, m_bg, ap.opacity);
        }
        m_apBrush[id] = ::CreateSolidBrush(bg);
        COLORREF fg = contrastText(bg);
        if (!ap.foreColor.empty())
        {
            COLORREF c = 0;
            if (xmlapp::ParseColor(ap.foreColor, c))
                fg = c;
        }
        if (HWND h = findByIdDeep(m_hWnd, id))
        {
            // EDIT 的 client edge / 1px 边框由系统按浅色主题画，深色界面上就是一道白边；
            // 只有 XML 显式声明了背景色（说明这个输入框归主题外观管）才去掉系统边框。
            if (!ap.backColor.empty() && isTextInput(h))
                stripEditFrame(h);
            applyNativeThemeColors(h, bg, fg);
            ::InvalidateRect(h, nullptr, TRUE);

            // 容器（空 STATIC 面板，非 owner-draw 节点）：它自己不会 WM_PAINT，宿主也不做
            // owner-draw，所以 div/面板的 Background 必须由绘制节点管线画，否则永远看不到。
            // 判据：窗口类是 Static、无文本、且该 id 没有 kind≠6 的绘制节点。
            if (m_newSurface)
            {
                wchar_t cls[64] = {};
                ::GetClassNameW(h, cls, 64);
                auto ni = m_nodes.find(id);
                const bool ownDrawNode = ni != m_nodes.end() && ni->second.kind != 6;
                if (_wcsicmp(cls, L"Static") == 0 && ::GetWindowTextLengthW(h) == 0 && !ownDrawNode)
                {
                    RECT rc;
                    ::GetWindowRect(h, &rc);
                    ::MapWindowPoints(nullptr, m_hWnd, reinterpret_cast<POINT*>(&rc), 2);
                    applyContainerBackdrop(id, rc.left, rc.top, rc.right - rc.left,
                                           rc.bottom - rc.top, ap);
                }
            }
        }
    }

void PyMfcHost::setPageBackdrop(COLORREF c, bool valid)
{
        m_pageBg = c;
        m_hasPageBg = valid;
        if (m_hasPageBg)
        {
            m_bg = c;
            ++m_themeGen;   // 主题底色变了：外观幂等键随之失效，下一帧重刷一遍
            // 背景色变更后按 setTheme(103/111) 同一口径删旧刷重建，避免 CTLCOLOR
            // 回退刷子仍持有旧色 GDI 刷（先 DeleteObject 再建，不泄漏）。
            if (m_bgBrush) { ::DeleteObject(m_bgBrush); m_bgBrush = nullptr; }
            m_bgBrush = ::CreateSolidBrush(m_bg);
        }
        Invalidate(FALSE);
    }

void PyMfcHost::setBackgroundColor(COLORREF c)
{
    m_bg = c;
    ++m_themeGen;   // 同上：底色变，外观幂等键失效
    m_pageBg = c;
    m_hasPageBg = true;
    if (m_bgBrush) { ::DeleteObject(m_bgBrush); m_bgBrush = nullptr; }
    m_bgBrush = ::CreateSolidBrush(m_bg);
    Invalidate(FALSE);
}
