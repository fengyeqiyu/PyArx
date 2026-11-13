// PyMfcProcs.cpp —— 窗口过程：全控件自绘/皮肤绘制、配电画布鼠标状态机、可编辑表格 Grid。
// 独立编译单元：依赖 PyMfcHost.h（类内静态过程与宿主方法均已在头里）。

#include "PyMfcHost.h"
#include "XamlSurface.h"

// 窗口过程与离屏渲染等自由函数（skin / wheel / grid / D2D）  （从 PyMfcXaml.cpp 按类/职责切出，行 6263-8587）

IMPLEMENT_DYNAMIC(PyMfcHost, CWnd)

BEGIN_MESSAGE_MAP(PyMfcHost, CWnd)
    ON_WM_PAINT()
    ON_CONTROL_RANGE(BN_CLICKED, kFirstId, kLastId, &PyMfcHost::OnCommandRange)
    ON_CONTROL_RANGE(BN_DOUBLECLICKED, kFirstId, kLastId, &PyMfcHost::OnDblClickRange)
    ON_CONTROL_RANGE(EN_CHANGE, kFirstId, kLastId, &PyMfcHost::OnEditChange)
    ON_CONTROL_RANGE(EN_KILLFOCUS, kFirstId, kLastId, &PyMfcHost::OnEditBlur)
    ON_CONTROL_RANGE(LBN_SELCHANGE, kFirstId, kLastId, &PyMfcHost::OnListChange)
    ON_CONTROL_RANGE(LBN_DBLCLK, kFirstId, kLastId, &PyMfcHost::OnDblClickRange)
    // 下拉框：选中项变化 / 列表展开，各自一条（原先 CBN_SELCHANGE 完全没接，
    // 下拉框选了项也不会有任何回调）
    ON_CONTROL_RANGE(CBN_SELCHANGE, kFirstId, kLastId, &PyMfcHost::OnListChange)
    ON_CONTROL_RANGE(CBN_DROPDOWN, kFirstId, kLastId, &PyMfcHost::OnComboDropDown)
    ON_WM_HSCROLL()
    ON_WM_ERASEBKGND()
    ON_WM_SIZE()
    ON_WM_MOUSEMOVE()
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_WM_CTLCOLOR()
    ON_WM_DRAWITEM()
    ON_MESSAGE(kPyMfcSurfaceProbeMsg, &PyMfcHost::onSurfaceProbe)
END_MESSAGE_MAP()

void PyMfcHost::OnCommandRange(UINT id)
{
    auto it = m_click.find(id);
    if (it != m_click.end())
        invokePython(it->second);

    // 勾选类控件（复选 / 单选 / 三态 / ToggleButton）的 Click 同时是 CheckedChanged，
    // 与 XamlEvents 契约一致：状态变化必须能被事件看到
    if (HWND c = ::GetDlgItem(m_hWnd, id))
    {
        const LONG_PTR kind = ::GetWindowLongPtrW(c, GWL_STYLE) & BS_TYPEMASK;
        if (kind == BS_AUTOCHECKBOX || kind == BS_AUTORADIOBUTTON || kind == BS_AUTO3STATE ||
            kind == BS_3STATE || kind == BS_CHECKBOX || kind == BS_RADIOBUTTON)
        {
            auto itc = m_checked.find(id);
            if (itc != m_checked.end())
                invokePython(itc->second);
        }
    }

    // 回调里可能 SetFocus(CAD)；这里把焦点还给被点击的控件，
    // 让窗体可以继续接收键盘/下一次点击。
    if (HWND control = ::GetDlgItem(m_hWnd, id))
        ::SetFocus(control);
}

void PyMfcHost::OnDblClickRange(UINT id)
{
    auto it = m_dblclick.find(id);
    if (it != m_dblclick.end())
        invokePython(it->second);
}

void PyMfcHost::OnComboDropDown(UINT id)
{
    // 展开的这一刻把"框里那行字"钉住：之后列表内划过高亮只属于列表，
    // 框内文字等真正点选（CBN_SELENDOK / 关闭列表）才更新。
    if (HWND h = ::GetDlgItem(m_hWnd, static_cast<int>(id)))
    {
        const int cur = static_cast<int>(::SendMessageW(h, CB_GETCURSEL, 0, 0));
        std::wstring text;
        if (cur >= 0)
        {
            const int len = static_cast<int>(::SendMessageW(h, CB_GETLBTEXTLEN, cur, 0));
            if (len > 0)
            {
                std::wstring buf(static_cast<size_t>(len) + 1, L'\0');
                ::SendMessageW(h, CB_GETLBTEXT, cur, reinterpret_cast<LPARAM>(buf.data()));
                text = buf.substr(0, static_cast<size_t>(len));
            }
        }
        if (!text.empty())
            m_comboHold[static_cast<int>(id)] = text;
    }
    auto it = m_dropdown.find(id);
    if (it != m_dropdown.end())
        invokePython(it->second);
}

void PyMfcHost::OnHScroll(UINT /*nSBCode*/, UINT /*nPos*/, CScrollBar* pScrollBar)
{
    if (pScrollBar != nullptr)
    {
        const int id = pScrollBar->GetDlgCtrlID();
        auto it = m_value.find(id);
        if (it != m_value.end())
            invokePython(it->second);
    }
}

BOOL PyMfcHost::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
    NMHDR* pNMHDR = reinterpret_cast<NMHDR*>(lParam);
    if (pNMHDR != nullptr)
    {
        const int cid = ::GetDlgCtrlID(pNMHDR->hwndFrom);
        switch (pNMHDR->code)
        {
        case TCN_SELCHANGE:      // 选项卡切换
        case TVN_SELCHANGEDW:
        case LVN_ITEMCHANGED:
        {
            auto it = m_select.find((UINT)cid);
            if (it != m_select.end())
                invokePython(it->second);
            break;
        }
        case NM_DBLCLK:
        case LVN_ITEMACTIVATE:
        {
            auto it = m_dblclick.find((UINT)cid);
            if (it != m_dblclick.end())
                invokePython(it->second);
            break;
        }
        case LVN_COLUMNCLICK:    // 点列头排序（数据顺序由 Python 决定，这里只报列号）
        {
            NMLISTVIEW* nv = reinterpret_cast<NMLISTVIEW*>(lParam);
            fireGridAction((int)cid, "sort", -1, nv != nullptr ? nv->iSubItem : 0, L"");
            break;
        }
        case NM_CLICK:          // SysLink（超链接）：点它只发 NM_CLICK，没有 BN_CLICKED
        case NM_RETURN:
        {
            auto it = m_click.find((UINT)cid);
            if (it != m_click.end())
                invokePython(it->second);
            break;
        }
        default:
            break;
        }
    }
    // 派发完仍然交回默认处理：列表/树控件还要自己画、自己滚动
    return CWnd::OnNotify(wParam, lParam, pResult);
}

void PyMfcHost::OnEditChange(UINT id)
{
    auto it = m_change.find(id);
    if (it != m_change.end())
        invokePython(it->second);
}

void PyMfcHost::OnEditBlur(UINT id)
{
    fireBlur(static_cast<int>(id));
}

void PyMfcHost::OnListChange(UINT id)
{
    // 列表 / 下拉框的"选中项变了"。这里既要满足 onSelect（XamlEvents 的 SelectionChanged），
    // 也要满足 onChange —— 原先只派发 onSelect，用 onChange 绑下拉框的（例如布局方向切换）
    // 永远收不到回调，表现为"选了没反应"。
    auto it = m_select.find(id);
    if (it != m_select.end())
        invokePython(it->second);
    auto ic = m_change.find(id);
    if (ic != m_change.end())
        invokePython(ic->second);
}

BOOL PyMfcHost::OnEraseBkgnd(CDC* pDC)
{
    // 新表面路径：背景由交换链后台 buffer 的不透明 Clear 负责，GDI 擦除直接吃掉。
    if (m_newSurface)
        return TRUE;
    if (pDC == nullptr)
        return FALSE;
    CRect rc;
    GetClientRect(&rc);
    // 窗口底色必须跟随当前页声明的底色：控件是按 XAML 矩形独立建窗的，
    // 控件之间/控件圆角处会露出 1px 窗口底色，若底色还是深色主题色，
    // 白底页上每个控件看上去都像被描了一圈深色边框。
    pDC->FillSolidRect(rc, m_hasPageBg ? m_pageBg : m_bg);

    // 容器声明的底色（Canvas/Grid 的 Background）：按声明叠画在窗口底色之上
    HDC hdc = pDC->GetSafeHdc();
    for (const auto& bd : m_backdrops)
    {
        CRect clipped;
        clipped.IntersectRect(bd.first, rc);
        if (clipped.IsRectEmpty())
            continue;
        ::SaveDC(hdc);
        ::IntersectClipRect(hdc, clipped.left, clipped.top, clipped.right, clipped.bottom);
        xamlapp::DrawBackplate(hdc, bd.first, bd.second, m_bg, m_border);
        ::RestoreDC(hdc, -1);
    }
    // 父表面装饰矩形（白卡/状态栏底/小地图框/详情卡/分割条）：画在窗口底色之上、
    // 子 HWND 之下，z 序由这里统一保证。
    for (int sid : m_surfaceOrder)
    {
        auto sit = m_surfaces.find(sid);
        if (sit == m_surfaces.end() || !sit->second.visible)
            continue;
        CRect sclip;
        CRect src(sit->second.rc);              // RECT -> CRect，走 (const RECT&, const RECT&) 重载
        sclip.IntersectRect(src, rc);
        if (sclip.IsRectEmpty())
            continue;
        pDC->FillSolidRect(sclip, sit->second.fill);
    }
    return TRUE;
}

