#include "stdafx.h"
#include "DesignerPaneView.h"
#include "DesignerPaneInternal.h"
#include <algorithm>
#include <unordered_map>

using namespace uidesign;
using namespace uidesign::detail;

// 临时诊断：右键菜单是否弹出、选中了哪一项（PYARX_DESIGN_DEBUG=1 时写日志）
static void menuDiag(const wchar_t* fmt, ...)
{
    if (::GetEnvironmentVariableW(L"PYARX_DESIGN_DEBUG", nullptr, 0) == 0) return;
    FILE* f = nullptr;
    if (_wfopen_s(&f, L"G:\\PyArx\\log\\menu_diag.log", L"a, ccs=UTF-8") != 0 || f == nullptr) return;
    va_list ap;
    va_start(ap, fmt);
    vfwprintf(f, fmt, ap);
    va_end(ap);
    fputws(L"\n", f);
    fclose(f);
}

// ===========================================================================
// 单表面模式：命中测试与交互
// 没有子窗口就没有系统控件帮忙处理鼠标，全部命中都靠上一帧留下的矩形表。
// ===========================================================================
DesignerPaneView::SurfaceHit DesignerPaneView::surfaceHitTest(CPoint pt) const
{
    SurfaceHit h;
    const int rowH = 24;

    // 菜单条在窗口最上方，优先于工具栏
    for (size_t i = 0; i < m_surfMenuRects.size(); ++i)
    {
        const RECT& r = m_surfMenuRects[i];
        if (pt.x >= r.left && pt.x < r.right && pt.y >= r.top && pt.y < r.bottom)
        {
            h.kind = SurfaceHit::Menu;
            h.index = (int)i;
            return h;
        }
    }

    for (size_t i = 0; i < m_surfToolRects.size(); ++i)
    {
        const RECT& r = m_surfToolRects[i];
        if (pt.x >= r.left && pt.x < r.right && pt.y >= r.top && pt.y < r.bottom)
        {
            h.kind = SurfaceHit::ToolButton;
            h.index = (int)i;
            return h;
        }
    }

    // 分类胶囊：控件箱顶部那排过滤按钮，命中优先于树行
    for (size_t i = 0; i < m_surfChipRects.size(); ++i)
    {
        const RECT& r = m_surfChipRects[i];
        if (pt.x >= r.left && pt.x < r.right && pt.y >= r.top && pt.y < r.bottom)
        {
            h.kind = SurfaceHit::Chip;
            h.index = (int)i;
            return h;
        }
    }

    // 分割条只有 4px 宽，命中区左右各放宽 2px
    for (size_t i = 0; i < m_surfSplitterRects.size(); ++i)
    {
        RECT r = m_surfSplitterRects[i];
        if (r.right - r.left < 5) { r.left -= 2; r.right += 2; }
        else { r.top -= 2; r.bottom += 2; }
        if (pt.x >= r.left && pt.x < r.right && pt.y >= r.top && pt.y < r.bottom)
        {
            h.kind = SurfaceHit::Splitter;
            h.index = (int)i;
            return h;
        }
    }

    auto inRect = [&](const RECT& r) {
        return r.right > r.left && pt.x >= r.left && pt.x < r.right &&
               pt.y >= r.top && pt.y < r.bottom;
    };
    auto rowOf = [&](const RECT& r, int scroll, int headH) {
        return (pt.y - r.top + scroll - headH) / rowH;
    };
    // 树行的展开/折叠方框命中：与 xmlDrawTree 同一套几何（padLeft 8 / indent 16 / 方框 9px）
    // 判定放宽到方框两侧各 3px，点起来不用那么准
    auto onTreeIcon = [&](const RECT& r, int scroll, int level) {
        const int rowTop = r.top - scroll + (int)((pt.y - (r.top - scroll)) / rowH) * rowH;
        const int iconLeft = r.left + 8 + level * 16;
        RECT box = { iconLeft - 3, rowTop + 4, iconLeft + 12, rowTop + rowH - 4 };
        return pt.x >= box.left && pt.x < box.right && pt.y >= box.top && pt.y < box.bottom;
    };

    if (inRect(m_surfWidgetRc))
    {
        h.kind = SurfaceHit::WidgetRow;
        int row = rowOf(m_surfWidgetRc, m_surfWidgetScroll, 0);
        if (row >= 0 && row < (int)m_surfWidgetTags.size())
        {
            h.index = row;
            const int lvl = (row < (int)m_surfWidgetLevels.size()) ? m_surfWidgetLevels[row] : 0;
            if (m_surfWidgetTags[row].IsEmpty())   // 只有分类行画了展开框
                h.onIcon = onTreeIcon(m_surfWidgetRc, m_surfWidgetScroll, lvl);
        }
        return h;
    }
    if (inRect(m_surfDocRc))
    {
        h.kind = SurfaceHit::DocRow;
        int row = rowOf(m_surfDocRc, m_surfDocScroll, 0);
        if (row >= 0 && row < (int)m_surfDocNodes.size())
        {
            h.index = row;
            const int lvl = (row < (int)m_surfDocLevels.size()) ? m_surfDocLevels[row] : 0;
            h.onIcon = onTreeIcon(m_surfDocRc, m_surfDocScroll, lvl);
        }
        return h;
    }
    if (inRect(m_surfPropRc))
    {
        h.kind = SurfaceHit::PropRow;
        int row = rowOf(m_surfPropRc, m_surfPropScroll, 26);   // 属性表有 26px 表头
        if (row >= 0) h.index = row;
        return h;
    }
    if (inRect(m_surfCodeRc))
    {
        h.kind = SurfaceHit::Code;
        return h;
    }
    return h;
}

