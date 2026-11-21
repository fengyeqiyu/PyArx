// PyMfcHost.Paint.cpp —— 拆分自 PyMfcHost.cpp（按职责分组；接口声明仍在 PyMfcHost.h）。
#include "PyMfcHostInternal.h"

void PyMfcHost::setSurface(int id, int x, int y, int w, int h, int fill)
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

void PyMfcHost::registerPaintNode(int id, int kind, int layer, int x, int y, int w, int h)
{
    XmlPaintNode& n = m_nodes[id];
    n.id = id;
    n.rc = RECT{ x, y, x + w, y + h };
    n.layer = layer;
    // 属性可能在注册之前就写过（XML 先逐个属性下发、后建窗注册节点），这里按宿主的
    // level 表回填一次，保证"先写 z-index 再注册"和"运行期改 z-index"结果一致。
    n.zLevel = levelOf(id);
    n.visible = true;
    n.dirty = true;
    n.kind = kind;
    n.order = static_cast<int>(m_nodeOrder.size());
    if (std::find(m_nodeOrder.begin(), m_nodeOrder.end(), id) == m_nodeOrder.end())
        m_nodeOrder.push_back(id);

    n.hWnd = findByIdDeep(m_hWnd, id);

    const int nid = id;
    switch (kind)
    {
    case 1:  // whiteRect
        n.draw = [this, nid](XmlCanvas& cv, const RECT&) {
            auto it = m_nodes.find(nid);
            if (it != m_nodes.end()) drawNodeWhiteRect(cv, it->second);
        };
        break;
    case 2:  // shadowBox
        n.draw = [this, nid](XmlCanvas& cv, const RECT&) {
            auto it = m_nodes.find(nid);
            if (it != m_nodes.end()) drawNodeShadowBox(cv, it->second);
        };
        break;
    case 3:  // staticLine
        n.draw = [this, nid](XmlCanvas& cv, const RECT&) {
            auto it = m_nodes.find(nid);
            if (it != m_nodes.end()) drawNodeStaticLine(cv, it->second);
        };
        break;
    case 4:  // label
        n.draw = [this, nid](XmlCanvas& cv, const RECT&) {
            auto it = m_nodes.find(nid);
            if (it != m_nodes.end()) drawNodeLabel(cv, it->second);
        };
        break;
    case 5:  // button
        n.draw = [this, nid](XmlCanvas& cv, const RECT&) {
            auto it = m_nodes.find(nid);
            if (it != m_nodes.end()) drawNodeButton(cv, it->second);
        };
        break;
    case 6:  // containerBox（容器背板：div/面板的 background）
        n.draw = [this, nid](XmlCanvas& cv, const RECT&) {
            auto it = m_nodes.find(nid);
            if (it != m_nodes.end()) drawNodeContainer(cv, it->second);
        };
        break;
    default:
        break;
    }
}

void PyMfcHost::updatePaintNodeRect(int id, int x, int y, int w, int h, bool visible)
{
    auto it = m_nodes.find(id);
    if (it == m_nodes.end()) return;
    it->second.rc = RECT{ x, y, x + w, y + h };
    it->second.visible = visible;
    it->second.dirty = true;
}

const PyMfcHost::XmlPaintNode* PyMfcHost::findPaintNode(int id) const
{
    auto it = m_nodes.find(id);
    return it != m_nodes.end() ? &it->second : nullptr;
}

bool PyMfcHost::ancestorTransformOf(HWND hw, xmlgeo::XmlMatrix4& out) const
{
    bool any = false;
    out = xmlgeo::XmlMatrix4();
    HWND w = (hw != nullptr) ? ::GetParent(hw) : nullptr;
    while (w != nullptr && w != m_hWnd)
    {
        const int aid = static_cast<int>(::GetWindowLongPtrW(w, GWLP_ID));
        auto apIt = m_ap.find(aid);
        if (apIt != m_ap.end() && !apIt->second.transform.empty())
        {
            const CRect rc = hostRectOfWindow(w);
            const int bw = rc.Width(), bh = rc.Height();
            xmlgeo::XmlMatrix4 anc;
            if (bw > 0 && bh > 0 &&
                xmlgeo::ParseCssTransform(apIt->second.transform, static_cast<float>(bw),
                                          static_cast<float>(bh), anc) &&
                !anc.IsIdentity2D())
            {
                anc = anc.AboutPoint((rc.left + rc.right) * 0.5f, (rc.top + rc.bottom) * 0.5f);
                out = xmlgeo::XmlMatrix4::Multiply(anc, out);
                any = true;
            }
        }
        w = ::GetParent(w);
    }
    return any;
}

PyMfcHost::NodeXf PyMfcHost::nodeXfOf(const XmlPaintNode& n) const
{
    NodeXf res;
    res.underContainer = ancestorTransformOf(n.hWnd, res.m);
    const xmlapp::XmlAppearance* ap = findAppearance(n.id);
    if (ap == nullptr)
        return res;
    // CSS 状态化 transform：`:hover{transform:scale(1.04)}` 这类"轻微放大/抬起"反馈，
    // 只换绘制变换、不改布局；优先级与状态选色一致（focus → hover）。
    std::wstring xfRaw = ap->transform;
    if (n.hWnd != nullptr && ::GetFocus() == n.hWnd && !ap->focusTransform.empty())
        xfRaw = ap->focusTransform;
    else if (n.hWnd != nullptr && PyMfcHovered(n.hWnd) && !ap->hoverTransform.empty())
        xfRaw = ap->hoverTransform;
    if (xfRaw.empty())
        return res;
    const int bw = static_cast<int>(n.rc.right - n.rc.left);
    const int bh = static_cast<int>(n.rc.bottom - n.rc.top);
    if (bw <= 0 || bh <= 0)
        return res;
    xmlgeo::XmlMatrix4 self;
    if (!xmlgeo::ParseCssTransform(xfRaw, static_cast<float>(bw),
                                  static_cast<float>(bh), self) ||
        self.IsIdentity2D())
        return res;
    self = self.AboutPoint((n.rc.left + n.rc.right) * 0.5f, (n.rc.top + n.rc.bottom) * 0.5f);
    res.m = xmlgeo::XmlMatrix4::Multiply(res.m, self);
    res.self = true;
    return res;
}