// ---------------------------------------------------------------------------
// 全控件自绘：控件在自己的 WM_PAINT 里画自己那块（Qt 也是各控件画自己），
// 这样既不用父窗口跨区域作画，也不会引出"父画→子失效"的重绘循环。
void PyMfcHost::drawSkinControl(XamlCanvas& cv, HWND child)
{
    // 控件自己的客户区坐标（不是父窗口坐标）
    RECT wr = {0, 0, 0, 0};
    ::GetClientRect(child, &wr);
    if (wr.right <= wr.left || wr.bottom <= wr.top)
        return;

    const int cid = ::GetDlgCtrlID(child);
    const xamlapp::XamlAppearance* ap = findAppearance(cid);
    const bool styled = ap != nullptr && !ap->empty();

    // 配色：声明了就用声明，没声明就跟主题走 —— 这一层是唯一配色来源
    COLORREF back = m_bg;
    COLORREF fore = m_text;
    COLORREF border = m_border;
    if (styled && !ap->backColor.empty())
    {
        COLORREF c = 0;
        if (xamlapp::ParseColor(ap->backColor, c))
            back = xamlapp::Blend(c, m_bg, ap->opacity);
    }
    if (styled && !ap->foreColor.empty())
    {
        COLORREF c = 0;
        if (xamlapp::ParseColor(ap->foreColor, c))
            fore = c;
    }
    if (styled && !ap->borderColor.empty())
    {
        COLORREF c = 0;
        if (xamlapp::ParseColor(ap->borderColor, c))
            border = c;
    }
    // ColorAnimation 的落点：动画期间按帧写进 m_ctrlBack，比 XAML 声明优先
    {
        auto it = m_ctrlBack.find(cid);
        if (it != m_ctrlBack.end())
            back = it->second;
    }

    pymfcskin::Palette pal;
    pal.back = back;
    pal.fore = fore;
    pal.border = border;
    pal.accent = m_dark ? RGB(0, 120, 212) : RGB(0, 102, 204);
    pal.muted = m_btn;
    pal.ptSize = (styled && ap->fontSize > 0) ? ap->fontSize : 9;
    if (styled && !ap->fontFamily.empty())
        pal.face = ap->fontFamily;

    wchar_t cls[64] = {};
    ::GetClassNameW(child, cls, 64);

    // PyMfcSkinBox：本仓库自己的"复合小控件"宿主窗口（Switch / SpinBox / RangeSlider /
    // BusyIndicator / PageIndicator）。它们没有系统类可用，窗口只是个画布，状态在宿主里。
    if (_wcsicmp(cls, L"PyMfcSkinBox") == 0)
    {
        const auto it = m_skinKinds.find(cid);
        const int kind = it != m_skinKinds.end() ? it->second : 0;
        const bool focus = ::GetFocus() == child;
        switch (kind)
        {
        case kSkinBoxSwitch:
        {
            const auto s = m_switchOn.find(cid);
            pymfcskin::drawSwitch(cv, wr, s != m_switchOn.end() && s->second, pal, focus);
            return;
        }
        case kSkinBoxSpin:
        {
            const auto v = m_spinVal.find(cid);
            pymfcskin::drawSpinBox(cv, wr, v != m_spinVal.end() ? v->second : 0, pal, focus);
            return;
        }
        case kSkinBoxRange:
        {
            const auto r = m_boxRange.find(cid);
            const int lo = r != m_boxRange.end() ? r->second.first : 0;
            const int hi = r != m_boxRange.end() ? r->second.second : 100;
            const auto l = m_rangeLo.find(cid);
            const auto h = m_rangeHi.find(cid);
            pymfcskin::drawRangeSlider(cv, wr, l != m_rangeLo.end() ? l->second : lo,
                                       h != m_rangeHi.end() ? h->second : hi, lo, hi, pal);
            return;
        }
        case kSkinBoxBusy:
        {
            const auto p = m_busyPhase.find(cid);
            pymfcskin::drawBusyRing(cv, wr, p != m_busyPhase.end() ? p->second : 0, pal);
            return;
        }
        case kSkinBoxPages:
        {
            const auto c = m_pageCount.find(cid);
            const auto cur = m_pageCur.find(cid);
            pymfcskin::drawPageDots(cv, wr, c != m_pageCount.end() ? c->second : 3,
                                    cur != m_pageCur.end() ? cur->second : 0, pal);
            return;
        }
        case kSkinBoxDial:
        {
            const auto v = m_dialVal.find(cid);
            const auto r = m_boxRange.find(cid);
            pymfcskin::drawDial(cv, wr, v != m_dialVal.end() ? v->second : 0,
                                r != m_boxRange.end() ? r->second.first : 0,
                                r != m_boxRange.end() ? r->second.second : 100, pal, focus);
            return;
        }
        case kSkinBoxTumbler:
        {
            const auto v = m_tumblerVal.find(cid);
            const auto c = m_tumblerCount.find(cid);
            pymfcskin::drawTumbler(cv, wr, v != m_tumblerVal.end() ? v->second : 0,
                                   c != m_tumblerCount.end() ? c->second : 10, pal, focus);
            return;
        }
        case kSkinBoxScrollBar:
        {
            const auto v = m_scrollVal.find(cid);
            const auto p = m_scrollPage.find(cid);
            const auto vert = m_scrollVert.find(cid);
            const auto r = m_boxRange.find(cid);
            pymfcskin::drawScrollBar(cv, wr, v != m_scrollVal.end() ? v->second : 0,
                                     r != m_boxRange.end() ? r->second.first : 0,
                                     r != m_boxRange.end() ? r->second.second : 100,
                                     p != m_scrollPage.end() ? p->second : 10,
                                     vert != m_scrollVert.end() && vert->second, pal, focus);
            return;
        }
        case kSkinBoxTable:
        {
            const auto cols = m_tblCols.find(cid);
            const auto rows = m_tblRows.find(cid);
            const auto sel = m_tblSel.find(cid);
            const auto sc = m_tblScroll.find(cid);
            const auto scol = m_tblSortCol.find(cid);
            const auto sa = m_tblSortAsc.find(cid);
            debugLogf(L"[table] cid=%d cols=%d rows=%d rc=%d,%d,%d,%d",
                      cid,
                      cols != m_tblCols.end() ? static_cast<int>(cols->second.size()) : -1,
                      rows != m_tblRows.end() ? static_cast<int>(rows->second.size()) : -1,
                      static_cast<int>(wr.left), static_cast<int>(wr.top),
                      static_cast<int>(wr.right), static_cast<int>(wr.bottom));
            const std::vector<std::wstring> emptyCols;
            const std::vector<std::vector<std::wstring>> emptyRows;
            pymfcskin::drawTable(cv, wr, cols != m_tblCols.end() ? cols->second : emptyCols,
                                 rows != m_tblRows.end() ? rows->second : emptyRows,
                                 sel != m_tblSel.end() ? sel->second : -1,
                                 sc != m_tblScroll.end() ? sc->second : 0,
                                 scol != m_tblSortCol.end() ? scol->second : -1,
                                 sa != m_tblSortAsc.end() && sa->second, pal, focus);
            return;
        }
        case kSkinBoxTree:
        {
            const auto t = m_treeText.find(cid);
            const auto lv = m_treeLevel.find(cid);
            const auto kid = m_treeKids.find(cid);
            const auto col = m_treeCollapsed.find(cid);
            const auto sel = m_treeSel.find(cid);
            const auto sc = m_treeScroll.find(cid);
            const std::vector<std::wstring> et;
            const std::vector<int> el;
            const std::vector<bool> eb;
            const std::vector<std::wstring>& texts = t != m_treeText.end() ? t->second : et;
            const std::vector<int>& levels = lv != m_treeLevel.end() ? lv->second : el;
            const std::vector<bool>& kids = kid != m_treeKids.end() ? kid->second : eb;
            const std::vector<bool>& coll = col != m_treeCollapsed.end() ? col->second : eb;
            // 折叠状态下把子树的文字/层级/箭头压成"可见序列"，绘制器不必懂树结构
            std::vector<std::wstring> vt;
            std::vector<int> vl;
            std::vector<bool> vk;
            std::vector<bool> vc;
            for (int i = 0; i < static_cast<int>(texts.size()); ++i)
            {
                if (isTreeRowHidden(cid, i))
                    continue;
                vt.push_back(texts[i]);
                vl.push_back(i < static_cast<int>(levels.size()) ? levels[i] : 0);
                vk.push_back(i < static_cast<int>(kids.size()) ? kids[i] : false);
                vc.push_back(i < static_cast<int>(coll.size()) ? coll[i] : false);
            }
            pymfcskin::drawTreeTable(cv, wr, vt, vl, vk, vc,
                                     sel != m_treeSel.end() ? sel->second : -1,
                                     sc != m_treeScroll.end() ? sc->second : 0, pal, focus);
            return;
        }
        case kSkinBoxToolBar:
        {
            const auto items = m_tbItems.find(cid);
            const auto sel = m_tbSel.find(cid);
            const auto hov = m_tbHover.find(cid);
            const std::vector<std::wstring> empty;
            pymfcskin::drawToolBar(cv, wr, items != m_tbItems.end() ? items->second : empty,
                                   sel != m_tbSel.end() ? sel->second : -1,
                                   hov != m_tbHover.end() ? hov->second : -1, pal, focus);
            return;
        }
        case kSkinBoxPager:
            pymfcskin::drawPager(cv, wr, viewIndex(cid), viewCount(cid), pal);
            return;
        case kSkinBoxFlick:
        {
            // 视口自己只铺底 + 描一条边；内容是真子窗，越界部分由父窗裁剪
            RECT rc = wr;
            cv.fillRect(rc, pal.back);
            cv.strokeRect(rc, pal.border, 1);
            return;
        }
        case kSkinBoxListView:
        {
            const auto tt = m_lvT.find(cid);
            const std::vector<std::wstring> empty;
            const std::vector<std::wstring>& allT = tt != m_lvT.end() ? tt->second : empty;
            const auto ss = m_lvS.find(cid);
            const auto vv = m_lvV.find(cid);
            const int rowH = lvRowH(cid);
            const int hgt = wr.bottom - wr.top;
            int sc = getListScroll(cid);
            const int maxSc = lvMaxScroll(cid);
            if (sc > maxSc) sc = maxSc;
            const int first = sc / rowH;
            const int over = m_lvOver.count(cid) ? m_lvOver[cid] : 0;
            const int dy = sc - first * rowH + over;
            const int cnt = hgt / rowH + 2;
            std::vector<std::wstring> vT, vS, vV;
            for (int i = 0; i < cnt; ++i)
            {
                const int idx = first + i;
                if (idx < 0 || idx >= static_cast<int>(allT.size()))
                    continue;
                vT.push_back(allT[idx]);
                vS.push_back((ss != m_lvS.end() && idx < static_cast<int>(ss->second.size())) ? ss->second[idx] : L"");
                vV.push_back((vv != m_lvV.end() && idx < static_cast<int>(vv->second.size())) ? vv->second[idx] : L"");
            }
            const auto sel = m_lvSel.find(cid);
            const auto hov = m_lvHover.find(cid);
            pymfcskin::drawListView(cv, wr, vT, vS, vV, first,
                                    sel != m_lvSel.end() ? sel->second : -1,
                                    hov != m_lvHover.end() ? hov->second : -1,
                                    dy, rowH, sc, maxSc, pal);
            return;
        }
        case kSkinBoxGridView:
        {
            const auto tt = m_lvT.find(cid);
            const std::vector<std::wstring> emptyG;
            const std::vector<std::wstring>& allG = tt != m_lvT.end() ? tt->second : emptyG;
            const auto ss = m_lvS.find(cid);
            int cols = 1, cw = 92, chh = 96;
            gvMetrics(cid, cols, cw, chh);
            const int rows = static_cast<int>((allG.size() + cols - 1) / cols);
            const int viewH = wr.bottom - wr.top;
            const int maxSc = (rows * chh - viewH) > 0 ? (rows * chh - viewH) : 0;
            int sc = getListScroll(cid);
            if (sc > maxSc) sc = maxSc;
            if (sc < 0) sc = 0;
            const int firstRow = sc / chh;
            const int dy = sc - firstRow * chh;
            const int visRows = viewH / chh + 2;
            const int firstCell = firstRow * cols;
            std::vector<std::wstring> vT, vS;
            for (int i = 0; i < visRows * cols; ++i)
            {
                const int idx = firstCell + i;
                if (idx < 0 || idx >= static_cast<int>(allG.size()))
                    continue;
                vT.push_back(allG[idx]);
                vS.push_back((ss != m_lvS.end() && idx < static_cast<int>(ss->second.size())) ? ss->second[idx] : L"");
            }
            const auto sel = m_lvSel.find(cid);
            const auto hov = m_lvHover.find(cid);
            pymfcskin::drawGridView(cv, wr, vT, vS, firstCell,
                                    sel != m_lvSel.end() ? sel->second : -1,
                                    hov != m_lvHover.end() ? hov->second : -1,
                                    dy, cw, chh, cols, sc, maxSc, pal);
            return;
        }
        case kSkinBoxPathView:
        {
            const auto tt = m_lvT.find(cid);
            const std::vector<std::wstring> emptyP;
            const std::vector<std::wstring>& allP = tt != m_lvT.end() ? tt->second : emptyP;
            const auto sel = m_lvSel.find(cid);
            pymfcskin::drawPathView(cv, wr, allP,
                                    sel != m_lvSel.end() ? sel->second : 0, pal);
            return;
        }
        case kSkinBoxQCanvas:
        {
            const auto cc = m_cvCmd.find(cid);
            pymfcskin::drawQCanvas(cv, wr, cc != m_cvCmd.end() ? cc->second : std::wstring(), pal);
            // 可编辑画布才画选中框：老用例没开编辑，画面与以前逐像素一致
            if (m_cvEdit.count(cid) != 0)
            {
                const auto ii = m_cvItems.find(cid);
                const auto ss = m_cvSel.find(cid);
                const std::vector<int> emptySel;
                const std::vector<int>& s = ss != m_cvSel.end() ? ss->second : emptySel;
                const auto bb = m_cvBand.find(cid);
                if (ii != m_cvItems.end())
                    pymfcskin::drawQcSelection(cv, ii->second, &s,
                                               bb != m_cvBand.end() ? &bb->second : nullptr);
            }
            return;
        }
        case kSkinBoxShadow:
        {
            const auto sb = m_shadowBox.find(cid);
            if (sb != m_shadowBox.end())
                pymfcskin::drawShadowBox(cv, wr, sb->second);
            return;
        }
        case kSkinBoxToolTip:
        {
            const auto tx = m_tipText.find(cid);
            pymfcskin::drawToolTip(cv, wr, tx != m_tipText.end() ? tx->second : L"", pal);
            return;
        }
        case kSkinBoxMenu:
        {
            const auto items = m_menuItems.find(cid);
            const auto hov = m_menuHover.find(cid);
            const auto sel = m_menuSel.find(cid);
            const std::vector<std::wstring> empty;
            pymfcskin::drawMenu(cv, wr, items != m_menuItems.end() ? items->second : empty,
                                hov != m_menuHover.end() ? hov->second : -1,
                                sel != m_menuSel.end() ? sel->second : -1, pal, focus);
            return;
        }
        case kSkinBoxMenuBar:
        {
            const auto items = m_mbItems.find(cid);
            const auto sel = m_mbSel.find(cid);
            const auto hov = m_mbHover.find(cid);
            const std::vector<std::wstring> empty;
            pymfcskin::drawMenuBar(cv, wr, items != m_mbItems.end() ? items->second : empty,
                                   sel != m_mbSel.end() ? sel->second : -1,
                                   hov != m_mbHover.end() ? hov->second : -1, pal);
            return;
        }
        case kSkinBoxPopup:
        {
            const auto ti = m_popTitle.find(cid);
            const auto ln = m_popLines.find(cid);
            const std::vector<std::wstring> empty;
            pymfcskin::drawPopup(cv, wr, ti != m_popTitle.end() ? ti->second : L"",
                                 ln != m_popLines.end() ? ln->second : empty, pal);
            return;
        }
        default:
            cv.fillRect(wr, pal.back);
            return;
        }
    }

    // ScrolledWindow：自绘滚动区域（滚动条也是自己画的，不再挂系统滚动条）
    if (m_scrollAreas.count(cid) != 0)
    {
        auto rng = m_scroll.find(cid);
        const int lo = rng != m_scroll.end() ? rng->second.first : 0;
        const int hi = rng != m_scroll.end() ? rng->second.second : 100;
        auto itPos = m_scrollPos.find(cid);
        const int pos = itPos != m_scrollPos.end() ? itPos->second : lo;
        pymfcskin::drawScrollArea(cv, wr, pos, lo, hi, pal, ::GetFocus() == child);
        return;
    }

    if (_wcsicmp(cls, L"msctls_progress32") == 0)
    {
        // PBM_GETRANGE 不是"取低/取高"两条消息：wParam=TRUE 表示取范围，
        // 低/高一起放进 lParam 给的 PBRANGE 里
        PBRANGE range = {};
        ::SendMessageW(child, PBM_GETRANGE, TRUE, reinterpret_cast<LPARAM>(&range));
        const int pos = static_cast<int>(::SendMessageW(child, PBM_GETPOS, 0, 0));
        debugLogf(L"[skin] id=%d cls=progress pos=%d low=%d high=%d rect=%ld,%ld,%ld,%ld",
                  cid, pos, range.iLow, range.iHigh, wr.left, wr.top, wr.right, wr.bottom);
        pymfcskin::drawProgressBar(cv, wr, pos, range.iLow, range.iHigh, pal);
        return;
    }

    if (_wcsicmp(cls, L"msctls_trackbar32") == 0)
    {
        const int pos = static_cast<int>(::SendMessageW(child, TBM_GETPOS, 0, 0));
        const int lo = static_cast<int>(::SendMessageW(child, TBM_GETRANGEMIN, 0, 0));
        const int hi = static_cast<int>(::SendMessageW(child, TBM_GETRANGEMAX, 0, 0));
        const LONG_PTR st = ::GetWindowLongPtrW(child, GWL_STYLE);
        const bool vertical = (st & TBS_VERT) != 0;
        pymfcskin::drawSlider(cv, wr, pos, lo, hi, vertical, pal);
        return;
    }

    if (_wcsicmp(cls, L"ListBox") == 0)
    {
        const int count = static_cast<int>(::SendMessageW(child, LB_GETCOUNT, 0, 0));
        const int top = static_cast<int>(::SendMessageW(child, LB_GETTOPINDEX, 0, 0));
        const int curSel = static_cast<int>(::SendMessageW(child, LB_GETCURSEL, 0, 0));
        const int rowH = 22;
        const int visible = ((wr.bottom - wr.top) - 2) / rowH + 1;

        std::vector<pymfcskin::Row> rows;
        for (int i = top; i < count && static_cast<int>(rows.size()) < visible; ++i)
        {
            const int len = static_cast<int>(::SendMessageW(child, LB_GETTEXTLEN, i, 0));
            std::wstring s;
            if (len > 0)
            {
                s.resize(static_cast<size_t>(len));
                std::wstring buf(static_cast<size_t>(len) + 1, L'\0');
                ::SendMessageW(child, LB_GETTEXT, i, reinterpret_cast<LPARAM>(buf.data()));
                s = buf.substr(0, static_cast<size_t>(len));
            }
            pymfcskin::Row r;
            r.text = s;
            r.selected = (i == curSel) ||
                         (::SendMessageW(child, LB_GETSEL, i, 0) > 0);
            rows.push_back(r);
        }
        pymfcskin::drawList(cv, wr, rows, pal, ::GetFocus() == child, rowH);
        return;
    }

    if (_wcsicmp(cls, L"ComboBox") == 0)
    {
        const int cid = ::GetDlgCtrlID(child);
        const LONG_PTR st = ::GetWindowLongPtrW(child, GWL_STYLE);
        std::wstring text;
        if (((st & 0x0003) == CBS_DROPDOWNLIST))
        {
            const int cur = static_cast<int>(::SendMessageW(child, CB_GETCURSEL, 0, 0));
            if (cur >= 0)
            {
                const int len = static_cast<int>(::SendMessageW(child, CB_GETLBTEXTLEN, cur, 0));
                if (len > 0)
                {
                    std::wstring buf(static_cast<size_t>(len) + 1, L'\0');
                    ::SendMessageW(child, CB_GETLBTEXT, cur, reinterpret_cast<LPARAM>(buf.data()));
                    text = buf.substr(0, static_cast<size_t>(len));
                }
            }
            // 列表展开着的时候，框里那行字用展开瞬间钉住的文本：鼠标划过哪一项，
            // 系统都会临时把 curSel 挪到那一项，但"确认"只能靠点选。
            COMBOBOXINFO cbi = {};
            cbi.cbSize = sizeof(cbi);
            const bool dropped = ::GetComboBoxInfo(child, &cbi) && cbi.hwndList != nullptr
                                 && ::IsWindowVisible(cbi.hwndList);
            if (dropped)
            {
                auto hit = m_comboHold.find(cid);
                if (hit == m_comboHold.end() || hit->second.empty())
                {
                    // 第一次看到"下拉已展开"，此刻 curSel 还是真值，就地钉住
                    m_comboHold[cid] = text;
                }
                else
                {
                    text = hit->second;
                }
            }
            else if (!m_comboHold.empty())
            {
                // 列表已经收起：这一轮起按真实 curSel 显示（点选生效 / Esc 取消都走这里）
                m_comboHold.erase(cid);
            }
        }
        else
        {
            wchar_t buf[512] = {};
            ::SendMessageW(child, WM_GETTEXT, 512, reinterpret_cast<LPARAM>(buf));
            text = buf;
        }
        pymfcskin::drawCombo(cv, wr, text, pal);
        return;
    }

    // ---------------- 第二批：页签 / 树 / 表格 / 日期框 ----------------
    if (_wcsicmp(cls, L"SysTabControl32") == 0)
    {
        std::vector<std::wstring> titles;
        const int n = static_cast<int>(::SendMessageW(child, TCM_GETITEMCOUNT, 0, 0));
        for (int i = 0; i < n; ++i)
        {
            wchar_t buf[128] = {};
            TCITEMW it = {};
            it.mask = TCIF_TEXT;
            it.pszText = buf;
            it.cchTextMax = 128;
            if (::SendMessageW(child, TCM_GETITEMW, i, reinterpret_cast<LPARAM>(&it)))
                titles.push_back(buf);
        }
        const int curSel = static_cast<int>(::SendMessageW(child, TCM_GETCURSEL, 0, 0));
        // 页签真实高度：第一个页签的矩形高（不同字体/DPI 下不一样）
        int tabH = 28;
        RECT ir = {};
        if (n > 0 && ::SendMessageW(child, TCM_GETITEMRECT, 0, reinterpret_cast<LPARAM>(&ir)))
            tabH = ir.bottom - ir.top;
        pymfcskin::drawTab(cv, wr, titles, curSel, pal, tabH);
        return;
    }

    if (_wcsicmp(cls, L"SysTreeView32") == 0)
    {
        std::vector<xamlctl::TreeRow> rows;
        struct Item { HTREEITEM h; int level; };
        std::vector<Item> stack;

        HTREEITEM root = reinterpret_cast<HTREEITEM>(
            ::SendMessageW(child, TVM_GETNEXTITEM, TVGN_ROOT, 0));
        if (root != nullptr)
            stack.push_back(Item{ root, 0 });

        // 栈式遍历：只走"已展开"的分支，把可见行拍平成列表交给共享绘制器
        while (!stack.empty() && rows.size() < 300)
        {
            const Item cur = stack.back();
            stack.pop_back();

            wchar_t buf[256] = {};
            TVITEMW it = {};
            it.mask = TVIF_TEXT | TVIF_STATE | TVIF_CHILDREN;
            it.hItem = cur.h;
            it.pszText = buf;
            it.cchTextMax = 256;
            it.stateMask = TVIS_EXPANDED | TVIS_SELECTED;
            ::SendMessageW(child, TVM_GETITEMW, 0, reinterpret_cast<LPARAM>(&it));

            xamlctl::TreeRow r;
            r.text = buf;
            r.level = cur.level;
            r.hasChildren = it.cChildren > 0;
            r.expanded = (it.state & TVIS_EXPANDED) != 0;
            r.selected = (it.state & TVIS_SELECTED) != 0;
            rows.push_back(r);

            HTREEITEM next = reinterpret_cast<HTREEITEM>(
                ::SendMessageW(child, TVM_GETNEXTITEM, TVGN_NEXT, reinterpret_cast<LPARAM>(cur.h)));
            if (next != nullptr)
                stack.push_back(Item{ next, cur.level });
            if (r.expanded && cur.level < 12)
            {
                HTREEITEM kid = reinterpret_cast<HTREEITEM>(
                    ::SendMessageW(child, TVM_GETNEXTITEM, TVGN_CHILD, reinterpret_cast<LPARAM>(cur.h)));
                if (kid != nullptr)
                    stack.push_back(Item{ kid, cur.level + 1 });
            }
        }
        pymfcskin::drawTree(cv, wr, rows, pal.back, pal.fore, pal.accent, 0, pal.ptSize, pal.face);
        return;
    }

    if (_wcsicmp(cls, L"SysListView32") == 0)
    {
        // 表头是独立子窗口（SysHeader32），它自己画自己的，这里只画表头以下的正文
        int headerH = 0;
        HWND hdr = reinterpret_cast<HWND>(::SendMessageW(child, LVM_GETHEADER, 0, 0));
        if (hdr != nullptr && ::IsWindowVisible(hdr))
        {
            RECT hr = {};
            ::GetWindowRect(hdr, &hr);
            ::MapWindowPoints(HWND_DESKTOP, child, reinterpret_cast<LPPOINT>(&hr), 2);
            if (hr.bottom > 0 && hr.bottom < wr.bottom)
                headerH = hr.bottom;
        }

        std::vector<std::wstring> cols;
        std::vector<int> widths;
        for (int i = 0; i < 16; ++i)
        {
            wchar_t cbuf[128] = {};
            LVCOLUMNW col = {};
            col.mask = LVCF_TEXT | LVCF_WIDTH;
            col.pszText = cbuf;
            col.cchTextMax = 128;
            if (!::SendMessageW(child, LVM_GETCOLUMNW, i, reinterpret_cast<LPARAM>(&col)))
                break;
            cols.push_back(cbuf);
            widths.push_back(col.cx);
        }

        const int count = static_cast<int>(::SendMessageW(child, LVM_GETITEMCOUNT, 0, 0));
        const int rowH = 22;
        const int top = static_cast<int>(::SendMessageW(child, LVM_GETTOPINDEX, 0, 0));
        const int bodyH = (wr.bottom - wr.top) - headerH;
        const int visible = bodyH / rowH + 1;

        std::vector<pymfcskin::GridRow> grows;
        for (int i = top; i < count && static_cast<int>(grows.size()) < visible; ++i)
        {
            const LRESULT st = ::SendMessageW(child, LVM_GETITEMSTATE, i, LVIS_SELECTED);
            pymfcskin::GridRow r;
            r.selected = (st & LVIS_SELECTED) != 0;
            for (size_t j = 0; j < cols.size(); ++j)
            {
                wchar_t buf[256] = {};
                LVITEMW it = {};
                it.iSubItem = static_cast<int>(j);
                it.pszText = buf;
                it.cchTextMax = 256;
                ::SendMessageW(child, LVM_GETITEMTEXTW, i, reinterpret_cast<LPARAM>(&it));
                r.cells.push_back(buf);
            }
            grows.push_back(r);
        }

        RECT body = { wr.left, wr.top + headerH, wr.right, wr.bottom };
        pymfcskin::drawGrid(cv, body, cols, widths, grows, pal, false, 0, rowH);
        return;
    }

    if (_wcsicmp(cls, L"SysHeader32") == 0)
    {
        std::vector<std::wstring> titles;
        std::vector<int> widths;
        const int n = static_cast<int>(::SendMessageW(child, HDM_GETITEMCOUNT, 0, 0));
        for (int i = 0; i < n && i < 16; ++i)
        {
            wchar_t buf[128] = {};
            HDITEMW it = {};
            it.mask = HDI_TEXT | HDI_WIDTH;
            it.pszText = buf;
            it.cchTextMax = 128;
            ::SendMessageW(child, HDM_GETITEMW, i, reinterpret_cast<LPARAM>(&it));
            titles.push_back(buf);
            widths.push_back(it.cxy);
        }
        pymfcskin::drawHeaderRow(cv, wr, titles, widths, pal);
        return;
    }

    if (_wcsicmp(cls, L"SysDateTimePick32") == 0)
    {
        wchar_t buf[256] = {};
        ::SendMessageW(child, WM_GETTEXT, 256, reinterpret_cast<LPARAM>(buf));
        pymfcskin::drawDatePicker(cv, wr, buf, pal);
        return;
    }

    if (_wcsicmp(cls, L"SysMonthCal32") == 0)
    {
        // 当前显示哪个月：MCM_GETMONTHRANGE 给"可见范围"的首尾日期，取其月份
        SYSTEMTIME range[2] = {};
        int year = 0, month = 0;
        if (::SendMessageW(child, MCM_GETMONTHRANGE, GMR_VISIBLE,
                           reinterpret_cast<LPARAM>(range)) == 2 &&
            range[0].wYear != 0)
        {
            SYSTEMTIME mid = range[0];
            mid.wDay = static_cast<WORD>(mid.wDay + 4);  // 首个可见日往后推几天，落到"主月份"
            FILETIME ft = {};
            SYSTEMTIME norm = {};
            if (::SystemTimeToFileTime(&mid, &ft) && ::FileTimeToSystemTime(&ft, &norm))
            {
                year = norm.wYear;
                month = norm.wMonth;
            }
        }
        if (year == 0)
        {
            SYSTEMTIME now = {};
            ::GetLocalTime(&now);
            year = now.wYear;
            month = now.wMonth;
        }

        SYSTEMTIME today = {};
        ::GetLocalTime(&today);
        const int todayDay = (today.wYear == year && today.wMonth == month)
                                 ? static_cast<int>(today.wDay)
                                 : 0;

        SYSTEMTIME sel = {};
        int selDay = 0;
        if (::SendMessageW(child, MCM_GETCURSEL, 0, reinterpret_cast<LPARAM>(&sel)) != GDT_NONE &&
            sel.wYear == year && sel.wMonth == month)
            selDay = static_cast<int>(sel.wDay);

        pymfcskin::drawCalendar(cv, wr, year, month, todayDay, selDay, pal,
                                ::GetFocus() == child);
        return;
    }

    if (_wcsicmp(cls, L"SysLink") == 0)
    {
        std::wstring text;
        auto it = m_links.find(cid);
        if (it != m_links.end())
            text = it->second;
        if (text.empty())
        {
            // 窗口文本是 <a href="...">显示文字</a>，只取中间的显示文字
            wchar_t buf[512] = {};
            ::SendMessageW(child, WM_GETTEXT, 512, reinterpret_cast<LPARAM>(buf));
            text = buf;
            const size_t close = text.find(L'>');
            const size_t open = text.rfind(L'<');
            if (close != std::wstring::npos && open != std::wstring::npos && open > close)
                text = text.substr(close + 1, open - close - 1);
        }
        cv.fillRect(wr, pal.back);
        RECT tr = { wr.left + 2, wr.top, wr.right - 2, wr.bottom };
        cv.text(tr, text, pal.accent,
                DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS, pal.ptSize, pal.face);
        const int uy = (wr.top + wr.bottom) / 2 + 8;
        if (uy < wr.bottom)
            cv.fillRect(RECT{ wr.left + 2, uy, wr.right - 2, uy + 1 }, pal.accent);
        return;
    }

    // Edit / RichText：文字、选区、光标全部自己画。
    // 布局不自己算 —— 分行和字符位置一律问系统（EM_LINEINDEX / EM_POSFROMCHAR），
    // 用控件自己的字体量宽度（WM_GETFONT + GetTextExtentPoint32），
    // 这样自绘出来的字和系统光标、输入法候选窗、鼠标点选位置才能对得上。
    if (_wcsicmp(cls, L"Edit") == 0 || _wcsicmp(cls, L"RICHEDIT50W") == 0)
    {
        const int len = static_cast<int>(::GetWindowTextLengthW(child));
        std::wstring text(static_cast<size_t>(len) + 1, L'\0');
        if (len > 0)
            ::GetWindowTextW(child, &text[0], len + 1);
        text.resize(static_cast<size_t>(len > 0 ? len : 0));

        const LONG_PTR st = ::GetWindowLongPtrW(child, GWL_STYLE);
        const bool multiline = (st & ES_MULTILINE) != 0;
        const bool password = (st & ES_PASSWORD) != 0;
        const bool readonly = (st & ES_READONLY) != 0;
        UINT align = DT_LEFT;
        if ((st & ES_CENTER) != 0)
            align = DT_CENTER;
        else if ((st & ES_RIGHT) != 0)
            align = DT_RIGHT;

        // 密码框不显示明文：按实际长度铺圆点，宽度仍用圆点量
        std::wstring shown = text;
        if (password)
            shown.assign(text.size(), L'\u25CF');

        DWORD selStart = 0;
        DWORD selEnd = 0;
        ::SendMessageW(child, EM_GETSEL, reinterpret_cast<WPARAM>(&selStart),
                       reinterpret_cast<LPARAM>(&selEnd));
        if (selEnd < selStart)
            selEnd = selStart;
        const bool hasFocus = ::GetFocus() == child;

        // 占位提示（EM_SETCUEBANNER 只是让控件自己画，绘制被接管后就没人画了）。
        // 与系统同一条规则：空文本且没有焦点时显示，用的是"次要文字色"。
        wchar_t cue[512] = {};
        ::SendMessageW(child, EM_GETCUEBANNER, reinterpret_cast<WPARAM>(cue), 512);
        const bool showCue = text.empty() && !hasFocus && cue[0] != L'\0';
        if (showCue)
        {
            RECT cr;
            ::GetClientRect(child, &cr);
            pymfcskin::EditLine ln;
            ln.x = 2;
            ln.y = 2 + (cr.bottom - cr.top < 24 ? 2 : 4);
            ln.text = cue;
            std::vector<pymfcskin::EditLine> cueLines;
            cueLines.push_back(ln);
            pymfcskin::Palette cuePal = pal;
            cuePal.fore = pal.muted;
            debugLogf(L"[edit] id=%d cls=%ls cue=%ls", cid, cls, cue);
            pymfcskin::drawEdit(cv, wr, cueLines, cuePal, hasFocus, true, 18, align, false);
            return;
        }

        HDC hdc = ::GetDC(child);
        HFONT font = reinterpret_cast<HFONT>(::SendMessageW(child, WM_GETFONT, 0, 0));
        HGDIOBJ oldFont = font != nullptr ? ::SelectObject(hdc, font) : nullptr;
        TEXTMETRICW tm = {};
        ::GetTextMetricsW(hdc, &tm);
        int lineH = static_cast<int>(tm.tmHeight) + 2;
        if (lineH < 12)
            lineH = 18;

        // 前缀宽度（字符下标 -> 像素）：每行只量选区起止和光标三处
        auto widthOf = [hdc, &shown](int from, int to) -> int {
            if (to <= from)
                return 0;
            const std::wstring prefix = shown.substr(static_cast<size_t>(from),
                                                     static_cast<size_t>(to - from));
            SIZE sz = {};
            ::GetTextExtentPoint32W(hdc, prefix.c_str(), static_cast<int>(prefix.size()), &sz);
            return static_cast<int>(sz.cx);
        };

        // 分行：
        //  * Edit 走系统的显示行（EM_LINEINDEX 把自动换行也算一行，拿到的就是屏幕上那一行）；
        //  * RichEdit 不支持 EM_LINEINDEX（返回 -1，只剩第一段），按段落分隔符自己切，
        //    再用 EM_GETFIRSTVISIBLELINE 把滚动过的段落让出去。
        std::vector<int> segStart;
        std::vector<int> segEnd;
        const bool rich = (_wcsicmp(cls, L"RICHEDIT50W") == 0);
        if (multiline && !rich)
        {
            const int lc = static_cast<int>(::SendMessageW(child, EM_GETLINECOUNT, 0, 0));
            for (int i = 0; i < lc && i < 400; ++i)
            {
                const int s = static_cast<int>(::SendMessageW(child, EM_LINEINDEX, i, 0));
                if (s < 0)
                    break;
                int e = static_cast<int>(::SendMessageW(child, EM_LINEINDEX, i + 1, 0));
                if (e < 0 || e > static_cast<int>(text.size()))
                    e = static_cast<int>(text.size());
                while (e > s && (text[static_cast<size_t>(e - 1)] == L'\r' ||
                                 text[static_cast<size_t>(e - 1)] == L'\n'))
                    --e;
                segStart.push_back(s);
                segEnd.push_back(e);
            }
        }
        else if (multiline && rich)
        {
            int s = 0;
            for (int i = 0; i <= static_cast<int>(text.size()); ++i)
            {
                const bool end = (i == static_cast<int>(text.size()));
                if (!end && text[static_cast<size_t>(i)] != L'\r')
                    continue;
                segStart.push_back(s);
                segEnd.push_back(i);
                s = i + 1;
                if (s < static_cast<int>(text.size()) && text[static_cast<size_t>(i)] == L'\r' &&
                    text[static_cast<size_t>(s)] == L'\n')
                    ++s;
                i = s - 1;
            }
        }
        else
        {
            segStart.push_back(0);
            segEnd.push_back(static_cast<int>(text.size()));
        }

        const int firstVisible = static_cast<int>(::SendMessageW(child, EM_GETFIRSTVISIBLELINE, 0, 0));
        const bool caretOn = caretBlinkOn(child);

        std::vector<pymfcskin::EditLine> lines;
        for (size_t i = 0; i < segStart.size(); ++i)
        {
            const int s = segStart[i];
            const int e = segEnd[i];

            // 行首坐标由系统给：横向滚动时 x 是负值，纵向滚动时 y 已经排好
            POINT pt = { -100000, -100000 };
            const LRESULT pos = ::SendMessageW(child, EM_POSFROMCHAR,
                                              reinterpret_cast<WPARAM>(&pt), static_cast<LPARAM>(s));
            pymfcskin::EditLine ln;
            if (!rich && pos != -1 && pt.x > -90000)
            {
                ln.x = static_cast<int>(pt.x);
                ln.y = static_cast<int>(pt.y);
            }
            else
            {
                // RichEdit 的字符坐标语义不同，按段落序号 + 首可见行自己排
                ln.x = 2;
                ln.y = wr.top + 2 + (static_cast<int>(i) - firstVisible) * lineH;
            }

            if (password)
                ln.text = shown.substr(static_cast<size_t>(s),
                                       static_cast<size_t>(e - s));
            else
                ln.text = shown.substr(static_cast<size_t>(s), static_cast<size_t>(e - s));

            // 选区：全文本下标 -> 行内像素（只在选中部分量宽度）
            const int from = std::max<int>(static_cast<int>(selStart), s);
            const int to = std::min<int>(static_cast<int>(selEnd), e);
            if (to > from)
            {
                ln.selFromPx = widthOf(s, from);
                ln.selToPx = widthOf(s, to);
            }

            // 光标：只有折叠选区（起止相同）才画竖线
            if (selStart == selEnd && static_cast<int>(selStart) >= s &&
                static_cast<int>(selStart) <= e)
                ln.caretPx = widthOf(s, static_cast<int>(selStart));

            lines.push_back(ln);
        }

        if (font != nullptr)
            ::SelectObject(hdc, oldFont);
        ::ReleaseDC(child, hdc);

        debugLogf(L"[edit] id=%d cls=%ls len=%d lines=%d sel=%u..%u lineH=%d focus=%d font=%p tm=%d first=%d",
                  cid, cls, len, static_cast<int>(lines.size()),
                  static_cast<unsigned>(selStart), static_cast<unsigned>(selEnd), lineH,
                  hasFocus ? 1 : 0, static_cast<void*>(font), static_cast<int>(tm.tmHeight), firstVisible);
        pymfcskin::drawEdit(cv, wr, lines, pal, hasFocus, readonly, lineH, align, caretOn);
        return;
    }
}