// 无边框窗体：贴窗口四边 6px 起缩放，标题行空白处起拖动；双击标题行切换最大化
bool DesignerPaneView::tryFrameChrome(CPoint pt)
{
    // 保留旧名字给外部调用：贴边按下即开始自己缩放
    const int edges = frameEdgesAt(pt);
    if (edges == 0)
        return false;
    startSystemChrome(pt, edges);
    return true;
}

// 1=左 2=右 4=上 8=下（相对顶层窗口）
int DesignerPaneView::frameEdgesAt(CPoint pt)
{
    HWND root = ::GetAncestor(m_hWnd, GA_ROOT);
    if (root == nullptr || ::IsZoomed(root))
        return 0;
    RECT wr = {};
    ::GetWindowRect(root, &wr);
    POINT sp = pt;
    ::ClientToScreen(m_hWnd, &sp);
    const int edge = 6;
    int edges = 0;
    if (sp.x - wr.left < edge) edges |= 1;
    if (wr.right - sp.x < edge) edges |= 2;
    if (sp.y - wr.top < edge) edges |= 4;
    if (wr.bottom - sp.y < edge) edges |= 8;
    return edges;
}

// Qt 的 QWindow::startSystemMove / startSystemResize（windows 平台）就是这个动作：
// ReleaseCapture() 后给自己发 WM_NCLBUTTONDOWN(HTCAPTION / HTLEFT / HTTOP / ...)，
// DefWindowProc 随即进入系统移动/缩放模态循环，由 DWM 合成整窗。
// 好处：不占我们的消息循环，不做任何自绘贴图，跟手由系统保证，也就没有拖影和"闪一下"。
void DesignerPaneView::startSystemChrome(CPoint pt, int edges)
{
    HWND root = ::GetAncestor(m_hWnd, GA_ROOT);
    if (root == nullptr)
        return;
    // HTCAPTION 不缩放但要能移动；四边/四角按位组合映射成系统命中码
    int hit = HTCAPTION;
    switch (edges)
    {
    case 1:  hit = HTLEFT; break;
    case 2:  hit = HTRIGHT; break;
    case 4:  hit = HTTOP; break;
    case 8:  hit = HTBOTTOM; break;
    case 5:  hit = HTTOPLEFT; break;
    case 6:  hit = HTTOPRIGHT; break;
    case 9:  hit = HTBOTTOMLEFT; break;
    case 10: hit = HTBOTTOMRIGHT; break;
    default: hit = HTCAPTION; break;
    }

    POINT sp = pt;
    ::ClientToScreen(m_hWnd, &sp);
    m_frameRoot = root;
    m_frameEdges = edges;
    m_frameStart = CPoint(sp.x, sp.y);
    ::GetWindowRect(root, &m_frameRect);
    // 让系统循环自己跑：必须先放开 capture，否则它拿不到鼠标
    if (::GetCapture() == m_hWnd)
        ::ReleaseCapture();
    ::PostMessageW(root, WM_NCLBUTTONDOWN, static_cast<WPARAM>(hit),
                   MAKELPARAM(sp.x, sp.y));
}

void DesignerPaneView::surfaceToolClick(int idx)
{
    if (idx < 0 || idx >= (int)m_surfToolNames.size()) return;
    const CString name(m_surfToolNames[idx].c_str());

    // 窗口按钮：最小化 / 最大化（再点还原）/ 关闭，直接发给顶层框架
    if (name.Compare(_T("btnWinMin")) == 0 || name.Compare(_T("btnWinMax")) == 0 ||
        name.Compare(_T("btnWinClose")) == 0)
    {
        HWND root = ::GetAncestor(m_hWnd, GA_ROOT);
        if (root != nullptr)
        {
            if (name.Compare(_T("btnWinMin")) == 0)
                ::PostMessageW(root, WM_SYSCOMMAND, SC_MINIMIZE, 0);
            else if (name.Compare(_T("btnWinClose")) == 0)
                ::PostMessageW(root, WM_SYSCOMMAND, SC_CLOSE, 0);
            else
                ::PostMessageW(root, WM_SYSCOMMAND, ::IsZoomed(root) ? SC_RESTORE : SC_MAXIMIZE, 0);
        }
        Invalidate(FALSE);
        return;
    }
    // Draw 工具条：点一下就把该图元交给画布"待放置"
    if (name.Left(6).Compare(_T("shape:")) == 0)
    {
        if (m_canvas != nullptr)
            m_canvas->SetPendingShapeTag(std::wstring(name.Mid(6).GetString()));
        Invalidate(FALSE);
        return;
    }

    const UINT id = ensureAction(name);   // 复用菜单同一套命令分发
    OnDynamicCmd(id);
    Invalidate(FALSE);
}