void PyMfcHost::drawNodeContainer(XmlCanvas& cv, const XmlPaintNode& n) const
{
    const xmlapp::XmlAppearance* ap = findAppearance(n.id);
    if (ap == nullptr)
        return;
    COLORREF bc = 0;
    const bool hasBack = !ap->backColor.empty() && xmlapp::ParseColor(ap->backColor, bc);
    COLORREF br = 0;
    const bool hasBorder = !ap->borderStyle.empty() &&
                           _wcsicmp(ap->borderStyle.c_str(), L"None") != 0 &&
                           !ap->borderColor.empty() && xmlapp::ParseColor(ap->borderColor, br);
    // 纯渐变背板同样不跳过：xmlDrawBackplate 内渐变分支优先于纯色。
    const bool hasGradient = !ap->bgImage.empty();
    if (!hasBack && !hasBorder && !hasGradient)
        return;
    // 与控件同一套背板实现（圆角/边框/opacity/前置层），fallback 取父表面色
    // 容器背板里也会画 box-shadow，所以状态化投影要先换进副本再交进去。
    xmlapp::XmlAppearance apa = *ap;
    const bool hov = (n.hWnd != nullptr && PyMfcHovered(n.hWnd)) || n.hover;
    apa.boxShadow = xmlapp::ShadowOf(*ap, hov, n.hWnd != nullptr && ::GetFocus() == n.hWnd);
    if (hov)
    {
        if (!ap->hoverBackColor.empty())   apa.backColor   = ap->hoverBackColor;
        if (!ap->hoverBorderColor.empty()) apa.borderColor = ap->hoverBorderColor;
        if (ap->hoverOpacity >= 0)         apa.opacity     = ap->hoverOpacity;
    }
    xmlDrawBackplate(cv, n.rc, apa, surfaceColorAt(CRect(n.rc)), m_border);
}

void PyMfcHost::drawNodeWhiteRect(XmlCanvas& cv, const XmlPaintNode& n) const
{
    COLORREF face = RGB(255, 255, 255);
    const xmlapp::XmlAppearance* ap = findAppearance(n.id);
    if (ap != nullptr && !ap->backColor.empty())
    {
        COLORREF c = 0;
        if (xmlapp::ParseColor(ap->backColor, c))
            face = xmlapp::Blend(c, m_bg, ap->opacity);
    }
    cv.fillRect(n.rc, face);
    // 与旧 SS_WHITERECT | WS_BORDER 对齐：1px 主题边框。
    cv.strokeRect(n.rc, m_border, 1);
}

void PyMfcHost::drawNodeShadowBox(XmlCanvas& cv, const XmlPaintNode& n) const
{
    auto it = m_shadowBox.find(n.id);
    if (it == m_shadowBox.end()) return;
    pymfcskin::drawShadowBox(cv, n.rc, it->second);
}

void PyMfcHost::drawNodeStaticLine(XmlCanvas& cv, const XmlPaintNode& n) const
{
    const RECT& itemRc = n.rc;
    const int wSpan = static_cast<int>(itemRc.right - itemRc.left);
    const int hSpan = static_cast<int>(itemRc.bottom - itemRc.top);
    cv.fillRect(itemRc, surfaceColorAt(CRect(itemRc)));
    if (hSpan > wSpan * 3)
    {
        if (wSpan >= 3)
        {
            cv.fillRect(itemRc, RGB(0xE4, 0xE7, 0xEC));
            const int x = (itemRc.left + itemRc.right) / 2;
            cv.fillRect(RECT{ x, itemRc.top, x + 1, itemRc.bottom }, m_border);
        }
        else
        {
            const int x = (itemRc.left + itemRc.right) / 2;
            cv.fillRect(RECT{ x, itemRc.top, x + 1, itemRc.bottom }, m_border);
        }
    }
    else
    {
        const int y = (itemRc.top + itemRc.bottom) / 2;
        cv.fillRect(RECT{ itemRc.left, y, itemRc.right, y + 1 }, m_border);
    }
}

void PyMfcHost::drawOwnerNode(XmlCanvas& cv, const XmlPaintNode& n, bool isStatic) const
{
    const int cid = n.id;
    const HWND hw = n.hWnd;
    const RECT& itemRc = n.rc;

    CString text;
    if (hw != nullptr)
    {
        ::GetWindowTextW(hw, text.GetBufferSetLength(512), 512);
        text.ReleaseBuffer();
    }

    const xmlapp::XmlAppearance* ap = findAppearance(cid);
    const bool styled = ap != nullptr && !ap->empty();
    const LONG_PTR wst = hw != nullptr ? ::GetWindowLongPtrW(hw, GWL_STYLE) : 0;
    const bool pressed = n.pressed;
    // 禁用态要认窗口真实状态：EnableWindow(FALSE) 会置 WS_DISABLED。
    // 以前这里硬编码 false，HTML `<button disabled>` 的"底变淡 + 字变灰"在 XML 通道里永远不生效。
    const bool disabled = hw != nullptr && (wst & WS_DISABLED) != 0;

    const COLORREF surface = surfaceColorAt(CRect(itemRc));
    auto accentIt = m_accent.find(cid);
    const bool hasAccent = (accentIt != m_accent.end()) && !isStatic;
    const COLORREF accent = hasAccent ? accentIt->second : m_border;

    // 三态选色对齐 OnDrawItem（PyMfcProcs.cpp 2416-2440）：focus 优先于 active/hover，
    // 只在声明了对应状态底色时才换；纯声明 bgImage 渐变的按钮同样要铺背板。
    const bool hovered = n.hover;
    const bool focused = n.focus;
    const std::wstring* backRaw = nullptr;
    if (styled)
    {
        if (focused && !ap->focusBackColor.empty()) backRaw = &ap->focusBackColor;
        else if (pressed && !ap->activeBackColor.empty()) backRaw = &ap->activeBackColor;
        else if (hovered && !ap->hoverBackColor.empty()) backRaw = &ap->hoverBackColor;
        else if (!ap->backColor.empty()) backRaw = &ap->backColor;
    }
    const bool paintBack = styled && (backRaw != nullptr ||
                                      !ap->bgImage.empty() ||
                                      !ap->borderColor.empty() ||
                                      !ap->borderStyle.empty());

    // CSS transform 由 drawPaintNodes 统一推入（含祖先容器链，见 nodeXfOf），
    // 这里只做一次幂等自检：本函数被直接调用（非节点帧）时不能假设外面已经推过。
    debugLogf(L"[xf] id=%d tf='%ls' box=%d,%d,%d,%d", cid,
              ap != nullptr ? ap->transform.c_str() : L"",
              static_cast<int>(itemRc.left), static_cast<int>(itemRc.top),
              static_cast<int>(itemRc.right), static_cast<int>(itemRc.bottom));

    if (paintBack)
    {
        xmlapp::XmlAppearance apDraw = *ap;
        if (backRaw != nullptr)
            apDraw.backColor = *backRaw;
        // CSS :disabled 降对比：底色仍保持原色，只把 opacity 压到 220（对齐 OnDrawItem）。
        if (disabled && apDraw.opacity > 220)
            apDraw.opacity = 220;
        xmlDrawBackplate(cv, itemRc, apDraw, surface, m_border);
    }
    else if (!styled)
    {
        cv.fillRect(itemRc, surface);
        const COLORREF face = isStatic ? surface
                                       : (hasAccent ? RGB(255, 255, 255)
                                                    : (pressed ? RGB(80, 90, 104) : m_btn));
        cv.fillRoundRect(itemRc, face, isStatic ? surface : (hasAccent ? accent : m_border), 8);
    }

    COLORREF back = surface;
    if (styled && backRaw != nullptr)
    {
        COLORREF c = 0;
        if (xmlapp::ParseColor(*backRaw, c))
            back = xmlapp::Blend(c, surface, ap->opacity);
    }
    COLORREF fg = disabled ? RGB(140, 140, 140) : contrastText(back);
    if (styled && !ap->foreColor.empty())
    {
        COLORREF c = 0;
        if (xmlapp::ParseColor(ap->foreColor, c))
            fg = disabled ? RGB(140, 140, 140) : c;
    }
    if (hasAccent)
        fg = disabled ? RGB(140, 140, 140) : accent;
    auto labelIt = m_labelFg.find(cid);
    if (labelIt != m_labelFg.end() && !disabled)
        fg = labelIt->second;

    UINT align = isStatic ? DT_LEFT : DT_CENTER;
    if (styled)
        align = xmlapp::TextFlag(*ap);

    RECT trc = itemRc;
    if (styled && align == DT_LEFT)
        trc.left += 6;

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
                cv.drawImage(ir, *iconPath);
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

    const UINT bsType = static_cast<UINT>(wst) & 0x0F;
    const bool isRadio = (bsType == BS_RADIOBUTTON) || (bsType == BS_AUTORADIOBUTTON);
    const bool isCheckButton = (bsType == BS_CHECKBOX || bsType == BS_AUTOCHECKBOX ||
                                bsType == BS_3STATE || bsType == BS_AUTO3STATE || isRadio);
    if (isCheckButton && hw != nullptr)
    {
        const int boxSide = 16;
        const int h = itemRc.bottom - itemRc.top;
        if (h >= boxSide + 8)
        {
            const int top = itemRc.top + (h - boxSide) / 2;
            RECT box = { itemRc.left + 4, top, itemRc.left + 4 + boxSide, top + boxSide };
            cv.fillRoundRect(box, back, m_border, 3);
            const LRESULT chk = ::SendMessageW(hw, BM_GETCHECK, 0, 0);
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
                cv.fillRoundRect(inner, RGB(0, 120, 212), RGB(0, 120, 212), isRadio ? 7 : 2);
            }
            align = DT_LEFT;
            trc.left = box.right + 6;
            if (trc.left >= trc.right)
                trc.left = itemRc.left + 4;
        }
    }

    const int ptSize = (styled && ap->fontSize > 0) ? ap->fontSize : 0;
    const std::wstring family = (styled && !ap->fontFamily.empty()) ? ap->fontFamily : std::wstring();
    // CSS text-shadow：label/button 的文字在这条宿主自绘路线上（不经 xmlDrawText），
    // 阴影必须在 cv.text 之前交给画布，否则 `text-shadow` 声明等于没写。
    {
        xmlapp::TextShadowSpec ts;
        if (styled && !ap->textShadow.empty() && xmlapp::ParseTextShadow(ap->textShadow, ts))
            cv.setTextShadow(ts.ox, ts.oy, ts.blur, ts.color);
        else
            cv.setTextShadow(0, 0, -1, 0);
    }
    const std::wstring sText(text.GetString(), static_cast<size_t>(text.GetLength()));
    cv.text(trc, sText, fg, align, ptSize, family);

    if (n.focus && !styled)
        cv.strokeRoundRect(itemRc, RGB(0, 120, 212), 8, 1);
}