void PyMfcHost::OnPaint()
{
    CPaintDC dc(this);
    CRect rc;
    GetClientRect(&rc);
    if (rc.IsRectEmpty())
        return;

    // 新表面路径：BeginFrame 不透明清屏 → 留空给 S2 绘制节点 → EndFrame Present。
    // S1 阶段表面只承担宿主客户区底衬；原生子 HWND 仍按各自 WM_PAINT 覆盖在上层。
    if (m_surface)
    {
        const COLORREF bg = m_hasPageBg ? m_pageBg : m_bg;
        const float r = GetRValue(bg) / 255.0f;
        const float g = GetGValue(bg) / 255.0f;
        const float b = GetBValue(bg) / 255.0f;
        LARGE_INTEGER q0 = {}, q1 = {}, qf = {};
        ::QueryPerformanceFrequency(&qf);
        ::QueryPerformanceCounter(&q0);
        if (m_surface->BeginFrame(r, g, b, 1.0f))
        {
            // S2-A：表面帧内按 z 序画绘制节点（白板 / 阴影底板 / 分割线 / 装饰矩形）。
            D2DCtxCanvas cv;
            void* rt = m_surface->GetRenderTarget();
            if (rt != nullptr &&
                cv.attach(reinterpret_cast<ID2D1RenderTarget*>(rt), rc.Width(), rc.Height()))
            {
                cv.begin(rc.Width(), rc.Height());
                drawPaintNodes(cv);
                cv.end();
            }
            m_surface->EndFrame(1);
            if (m_surface->IsDeviceLost())
                m_surface->Recreate();
        }
        ::QueryPerformanceCounter(&q1);
        if (paintTimingEnabled())
            paintLogf(L"[surface] frame begin+end %.2f ms lost=%d",
                      (double)(q1.QuadPart - q0.QuadPart) * 1000.0 / (double)qf.QuadPart,
                      m_surface->IsDeviceLost() ? 1 : 0);
        return;
    }

    // 窗口底 + 容器声明的底色（与 OnEraseBkgnd 同一套规则）
    HDC hdc = dc.GetSafeHdc();

    dc.FillSolidRect(rc, m_hasPageBg ? m_pageBg : m_bg);
    for (const auto& bd : m_backdrops)
    {
        CRect clipped;
        clipped.IntersectRect(bd.first, rc);
        if (clipped.IsRectEmpty())
            continue;
        ::SaveDC(hdc);
        ::IntersectClipRect(hdc, clipped.left, clipped.top, clipped.right, clipped.bottom);
        xamlapp::DrawBackplate(hdc, bd.first, bd.second, m_bg, m_border);
        ::RestoreDC(hdc, -1);
    }
}