void DesignerPaneView::surfaceLButtonDown(CPoint pt)
{
    // 有内联编辑框时先落定上一次编辑：点别处即提交
    if (m_surfPropEditRow != -1) commitPropEditSurface();
    if (m_codeEditing) commitCodeEdit();

    // 无边框窗体（对齐 Qt FramelessWindowHint + startSystemResize/startSystemMove）：
    // 贴边 → 交给系统缩放循环；标题行空白 → 交给系统移动循环，都是 DWM 合成。
    const int edges = frameEdgesAt(pt);
    if (edges != 0)
    {
        startSystemChrome(pt, edges);
        return;
    }
    if (pt.y < m_surfTitleStrip && surfaceHitTest(pt).kind == SurfaceHit::None)
    {
        startSystemChrome(pt, 0);
        return;
    }

    SurfaceHit h = surfaceHitTest(pt);
    switch (h.kind)
    {
    case SurfaceHit::Menu:
        // 按下瞬间锁定节点与屏幕锚点：重绘会重建矩形表，索引会失效
        if (h.index >= 0 && h.index < (int)m_surfMenuNodes.size())
        {
            m_surfPendingMenuNode = m_surfMenuNodes[h.index];
            POINT anchor = { m_surfMenuRects[h.index].left, m_surfMenuRects[h.index].bottom };
            ::ClientToScreen(m_hWnd, &anchor);
            m_surfPendingMenuAnchor = anchor;
        }
        break;

    case SurfaceHit::ToolButton:
        surfaceToolClick(h.index);
        break;

    case SurfaceHit::WidgetRow:
        // 点在展开框上：只折叠/展开，不进入"待创建"
        if (h.onIcon && h.index >= 0 && h.index < (int)m_surfWidgetCats.size() &&
            !m_surfWidgetCats[h.index].IsEmpty())
        {
            const CString& cat = m_surfWidgetCats[h.index];
            if (m_collapsedCats.find(cat) == m_collapsedCats.end())
                m_collapsedCats.insert(cat);
            else
                m_collapsedCats.erase(cat);
            Invalidate(FALSE);
            break;
        }
        // 分类行不可放置；叶子行点中即进入"待创建"状态，再点画布落下
        if (h.index >= 0 && h.index < (int)m_surfWidgetTags.size() && !m_surfWidgetTags[h.index].IsEmpty())
        {
            m_surfSelWidgetRow = h.index;
            m_pendingCreateTag = m_surfWidgetTags[h.index];
            Invalidate(FALSE);
        }
        break;

    case SurfaceHit::DocRow:
        if (h.onIcon && h.index >= 0 && h.index < (int)m_surfDocNodes.size())
        {
            XmlElement* node = m_surfDocNodes[h.index];
            if (node != nullptr && !node->children.empty())
            {
                if (m_collapsedNodes.find(node) == m_collapsedNodes.end())
                    m_collapsedNodes.insert(node);
                else
                    m_collapsedNodes.erase(node);
                Invalidate(FALSE);
            }
            break;
        }
        if (h.index >= 0 && h.index < (int)m_surfDocNodes.size())
        {
            XmlElement* node = m_surfDocNodes[h.index];
            Path p;
            XmlElement* root = m_doc.root();
            if (root != nullptr)
            {
                uidesign::Walk(root, [&](const Path& pp, XmlElement& el) {
                    if (p.empty() && &el == node) p = pp;
                });
            }
            if (!p.empty())
            {
                // Ctrl+单击：在对象检查器里把这一行加进 / 移出多选集合（与画布上的 Ctrl+单击一致）
                if ((::GetKeyState(VK_CONTROL) & 0x8000) != 0)
                {
                    std::vector<Path> v = m_selSet;
                    bool found = false;
                    for (size_t i = 0; i < v.size(); ++i)
                        if (v[i] == p) { v.erase(v.begin() + i); found = true; break; }
                    if (!found) v.push_back(p);
                    if (v.empty()) clearSelection();
                    else selectMulti(v);
                }
                else
                {
                    selectNode(p);
                }
                Invalidate(FALSE);
                // 双击同一行 → 就地改名（Qt Designer 的对象检查器同样行为）
                const DWORD now = ::GetTickCount();
                const bool dbl = (now - m_surfClickTick) < 500 &&
                                 (abs(pt.x - m_surfClickPt.x) < 4) && (abs(pt.y - m_surfClickPt.y) < 4);
                if (dbl) beginRenameDocRow(h.index);
            }
            m_surfClickTick = ::GetTickCount();
            m_surfClickPt = pt;
        }
        break;

    case SurfaceHit::PropRow:
    {
        // 双击同一格（500ms 内、位移 <4px）进入就地编辑
        const DWORD now = ::GetTickCount();
        const bool dbl = (now - m_surfClickTick) < 500 &&
                         (abs(pt.x - m_surfClickPt.x) < 4) && (abs(pt.y - m_surfClickPt.y) < 4);
        m_surfClickTick = now;
        m_surfClickPt = pt;
        m_surfSelPropRow = h.index;
        if (dbl) beginPropEditSurface(h.index);
        Invalidate(FALSE);
        break;
    }

    case SurfaceHit::Code:
        // 代码区自绘、没有真实控件，点进去贴一个多行 CEdit 让源码可编辑
        beginCodeEdit();
        break;

    case SurfaceHit::Chip:
        if (h.index >= 0 && h.index < (int)m_surfChipCats.size())
        {
            m_widgetFilter = m_surfChipCats[h.index];
            m_surfSelWidgetRow = -1;
            m_surfWidgetScroll = 0;
            Invalidate(FALSE);
        }
        break;

    case SurfaceHit::Splitter:
        if (h.index >= 0 && h.index < (int)m_surfSplitterIdx.size())
        {
            const int si = m_surfSplitterIdx[h.index];
            if (si >= 0 && si < (int)m_splitters.size())
            {
                SplitterInfo& sp = m_splitters[si];
                m_dragSplit = si;
                m_splitDragLastMove = 0;
                sp.startPt = pt;
                sp.startValue = sp.vertical
                    ? _wtoi(sp.grid->gridRows[sp.index].second.c_str())
                    : _wtoi(sp.grid->gridCols[sp.index].second.c_str());
                SetCapture();
            }
        }
        break;

    default:
        break;
    }
}