void PyMfcHost::drawNodeLabel(XmlCanvas& cv, const XmlPaintNode& n) const
{
    drawOwnerNode(cv, n, true);
}

void PyMfcHost::drawNodeButton(XmlCanvas& cv, const XmlPaintNode& n) const
{
    drawOwnerNode(cv, n, false);
}

int PyMfcHost::hitNode(const CPoint& pt) const
{
    for (int id : m_nodeOrder)
    {
        auto it = m_nodes.find(id);
        if (it == m_nodes.end()) continue;
        const XmlPaintNode& n = it->second;
        const int k = n.kind;
        if (k != 4 && k != 5) continue;
        // 容器 transform 后窗口被藏、命中落回宿主：必须按"变换后的四边形"判，
        // 否则点到的位置和看到的图形对不上（旋转 30° 的按钮斜着看得到、正着点不到）。
        const NodeXf xf = nodeXfOf(n);
        if (xf.self || xf.underContainer)
        {
            const xmlgeom::GeoPoly quad = xmlgeom::QuadOf(n.rc, xf.m);
            if (xmlgeom::Contains(quad, static_cast<double>(pt.x), static_cast<double>(pt.y)))
                return id;
            continue;
        }
        if (!n.visible) continue;
        if (PtInRect(&n.rc, pt))
            return id;
    }
    return -1;
}

void PyMfcHost::OnMouseMove(UINT /*nFlags*/, CPoint pt)
{
    // 真光标是否就在客户区内、且确实压在本窗口上：合成 WM_MOUSEMOVE（测试注入/脚本发送）
    // 或窗口被别的窗口盖住时，系统会把登记立刻判成"已离开"，不能让它擦掉刚置上的 hover。
    CPoint curScreen;
    m_realHover = false;
    if (::GetCursorPos(&curScreen) != FALSE)
    {
        CRect rcClient;
        GetClientRect(&rcClient);
        CPoint cur = curScreen;
        ScreenToClient(&cur);
        m_realHover = rcClient.PtInRect(cur) != FALSE &&
                      ::GetAncestor(::WindowFromPoint(curScreen), GA_ROOT) ==
                          ::GetAncestor(m_hWnd, GA_ROOT);
    }
    if (m_realHover && !m_trackLeave)
    {
        // 登记一次 TME_LEAVE：光标甩出窗口时系统才会发 WM_MOUSELEAVE（宿主只在客户区收移动）
        TRACKMOUSEEVENT tme = {};
        tme.cbSize = sizeof(tme);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = m_hWnd;
        m_trackLeave = ::TrackMouseEvent(&tme) != FALSE;
    }
    updateNodeHover(&pt);
}

// 光标离开宿主客户区：:hover 必须失效，否则底色残影（HTML 里不会这样）
void PyMfcHost::OnMouseLeave()
{
    m_trackLeave = false;
    if (!m_realHover) return;      // 合成鼠标消息触发的假离开，不清状态
    updateNodeHover(nullptr);
}

void PyMfcHost::updateNodeHover(const CPoint* pt)
{
    const int hit = pt != nullptr ? hitNode(*pt) : -1;
    // 容器命中态：容器窗口被上层画面窗口盖住、收不到 WM_MOUSEMOVE，只能在这里判。
    // 取"面积最小的包含矩形"当最上层，与绘制顺序一致。
    int bestContainer = -1;
    if (pt != nullptr)
    {
        long bestArea = -1;
        for (int id : m_nodeOrder)
        {
            auto it = m_nodes.find(id);
            if (it == m_nodes.end() || it->second.kind != 6) continue;
            if (!it->second.visible || !PtInRect(&it->second.rc, *pt)) continue;
            const long area = static_cast<long>(it->second.rc.right - it->second.rc.left) *
                              (it->second.rc.bottom - it->second.rc.top);
            if (bestArea < 0 || area < bestArea)
            {
                bestArea = area;
                bestContainer = id;
            }
        }
    }
    for (int id : m_nodeOrder)
    {
        auto it = m_nodes.find(id);
        if (it == m_nodes.end()) continue;
        // kind 5：owner-draw 控件节点；kind 6：容器节点（div 的 :hover 靠这条）
        const int kind = it->second.kind;
        if (kind != 5 && kind != 6) continue;
        const bool want = (kind == 6) ? (id == bestContainer) : (id == hit);
        if (want != it->second.hover)
        {
            it->second.hover = want;
            RECT inv = it->second.rc;
            ::InvalidateRect(m_hWnd, &inv, FALSE);
            // 容器盒内底色由容器自己的 CTLCOLOR 刷子出图，必须连它一起擦
            if (kind == 6)
            {
                HWND ch = findByIdDeep(m_hWnd, id);
                if (ch != nullptr) ::InvalidateRect(ch, nullptr, TRUE);
            }
        }
    }
}