void PyMfcHost::OnSize(UINT nType, int cx, int cy)
{
    CWnd::OnSize(nType, cx, cy);
    // 新骨架：真实客户区到位 / Resize 后补一次 fit；=0 保持旧 OnSize 行为。
    if (m_newSurface)
    {
        for (auto& kv : m_planCv)
        {
            if (kv.second.mode != 0)
                fitPlanCanvas(kv.first);
        }
    }
    // 新表面路径：尺寸变化走 ResizeBuffers（释放 target → ResizeBuffers → 重建后台 target）。
    if (m_surface)
        m_surface->Resize(cx, cy);
}

LRESULT PyMfcHost::onSurfaceProbe(WPARAM wParam, LPARAM /*lParam*/)
{
    if (m_surface == nullptr)
        return 0;
    switch (wParam)
    {
    case 1:  // 模拟设备丢失：释放后端资源，下一帧 BeginFrame 全量重建
        m_surface->SimulateDeviceLost();
        return 1;
    case 2:  // 强制立即重建
        return m_surface->Recreate() ? 1 : 0;
    case 3:  // 取诊断串（写成员 + 调试日志）
    {
        wchar_t diag[512] = {};
        m_surface->GetDiagnostics(diag, _countof(diag));
        m_surfaceDiag = diag;
        debugLogf(L"[surface] %ls", diag);
        return 1;
    }
    default:
        return 0;
    }
}