void DesignerPaneView::surfaceLButtonUp(CPoint pt)
{
    if (m_frameRoot != nullptr && ::GetCapture() == m_hWnd)
    {
        m_frameRoot = nullptr;
        m_frameEdges = 0;
        ::ReleaseCapture();
        endSizeMovePreview();
        return;
    }
    (void)pt;
    // 枚举属性候选也在松开时弹（按下弹会被随后的 WM_LBUTTONUP 立刻关掉）
    if (m_surfEnumPendingRow >= 0)
    {
        const int row = m_surfEnumPendingRow;
        m_surfEnumPendingRow = -1;
        popupPropEnum(row);
        return;
    }
    // 下拉菜单在松开时弹：按下就弹会被随后的 WM_LBUTTONUP 立刻关掉
    if (m_surfPendingMenuNode != nullptr)
    {
        XmlElement* top = m_surfPendingMenuNode;
        m_surfPendingMenuNode = nullptr;
        surfaceMenuPopup(top, m_surfPendingMenuAnchor);
        return;
    }
    if (m_dragSplit >= 0)
    {
        splitterEnd(m_dragSplit);
        m_dragSplit = -1;
        ReleaseCapture();
        Invalidate(FALSE);
    }
}

void DesignerPaneView::surfaceMouseMove(CPoint pt, UINT flags)
{
    (void)flags;
    // 窗口移动/缩放已交回系统模态循环（Qt startSystemMove/startSystemResize 等价），
    // 这里不再按位移改窗口矩形：自己 SetWindowPos 追鼠标是拖影和边缘闪烁的来源。
    if (m_dragSplit >= 0)
    {
        splitterDrag(m_dragSplit, pt);
        return;
    }

    SurfaceHit h = surfaceHitTest(pt);
    const int hotTool = (h.kind == SurfaceHit::ToolButton) ? h.index : -1;
    const int hotSplit = (h.kind == SurfaceHit::Splitter) ? h.index : -1;
    const int hotChip = (h.kind == SurfaceHit::Chip) ? h.index : -1;
    if (hotTool != m_surfHotTool || hotSplit != m_surfHotSplit || hotChip != m_surfHotChip)
    {
        m_surfHotTool = hotTool;
        m_surfHotSplit = hotSplit;
        m_surfHotChip = hotChip;
        ::SetCursor(::LoadCursor(nullptr, hotTool >= 0 ? IDC_HAND : IDC_ARROW));
        Invalidate(FALSE);
    }
}

bool DesignerPaneView::surfaceScrollRect(const RECT& rc, CPoint pt, int& scroll, int total,
                                  int pageH, short delta)
{
    if (rc.right <= rc.left || pageH <= 0) return false;
    if (pt.x < rc.left || pt.x >= rc.right || pt.y < rc.top || pt.y >= rc.bottom) return false;

    const int maxScroll = (total > pageH) ? (total - pageH) : 0;
    scroll += (delta > 0) ? -48 : 48;
    if (scroll > maxScroll) scroll = maxScroll;
    if (scroll < 0) scroll = 0;
    Invalidate(FALSE);
    return true;
}

BOOL DesignerPaneView::surfaceMouseWheel(CPoint pt, short delta)
{
    // 光标在画布上 → 滚轮缩放（以光标为锚点），走画布自己的视图变换
    if (m_canvas != nullptr && ::IsWindow(m_canvas->m_hWnd))
    {
        CRect cr;
        m_canvas->GetWindowRect(&cr);
        ScreenToClient(&cr);
        if (cr.PtInRect(pt))
        {
            CPoint local = pt - cr.TopLeft();
            m_canvas->zoomAt(local, delta);
            updateStatus();
            return TRUE;
        }
    }
    const int page = 0;
    (void)page;
    int pw = (int)(m_surfWidgetRc.bottom - m_surfWidgetRc.top);
    int pd = (int)(m_surfDocRc.bottom - m_surfDocRc.top);
    int pp = (int)(m_surfPropRc.bottom - m_surfPropRc.top);
    int pc = (int)(m_surfCodeRc.bottom - m_surfCodeRc.top);

    if (surfaceScrollRect(m_surfWidgetRc, pt, m_surfWidgetScroll, m_surfWidgetTotal, pw, delta)) return TRUE;
    if (surfaceScrollRect(m_surfDocRc, pt, m_surfDocScroll, m_surfDocTotal, pd, delta)) return TRUE;
    if (surfaceScrollRect(m_surfPropRc, pt, m_surfPropScroll, m_surfPropTotal, pp, delta)) return TRUE;
    if (surfaceScrollRect(m_surfCodeRc, pt, m_surfCodeScroll, m_surfCodeTotal, pc, delta)) return TRUE;
    return FALSE;
}

void DesignerPaneView::clampSurfaceScroll()
{
    auto clampOne = [](int& s0, int total, const RECT& rc) {
        const int page = (int)(rc.bottom - rc.top);
        const int maxScroll = (total > page) ? (total - page) : 0;
        if (s0 > maxScroll) s0 = maxScroll;
        if (s0 < 0) s0 = 0;
    };
    clampOne(m_surfWidgetScroll, m_surfWidgetTotal, m_surfWidgetRc);
    clampOne(m_surfDocScroll, m_surfDocTotal, m_surfDocRc);
    clampOne(m_surfPropScroll, m_surfPropTotal, m_surfPropRc);
    clampOne(m_surfCodeScroll, m_surfCodeTotal, m_surfCodeRc);
}

BOOL DesignerPaneView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
    (void)nFlags;
    CPoint c = pt;
    ScreenToClient(&c);
    return surfaceMouseWheel(c, zDelta) ? TRUE : FALSE;
}