// 容器当前是否处于命中态（CTLCOLOR 刷子与背板都要按这个值取 hover 声明）
bool PyMfcHost::containerHovered(int id) const
{
    auto it = m_nodes.find(id);
    return it != m_nodes.end() && it->second.kind == 6 && it->second.hover;
}

void PyMfcHost::OnLButtonDown(UINT /*nFlags*/, CPoint pt)
{
    const int hit = hitNode(pt);
    if (hit < 0) return;
    auto it = m_nodes.find(hit);
    if (it == m_nodes.end()) return;
    it->second.pressed = true;
    ::SetCapture(m_hWnd);
    ::InvalidateRect(m_hWnd, &it->second.rc, FALSE);
}

void PyMfcHost::OnLButtonUp(UINT /*nFlags*/, CPoint pt)
{
    const int hit = hitNode(pt);
    if (hit < 0) return;
    auto it = m_nodes.find(hit);
    if (it == m_nodes.end()) return;
    const bool wasPressed = it->second.pressed;
    it->second.pressed = false;
    ::ReleaseCapture();
    ::InvalidateRect(m_hWnd, &it->second.rc, FALSE);
    if (wasPressed)
    {
        // 与 BN_CLICKED 同一入口：直接走宿主命令分发，Python m_click 回调同参同时序。
        OnCommandRange(static_cast<UINT>(hit));
        it->second.focus = true;
    }
}

void PyMfcHost::drawPaintNodes(XmlCanvas& cv)
{
    std::vector<XmlPaintNode*> ordered;
    ordered.reserve(m_nodes.size());
    for (int id : m_nodeOrder)
    {
        auto it = m_nodes.find(id);
        if (it == m_nodes.end()) continue;
        // 被容器 transform 接管（容器 HWND 已藏）的子树：窗口不可见，但节点必须照画。
        if (!it->second.visible)
        {
            const NodeXf probe = nodeXfOf(it->second);
            if (!probe.underContainer) continue;
        }
        ordered.push_back(&it->second);
    }
    std::stable_sort(ordered.begin(), ordered.end(),
                     [](const XmlPaintNode* a, const XmlPaintNode* b) {
                         if (a->layer != b->layer) return a->layer < b->layer;
                         // 层内先按 CSS z-index，再落回注册次序（= HTML 的同层叠上下文）
                         if (a->zLevel != b->zLevel) return a->zLevel < b->zLevel;
                         return a->order < b->order;
                     });

    // 底板层：m_surfaces 装饰矩形（旧路径在 OnEraseBkgnd 铺底，表面路径由节点帧承担）。
    for (int sid : m_surfaceOrder)
    {
        auto sit = m_surfaces.find(sid);
        if (sit == m_surfaces.end() || !sit->second.visible) continue;
        cv.fillRect(sit->second.rc, sit->second.fill);
    }

    // 归一到宿主客户区坐标：子控件被 SetParent 到容器后，布局/建窗给的是**容器内相对坐标**，
    // 而节点帧是整窗一张画布、只认宿主绝对坐标。不归一就会出现"三张卡片的文字全叠在页面左上角、
    // 被各自 24px 的 clip 切成一条窄带"（用户看到的"文字都被覆盖了"）。既然控件已经有 HWND，
    // 画布就应该跟着窗口真实几何走 —— 这也是 HTML 里"绘制跟随元素盒子"的语义。
    // kind==2（阴影盒）例外：它的 rc 故意比控件外扩 pad（盒外投影），不能覆盖成窗口矩形。
    for (XmlPaintNode* n : ordered)
    {
        if (n->kind == 2 || n->hWnd == nullptr || !::IsWindow(n->hWnd))
            continue;
        const CRect rc = hostRectOfWindow(n->hWnd);
        n->rc = RECT{ rc.left, rc.top, rc.right, rc.bottom };
    }

    // CSS transform 只能走"节点单通路"：WM_DRAWITEM 画不了旋转/缩放，留着就会在变换后的
    // 图形上再叠一份没变换的文字。对外观下发那一刻不可靠（那时窗口还没 Show），所以放在每帧。
    // 容器（kind 6 的空 STATIC）同样适用：藏掉容器会连同它的子窗口一起不可见，
    // 整棵子树就只剩节点帧一条通路 —— 这是 HTML "transform 作用于整个子树"的对应实现。
    for (XmlPaintNode* n : ordered)
    {
        if (n->hWnd == nullptr || !::IsWindow(n->hWnd))
            continue;
        const xmlapp::XmlAppearance* ap = findAppearance(n->id);
        // 状态化 transform 同样需要走节点单通路（hover 时才藏窗口，离开即还原）
        bool hasTf = ap != nullptr && !ap->transform.empty();
        if (ap != nullptr && n->hWnd != nullptr)
        {
            if (!ap->hoverTransform.empty() && PyMfcHovered(n->hWnd))
                hasTf = true;
            else if (!ap->focusTransform.empty() && ::GetFocus() == n->hWnd)
                hasTf = true;
        }
        if (hasTf)
        {
            if (::IsWindowVisible(n->hWnd))
            {
                ::SetWindowPos(n->hWnd, nullptr, 0, 0, 0, 0,
                               SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_HIDEWINDOW);
                m_xfHidden.insert(n->id);
            }
            n->visible = true;
        }
        else if (m_xfHidden.count(n->id) != 0)
        {
            // 只还原"自己藏过的"：标签/分割线本来就隐藏 + 节点绘制，
            // 凭 IsWindowVisible 反向判断会把它们放出来，同一段文字画两遍。
            ::SetWindowPos(n->hWnd, nullptr, 0, 0, 0, 0,
                           SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);
            m_xfHidden.erase(n->id);
        }
    }

    // 调试：PYARX_NODE_DUMP=1 时把本帧全部绘制节点的几何打进日志，排查"控件有 HWND 但屏幕上是空的/
    // 文字被别的东西盖住"这类问题（坐标到底是节点错还是窗口错，一眼就能分开）。
    static const bool dumpNodes = [] {
        char buf[8]{};
        return ::GetEnvironmentVariableA("PYARX_NODE_DUMP", buf, sizeof(buf)) > 0;
    }();
    if (dumpNodes)
    {
        for (const XmlPaintNode* n : ordered)
            debugLogf(L"[node] id=%d kind=%d layer=%d z=%d rc=%d,%d,%d,%d vis=%d hw=%p par=%p isw=%d",
                      n->id, n->kind, n->layer, n->zLevel,
                      static_cast<int>(n->rc.left), static_cast<int>(n->rc.top),
                      static_cast<int>(n->rc.right), static_cast<int>(n->rc.bottom),
                      n->visible ? 1 : 0, static_cast<void*>(n->hWnd),
                      static_cast<void*>(n->hWnd != nullptr ? ::GetParent(n->hWnd) : nullptr),
                      (n->hWnd != nullptr && ::IsWindow(n->hWnd)) ? 1 : 0);
    }

    for (XmlPaintNode* n : ordered)
    {
        const xmlapp::XmlAppearance* apn = findAppearance(n->id);
        const bool hasOutline = apn != nullptr && apn->outlineWidth > 0;
        // 状态化 box-shadow：常态浅投影、hover/focus 加深（HTML "抬起"写法）
        const bool nHover = n->hWnd != nullptr && PyMfcHovered(n->hWnd);
        const bool nFocus = n->hWnd != nullptr && ::GetFocus() == n->hWnd;
        const bool hasShadow = apn != nullptr &&
                               !xmlapp::ShadowOf(*apn, nHover, nFocus).empty();
        // 容器节点（div/面板）没有 draw 回调，但 outline / box-shadow 仍然要能画 ——
        // 否则 `<div style="outline:2px solid #0af">`、`box-shadow` 在 XML 侧依旧做不到。
        if (n->draw == nullptr && !hasOutline && !hasShadow) continue;
        // box-shadow 全落在盒子外侧（盒下压暗带），必须在控件自己的窗口画板之外落笔：
        // 先画影子，再让节点/控件盖上盒子本体，顺序与 CSS 一致（影子在盒子下层）。
        if (hasShadow)
            xmlDrawBoxShadow(cv, n->rc, xmlapp::ShadowOf(*apn, nHover, nFocus));
        // 累计变换 = 祖先容器链 × 自身（见 nodeXfOf）。有变换时裁剪放开到宿主客户区：
        // CSS 的 transform 不裁剪自己，按盒裁会把放大/旋转出去的部分切掉。
        const NodeXf xf = nodeXfOf(*n);
        const bool tf = xf.self || xf.underContainer;
        RECT clip = n->rc;
        if (tf)
        {
            CRect clientRc;
            ::GetClientRect(m_hWnd, &clientRc);
            clip = RECT{ clientRc.left, clientRc.top, clientRc.right, clientRc.bottom };
        }
        if (n->draw != nullptr)
        {
            // 顺序很重要：裁剪框必须在**变换前**压栈。D2D 的轴对齐裁剪会按当前 transform
            // 解释坐标，先推变换再压裁剪 = 交付一个被旋转过的斜四边形裁剪区，内容会被切掉大半。
            // CSS overflow:hidden 的祖先容器链：绘制层按容器矩形求交（子窗口裁剪只管窗口，
            // 宿主画布上的节点绘制不受它约束，所以必须在 clip 里显式收进来）。
            if (!tf)
            {
                HWND p = (n->hWnd != nullptr && ::IsWindow(n->hWnd)) ? ::GetParent(n->hWnd) : nullptr;
                while (p != nullptr && p != m_hWnd && ::IsWindow(p))
                {
                    const xmlapp::XmlAppearance* cap = findAppearance(::GetDlgCtrlID(p));
                    if (cap != nullptr &&
                        (::_wcsicmp(cap->overflow.c_str(), L"hidden") == 0 ||
                         ::_wcsicmp(cap->overflow.c_str(), L"auto") == 0 ||
                         ::_wcsicmp(cap->overflow.c_str(), L"scroll") == 0))
                    {
                        RECT wr = {};
                        ::GetWindowRect(p, &wr);
                        ::MapWindowPoints(HWND_DESKTOP, m_hWnd,
                                          reinterpret_cast<LPPOINT>(&wr), 2);
                        clip.left = (std::max)(clip.left, wr.left);
                        clip.top = (std::max)(clip.top, wr.top);
                        clip.right = (std::min)(clip.right, wr.right);
                        clip.bottom = (std::min)(clip.bottom, wr.bottom);
                    }
                    p = ::GetParent(p);
                }
            }
            cv.clipPush(clip);
            if (tf)
                cv.pushTransform4(xf.m);
            n->draw(cv, n->rc);
            if (tf)
                cv.popTransform();
            cv.clipPop();
        }
        // CSS outline 必须画在宿主画布上：它全落在盒子外侧，盒子裁剪（上面那层 clip）和
        // 控件自己的子窗口都会把它切掉。所以单开一层"外扩裁剪"落笔。
        if (hasOutline && !tf)
        {
            const int pad = (std::max)(0, apn->outlineOffset) + apn->outlineWidth + 1;
            RECT oe = n->rc;
            ::InflateRect(&oe, pad, pad);
            cv.clipPush(oe);
            xmlDrawOutline(cv, n->rc, *apn);
            cv.clipPop();
        }
    }
}