// 自绘控件在自己窗口里画：绑定到它自己的 HDC，画完即结束
void PyMfcHost::paintSkinControl(HDC hdc, HWND child)
{
    RECT cr = {0, 0, 0, 0};
    ::GetClientRect(child, &cr);
    if (cr.right <= 0 || cr.bottom <= 0)
        return;

    // BeginPaint 交来的 HDC 带着"本次更新区"裁剪：整块底色画不全，
    // 边上会留下系统上一次画的像素（滚动条、旧边框一类）。画自己时整块重画。
    ::SelectClipRgn(hdc, nullptr);

    // 画布（含 D2D 渲染目标与笔刷/文字格式缓存）按"控件 id"复用。
    // 原来全进程只有一个，十几尺寸不同的控件轮流用：每次尺寸一变就 releaseTarget()，
    // 把目标 + 所有笔刷 + 文字格式重建一遍，一块纯色矩形也要 16 ms 就是这么来的。
    static std::map<int, D2DDcCanvas*> s_ctlCanvases;
    D2DDcCanvas*& canvasPtr = s_ctlCanvases[::GetDlgCtrlID(child)];
    if (canvasPtr == nullptr) canvasPtr = new D2DDcCanvas();

    // 双缓冲：直接往窗口 DC 上画时，一次消息里多次 Invalidate 会看到中间态（闪一下），
    // 页签切换尤其明显。先画到一张 32bpp DIB，画完整块 BitBlt 上去。
    static HDC s_memDc = nullptr;
    static HBITMAP s_memBmp = nullptr;
    static void* s_memBits = nullptr;
    static int s_memW = 0, s_memH = 0;

    // 阴影底板是纯静态装饰：首轮 D2D 把 12 层圆角阴影 + 白卡 + 边框合成到一张
    // 32bpp DIB 缓存，之后每次 WM_PAINT 只 BitBlt，不再走 D2D 重画阴影。
    struct ShadowCache { HDC dc = nullptr; HBITMAP bmp = nullptr; int w = 0, h = 0; };
    static std::map<int, ShadowCache> s_shadowCache;
    const int skinId = ::GetDlgCtrlID(child);
    auto kIt = m_skinKinds.find(skinId);
    int skinKind = kIt != m_skinKinds.end() ? kIt->second : 0;
    if (skinKind == 0)
    {
        const INT_PTR pk = reinterpret_cast<INT_PTR>(::GetPropW(child, kSkinKindProp));
        skinKind = static_cast<int>(pk);
    }

    if (skinKind == kSkinBoxShadow)
    {
        auto& sc = s_shadowCache[skinId];
        if (sc.bmp != nullptr && sc.w == cr.right && sc.h == cr.bottom)
        {
            LARGE_INTEGER q0 = {}, q1 = {}, qf = {};
            ::QueryPerformanceCounter(&q0);
            ::BitBlt(hdc, 0, 0, cr.right, cr.bottom, sc.dc, 0, 0, SRCCOPY);
            ::QueryPerformanceCounter(&q1);
            ::QueryPerformanceFrequency(&qf);
            debugLogf(L"[shadow-cache] HIT id=%d %dx%d blit=%.2f ms", skinId, cr.right, cr.bottom,
                      (double)(q1.QuadPart - q0.QuadPart) * 1000.0 / (double)qf.QuadPart);
            return;
        }
        debugLogf(L"[shadow-cache] MISS id=%d kind=%d bmp=%p %dx%d", skinId, skinKind,
                  static_cast<void*>(sc.bmp), cr.right, cr.bottom);
    }

    if (s_memW < cr.right || s_memH < cr.bottom)
    {
        const int nw = cr.right > s_memW ? cr.right : s_memW;
        const int nh = cr.bottom > s_memH ? cr.bottom : s_memH;
        if (s_memBmp != nullptr) { ::DeleteObject(s_memBmp); s_memBmp = nullptr; }
        if (s_memDc != nullptr) { ::DeleteDC(s_memDc); s_memDc = nullptr; }

        BITMAPINFO bi = {};
        bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bi.bmiHeader.biWidth = nw;
        bi.bmiHeader.biHeight = -nh;      // 负高度 = 自顶向下，和 D2D 坐标一致
        bi.bmiHeader.biPlanes = 1;
        bi.bmiHeader.biBitCount = 32;
        bi.bmiHeader.biCompression = BI_RGB;
        s_memBmp = ::CreateDIBSection(nullptr, &bi, DIB_RGB_COLORS, &s_memBits, nullptr, 0);
        s_memDc = ::CreateCompatibleDC(nullptr);
        if (s_memBmp != nullptr)
            ::SelectObject(s_memDc, s_memBmp);
        else
            s_memDc = nullptr;
        s_memW = nw;
        s_memH = nh;
    }

    // 快路径：纯底色面板（Static 类、没有文字、没声明边框样式、不透明）不值得走 D2D。
    // 这类控件的耗时是 bind + begin + 整块 BitBlt，与"画的内容"无关：一块纯色也要 13 ms。
    if (::GetWindowTextLengthW(child) == 0)
    {
        wchar_t clsName[32] = {};
        ::GetClassNameW(child, clsName, 32);
        const xamlapp::XamlAppearance* fap = findAppearance(::GetDlgCtrlID(child));
        if (_wcsicmp(clsName, L"Static") == 0 && fap != nullptr && !fap->backColor.empty()
            && fap->borderStyle.empty() && fap->opacity >= 255)
        {
            COLORREF fc = m_bg;
            xamlapp::ParseColor(fap->backColor, fc);
            HBRUSH fb = ::CreateSolidBrush(fc);
            if (fb != nullptr)
            {
                ::FillRect(hdc, &cr, fb);
                ::DeleteObject(fb);
            }
            return;
        }
    }

    HDC renderDc = s_memDc != nullptr ? s_memDc : hdc;
    canvasPtr->bind(renderDc, cr);
    if (!canvasPtr->begin(cr.right, cr.bottom))
    {
        debugLogf(L"[skin] id=%d begin failed w=%d h=%d", ::GetDlgCtrlID(child),
                  static_cast<int>(cr.right), static_cast<int>(cr.bottom));
        return;
    }
    LARGE_INTEGER perf0 = {}, perfFreq = {};
    if (paintTimingEnabled())
    {
        ::QueryPerformanceCounter(&perf0);
        ::QueryPerformanceFrequency(&perfFreq);
    }

    // 控件级真 opacity 图层：整块（底板 + 形状 + 文字）在 D2D 图层里按 alpha 合成。
    // 必须在 drawSkinControl 之前 push、之后 pop，否则 end() 时图层没关会丢掉内容。
    const int layerAlpha = getLayerOpacity(::GetDlgCtrlID(child));
    if (layerAlpha < 255)
        canvasPtr->pushOpacity(layerAlpha);
    drawSkinControl(*canvasPtr, child);
    if (layerAlpha < 255)
        canvasPtr->popOpacity();
    canvasPtr->end();
    if (renderDc != hdc)
        ::BitBlt(hdc, 0, 0, cr.right, cr.bottom, renderDc, 0, 0, SRCCOPY);

    if (skinKind == kSkinBoxShadow && renderDc != nullptr)
    {
        auto& sc = s_shadowCache[skinId];
        if (sc.bmp == nullptr || sc.w != cr.right || sc.h != cr.bottom)
        {
            if (sc.bmp != nullptr) { ::DeleteObject(sc.bmp); sc.bmp = nullptr; }
            if (sc.dc != nullptr) { ::DeleteDC(sc.dc); sc.dc = nullptr; }
            BITMAPINFO bi = {};
            bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bi.bmiHeader.biWidth = cr.right;
            bi.bmiHeader.biHeight = -cr.bottom;
            bi.bmiHeader.biPlanes = 1;
            bi.bmiHeader.biBitCount = 32;
            bi.bmiHeader.biCompression = BI_RGB;
            void* bits = nullptr;
            sc.bmp = ::CreateDIBSection(nullptr, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
            sc.dc = ::CreateCompatibleDC(nullptr);
            if (sc.bmp != nullptr && sc.dc != nullptr)
            {
                ::SelectObject(sc.dc, sc.bmp);
                sc.w = cr.right; sc.h = cr.bottom;
                ::BitBlt(sc.dc, 0, 0, cr.right, cr.bottom, renderDc, 0, 0, SRCCOPY);
            }
            else
            {
                if (sc.bmp != nullptr) { ::DeleteObject(sc.bmp); sc.bmp = nullptr; }
                if (sc.dc != nullptr) { ::DeleteDC(sc.dc); sc.dc = nullptr; }
                sc.w = sc.h = 0;
            }
        }
        else
        {
            ::BitBlt(sc.dc, 0, 0, cr.right, cr.bottom, renderDc, 0, 0, SRCCOPY);
        }
    }

    if (paintTimingEnabled())
    {
        LARGE_INTEGER perf1 = {};
        ::QueryPerformanceCounter(&perf1);
        const double ms = static_cast<double>(perf1.QuadPart - perf0.QuadPart) * 1000.0 /
                          static_cast<double>(perfFreq.QuadPart);
        wchar_t cls[32] = {};
        ::GetClassNameW(child, cls, 32);
        static LARGE_INTEGER qpcOrigin = {};
        if (qpcOrigin.QuadPart == 0) qpcOrigin = perf0;
        const double tms = static_cast<double>(perf0.QuadPart - qpcOrigin.QuadPart) * 1000.0 /
                           static_cast<double>(perfFreq.QuadPart);
        paintLogf(L"[paint] t=%.1f id=%d cls=%ls ms=%.2f size=%dx%d", tms, ::GetDlgCtrlID(child), cls, ms,
                  static_cast<int>(cr.right), static_cast<int>(cr.bottom));
    }
}

// ---- 自绘滚动区域（ScrolledWindow）的交互 -------------------------------
bool PyMfcHost::isScrollArea(HWND child) const
{
    return child != nullptr && m_scrollAreas.count(::GetDlgCtrlID(child)) != 0;
}

void PyMfcHost::nudgeScroll(HWND child, int delta)
{
    const int id = ::GetDlgCtrlID(child);
    if (m_scrollAreas.count(id) == 0)
        return;

    auto rng = m_scroll.find(id);
    const int lo = rng != m_scroll.end() ? rng->second.first : 0;
    const int hi = rng != m_scroll.end() ? rng->second.second : 100;

    auto itPos = m_scrollPos.find(id);
    int pos = itPos != m_scrollPos.end() ? itPos->second : lo;
    // delta 以"一档"为单位（一档 ≈ 范围的 1/12），滚轮一次给 ±1
    const int step = (hi - lo) / 12 > 0 ? (hi - lo) / 12 : 1;
    pos += delta * step;
    if (pos < lo)
        pos = lo;
    if (pos > hi)
        pos = hi;
    setScrollAreaPos(child, pos, true);
}

// 滚动区域位置唯一写入口：夹边、变了才重画、notify 时派发 Scroll 事件。
// 与独立 ScrollBar 的 setScrollValueUser 对齐：用户改值才发事件，程序设值不发。
void PyMfcHost::setScrollAreaPos(HWND child, int pos, bool notify)
{
    const int id = ::GetDlgCtrlID(child);
    if (m_scrollAreas.count(id) == 0)
        return;

    auto rng = m_scroll.find(id);
    const int lo = rng != m_scroll.end() ? rng->second.first : 0;
    const int hi = rng != m_scroll.end() ? rng->second.second : 100;
    if (pos < lo)
        pos = lo;
    if (pos > hi)
        pos = hi;

    auto itPos = m_scrollPos.find(id);
    const int before = itPos != m_scrollPos.end() ? itPos->second : lo;
    if (pos == before)
        return;                       // 滚到头不重复派发

    m_scrollPos[id] = pos;
    ::InvalidateRect(child, nullptr, FALSE);
    if (notify && m_anyCtlEvent)
        fireControlEvent(id, L"scroll");
}

// 悬浮滚轮：从光标下的窗口往上找宿主，再判断这一个窗口是不是滚动件。
// notches > 0 = 上滚（值变小），与点轨道、滚轮既有方向保持一致。
bool PyMfcHost::wheelOnHoveredWindow(HWND w, POINT screenPt, int notches)
{
    const int id = ::GetDlgCtrlID(w);
    auto itKind = m_skinKinds.find(id);
    if (itKind != m_skinKinds.end() && itKind->second == kSkinBoxScrollBar)
    {
        // 独立 ScrollBar：滚轮半页（整块都能滚）
        const int lo = rangeMinOf(id);
        const int page = m_scrollPage.count(id) ? m_scrollPage[id] : 10;
        const int cur = m_scrollVal.count(id) ? m_scrollVal[id] : lo;
        const int half = page / 2 > 1 ? page / 2 : 1;
        setScrollValueUser(id, cur - notches * half);
        ::InvalidateRect(w, nullptr, TRUE);
        return true;
    }
    if (m_scrollAreas.count(id) != 0)
    {
        // 滚动区域：只认右侧滚动条条带，内容区里的滚轮留给内容控件自己
        RECT cr = {};
        ::GetClientRect(w, &cr);
        POINT pt = screenPt;
        ::ScreenToClient(w, &pt);
        if (pt.x >= cr.right - 18)
        {
            nudgeScroll(w, -notches);
            return true;
        }
    }
    return false;
}

bool PyMfcHost::wheelHoverAtPoint(POINT screenPt, int notches)
{
    HWND w = ::WindowFromPoint(screenPt);
    while (w != nullptr)
    {
        PyMfcHost* host = reinterpret_cast<PyMfcHost*>(::GetPropW(w, kSkinHostProp));
        if (host != nullptr)
            return host->wheelOnHoveredWindow(w, screenPt, notches);
        w = ::GetParent(w);
    }
    return false;
}

void PyMfcHost::scrollAreaMouse(HWND child, UINT msg, WPARAM wParam, LPARAM lParam)
{
    const int id = ::GetDlgCtrlID(child);
    if (m_scrollAreas.count(id) == 0)
        return;

    auto rng = m_scroll.find(id);
    const int lo = rng != m_scroll.end() ? rng->second.first : 0;
    const int hi = rng != m_scroll.end() ? rng->second.second : 100;
    if (hi <= lo)
        return;

    RECT cr = {0, 0, 0, 0};
    ::GetClientRect(child, &cr);
    const int mx = static_cast<int>(static_cast<short>(LOWORD(lParam)));
    const int my = static_cast<int>(static_cast<short>(HIWORD(lParam)));
    const POINT pt = { mx, my };
    debugLogf(L"[scroll] id=%d msg=0x%X pt=%d,%d client=%dx%d", id, msg, mx, my,
              static_cast<int>(cr.right), static_cast<int>(cr.bottom));

    auto itPos = m_scrollPos.find(id);
    const int pos = itPos != m_scrollPos.end() ? itPos->second : lo;
    RECT thumb = pymfcskin::scrollThumbRect(cr, pos, lo, hi);
    if (thumb.bottom <= thumb.top)
        return;

    const int span = hi - lo;
    const int trackTop = cr.top + 3;
    const int trackBottom = cr.bottom - 3;
    const int thumbH = thumb.bottom - thumb.top;
    const int freeH = (trackBottom - trackTop) - thumbH;

    switch (msg)
    {
    case WM_LBUTTONDOWN:
        if (PtInRect(&thumb, pt))
        {
            m_dragScroll[id] = my - thumb.top;   // 抓取偏移
            ::SetCapture(child);
        }
        else if (mx >= cr.right - 18)
        {
            // 点轨道：翻一页（一页 = 视口高度），与独立 ScrollBar 的"点轨道跳一页"一致
            const int pageH = (cr.bottom - cr.top) > 1 ? (cr.bottom - cr.top) : 1;
            setScrollAreaPos(child, pos + (my < thumb.top ? -pageH : pageH), true);
        }
        break;

    case WM_MOUSEMOVE:
    {
        auto itDrag = m_dragScroll.find(id);
        if (itDrag == m_dragScroll.end() || ::GetCapture() != child || freeH <= 0)
            break;
        const int y = my - itDrag->second;              // 拇指左上角想落在哪
        int p = (y - trackTop) * span / freeH;
        if (p < 0)
            p = 0;
        if (p > span)
            p = span;
        setScrollAreaPos(child, lo + p, true);
        break;
    }

    case WM_LBUTTONUP:
        if (m_dragScroll.erase(id) > 0 && ::GetCapture() == child)
            ::ReleaseCapture();
        break;

    default:
        break;
    }
}


// 动画时钟：消息专用窗口自己的过程，只处理 WM_TIMER，其余交给系统默认。
// 宿主主窗口的滚轮拦截：没命中注册区域时原样交回 MFC 的 AfxWndProc。
static std::map<HWND, WNDPROC>& wheelOldProcs()
{
    static std::map<HWND, WNDPROC> table;
    return table;
}
static const wchar_t* kWheelHostProp = L"PyMfcWheelHost";

// 手型：可点控件（按钮 / 滚动条 / 链接…）统一用它
HCURSOR pyMfcHandCursor()
{
    static HCURSOR cur = nullptr;
    if (cur == nullptr)
        cur = ::LoadCursorW(nullptr, IDC_HAND);
    return cur;
}

// 爪型：系统光标表里只有"指向手"，没有 grab 手型，从 res\grab.cur 载入。
// 载不到退回手型（绝不退回箭头，否则画布上看不出可拖）。
HCURSOR pyMfcGrabCursor()
{
    static HCURSOR cur = nullptr;
    static bool tried = false;
    if (!tried)
    {
        tried = true;
        const std::wstring dir = pymfcDir();
        const std::wstring cand[] = { dir + L"\\res\\grab.cur", dir + L"\\grab.cur" };
        for (const std::wstring& p : cand)
        {
            cur = reinterpret_cast<HCURSOR>(
                ::LoadImageW(nullptr, p.c_str(), IMAGE_CURSOR, 0, 0, LR_LOADFROMFILE));
            if (cur != nullptr)
                break;
        }
        debugLogf(L"[cursor] grab=%p", reinterpret_cast<void*>(cur));
    }
    return cur != nullptr ? cur : pyMfcHandCursor();
}

// "握住的"（grabbing）：按下画布拖动时用。资源 res\grab_down.cur，
// 没有该文件就退回 grab（拖起来至少还是爪型，不会掉成箭头）。
HCURSOR pyMfcGrabDownCursor()
{
    static HCURSOR cur = nullptr;
    static bool tried = false;
    if (!tried)
    {
        tried = true;
        const std::wstring dir = pymfcDir();
        const std::wstring cand[] = { dir + L"\\res\\grab_down.cur", dir + L"\\grab_down.cur" };
        for (const std::wstring& p : cand)
        {
            cur = reinterpret_cast<HCURSOR>(
                ::LoadImageW(nullptr, p.c_str(), IMAGE_CURSOR, 0, 0, LR_LOADFROMFILE));
            if (cur != nullptr)
                break;
        }
        debugLogf(L"[cursor] grab_down=%p", reinterpret_cast<void*>(cur));
    }
    return cur != nullptr ? cur : pyMfcGrabCursor();
}

// 画布当前该用的光标：左键按住 + 本窗口捕获 = 拖动中 → 握住的；否则 grab
HCURSOR pyMfcCanvasCursor(HWND hwnd)
{
    const bool pressed = (::GetCapture() == hwnd) &&
                         ((::GetKeyState(VK_LBUTTON) & 0x8000) != 0);
    return pressed ? pyMfcGrabDownCursor() : pyMfcGrabCursor();
}
static LRESULT CALLBACK PyMfcWheelProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    auto it = wheelOldProcs().find(hwnd);
    WNDPROC old = it != wheelOldProcs().end() ? it->second : nullptr;
    if (msg == WM_MOUSEWHEEL)
    {
        PyMfcHost* host = reinterpret_cast<PyMfcHost*>(::GetPropW(hwnd, kWheelHostProp));
        if (host != nullptr && host->fireWheelMessage(wp, lp))
            return 0;
    }
    return old != nullptr ? ::CallWindowProcW(old, hwnd, msg, wp, lp)
                          : ::DefWindowProcW(hwnd, msg, wp, lp);
}