// ===========================================================================
// 属性表就地编辑（双击值）与右键增删复移
// 表面是自绘的，没有真实控件承接输入，所以用一个真实 CEdit 贴在值列位置：
// 键盘、输入法、选区全交给系统，表面只负责在旁边画。
// ===========================================================================
void DesignerPaneView::beginRenameDocRow(int row)
{
    if (row < 0 || row >= (int)m_surfDocNodes.size()) return;
    XmlElement* node = m_surfDocNodes[row];
    if (node == nullptr) return;
    if (m_surfDocRc.right <= m_surfDocRc.left) return;

    const int rowH = 24;                                   // 与 TreeStyle 默认行高一致
    const int top = m_surfDocRc.top + row * rowH - m_surfDocScroll;
    const int level = (row < (int)m_surfDocLevels.size() ? m_surfDocLevels[row] : 0);
    const int indent = 8 + level * 16 + 13;                // padLeft + 层级缩进 + 图标列
    CRect cell(m_surfDocRc.left + indent, top, m_surfDocRc.right - 8, top + rowH);
    if (cell.bottom < m_surfDocRc.top || cell.top > m_surfDocRc.bottom) return;   // 行不在可视区

    if (m_propEdit.GetSafeHwnd() == nullptr)
        m_propEdit.Create(WS_CHILD | WS_BORDER | ES_AUTOHSCROLL, cell, this, 2003);
    else
        m_propEdit.MoveWindow(cell);

    m_propEdit.ShowWindow(SW_SHOW);
    m_propEdit.SetWindowText(node->name.empty() ? node->tag.c_str() : node->name.c_str());
    m_propEdit.SetSel(0, -1);
    m_propEdit.SetFocus();

    m_surfPropEditRow = kRenameDocRow;   // 特殊行号：提交时按 Name 落到节点上
    m_surfPropEditKey.Empty();
}

void DesignerPaneView::beginPropEditSurface(int row)
{
    if (m_surfPropRc.right <= m_surfPropRc.left) return;    // 属性表还没画过（无矩形可用）

    const int headH = 26, rowH = 24, keyW = 104;
    CRect cell;
    if (row == -2)          // -2 = 新增属性：编辑框放在现有行之外的最后一行
    {
        const int top = m_surfPropRc.top + headH + (int)m_surfPropRows.size() * rowH - m_surfPropScroll;
        cell.SetRect(m_surfPropRc.left + keyW, top, m_surfPropRc.right, top + rowH);
    }
    else
    {
        if (row < 0 || row >= (int)m_surfPropRows.size()) return;
        if (m_surfPropRows[row].group) return;               // 分组标题行不可编辑

        // 枚举类属性（Shape/BorderStyle/Dock/bool…）弹候选，不让用户手敲字符串。
        // 但候选不能在 mouse-down 里弹：紧接着的 WM_LBUTTONUP 会把它秒关，所以记 pending 交给 up。
        XmlElement* node = Resolve(m_doc.root(), m_selected);
        if (node != nullptr)
        {
            const CString key(m_surfPropRows[row].key.c_str());
            const CString opts = propOptions(CString(node->tag.c_str()), key);
            if (!opts.IsEmpty())
            {
                m_surfEnumPendingRow = row;
                const int top = m_surfPropRc.top + headH + row * rowH - m_surfPropScroll;
                CPoint scr(m_surfPropRc.left + keyW, top + rowH);
                ClientToScreen(&scr);
                m_surfEnumPendingPt = scr;
                return;
            }
        }

        const int top = m_surfPropRc.top + headH + row * rowH - m_surfPropScroll;
        cell.SetRect(m_surfPropRc.left + keyW, top, m_surfPropRc.right, top + rowH);
    }

    if (m_propEdit.GetSafeHwnd() == nullptr)
        m_propEdit.Create(WS_CHILD | WS_BORDER | ES_AUTOHSCROLL, cell, this, 2003);
    else
        m_propEdit.MoveWindow(cell);

    m_propEdit.ShowWindow(SW_SHOW);
    m_propEdit.SetWindowText(row == -2 ? L"" : m_surfPropRows[row].val.c_str());
    m_propEdit.SetSel(0, -1);
    m_propEdit.SetFocus();

    m_surfPropEditRow = row;
    m_surfPropEditKey = (row == -2) ? CString() : CString(m_surfPropRows[row].key.c_str());
}

void DesignerPaneView::popupPropEnum(int row)
{
    if (row < 0 || row >= (int)m_surfPropRows.size()) return;
    if (m_surfPropRows[row].group) return;

    XmlElement* node = Resolve(m_doc.root(), m_selected);
    if (node == nullptr) return;

    const CString key(m_surfPropRows[row].key.c_str());
    const CString opts = propOptions(CString(node->tag.c_str()), key);
    if (opts.IsEmpty()) return;

    std::vector<CString> cand;
    CString rest = opts;
    while (!rest.IsEmpty())
    {
        const int bar = rest.Find(L'|');
        if (bar < 0) { cand.push_back(rest); break; }
        cand.push_back(rest.Left(bar));
        rest = rest.Mid(bar + 1);
    }
    if (cand.empty()) return;

    CMenu menu;
    menu.CreatePopupMenu();
    for (size_t i = 0; i < cand.size(); ++i)
        menu.AppendMenuW(MF_STRING, (UINT)(i + 1), cand[i].GetString());

    const int cmd = (int)menu.TrackPopupMenu(
        TPM_RETURNCMD | TPM_LEFTALIGN | TPM_TOPALIGN,
        m_surfEnumPendingPt.x, m_surfEnumPendingPt.y, this);
    if (cmd > 0 && cmd <= (int)cand.size())
        applyPropVal(key, cand[cmd - 1]);
}