void PyMfcHost::drawRibbon(XmlCanvas& cv, int id, const RECT& rc)
{
    auto it = m_ribbon.find(id);
    if (it == m_ribbon.end()) return;
    RibbonState& st = it->second;
    const int tabH = 34;
    RECT tabRc  = { rc.left, rc.top, rc.right, rc.top + tabH };
    RECT bodyRc = { rc.left, rc.top + tabH, rc.right, rc.bottom };
    xmlctl::XRibbonMetrics mx;
    mx.bodyH = bodyRc.bottom - bodyRc.top;
    mx.titleTop = (std::max)(36, mx.bodyH - 24);
    xmlctl::ribbonLayout(st.model, mx, tabRc, bodyRc, st.lay);
    xmlctl::ribbonDraw(cv, tabRc, bodyRc, st.model, mx, st.lay,
                        st.hotTab, st.hotBtn, st.pressedBtn, 10, 9);
}

void PyMfcHost::drawFileTab(XmlCanvas& cv, int id, const RECT& rc)
{
    auto it = m_fileTabs.find(id);
    if (it == m_fileTabs.end()) return;
    FileTabState& st = it->second;
    xmlctl::FileTabMetrics mx;
    xmlctl::filetabLayout(st.model, mx, rc, cv, st.lay, st.scrollX);
    xmlctl::filetabDraw(cv, rc, st.model, mx, st.lay,
                        st.hotTab, st.hotClose, st.hotAdd, st.pressed, mx.pt);
}