// 供其它编译单元使用：安装滚轮拦截（顶层窗口专用）
WNDPROC installWheelHost(HWND hwnd, void* host)
{
    ::SetPropW(hwnd, kWheelHostProp, reinterpret_cast<HANDLE>(host));
    WNDPROC old = reinterpret_cast<WNDPROC>(
        ::SetWindowLongPtrW(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&PyMfcWheelProc)));
    wheelOldProcs()[hwnd] = old;
    return old;
}

LRESULT CALLBACK PyMfcHost::animHostProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_NCCREATE)
    {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        ::SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
        return TRUE;
    }
    if (msg == WM_TIMER && wParam == kAnimTimerId)
    {
        auto* host = reinterpret_cast<PyMfcHost*>(::GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (host != nullptr)
            host->animTick();
        return 0;
    }
    return ::DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK PyMfcHost::PyMfcSkinWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    WNDPROC old = reinterpret_cast<WNDPROC>(::GetPropW(hwnd, L"PyMfcSkinOldProc"));

    if (msg == WM_PAINT)
    {
        PAINTSTRUCT ps = {};
        HDC hdc = ::BeginPaint(hwnd, &ps);
        PyMfcHost* host = reinterpret_cast<PyMfcHost*>(::GetPropW(hwnd, L"PyMfcSkinHost"));
        if (host != nullptr && hdc != nullptr)
            host->paintSkinControl(hdc, hwnd);
        ::EndPaint(hwnd, &ps);
        return 0;
    }
    if (msg == WM_ERASEBKGND)
        return 1;

    // 悬浮光标：可点控件手型、画布爪型；容器/空白沿用系统箭头（不拦就不返回）
    if (msg == WM_SETCURSOR)
    {
        if (LOWORD(lParam) == HTCLIENT)
        {
            PyMfcHost* host = reinterpret_cast<PyMfcHost*>(::GetPropW(hwnd, L"PyMfcSkinHost"));
            if (host != nullptr)
            {
                HCURSOR cur = host->hoverCursor(hwnd);
                if (cur != nullptr)
                {
                    ::SetCursor(cur);
                    TRACKMOUSEEVENT tme = {};
                    tme.cbSize = sizeof(tme);
                    tme.dwFlags = TME_LEAVE;
                    tme.hwndTrack = hwnd;
                    ::TrackMouseEvent(&tme);
                    return TRUE;
                }
            }
        }
        return old != nullptr ? ::CallWindowProcW(old, hwnd, msg, wParam, lParam)
                              : ::DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    // 离开时复位成箭头，否则最后设的手型会一直挂着
    if (msg == WM_MOUSELEAVE)
        ::SetCursor(::LoadCursorW(nullptr, IDC_ARROW));

    // 画布子窗口改尺寸（含窗口最大化）：按新客户区重新 fit 一次。
    // 这条消息原先只在 PyMfcHost.cpp 的早期子类过程里处理，控件改由皮肤过程画以后
    // 就不再走到那里，于是最大化后画布变大而 zoom 不动，看着像"缩成全局视图"。
    if (msg == WM_SIZE)
    {
        HANDLE ph = ::GetPropW(hwnd, kPlanHostProp);
        if (ph != nullptr)
        {
            PyMfcHost* host = reinterpret_cast<PyMfcHost*>(ph);
            host->fitPlanCanvas(::GetDlgCtrlID(hwnd));
            ::InvalidateRect(hwnd, nullptr, FALSE);
        }
    }

    // WM_PRINTCLIENT：把"控件自己那一格"画到调用方给的 DC 上。
    // 验收脚本靠它取像素（屏幕抓图会被别的窗口压住，PrintWindow 又不含 D2D 子窗口）。
    if (msg == WM_PRINTCLIENT && wParam != 0)
    {
        PyMfcHost* host = reinterpret_cast<PyMfcHost*>(::GetPropW(hwnd, L"PyMfcSkinHost"));
        if (host != nullptr)
        {
            host->paintSkinControl(reinterpret_cast<HDC>(wParam), hwnd);
            return 0;
        }
    }

    // 光标闪烁：拿焦点起表、失去焦点停表。间隔沿用系统的 GetCaretBlinkTime，
    // 用户换"光标闪烁速度"时跟着变。
    if (msg == WM_SETFOCUS)
    {
        caretBlinkStates()[hwnd] = true;
        ::SetTimer(hwnd, kCaretTimerId, static_cast<UINT>(::GetCaretBlinkTime()), nullptr);
    }
    else if (msg == WM_KILLFOCUS)
    {
        ::KillTimer(hwnd, kCaretTimerId);
        caretBlinkStates().erase(hwnd);
    }
    else if (msg == WM_TIMER && wParam == kCaretTimerId)
    {
        const bool on = !caretBlinkOn(hwnd);
        caretBlinkStates()[hwnd] = on;
        debugLogf(L"[caret] id=%d on=%d", ::GetDlgCtrlID(hwnd), on ? 1 : 0);
        ::InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }
    else if (msg == WM_NCDESTROY)
    {
        ::KillTimer(hwnd, kCaretTimerId);
        caretBlinkStates().erase(hwnd);
    }

    PyMfcHost* host = reinterpret_cast<PyMfcHost*>(::GetPropW(hwnd, L"PyMfcSkinHost"));

    // PyMfcSkinBox：自建画布窗口，鼠标语义（点开关/拖拇指/点三角/点页码）完全由宿主决定，
    // 不能交给 DefWindowProc（它什么都不做，点了就等于没响应）。
    if (host != nullptr && ::GetPropW(hwnd, kSkinKindProp) != nullptr)
    {
        // Flickable / 页容器会把普通控件 SetParent 到自己家里（为了让视口裁剪生效），
        // 于是这些控件的 owner-draw 通知（WM_DRAWITEM）和背景询色（WM_CTLCOLOR*）
        // 会发给本窗口而不是宿主。这里原样转给宿主，否则搬进来的控件只剩一片白底、没有文字。
        if (msg == WM_DRAWITEM || msg == WM_CTLCOLORSTATIC || msg == WM_CTLCOLORBTN ||
            msg == WM_CTLCOLOREDIT || msg == WM_CTLCOLORLISTBOX)
        {
            HWND hp = ::GetParent(hwnd);
            if (hp != nullptr)
                return ::SendMessageW(hp, msg, wParam, lParam);
        }
        if (msg == WM_TIMER && wParam == kBusyTimerId)
        {
            const int id = ::GetDlgCtrlID(hwnd);
            host->bumpBusyPhase(id);
            ::InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        if (msg == WM_TIMER && wParam == kPageTimerId)
        {
            const int id = ::GetDlgCtrlID(hwnd);
            if (host->pageTick(id) == 0)
                ::KillTimer(hwnd, kPageTimerId);
            return 0;
        }
        if (msg == WM_TIMER && wParam == kFlickTimerId)
        {
            const int id = ::GetDlgCtrlID(hwnd);
            if (host->flickTick(id) == 0)
                ::KillTimer(hwnd, kFlickTimerId);
            return 0;
        }
        if (msg == WM_TIMER && wParam == kTipTimerId)
        {
            ::KillTimer(hwnd, kTipTimerId);      // ToolTip 到点自己消失
            ::ShowWindow(hwnd, SW_HIDE);
            return 0;
        }
        if (msg == WM_LBUTTONDBLCLK)
        {
            host->skinBoxMouse(hwnd, WM_LBUTTONDBLCLK, wParam, lParam);
            return 0;
        }

        if (host->skinBoxMouse(hwnd, msg, wParam, lParam))
            return 0;
    }

    // 命中测试：Static / SysLink 的类过程默认回 HTTRANSPARENT（把点击让给父窗口），
    // 一旦绘制被接管、交互要自己处理，这个"透明"就让滚动条拖不动、链接点不动。
    // 装上皮肤过程的控件一律声明自己是可点的客户区。
    if (msg == WM_NCHITTEST)
        return HTCLIENT;

    // 自绘滚动区域：滚轮与滚动条拖动由宿主处理，位置变化后自己重画
    if (host != nullptr && host->isScrollArea(hwnd))
    {
        if (msg == WM_MOUSEWHEEL)
        {
            const int delta = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
            host->nudgeScroll(hwnd, -delta);          // 往下滚 = 位置增加
            return 0;
        }
        if (msg == WM_LBUTTONDOWN || msg == WM_MOUSEMOVE || msg == WM_LBUTTONUP)
        {
            host->scrollAreaMouse(hwnd, msg, wParam, lParam);
            return 0;
        }
    }

    LRESULT result = old != nullptr
        ? ::CallWindowProcW(old, hwnd, msg, wParam, lParam)
        : ::DefWindowProcW(hwnd, msg, wParam, lParam);

    // SysLink：接管 WM_PAINT 之后系统类一个 NM_CLICK 都不发（它的链接命中表由自己的
    // 绘制路径建立，绘制被掏空后就永远是空的）。超链接整块控件就是链接本身，
    // 抬起即派发 NM_CLICK，命中由我们自己决定。
    if (host != nullptr && msg == WM_LBUTTONUP)
    {
        wchar_t clsName[64] = {};
        ::GetClassNameW(hwnd, clsName, 64);
        if (_wcsicmp(clsName, L"SysLink") == 0)
        {
            debugLogf(L"[link] lbuttonup id=%d", ::GetDlgCtrlID(hwnd));
            static DWORD lastTick = 0;
            const DWORD now = ::GetTickCount();
            if (now - lastTick > 300)         // 去重：同一次点击只派发一次
            {
                lastTick = now;
                NMMOUSE nm = {};
                nm.hdr.hwndFrom = hwnd;
                nm.hdr.idFrom = static_cast<UINT_PTR>(::GetDlgCtrlID(hwnd));
                nm.hdr.code = NM_CLICK;
                ::SendMessageW(::GetParent(hwnd), WM_NOTIFY, nm.hdr.idFrom,
                               reinterpret_cast<LPARAM>(&nm));
            }
            ::InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
    }

    // 原地单元格编辑：EDIT 上按回车（确认）/ Esc（取消）直接回抛给 Python。
    // 不回抛就只能靠失焦提交，用户按回车会"响一声但没反应"。
    if (host != nullptr && msg == WM_KEYDOWN &&
        (wParam == VK_RETURN || wParam == VK_ESCAPE))
    {
        wchar_t clsName2[64] = {};
        ::GetClassNameW(hwnd, clsName2, 64);
        if (_wcsicmp(clsName2, L"Edit") == 0)
        {
            wchar_t buf[1024] = {};
            ::GetWindowTextW(hwnd, buf, 1024);
            host->fireGridAction(::GetDlgCtrlID(hwnd), "cellend", -1, 0, std::wstring(buf));
            return 0;
        }
    }

    // 状态类消息之后重画自己：内容/选中/滚动/焦点都会改变外观。
    // 注意不要把 WM_MOUSEMOVE / WM_SETCURSOR 放进来：这两个在鼠标划过时每秒能来几十次，
    // 等于让整个控件跟着鼠标不停重画 —— 表现出来就是"选项卡切换卡顿/闪"。
    // 现在没有悬停态绘制，移动鼠标不需要重画。
    switch (msg)
    {
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_MOUSEWHEEL:
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_CHAR:
    case WM_VSCROLL:
    case WM_HSCROLL:
    case WM_SETFOCUS:
    case WM_KILLFOCUS:
    case WM_ENABLE:
    case WM_SETTEXT:
    case WM_SYSKEYDOWN:
    case WM_LBUTTONDBLCLK:          // 双击选词
    case WM_CUT:                    // 剪切
    case WM_COPY:                   // 复制
    case WM_PASTE:                  // 粘贴
    case WM_CLEAR:                  // 删除选区
    case EM_SETSEL:                 // 程序化改选区（0xB1，0xB0 是 GETSEL）
    case EM_UNDO:                   // 撤销
    case 0x00C8:                    // EM_REDO（richedit.h 里的名字，这里用值免头文件依赖）
        ::InvalidateRect(hwnd, nullptr, FALSE);
        break;
    default:
        break;
    }
    return result;
}

HBRUSH PyMfcHost::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor){
    // XAML 声明了外观的控件优先按声明渲染（所有控件共用同一套外观契约）
    if (pWnd != nullptr)
    {
        const int cid = pWnd->GetDlgCtrlID();
        if (const xamlapp::XamlAppearance* ap = findAppearance(cid))
        {
            COLORREF bg = m_bg;
            if (!ap->backColor.empty())
            {
                COLORREF c = 0;
                if (xamlapp::ParseColor(ap->backColor, c))
                    bg = xamlapp::Blend(c, m_bg, ap->opacity);
            }
            COLORREF fg = contrastText(bg);
            if (!ap->foreColor.empty())
            {
                COLORREF c = 0;
                if (xamlapp::ParseColor(ap->foreColor, c))
                    fg = c;
            }
            pDC->SetBkMode(nCtlColor == CTLCOLOR_STATIC ? OPAQUE : TRANSPARENT);
            pDC->SetBkColor(bg);
            pDC->SetTextColor(fg);
            auto bit = m_apBrush.find(cid);
            if (bit != m_apBrush.end() && bit->second != nullptr)
                return bit->second;
        }
    }

    if (nCtlColor == CTLCOLOR_STATIC)
    {
        // 不透明背景，避免 Static 重绘时旧文字残留形成“阴影/重影”。
        pDC->SetBkMode(OPAQUE);
        pDC->SetBkColor(m_bg);
        pDC->SetTextColor(m_text);
        return m_bgBrush;
    }
    if (nCtlColor == CTLCOLOR_EDIT || nCtlColor == CTLCOLOR_LISTBOX)
    {
        pDC->SetBkColor(m_bg);
        pDC->SetTextColor(m_text);
        return m_bgBrush;
    }
    return CWnd::OnCtlColor(pDC, pWnd, nCtlColor);
}

void PyMfcHost::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lp)
{
    if (lp == nullptr || lp->hwndItem == nullptr)
        return;

    CDC dc;
    dc.Attach(lp->hDC);
    CRect rc = lp->rcItem;

    // 所有控件共用同一套外观契约：声明了就按声明画，没声明走主题默认
    const int cid = ::GetDlgCtrlID(lp->hwndItem);
    const xamlapp::XamlAppearance* ap = findAppearance(cid);
    const bool styled = ap != nullptr && !ap->empty();
    RECT wr = {}, cr = {};
    ::GetWindowRect(lp->hwndItem, &wr);
    ::GetClientRect(lp->hwndItem, &cr);
    const LONG_PTR wst = ::GetWindowLongPtrW(lp->hwndItem, GWL_STYLE);
    debugLogf(L"[drawitem] id=%d styled=%d raw=%dx%d client=%dx%d style=0x%llX bs='%ls' bc='%ls'",
              cid, styled ? 1 : 0, static_cast<int>(wr.right - wr.left), static_cast<int>(wr.bottom - wr.top),
              static_cast<int>(cr.right), static_cast<int>(cr.bottom),
              static_cast<unsigned long long>(wst),
              ap ? ap->borderStyle.c_str() : L"-", ap ? ap->borderColor.c_str() : L"-");
    const bool pressed = (lp->itemState & ODS_SELECTED) != 0;
    const bool disabled = (lp->itemState & ODS_DISABLED) != 0;
    // CtlType 对 SS_OWNERDRAW 的 Static 并不总是回 ODT_STATIC（实测会回 ODT_STATBAR），
    // 一旦判错就会把 Static 当按钮、用深色笔画圆角边框，白底页上就是"每个标签都有黑框"。
    // 所以按窗口类名判定，比 CtlType 可靠。
    wchar_t clsName[64] = {};
    ::GetClassNameW(lp->hwndItem, clsName, 64);
    const bool isStatic = (_wcsicmp(clsName, L"Static") == 0) || (lp->CtlType == ODT_STATIC);

    CString text;
    ::GetWindowTextW(lp->hwndItem, text.GetBufferSetLength(512), 512);
    text.ReleaseBuffer();

    // ---- 绘制：只有一套后端，Direct2D（DCRenderTarget 直接画在 owner-draw 的 HDC 上）----
    // 不再保留 GDI 第二套绘制实现：D2D 自身在渲染目标类型 DEFAULT 下已有硬件→软件回退，
    // 真建不起来（D2D 工厂缺失）就让该控件留空并记一次日志，也绝不留空指针给下面的 cv->xxx。
    // 与 L1103 同一个根因：全进程一个画布、控件尺寸轮流变 -> 每帧 releaseTarget()
    // 重建目标与全部笔刷/文字格式。这里按控件 id 各持一个，尺寸不变时只 BindDC。
    static std::map<int, D2DDcCanvas*> s_d2dCanvases;
    D2DDcCanvas*& d2dCanvasPtr = s_d2dCanvases[cid];
    if (d2dCanvasPtr == nullptr) d2dCanvasPtr = new D2DDcCanvas();
    D2DDcCanvas& d2dCanvas = *d2dCanvasPtr;
    XamlCanvas* cv = nullptr;
    if (cr.right > 0 && cr.bottom > 0)
    {
        const RECT bindRc = {0, 0, cr.right, cr.bottom};
        d2dCanvas.bind(static_cast<HDC>(dc.GetSafeHdc()), bindRc);
        if (d2dCanvas.begin(cr.right, cr.bottom))
        {
            cv = &d2dCanvas;
            static int loggedD2D = 0;
            if (loggedD2D++ == 0)
                debugLogf(L"[canvas] id=%d backend=d2d hw=%d hr=0x%08X size=%dx%d",
                          cid, d2dCanvas.isHardware() ? 1 : 0,
                          static_cast<unsigned>(d2dCanvas.lastError()),
                          static_cast<int>(cr.right), static_cast<int>(cr.bottom));
        }
        else
        {
            static int loggedFail = 0;
            if (loggedFail++ == 0)
                debugLogf(L"[canvas] id=%d backend=d2d FAILED hr=0x%08X size=%dx%d",
                          cid, static_cast<unsigned>(d2dCanvas.lastError()),
                          static_cast<int>(cr.right), static_cast<int>(cr.bottom));
        }
    }
    if (cv == nullptr)
    {
        dc.Detach();
        return;
    }

    const RECT itemRc = rc;
    // 配电画布：数据在宿主里，按 zoom/offset 每帧重画，与其它控件的"声明式外观"无关
    if (m_planCv.find(cid) != m_planCv.end())
    {
        drawPlanCanvas(*cv, cid, itemRc);
        cv->end();
        dc.Detach();
        return;
    }
    // 控件矩形映射到父客户区：容器底色表存的是父客户区坐标，用屏幕坐标永远命中不到。
    RECT wrParent = wr;
    ::MapWindowPoints(nullptr, m_hWnd, reinterpret_cast<LPPOINT>(&wrParent), 2);

    // 分隔线：整块铺父表面色，仅在竖直中线上画一条主题色细线
    if (m_lines.find(cid) != m_lines.end())
    {
        const int y = (itemRc.top + itemRc.bottom) / 2;
        const int wSpan = static_cast<int>(itemRc.right - itemRc.left);
        const int hSpan = static_cast<int>(itemRc.bottom - itemRc.top);
        debugLogf(L"[line] id=%d item=%d,%d,%d,%d y=%d surface=%06X border=%06X",
                  cid, static_cast<int>(itemRc.left), static_cast<int>(itemRc.top),
                  static_cast<int>(itemRc.right), static_cast<int>(itemRc.bottom), y,
                  static_cast<unsigned>(surfaceColorAt(CRect(wrParent))), static_cast<unsigned>(m_border));
        cv->fillRect(itemRc, surfaceColorAt(CRect(wrParent)));
        // 按形状决定方向：竖条（高 >> 宽）画竖直中线，横条画水平中线。
        // 原来一律按"横线"画，传进来的竖向分割条就只剩一条看不见的横短线。
        if (hSpan > wSpan * 3)
        {
            // 宽条（可拖动分割条）整块铺一条可见的浅灰，再压 1px 深线；
            // 细条（1~2px）只画中间那条深线。否则 6px 分割条在浅色背景上等于没有。
            if (wSpan >= 3)
            {
                cv->fillRect(itemRc, RGB(0xE4, 0xE7, 0xEC));
                const int x = (itemRc.left + itemRc.right) / 2;
                cv->fillRect(RECT{ x, itemRc.top, x + 1, itemRc.bottom }, m_border);
            }
            else
            {
                const int x = (itemRc.left + itemRc.right) / 2;
                cv->fillRect(RECT{ x, itemRc.top, x + 1, itemRc.bottom }, m_border);
            }
        }
        else
        {
            cv->fillRect(RECT{ itemRc.left, y, itemRc.right, y + 1 }, m_border);
        }
        cv->end();
        dc.Detach();
        return;
    }
    // 该控件所在的父表面底色：未声明 Background 时用它垫底，
    // 否则白卡上的文字会被刷成页面色，每行挂一条横带（见 surfaceColorAt）
    const COLORREF surface = surfaceColorAt(CRect(wrParent));
    auto accentIt = m_accent.find(cid);
    const bool hasAccent = (accentIt != m_accent.end()) && !isStatic;
    const COLORREF accent = hasAccent ? accentIt->second : m_border;
    if (styled)
    {
        debugLogf(L"[styled] id=%d cls='%ls' bs='%ls' bc='%ls' shape='%ls'",
                  cid, clsName, ap->borderStyle.c_str(), ap->borderColor.c_str(), ap->shape.c_str());
        xamlDrawBackplate(*cv, itemRc, *ap, surface, m_border);
    }
    else
    {
        debugLogf(L"[else] id=%d cls='%ls' isStatic=%d", cid, clsName, isStatic ? 1 : 0);
        cv->fillRect(itemRc, surface);
        // primary 按钮：白底（对应 HTML .btn 的 background:#fff）
        const COLORREF face = isStatic ? surface
                                       : (hasAccent ? RGB(255, 255, 255)
                                                    : (pressed ? RGB(80, 90, 104) : m_btn));
        cv->fillRoundRect(itemRc, face, isStatic ? surface : (hasAccent ? accent : m_border), 8);
    }

    COLORREF back = surface;
    if (styled && !ap->backColor.empty())
    {
        COLORREF c = 0;
        if (xamlapp::ParseColor(ap->backColor, c))
            back = xamlapp::Blend(c, surface, ap->opacity);
    }
    COLORREF fg = disabled ? RGB(140, 140, 140) : contrastText(back);
    if (styled && !ap->foreColor.empty())
    {
        COLORREF c = 0;
        if (xamlapp::ParseColor(ap->foreColor, c))
            fg = disabled ? RGB(140, 140, 140) : c;
    }
    if (hasAccent)
        fg = disabled ? RGB(140, 140, 140) : accent;
    auto labelIt = m_labelFg.find(cid);
    if (labelIt != m_labelFg.end() && !disabled)
        fg = labelIt->second;

    UINT align = isStatic ? DT_LEFT : DT_CENTER;
    if (styled)
        align = xamlapp::TextFlag(*ap);

    RECT trc = itemRc;
    if (styled && align == DT_LEFT)
        trc.left += 6;

    // 有 Icon 时：左侧方形图标，文字右移；只有图标时图标居中
    if (const std::wstring* iconPath = findIcon(cid))
    {
        const int side = (itemRc.bottom - itemRc.top) - 8;
        if (side > 0)
        {
            RECT ir = { itemRc.left + 4, itemRc.top + 4, itemRc.left + 4 + side, itemRc.top + 4 + side };
            if (text.IsEmpty())
            {
                ir.left = itemRc.left + ((itemRc.right - itemRc.left) - side) / 2;
                ir.right = ir.left + side;
            }
            if (ir.right <= itemRc.right)
            {
                cv->drawImage(ir, *iconPath);
                if (!text.IsEmpty())
                {
                    align = DT_LEFT;
                    const int textLeft = ir.right + 4;
                    if (trc.left < textLeft)
                        trc.left = textLeft;
                }
            }
        }
    }

    // 复选框 / 单选按钮：勾选块必须由这里画出来。
    // 这两个类被 owner-draw 接管 WM_PAINT 后，原生勾选标记根本不会上屏，
    // 屏幕上只剩标签文字 —— 看上去就是"只有壳、不知道选没选"。
    const UINT bsType = static_cast<UINT>(wst) & 0x0F;
    const bool isRadio = (bsType == BS_RADIOBUTTON) || (bsType == BS_AUTORADIOBUTTON);
    const bool isCheckButton = (_wcsicmp(clsName, L"Button") == 0) &&
                               (bsType == BS_CHECKBOX || bsType == BS_AUTOCHECKBOX ||
                                bsType == BS_3STATE || bsType == BS_AUTO3STATE || isRadio);
    if (isCheckButton)
    {
        const int boxSide = 16;
        const int h = itemRc.bottom - itemRc.top;
        if (h >= boxSide + 8)
        {
            const int top = itemRc.top + (h - boxSide) / 2;
            RECT box = { itemRc.left + 4, top, itemRc.left + 4 + boxSide, top + boxSide };
            cv->fillRoundRect(box, back, m_border, 3);
            const LRESULT chk = ::SendMessageW(lp->hwndItem, BM_GETCHECK, 0, 0);
            if (chk == BST_CHECKED)
            {
                RECT inner = { box.left + 3, box.top + 3, box.right - 3, box.bottom - 3 };
                if (isRadio)
                {
                    inner.left += 3;
                    inner.top += 3;
                    inner.right -= 3;
                    inner.bottom -= 3;
                }
                cv->fillRoundRect(inner, RGB(0, 120, 212), RGB(0, 120, 212), isRadio ? 7 : 2);
            }
            align = DT_LEFT;
            trc.left = box.right + 6;
            if (trc.left >= trc.right)
                trc.left = itemRc.left + 4;
        }
    }

    const int ptSize = (styled && ap->fontSize > 0) ? ap->fontSize : 0;
    const std::wstring family = (styled && !ap->fontFamily.empty()) ? ap->fontFamily : std::wstring();
    const std::wstring sText(text.GetString(), static_cast<size_t>(text.GetLength()));
    cv->text(trc, sText, fg, align, ptSize, family);

    // 焦点环：不用 DrawFocusRect —— 它是 XOR 虚线，深色底上又碎又刺眼，还会和圆角边缘错位。
    // 改成 1px 强调色圆角描边，跟皮肤同一套视觉。
    if ((lp->itemState & ODS_FOCUS) != 0 && !styled)
        cv->strokeRoundRect(itemRc, RGB(0, 120, 212), 8, 1);

    cv->end();

    dc.Detach();
}


// ---------------------------------------------------------------------------
// 配电画布的鼠标状态机：滚轮围绕光标缩放、左键拖动平移、单击命中卡片选中。
// 缩放不重新排版，只改 zoom/offset —— 文字与曲线都是每帧按新比例重画的，
// 所以放大后依然是矢量，不会像"放大一张位图"那样变糊。
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// 可编辑表格：SysListView32 本身不能编辑单元格，也没有 Ctrl+C/V / F2 / Insert 语义。
// 这里给它子类化一层窗口过程：记录活动单元格、拦键盘，把动作回抛给 Python
// （Python 手里才是真正的数据源，由它改完再 setTableCell 写回来，两边不会打架）。

static std::wstring gridCollectText(HWND lv)
{
    std::wstring out;
    int item = -1;
    bool first = true;
    while (true)
    {
        item = static_cast<int>(::SendMessageW(lv, LVM_GETNEXTITEM,
                             static_cast<WPARAM>(item), LVNI_SELECTED));
        if (item < 0)
            break;
        if (const HWND hdr = reinterpret_cast<HWND>(::SendMessageW(lv, LVM_GETHEADER, 0, 0)))
        {
            const int cols = static_cast<int>(::SendMessageW(hdr, HDM_GETITEMCOUNT, 0, 0));
            if (!first)
                out += L"\r\n";
            first = false;
            for (int c = 0; c < cols; ++c)
            {
                wchar_t buf[512] = {};
                LVITEMW it = {};
                it.iSubItem = c;
                it.pszText = buf;
                it.cchTextMax = 512;
                ::SendMessageW(lv, LVM_GETITEMTEXTW, static_cast<WPARAM>(item),
                               reinterpret_cast<LPARAM>(&it));
                if (c > 0)
                    out += L"\t";
                out += buf;
            }
        }
    }
    return out;
}

static void gridSetClipboard(HWND owner, const std::wstring& text)
{
    if (!::OpenClipboard(owner))
        return;
    ::EmptyClipboard();
    const size_t bytes = (text.size() + 1) * sizeof(wchar_t);
    if (HGLOBAL mem = ::GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, bytes))
    {
        if (void* p = ::GlobalLock(mem))
        {
            memcpy(p, text.c_str(), bytes);
            ::GlobalUnlock(mem);
            ::SetClipboardData(CF_UNICODETEXT, mem);   // 成功后内存归系统所有，不能再释放
        }
    }
    ::CloseClipboard();
}