void DesignerPaneView::commitPropEditSurface()
{
    if (m_surfPropEditRow == -1) return;
    if (m_propEdit.GetSafeHwnd() == nullptr) { m_surfPropEditRow = -1; return; }

    CString val;
    m_propEdit.GetWindowText(val);
    const int row = m_surfPropEditRow;
    const CString key = m_surfPropEditKey;
    m_surfPropEditRow = -1;
    m_surfPropEditKey.Empty();
    m_propEdit.ShowWindow(SW_HIDE);
    // 隐藏后把焦点还给视图：否则后续快捷键（Ctrl+C/X/V、Delete）继续落进这个不可见的编辑框，
    // 表现就是"Ctrl+V 有时完全没反应"。
    if (::GetFocus() == m_propEdit.m_hWnd) ::SetFocus(m_hWnd);

    if (row == -3)          // 对象检查器双击改名：直接写 Name
    {
        XmlElement* node = Resolve(m_doc.root(), m_selected);
        if (node != nullptr && node != m_doc.root())
        {
            val.Trim();
            if (!val.IsEmpty())
            {
                pushUndo();
                node->name = val.GetString();
                node->set(L"Name", node->name);
                Invalidate(FALSE);
                updateStatus();
                updateXmlView();
            }
        }
        return;
    }

    if (row == -2)          // 新增属性：输入 "Key=Value"
    {
        CString k = val, v;
        const int eq = k.Find(L'=');
        if (eq >= 0) { v = k.Mid(eq + 1); k = k.Left(eq); }
        k.Trim();
        v.Trim();
        if (!k.IsEmpty()) applyPropVal(k, v);
        return;
    }

    if (key.IsEmpty()) return;
    CString oldVal;
    if (row >= 0 && row < (int)m_surfPropRows.size())
        oldVal = CString(m_surfPropRows[row].val.c_str());
    if (val == oldVal) return;
    applyPropVal(key, val);
}

void DesignerPaneView::cancelPropEditSurface()
{
    m_surfPropEditRow = -1;
    m_surfPropEditKey.Empty();
    if (m_propEdit.GetSafeHwnd() != nullptr)
    {
        m_propEdit.ShowWindow(SW_HIDE);
        if (::GetFocus() == m_propEdit.m_hWnd) ::SetFocus(m_hWnd);
    }
    Invalidate(FALSE);
}

void DesignerPaneView::OnRButtonDown(UINT nFlags, CPoint point)
{
    (void)nFlags;
    if (m_surfPropEditRow != -1) commitPropEditSurface();

    SurfaceHit h = surfaceHitTest(point);
    if (h.kind == SurfaceHit::DocRow)
    {
        // 对象树右键：复制 / 粘贴 / 上移 / 下移 / 删除
        if (h.index >= 0 && h.index < (int)m_surfDocNodes.size())
        {
            XmlElement* node = m_surfDocNodes[h.index];
            XmlElement* root = m_doc.root();
            if (root != nullptr)
            {
                Path p;
                uidesign::Walk(root, [&](const Path& pp, XmlElement& el) {
                    if (p.empty() && &el == node) p = pp;
                });
                if (!p.empty())
                {
                    // 必须先落到 m_selected：surfaceDocMenu 靠 m_selected 解析目标节点，
                    // 只调 selectNode 而不赋值会让它拿到上一条选择（甚至空路径解析成 root），
                    // 结果"删除节点"被判成根节点而变灰。
                    m_selected = p;
                    selectNode(p);
                    Invalidate(FALSE);
                    surfaceDocMenu(point);
                }
            }
        }
        return;
    }
    if (h.kind != SurfaceHit::PropRow) return;
    m_surfSelPropRow = h.index;
    Invalidate(FALSE);
    surfacePropMenu(point);
}

void DesignerPaneView::OnContextMenu(CWnd* pWnd, CPoint pos)
{
    // 右键菜单只保留自绘那一套（OnRButtonDown 里按命中区域弹），
    // MFC/pane 默认的"停靠/自动隐藏/隐藏"系统菜单在这里整条吞掉。
    (void)pWnd;
    (void)pos;
}