void PyMfcHost::drawView3D(XmlCanvas& cv, int id, const RECT& rc)
{
    auto it = m_view3D.find(id);
    if (it == m_view3D.end())
        return;
    View3DState& st = it->second;
    cv.fillRect(rc, st.bg);
    const float w = static_cast<float>(rc.right - rc.left);
    const float h = static_cast<float>(rc.bottom - rc.top);
    cv.text(rc, L"", RGB(0, 0, 0), DT_CENTER, 10, L"Microsoft YaHei");
    if (st.empty || w < 8.0f || h < 8.0f)
    {
        cv.text(rc, L"（三维画布：等待模型）", RGB(0x8A, 0x94, 0xA6), DT_CENTER, 10, L"Microsoft YaHei");
        return;
    }

    const float cxw = (st.lo[0] + st.hi[0]) * 0.5f;
    const float cyw = (st.lo[1] + st.hi[1]) * 0.5f;
    const float czw = (st.lo[2] + st.hi[2]) * 0.5f;
    const float dx = st.hi[0] - st.lo[0], dy = st.hi[1] - st.lo[1], dz = st.hi[2] - st.lo[2];
    const float radius = 0.5f * std::sqrt(dx * dx + dy * dy + dz * dz);
    if (radius <= 0.0f) return;

    const float yaw = st.yaw * 3.14159265f / 180.0f;
    const float pit = st.pitch * 3.14159265f / 180.0f;
    const float cyaw = std::cos(yaw), syaw = std::sin(yaw);
    const float cpit = std::cos(pit), spit = std::sin(pit);
    const float dist = radius * 3.0f;
    // 缩放系数按"旋转后包围盒的投影半宽/半高"算：用 min(w,h) 会让长条形模型只占半个屏幕。
    float maxRx = 1e-6f, maxRz = 1e-6f;
    for (int i = 0; i < 8; ++i)
    {
        const float vx = ((i & 1) ? st.hi[0] : st.lo[0]) - cxw;
        const float vy = ((i & 2) ? st.hi[1] : st.lo[1]) - cyw;
        const float vz = ((i & 4) ? st.hi[2] : st.lo[2]) - czw;
        const float rx = vx * cyaw - vy * syaw;
        const float ry0 = vx * syaw + vy * cyaw;
        const float rz = ry0 * spit + vz * cpit;
        maxRx = (std::max)(maxRx, std::fabs(rx));
        maxRz = (std::max)(maxRz, std::fabs(rz));
    }
    const float kx = w * 0.46f / maxRx;
    const float ky = h * 0.46f / maxRz;
    const float k = (std::min)(kx, ky) * st.zoom;
    const float scx = w * 0.5f + static_cast<float>(st.panX);
    const float scy = h * 0.5f + static_cast<float>(st.panY);

    auto project = [&](float x, float y, float z, POINT& outPt, float& depth) {
        const float vx = x - cxw, vy = y - cyw, vz = z - czw;
        const float rx = vx * cyaw - vy * syaw;
        const float ry0 = vx * syaw + vy * cyaw;
        const float ry = ry0 * cpit - vz * spit;
        const float rz = ry0 * spit + vz * cpit;
        float f = dist / (dist + ry);
        if (f < 0.05f) f = 0.05f;
        if (f > 6.0f) f = 6.0f;
        outPt.x = static_cast<LONG>(scx + rx * k * f);
        outPt.y = static_cast<LONG>(scy - rz * k * f);
        depth = ry;
    };

    struct V3Tri { POINT p[3]; float depth; COLORREF col; };
    std::vector<V3Tri> tris;
    tris.reserve(st.prims.size() * 12);

    auto pushTri = [&](float ax, float ay, float az, float bx, float by, float bz,
                       float cx2, float cy2, float cz2, COLORREF col, float nx, float ny, float nz) {
        // 法向按同一套旋转转到视空间，用来做朗伯明暗
        const float rx = nx * cyaw - ny * syaw;
        const float ry0 = nx * syaw + ny * cyaw;
        const float ry = ry0 * cpit - nz * spit;
        const float rz = ry0 * spit + nz * cpit;
        const float lit = 0.42f + 0.58f * std::fabs(rz * 0.72f + rx * 0.35f + ry * 0.60f);
        V3Tri t;
        float d1, d2, d3;
        project(ax, ay, az, t.p[0], d1);
        project(bx, by, bz, t.p[1], d2);
        project(cx2, cy2, cz2, t.p[2], d3);
        t.depth = (d1 + d2 + d3) / 3.0f;
        t.col = v3Shade(col, lit);
        tris.push_back(t);
    };

    auto pushQuad = [&](const float* v0, const float* v1, const float* v2, const float* v3,
                        COLORREF col, float nx, float ny, float nz) {
        pushTri(v0[0], v0[1], v0[2], v1[0], v1[1], v1[2], v2[0], v2[1], v2[2], col, nx, ny, nz);
        pushTri(v0[0], v0[1], v0[2], v2[0], v2[1], v2[2], v3[0], v3[1], v3[2], col, nx, ny, nz);
    };

    std::vector<std::pair<std::vector<POINT>, std::pair<COLORREF, float>>> lines;
    for (const V3Prim& pr : st.prims)
    {
        if (pr.kind == 1)
        {
            POINT a, b;
            float da, db;
            project(pr.p[0], pr.p[1], pr.p[2], a, da);
            project(pr.p[3], pr.p[4], pr.p[5], b, db);
            std::vector<POINT> pts = { a, b };
            lines.push_back({ pts, { pr.color, pr.width } });
            continue;
        }
        if (pr.kind == 2)
        {
            pushTri(pr.p[0], pr.p[1], pr.p[2], pr.p[3], pr.p[4], pr.p[5],
                    pr.p[6], pr.p[7], pr.p[8], pr.color, 0.0f, 0.0f, 1.0f);
            continue;
        }
        // box：6 个面
        const float x0 = pr.p[0], y0 = pr.p[1], z0 = pr.p[2];
        const float x1 = pr.p[3], y1 = pr.p[4], z1 = pr.p[5];
        const float v000[3] = { x0, y0, z0 }, v100[3] = { x1, y0, z0 };
        const float v110[3] = { x1, y1, z0 }, v010[3] = { x0, y1, z0 };
        const float v001[3] = { x0, y0, z1 }, v101[3] = { x1, y0, z1 };
        const float v111[3] = { x1, y1, z1 }, v011[3] = { x0, y1, z1 };
        pushQuad(v001, v101, v111, v011, pr.color, 0.0f, 0.0f, 1.0f);    // 顶
        pushQuad(v000, v100, v110, v010, pr.color, 0.0f, 0.0f, -1.0f);   // 底
        pushQuad(v010, v110, v111, v011, pr.color, 0.0f, 1.0f, 0.0f);    // +Y
        pushQuad(v000, v100, v101, v001, pr.color, 0.0f, -1.0f, 0.0f);   // -Y
        pushQuad(v100, v110, v111, v101, pr.color, 1.0f, 0.0f, 0.0f);    // +X
        pushQuad(v000, v010, v011, v001, pr.color, -1.0f, 0.0f, 0.0f);   // -X
    }

    // 画家算法：远的先画
    std::sort(tris.begin(), tris.end(), [](const V3Tri& a, const V3Tri& b) {
        return a.depth > b.depth;
    });
    for (const V3Tri& t : tris)
    {
        std::vector<POINT> pts = { t.p[0], t.p[1], t.p[2] };
        cv.fillPath(pts, t.col);
    }
    for (const auto& ln : lines)
        cv.strokePath(ln.first, ln.second.first,
                      (std::max)(1, static_cast<int>(ln.second.second)), false);

    // 坐标轴三角标（左下角）
    if (st.axes)
    {
        const float ax = 12.0f;
        const float ay = h - 58.0f;
        const float len = 42.0f;
        const struct { float dx, dy, dz; COLORREF c; const wchar_t* lab; } kAxes[3] = {
            { 1, 0, 0, RGB(0xC0, 0x3A, 0x2B), L"X" },
            { 0, 1, 0, RGB(0x27, 0x8A, 0x45), L"Y" },
            { 0, 0, 1, RGB(0x1D, 0x5F, 0xB4), L"Z" } };
        for (const auto& a : kAxes)
        {
            const float vx = a.dx, vy = a.dy, vz = a.dz;
            const float rx = vx * cyaw - vy * syaw;
            const float ry0 = vx * syaw + vy * cyaw;
            const float ry = ry0 * cpit - vz * spit;
            const float rz = ry0 * spit + vz * cpit;
            POINT p0 = { static_cast<LONG>(ax), static_cast<LONG>(ay) };
            POINT p1 = { static_cast<LONG>(ax + rx * len), static_cast<LONG>(ay - rz * len) };
            cv.strokeLine(p0, p1, a.c, 2);
            RECT tr = { p1.x + 3, p1.y - 8, p1.x + 24, p1.y + 10 };
            cv.text(tr, a.lab, a.c, DT_LEFT, 9, L"Microsoft YaHei");
        }
        wchar_t hud[128] = {};
        swprintf_s(hud, L"yaw %.0f°  pitch %.0f°  zoom %.2f   图元 %d",
                   st.yaw, st.pitch, st.zoom, static_cast<int>(st.prims.size()));
        RECT hr = { 10, 6, static_cast<LONG>(w) - 10, 26 };
        cv.text(hr, hud, RGB(0x4F, 0x6F, 0x8A), DT_LEFT, 9, L"Microsoft YaHei");
    }
}