static std::wstring gridGetClipboard(HWND owner)
{
    std::wstring out;
    if (!::OpenClipboard(owner))
        return out;
    if (HANDLE h = ::GetClipboardData(CF_UNICODETEXT))
    {
        if (const wchar_t* p = static_cast<const wchar_t*>(::GlobalLock(h)))
        {
            out = p;
            ::GlobalUnlock(h);
        }
    }
    ::CloseClipboard();
    return out;
}

LRESULT CALLBACK PyMfcGridProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    PyMfcHost* host = reinterpret_cast<PyMfcHost*>(::GetPropW(hwnd, kGridHostProp));
    if (host != nullptr)
    {
        const int id = ::GetDlgCtrlID(hwnd);
        if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONDBLCLK)
        {
            LVHITTESTINFO hi = {};
            hi.pt.x = GET_X_LPARAM(lParam);
            hi.pt.y = GET_Y_LPARAM(lParam);
            ::SendMessageW(hwnd, LVM_SUBITEMHITTEST, 0, reinterpret_cast<LPARAM>(&hi));
            host->m_gridCell[id] = std::make_pair(hi.iItem, hi.iSubItem);
            if (hi.iItem >= 0)
            {
                // 单击别的格：先让 Python 把上一格的原地编辑收尾（提交）。
                // 不能只靠 EN_KILLFOCUS —— 焦点若本来就不在编辑框上，失焦通知不会来。
                if (msg == WM_LBUTTONDOWN)
                    host->fireGridAction(id, "cellclick", hi.iItem, hi.iSubItem, L"");
                // 双击单元格：把 (行, 列) 回给 Python，由它把 EDIT 盖到该格上原地编辑
                else
                    host->fireGridAction(id, "dbl", hi.iItem, hi.iSubItem, L"");
            }
        }
        else if (msg == WM_KEYDOWN)
        {
            // 行优先取"当前选中项"（用户点过行就一定有选中），点过单元格时用它补齐列
            int row = static_cast<int>(::SendMessageW(hwnd, LVM_GETNEXTITEM,
                          static_cast<WPARAM>(-1), LVNI_SELECTED));
            auto it = host->m_gridCell.find(id);
            if (row < 0 && it != host->m_gridCell.end())
                row = it->second.first;
            const int col = (it != host->m_gridCell.end() && it->second.first >= 0)
                            ? it->second.second : 0;
            const bool ctrl = (::GetKeyState(VK_CONTROL) & 0x8000) != 0;
            if (wParam == VK_F2)
            {
                host->fireGridAction(id, "edit", row, col, L"");
                return 0;
            }
            if (wParam == VK_DELETE)
            {
                host->fireGridAction(id, "delete", row, col, L"");
                return 0;
            }
            if (wParam == VK_INSERT)
            {
                host->fireGridAction(id, "insert", row, col, L"");
                return 0;
            }
            if (ctrl && (wParam == 'C' || wParam == 'c'))
            {
                const std::wstring s = gridCollectText(hwnd);
                if (!s.empty())
                    gridSetClipboard(hwnd, s);
                return 0;
            }
            if (ctrl && (wParam == 'V' || wParam == 'v'))
            {
                host->fireGridAction(id, "paste", row, col, gridGetClipboard(hwnd));
                return 0;
            }
        }
    }
    if (WNDPROC old = reinterpret_cast<WNDPROC>(::GetPropW(hwnd, kGridOldProp)))
        return ::CallWindowProcW(old, hwnd, msg, wParam, lParam);
    return ::DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT PyMfcPlanCanvasMouse(PyMfcHost* host, HWND hwnd, UINT msg,
                                    WPARAM wParam, LPARAM lParam, bool& handled)
{
    handled = false;
    if (host == nullptr)
        return 0;
    const int id = ::GetDlgCtrlID(hwnd);
    auto it = host->m_planCv.find(id);
    if (it == host->m_planCv.end())
        return 0;
    PyMfcHost::PlanCanvasState& st = it->second;

    if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONUP || msg == WM_MOUSEWHEEL)
        debugLogf(L"[plan-mouse] msg=0x%04X pt=(%d,%d) wp=0x%04X drag=%d moved=%d",
                  msg, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam),
                  static_cast<unsigned>(wParam), st.drag ? 1 : 0, st.moved ? 1 : 0);

    switch (msg)
    {
    case WM_MOUSEWHEEL:
    {
        // 滚轮消息带的是屏幕坐标，必须换算成客户区坐标，否则缩放的锚点永远偏
        POINT pt = {};
        pt.x = GET_X_LPARAM(lParam);
        pt.y = GET_Y_LPARAM(lParam);
        ::ScreenToClient(hwnd, &pt);

        const int delta = GET_WHEEL_DELTA_WPARAM(wParam);
        float k = st.zoom;
        const float step = (delta > 0) ? 1.12f : (1.0f / 1.12f);
        float nk = k * step;
        wchar_t surfBuf[16] = {};
        const bool surfOn = (::GetEnvironmentVariableW(L"PYARX_SURFACE", surfBuf, 16) > 0)
                            && surfBuf[0] == L'1';
        nk = surfOn ? ((nk < 0.05f) ? 0.05f : (nk > 4.0f ? 4.0f : nk))
                    : ((nk < 0.2f) ? 0.2f : (nk > 4.0f ? 4.0f : nk));
        if (std::abs(nk - k) < 0.0001f)
        {
            handled = true;
            return 0;
        }
        // 光标下的模型点保持不动：ox' = mx - (mx - ox) * (nk / k)
        const float mx = (static_cast<float>(pt.x) - st.ox) / k;
        const float my = (static_cast<float>(pt.y) - st.oy) / k;
        st.ox = static_cast<int>(lroundf(pt.x - mx * nk));
        st.oy = static_cast<int>(lroundf(pt.y - my * nk));
        st.zoom = nk;
        ::InvalidateRect(hwnd, nullptr, FALSE);
        handled = true;
        return 0;
    }
    case WM_LBUTTONDOWN:
    {
        ::SetFocus(hwnd);
        ::SetCapture(hwnd);
        st.drag = true;
        st.moved = false;
        st.down.x = GET_X_LPARAM(lParam);
        st.down.y = GET_Y_LPARAM(lParam);
        st.downOx = st.ox;
        st.downOy = st.oy;
        ::SetCursor(pyMfcGrabDownCursor());
        handled = true;
        return 0;
    }
    case WM_MOUSEMOVE:
    {
        if (st.drag && ::GetCapture() == hwnd)
        {
            const int x = GET_X_LPARAM(lParam);
            const int y = GET_Y_LPARAM(lParam);
            const int dx = x - st.down.x;
            const int dy = y - st.down.y;
            if (!st.moved && (std::abs(dx) > 3 || std::abs(dy) > 3))
                st.moved = true;
            if (st.moved)
            {
                st.ox = st.downOx + dx;
                st.oy = st.downOy + dy;
                ::SetCursor(pyMfcCanvasCursor(hwnd));
                ::InvalidateRect(hwnd, nullptr, FALSE);
            }
            handled = true;
            return 0;
        }
        // 登记 WM_MOUSELEAVE：鼠标离开画布时必须自己复位成箭头，
        // 否则最后设的手形会一直挂着（宿主不会替子窗口复位光标）。
        TRACKMOUSEEVENT tme = {};
        tme.cbSize = sizeof(tme);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = hwnd;
        ::TrackMouseEvent(&tme);
        ::SetCursor(pyMfcCanvasCursor(hwnd));
        handled = true;
        return 0;
    }
    case WM_MOUSELEAVE:
        ::SetCursor(::LoadCursorW(nullptr, IDC_ARROW));
        handled = true;
        return 0;
    case WM_LBUTTONUP:
    {
        // 顺序很关键：ReleaseCapture 会同步发出 WM_CAPTURECHANGED，
        // 那个分支会把 st.drag 清成 false —— 先 Release 再读，这一击就被当成
        // "从没按下过"，卡片永远选不中。所以先把状态取下来再释放捕获。
        const bool wasDrag = st.drag;
        st.drag = false;
        st.upWasDrag = wasDrag;
        st.lastMsg = WM_LBUTTONUP;
        st.upX = GET_X_LPARAM(lParam);
        st.upY = GET_Y_LPARAM(lParam);
        st.lastHit.clear();
        if (::GetCapture() == hwnd)
            ::ReleaseCapture();
        // 松开立即回到 grab，不必等下一次 WM_SETCURSOR（拖动状态已结束）
        ::SetCursor(pyMfcGrabCursor());
        if (wasDrag && !st.moved)
        {
            // 没拖动 = 单击：命中卡片就选中并上报（Python 侧可取回编号）
            const int x = GET_X_LPARAM(lParam);
            const int y = GET_Y_LPARAM(lParam);
            // 小地图：点哪就把它当模型坐标，主画布居中到该点（视口红框随之更新）
            if (st.mini && st.miniOf >= 0 && st.plan.width > 0 && st.plan.height > 0)
            {
                auto mit = host->m_planCv.find(st.miniOf);
                if (mit != host->m_planCv.end())
                {
                    PyMfcHost::PlanCanvasState& ms = mit->second;
                    RECT mrc = { 0, 0, 1, 1 };
                    ::GetClientRect(hwnd, &mrc);
                    const float w = static_cast<float>(st.plan.width);
                    const float h = static_cast<float>(st.plan.height);
                    const float k = (std::min)((static_cast<float>(mrc.right) - 8.0f) / w,
                                               (static_cast<float>(mrc.bottom) - 8.0f) / h);
                    if (k > 0.0f)
                    {
                        const float ox = (static_cast<float>(mrc.right) - w * k) * 0.5f;
                        const float oy = (static_cast<float>(mrc.bottom) - h * k) * 0.5f;
                        const float mx = (static_cast<float>(x) - ox) / k;
                        const float my = (static_cast<float>(y) - oy) / k;
                        if (HWND mh = ::GetDlgItem(host->m_hWnd, st.miniOf))
                        {
                            RECT mrc2 = { 0, 0, 1, 1 };
                            ::GetClientRect(mh, &mrc2);
                            ms.ox = static_cast<int>(lroundf((mrc2.right - mrc2.left) * 0.5f - mx * ms.zoom));
                            ms.oy = static_cast<int>(lroundf((mrc2.bottom - mrc2.top) * 0.5f - my * ms.zoom));
                            ::InvalidateRect(mh, nullptr, FALSE);
                            ::InvalidateRect(hwnd, nullptr, FALSE);
                        }
                    }
                }
                handled = true;
                return 0;
            }
            const std::wstring code = host->planHit(id, x, y);
            st.lastHit = code;
            if (code != st.sel)
            {
                st.sel = code;
                ::InvalidateRect(hwnd, nullptr, FALSE);
                host->firePlanSelect(id, code);
            }
        }
        handled = true;
        return 0;
    }
    case WM_LBUTTONDBLCLK:
    {
        const int x = GET_X_LPARAM(lParam);
        const int y = GET_Y_LPARAM(lParam);
        const std::wstring code = host->planHit(id, x, y);
        if (!code.empty())
            host->firePlanDoubleClick(id, code);
        handled = true;
        return 0;
    }
    case WM_CAPTURECHANGED:
        st.drag = false;
        handled = true;
        return 0;
    case WM_SETCURSOR:
        // 和网页一致：静止 cursor:grab，按住拖动 cursor:grabbing；
        // 离开画布由 WM_MOUSELEAVE 复位成箭头。
        ::SetCursor(pyMfcCanvasCursor(hwnd));
        handled = true;
        return TRUE;
    default:
        break;
    }
    return 0;
}