void DesignerPaneView::surfacePropMenu(CPoint pt)
{
    const int row = m_surfSelPropRow;
    const bool hasRow = (row >= 0 && row < (int)m_surfPropRows.size() && !m_surfPropRows[row].group);

    CMenu menu;
    menu.CreatePopupMenu();
    menu.AppendMenu(MF_STRING | (hasRow ? MF_ENABLED : MF_GRAYED), 1, L"复制值");
    menu.AppendMenu(MF_STRING | (hasRow ? MF_ENABLED : MF_GRAYED), 2, L"粘贴值");
    menu.AppendMenu(MF_SEPARATOR);
    menu.AppendMenu(MF_STRING, 3, L"新增属性…（输入 Key=Value）");
    menu.AppendMenu(MF_STRING | (hasRow ? MF_ENABLED : MF_GRAYED), 4, L"删除此属性");

    CPoint scr = pt;
    ClientToScreen(&scr);
    const int cmd = (int)menu.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_TOPALIGN, scr.x, scr.y, this);
    if (cmd <= 0) return;

    if (cmd == 1)
    {
        if (!::OpenClipboard(m_hWnd)) return;
        ::EmptyClipboard();
        const std::wstring v = m_surfPropRows[row].val;
        const size_t bytes = (v.size() + 1) * sizeof(wchar_t);
        HGLOBAL hg = ::GlobalAlloc(GMEM_MOVEABLE, bytes);
        if (hg != nullptr)
        {
            void* p = ::GlobalLock(hg);
            if (p != nullptr)
            {
                memcpy(p, v.c_str(), bytes);
                ::GlobalUnlock(hg);
                ::SetClipboardData(CF_UNICODETEXT, hg);
            }
        }
        ::CloseClipboard();
        return;
    }
    if (cmd == 2)
    {
        if (!::OpenClipboard(m_hWnd)) return;
        CString v;
        HGLOBAL hg = ::GetClipboardData(CF_UNICODETEXT);
        if (hg != nullptr)
        {
            const wchar_t* p = reinterpret_cast<const wchar_t*>(::GlobalLock(hg));
            if (p != nullptr) v = p;
            ::GlobalUnlock(hg);
        }
        ::CloseClipboard();
        if (v.IsEmpty()) return;
        applyPropVal(CString(m_surfPropRows[row].key.c_str()), v);
        return;
    }
    if (cmd == 3)
    {
        beginPropEditSurface(-2);
        return;
    }
    if (cmd == 4)
    {
        XmlElement* node = Resolve(m_doc.root(), m_selected);
        if (node == nullptr) return;
        const CString key(m_surfPropRows[row].key.c_str());
        bool removed = false;
        auto& a = node->attrs;
        for (auto it = a.begin(); it != a.end(); ++it)
        {
            if (key.CompareNoCase(it->first.c_str()) == 0) { a.erase(it); removed = true; break; }
        }
        // X/Y/Width/Height 是布局字段，删掉对应声明后把内存值也复位，否则看不出变化
        if (key == L"X" || key == L"Left") node->x = 0;
        else if (key == L"Y" || key == L"Top") node->y = 0;
        else if (key == L"Width") { node->w = 0; node->hasW = false; }
        else if (key == L"Height") { node->h = 0; node->hasH = false; }
        if (!removed && key != L"X" && key != L"Y" && key != L"Width" && key != L"Height") return;

        pushUndo();
        m_doc.applyBindings();
        XmlElement* root = m_doc.root();
        if (root != nullptr) m_doc.layout(0, 0, root->w, root->h);
        rebuildTree();
        selectNode(m_selected);
        updateStatus();
        updateXmlView();
        Invalidate(FALSE);
        return;
    }
}
// 对象树右键：复制节点 / 粘贴为子节点 / 上移 / 下移 / 删除节点
// "增"走控件箱点选 + 画布落点，这里补的是"复"和"移"。
void DesignerPaneView::surfaceDocMenu(CPoint pt)
{
    XmlElement* root = m_doc.root();
    XmlElement* node = Resolve(root, m_selected);
    if (root == nullptr || node == nullptr) return;
    const bool isRoot = (node == root);

    int idx = -1;
    XmlElement* parent = isRoot ? nullptr : uidesign::ParentOf(root, m_selected, idx);
    const bool canMoveUp = (!isRoot && parent != nullptr && idx > 0);
    const bool canMoveDown = (!isRoot && parent != nullptr && idx >= 0 && idx + 1 < (int)parent->children.size());

    CMenu menu;
    menu.CreatePopupMenu();
    menu.AppendMenu(MF_STRING, 1, L"复制节点");
    menu.AppendMenu(MF_STRING, 6, L"剪切节点");
    menu.AppendMenu(MF_STRING | (m_hasClipNode ? MF_ENABLED : MF_GRAYED), 2, L"粘贴为子节点");
    menu.AppendMenu(MF_SEPARATOR);
    // 坐标定位模式：把当前摆放（Left/Top）一键换成 Grid 行列轨道，之后拖动/对齐都按网格走
    menu.AppendMenu(MF_STRING, 7, L"转为 Grid 布局（按当前位置推断行列）");
    menu.AppendMenu(MF_SEPARATOR);
    menu.AppendMenu(MF_STRING | (canMoveUp ? MF_ENABLED : MF_GRAYED), 3, L"上移");
    menu.AppendMenu(MF_STRING | (canMoveDown ? MF_ENABLED : MF_GRAYED), 4, L"下移");
    menu.AppendMenu(MF_SEPARATOR);
    menu.AppendMenu(MF_STRING | (isRoot ? MF_GRAYED : MF_ENABLED), 5, L"删除节点");
    CPoint scr = pt;
    ClientToScreen(&scr);
    menuDiag(L"[menu] open pt=%d,%d scr=%d,%d node=%s selLen=%d hasClip=%d", pt.x, pt.y, scr.x, scr.y,
             CString(node->tag.c_str()).GetString(), (int)m_selected.size(), (int)m_hasClipNode);
    const int cmd = (int)menu.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_TOPALIGN, scr.x, scr.y, this);
    menuDiag(L"[menu] cmd=%d", cmd);
    if (cmd <= 0) return;

    bool changed = false;
    Path newSel = m_selected;

    if (cmd == 1)
    {
        m_clipNode = *node;      // XmlElement 是值语义树，整棵拷贝
        m_hasClipNode = true;
        notImpl(L"已复制节点 " + CString(node->tag.c_str()));
        return;
    }
    else if (cmd == 2)
    {
        if (!m_hasClipNode) return;
        pushUndo();
        // 粘贴出来的副本自动改名：尾部数字 +1（Button3 → Button4），全文档查重保唯一
        CString base = m_clipNode.name.empty() ? CString(m_clipNode.tag.c_str())
                                              : CString(m_clipNode.name.c_str());
        CString nm = makeUniqueName(base);
        XmlElement clip = m_clipNode;
        clip.name = nm.GetString();
        clip.set(L"Name", clip.name);

        // 目标不是容器时（右键点在 Button 上）粘到它的父容器里当兄弟，别把按钮塞进按钮
        XmlElement* target = node;
        Path targetPath = m_selected;
        if (!uidesign::IsContainer(target->tag))
        {
            int pidx = -1;
            XmlElement* par = isRoot ? nullptr : uidesign::ParentOf(root, m_selected, pidx);
            if (par != nullptr)
            {
                target = par;
                targetPath = m_selected;
                targetPath.pop_back();
            }
        }
        target->children.push_back(clip);
        newSel = targetPath;
        newSel.push_back((int)target->children.size() - 1);
        changed = true;
    }
    else if (cmd == 6)      // 剪切：复制到内部剪贴板后删掉原节点
    {
        m_clipNode = *node;
        m_hasClipNode = true;
        CString cutBase = m_clipNode.name.empty() ? CString(m_clipNode.tag.c_str())
                                                  : CString(m_clipNode.name.c_str());
        m_clipNode.name = makeUniqueName(cutBase).GetString();
        m_clipNode.set(L"Name", m_clipNode.name);
        pushUndo();
        uidesign::RemoveChild(parent, idx);
        newSel.clear();
        changed = true;
    }
    else if (cmd == 7)      // 转为 Grid 布局：按现有摆放位置推断行/列轨道，去掉绝对 Left/Top
    {
        // 目标容器：右键的是容器就转它自己；右键的是按钮等叶子，就转它的父容器
        XmlElement* target = node;
        Path targetPath = m_selected;
        if (!uidesign::IsContainer(target->tag))
        {
            int pidx = -1;
            XmlElement* par = isRoot ? nullptr : uidesign::ParentOf(root, m_selected, pidx);
            if (par == nullptr) return;
            target = par;
            targetPath = m_selected;
            targetPath.pop_back();
        }
        if (target->children.empty()) return;

        pushUndo();
        const int cw = target->hasW ? target->w : 640;
        const int ch = target->hasH ? target->h : 420;
        const int ox0 = target->previewX, oy0 = target->previewY;

        // 1) 收集子元素的相对起点（没写 Left/Top 的用预览坐标反推）
        std::vector<int> lefts, tops;
        for (auto& c : target->children)
        {
            if (c.templateChild) continue;
            lefts.push_back(c.hasX ? c.x : (int)c.previewX - ox0);
            tops.push_back(c.hasY ? c.y : (int)c.previewY - oy0);
        }
        // 2) 起点聚类：间距在容差内视作同一列/行
        auto cluster = [](std::vector<int> v, int tol) {
            std::vector<int> u;
            std::sort(v.begin(), v.end());
            for (int x : v)
                if (u.empty() || x - u.back() > tol) u.push_back(x);
            return u;
        };
        std::vector<int> xs = cluster(lefts, 12);
        std::vector<int> ys = cluster(tops, 12);
        if (xs.empty() || ys.empty()) return;

        // 3) 轨道尺寸：相邻起点之差；最后一列/行补到容器边缘
        target->gridCols.clear();
        for (size_t i = 0; i < xs.size(); ++i)
        {
            const int next = (i + 1 < xs.size()) ? xs[i + 1] : (cw > xs[i] ? cw : xs[i] + 120);
            int w2 = next - xs[i];
            if (w2 < 1) w2 = 1;
            target->gridCols.emplace_back(L"Width", std::to_wstring(w2));
        }
        target->gridRows.clear();
        for (size_t i = 0; i < ys.size(); ++i)
        {
            const int next = (i + 1 < ys.size()) ? ys[i + 1] : (ch > ys[i] ? ch : ys[i] + 60);
            int h2 = next - ys[i];
            if (h2 < 1) h2 = 1;
            target->gridRows.emplace_back(L"Height", std::to_wstring(h2));
        }

        // 4) 子元素落位：就近匹配列/行，删掉 Left/Top（有轨道后绝对坐标会让布局失效）
        for (auto& c : target->children)        {
            if (c.templateChild) continue;
            const int cx = c.hasX ? c.x : (int)c.previewX - ox0;
            const int cy = c.hasY ? c.y : (int)c.previewY - oy0;
            int ci = 0, ri = 0;
            for (size_t i = 1; i < xs.size(); ++i)
                if (abs(cx - xs[i]) < abs(cx - xs[ci])) ci = (int)i;
            for (size_t i = 1; i < ys.size(); ++i)
                if (abs(cy - ys[i]) < abs(cy - ys[ri])) ri = (int)i;

            c.hasX = false;
            c.hasY = false;
            c.attrs.erase(std::remove_if(c.attrs.begin(), c.attrs.end(),
                [](const std::pair<std::wstring, std::wstring>& kv) {
                    return _wcsicmp(kv.first.c_str(), L"Left") == 0 ||
                           _wcsicmp(kv.first.c_str(), L"Top") == 0;
                }), c.attrs.end());
            c.row = ri; c.col = ci; c.rowSpan = 1; c.colSpan = 1;
            c.set(L"Grid.Row", std::to_wstring(ri));
            c.set(L"Grid.Column", std::to_wstring(ci));
        }

        // 容器标签：普通容器改名为 Grid（XML 与属性面板口径一致）；
        // 文档根必须是 Window（运行时按窗口根解析），只给它挂轨道，标签不动
        if (target != root) target->tag = L"Grid";
        menuDiag(L"[gridconv] target=%s cols=%d rows=%d xs=%d ys=%d kids=%d",
                 CString(target->tag.c_str()).GetString(), (int)target->gridCols.size(),
                 (int)target->gridRows.size(), (int)xs.size(), (int)ys.size(),
                 (int)target->children.size());
        newSel = targetPath;
        changed = true;
    }
    else if (cmd == 3 || cmd == 4)
    {
        const int to = (cmd == 3) ? idx - 1 : idx + 1;
        pushUndo();
        std::swap(parent->children[idx], parent->children[to]);
        newSel = m_selected;
        newSel.back() = to;
        changed = true;
    }
    else if (cmd == 5)
    {
        pushUndo();
        uidesign::RemoveChild(parent, idx);
        newSel.clear();
        changed = true;
    }

    if (!changed) return;
    m_doc.applyBindings();
    if (root != nullptr) m_doc.layout(0, 0, root->w, root->h);
    if (m_canvas != nullptr) m_canvas->setDocument(&m_doc);
    rebuildTree();
    m_selected = newSel;
    selectNode(m_selected);
    updateStatus();
    updateXmlView();
    Invalidate(FALSE);
}