void PyMfcHost::drawDwgCanvas(XmlCanvas& cv, int id, const RECT& rc)
{
        auto it = m_dwgCv.find(id);
        if (it == m_dwgCv.end())
            return;
        DwgCanvasState& st = it->second;
        // SVG 排版面板走深色主题（自带全屏底色矩形），铺深底；plan 卡片画布/空画布用浅底
        cv.fillRect(rc, st.mode == 2 ? RGB(0x26, 0x29, 0x2D) : RGB(0xF3, 0xF5, 0xF8));
        // OpenGL 画布后端（PYARX_CANVAS_GL=1）：属于阶段一，先验证 WGL 上下文与呈现链路，
        // 矢量批处理（卡片/连线/字形图集）在此之后接入；失败自动落回下面的 GDI 路径。
        // OpenGL 画布后端（阶段二）尚未完成：字形重影、与 owner-draw 双缓冲互相覆盖、
        // 视口未按客户区取值，会导致闪烁与内容缺失。先不接管，保持 GDI 路径可用。
        // 完成字形 alpha 重建 + GL 独占画布清屏 + 客户区 viewport 后再打开下面这段。
#if 0
        if (pymfcgl::enabled() && st.mode == 1 && !st.mini)
        {
            HWND gh = findByIdDeep(m_hWnd, id);
            const int w = rc.right, h = rc.bottom;
            if (pymfcgl::drawDwg(gh, st.plan, st.zoom, st.ox, st.oy, st.sel, st.showCable, w, h))
                return;
        }
#endif
        if (st.mode == 0)
        {
            // 空画布：给一句提示，免得看起来像"没画出来"
            cv.setTextStyle(L"Normal", L"Normal", L"NoWrap", L"Center");
            cv.text(rc, L"（绘图画布（DWG 为主，兼容已算坐标的布局输入）：等待数据）", RGB(0x8A, 0x94, 0xA6), DT_CENTER, 10, L"Microsoft YaHei");
            return;
        }
        if (st.mode == 2)
        {
            xmlsvg::drawSvgScene(cv, st.svg, st.zoom, st.ox, st.oy);
            // 叠加层与底图同一坐标系、同一视图参数，画在最上面
            if (!st.ovl.shapes.empty() || !st.ovl.texts.empty())
                xmlsvg::drawSvgScene(cv, st.ovl, st.zoom, st.ox, st.oy);
            return;
        }
        if (st.mini)
        {
            // 与 HTML 的 drawMinimap 一致：底色 #FBFCFE + 1px #E4E7EC 边框，
            // 卡片画成实心 #CFE0F3 小块（不画文字/纸面），最后叠红色视口框
            cv.fillRect(rc, RGB(0xFB, 0xFC, 0xFE));
            cv.strokeRect(rc, RGB(0xE4, 0xE7, 0xEC), 1);
            const float w = static_cast<float>(st.plan.width);
            const float h = static_cast<float>(st.plan.height);
            if (w <= 0.0f || h <= 0.0f) return;
            const float pad = 4.0f;
            const float k = (std::min)((static_cast<float>(rc.right) - pad * 2) / w,
                                       (static_cast<float>(rc.bottom) - pad * 2) / h);
            if (k <= 0.0f) return;
            const float ox = (static_cast<float>(rc.right) - w * k) * 0.5f;
            const float oy = (static_cast<float>(rc.bottom) - h * k) * 0.5f;
            for (const xmlplan::PowerBox& b : st.plan.boxes)
            {
                const int x0 = static_cast<int>(lroundf(ox + b.x * k));
                const int y0 = static_cast<int>(lroundf(oy + b.y * k));
                const int x1 = static_cast<int>(lroundf(ox + (b.x + b.w) * k));
                const int y1 = static_cast<int>(lroundf(oy + (b.y + b.h) * k));
                RECT br = { x0, y0, (x1 > x0 + 1 ? x1 : x0 + 1), (y1 > y0 + 1 ? y1 : y0 + 1) };
                cv.fillRect(br, RGB(0xCF, 0xE0, 0xF3));
            }

            auto main = m_dwgCv.find(st.miniOf);
            if (main != m_dwgCv.end() && main->second.mode == 1)
            {
                const DwgCanvasState& ms = main->second;
                RECT mrc = { 0, 0, 1, 1 };
                if (HWND mh = ::GetDlgItem(m_hWnd, st.miniOf))
                    ::GetClientRect(mh, &mrc);
                const float vx1 = (-ms.ox) / (ms.zoom > 0 ? ms.zoom : 1.0f);
                const float vy1 = (-ms.oy) / (ms.zoom > 0 ? ms.zoom : 1.0f);
                const float vx2 = (static_cast<float>(mrc.right) - ms.ox) / (ms.zoom > 0 ? ms.zoom : 1.0f);
                const float vy2 = (static_cast<float>(mrc.bottom) - ms.oy) / (ms.zoom > 0 ? ms.zoom : 1.0f);
                RECT vr = { static_cast<int>(lroundf(ox + vx1 * k)), static_cast<int>(lroundf(oy + vy1 * k)),
                            static_cast<int>(lroundf(ox + vx2 * k)), static_cast<int>(lroundf(oy + vy2 * k)) };
                if (vr.right > vr.left && vr.bottom > vr.top)
                {
                    ::IntersectRect(&vr, &vr, &rc);
                    cv.strokeRect(vr, RGB(0xE1, 0x57, 0x59), 1);
                }
            }
            return;
        }
        drawDwgPlanContent(cv, st.plan, st.zoom, st.ox, st.oy, st.sel, st.showCable, &rc);

        if (st.detailVisible)
        {
            pymfcskin::ShadowBoxStyle sb;
            sb.pad = 14;
            sb.radius = 6;
            sb.dx = 0;
            sb.dy = 4;
            sb.blur = 12;
            sb.alpha = 26;
            sb.shadow = RGB(16, 24, 40);
            sb.bg = RGB(255, 255, 255);
            sb.back = RGB(255, 255, 255);
            sb.border = RGB(208, 213, 221);
            sb.borderW = 1;
            RECT outer = st.detailRc;
            ::InflateRect(&outer, sb.pad, sb.pad);
            pymfcskin::drawShadowBox(cv, outer, sb);
            for (const DetailText& dt : st.detailTexts)
            {
                cv.setTextStyle(dt.bold ? L"SemiBold" : L"Normal", L"Normal", L"NoWrap", L"Left");
                cv.text(dt.rc, dt.text, dt.color, DT_LEFT | DT_VCENTER | DT_SINGLELINE,
                        dt.size, L"Microsoft YaHei");
            }
        }
    }

void PyMfcHost::repaintCanvas(int id)
{
        RECT rc = {0, 0, 1, 1};
        if (HWND h = findByIdDeep(m_hWnd, id))
        {
            ::GetClientRect(h, &rc);
            ::InvalidateRect(h, nullptr, TRUE);
        }
        else
        {
            ::InvalidateRect(m_hWnd, nullptr, TRUE);
        }
        if (rc.right <= 0 || rc.bottom <= 0)
            rc = RECT{0, 0, 1, 1};
        const auto it = m_cvCmd.find(id);
        m_cvItems[id] = pymfcskin::qcParse(it != m_cvCmd.end() ? it->second : std::wstring(), rc);
        // 重解析后下标可能整体变了（删过图元），选中集合夹到合法范围
        auto& sel = m_cvSel[id];
        const int n = static_cast<int>(m_cvItems[id].size());
        for (auto i = sel.begin(); i != sel.end();)
        {
            if (*i < 0 || *i >= n)
                i = sel.erase(i);
            else
                ++i;
        }
    }

void PyMfcHost::applyNativeThemeColors(HWND h, COLORREF bg, COLORREF fg)
{
        if (h == nullptr)
            return;
        wchar_t cls[64] = {};
        ::GetClassNameW(h, cls, 64);
        if (_wcsicmp(cls, L"SysListView32") == 0)
        {
            ::SendMessageW(h, LVM_SETBKCOLOR, 0, static_cast<LPARAM>(bg));
            ::SendMessageW(h, LVM_SETTEXTCOLOR, 0, static_cast<LPARAM>(fg));
            ::SendMessageW(h, LVM_SETTEXTBKCOLOR, 0, static_cast<LPARAM>(bg));
        }
        else if (_wcsicmp(cls, L"SysTreeView32") == 0)
        {
            ::SendMessageW(h, TVM_SETBKCOLOR, 0, static_cast<LPARAM>(bg));
            ::SendMessageW(h, TVM_SETTEXTCOLOR, 0, static_cast<LPARAM>(fg));
        }
        else if (_wcsicmp(cls, L"Edit") == 0 || _wcsicmp(cls, L"RICHEDIT50W") == 0)
        {
            // EM_SETBKGNDCOLOR：主题化 Edit 只认这条，WM_CTLCOLOR 的刷子会被忽略
            ::SendMessageW(h, EM_SETBKGNDCOLOR, 0, static_cast<LPARAM>(bg));
        }
        else if (_wcsicmp(cls, L"SysMonthCal32") == 0)
        {
            ::SendMessageW(h, MCM_SETCOLOR, MCSC_BACKGROUND, static_cast<LPARAM>(bg));
            ::SendMessageW(h, MCM_SETCOLOR, MCSC_MONTHBK, static_cast<LPARAM>(bg));
            ::SendMessageW(h, MCM_SETCOLOR, MCSC_TEXT, static_cast<LPARAM>(fg));
            ::SendMessageW(h, MCM_SETCOLOR, MCSC_TITLEBK, static_cast<LPARAM>(m_btn));
            ::SendMessageW(h, MCM_SETCOLOR, MCSC_TITLETEXT, static_cast<LPARAM>(fg));
            ::SendMessageW(h, MCM_SETCOLOR, MCSC_TRAILINGTEXT, static_cast<LPARAM>(fg));
        }
        else if (_wcsicmp(cls, L"msctls_progress32") == 0)
        {
            ::SendMessageW(h, PBM_SETBKCOLOR, 0, static_cast<LPARAM>(bg));
            ::SendMessageW(h, PBM_SETBARCOLOR, 0, static_cast<LPARAM>(RGB(0, 120, 212)));
        }
    }

void PyMfcHost::drawContainerBackdrops(HDC hdc, const CRect& client) const
{
    // 容器（div）的 Background 由宿主在窗口 DC 上出图：容器本身是空 STATIC，自己不画，
    // 而它的窗口区域又被 WS_CLIPCHILDREN 从父 DC 里裁掉 —— 所以必须画在"父 DC 上、
    // 子窗口之下"，这正是 OnEraseBkgnd/OnPaint 里 m_backdrops 那段的位置。
    debugLogf(L"[bd-draw] count=%d client=%d,%d", static_cast<int>(m_containerBackdrops.size()),
              client.right, client.bottom);
    for (const auto& kv : m_containerBackdrops)
    {
        CRect rc = kv.second.first;
        CRect clipped;
        clipped.IntersectRect(rc, client);
        if (clipped.IsRectEmpty())
            continue;
        const xmlapp::XmlAppearance& ap = kv.second.second;
        if (ap.empty())
            continue;
        if (ap.backColor.empty() && ap.borderColor.empty() && ap.borderStyle.empty())
            continue;   // 完全透明的 div：不画，露出页面底色
        ::SaveDC(hdc);
        ::IntersectClipRect(hdc, clipped.left, clipped.top, clipped.right, clipped.bottom);
        xmlapp::DrawBackplate(hdc, rc, ap, m_hasPageBg ? m_pageBg : m_bg, m_border);
        ::RestoreDC(hdc, -1);
    }
}

COLORREF PyMfcHost::surfaceColorAt(const CRect& rc) const
{
        const COLORREF pageBack = m_hasPageBg ? m_pageBg : m_bg;
        const CPoint pt((rc.left + rc.right) / 2, (rc.top + rc.bottom) / 2);
        long bestArea = 0x7FFFFFFF;
        COLORREF best = pageBack;
        for (const auto& bd : m_backdrops)
        {
            if (!bd.first.PtInRect(pt))
                continue;
            const long area = static_cast<long>(bd.first.Width()) * bd.first.Height();
            if (area >= bestArea)
                continue;
            COLORREF c = 0;
            if (!xmlapp::ParseColor(bd.second.backColor, c))
                continue;
            best = xmlapp::Blend(c, pageBack, bd.second.opacity);
            bestArea = area;
        }
        return best;
    }
