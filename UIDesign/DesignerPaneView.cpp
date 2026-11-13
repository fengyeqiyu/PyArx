#include "stdafx.h"
#include "DesignerPaneView.h"
#include "ControlCatalog.h"
#include <shellapi.h>
#include <algorithm>
#include <unordered_map>

using namespace uidesign;

BEGIN_MESSAGE_MAP(DesignerPaneView, CWnd)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_COMMAND_RANGE(kId_CMD_MIN, kId_CMD_MAX, OnDynamicCmd)
    ON_WM_DRAWITEM()
    ON_WM_MEASUREITEM()
    ON_WM_MOUSEMOVE()
    ON_WM_LBUTTONUP()
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONDBLCLK()
    ON_WM_SETCURSOR()
    ON_MESSAGE(WM_CANVAS_SELECT, OnCanvasSelect)
    ON_MESSAGE(WM_CANVAS_SELECT_MULTI, OnCanvasSelectMulti)
    ON_MESSAGE(WM_DOC_CHANGED, OnDocChanged)
    ON_WM_CTLCOLOR()
    ON_WM_ERASEBKGND()
    ON_WM_PAINT()
    ON_WM_MOUSEWHEEL()
    ON_WM_RBUTTONDOWN()
    ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()

int DesignerPaneView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CWnd::OnCreate(lpCreateStruct) == -1)
        return -1;
    // 父窗口不重绘子控件区域，避免拖动/刷新时窗口闪
    ModifyStyle(0, WS_CLIPCHILDREN);
    // 整窗只有一张 D2D 表面（单表面模式），GDI 多子窗口界面已移除
    m_brushPanel.CreateSolidBrush(RGB(37, 37, 38));
    m_brushEdit.CreateSolidBrush(RGB(30, 30, 32));
    m_brushGroup.CreateSolidBrush(RGB(45, 45, 48));
    if (!loadLayout())
        return -1;
    return 0;
}

// 声明给出的底色是逐控件不同的，画刷必须按颜色各存一支：
// 早先只用一支 static 画刷，导致"第一个出现的背景色"被复用给所有同类型控件。
CBrush* DesignerPaneView::brushFor(COLORREF c)
{
    auto it = m_brushByColor.find(c);
    if (it != m_brushByColor.end())
        return it->second;
    CBrush* b = new CBrush(c);
    m_brushByColor[c] = b;
    return b;
}

HBRUSH DesignerPaneView::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor){
    HWND hwnd = pWnd->m_hWnd;
    COLORREF bg = (COLORREF)-1, fg = (COLORREF)-1;
    auto bit = m_bgByHwnd.find(hwnd);
    if (bit != m_bgByHwnd.end()) bg = bit->second;
    auto fit = m_fgByHwnd.find(hwnd);
    if (fit != m_fgByHwnd.end()) fg = fit->second;

    if (fg != (COLORREF)-1) pDC->SetTextColor(fg);

    switch (nCtlColor)
    {
    case CTLCOLOR_STATIC:
        // 分割条 Static：深色实心块（配合 SS_GRAYRECT，无边框单线）
        for (auto& sp : m_splitters)
            if (sp.hwnd == hwnd)
            {
                pDC->SetBkColor(RGB(58, 64, 70));
                static CBrush s_brushSplit;
                static bool s_splitInited = false;
                if (!s_splitInited) { s_brushSplit.CreateSolidBrush(RGB(58, 64, 70)); s_splitInited = true; }
                return (HBRUSH)s_brushSplit;
            }
        if (bg != (COLORREF)-1)
        {
            pDC->SetBkColor(bg);
            return (HBRUSH)brushFor(bg);
        }
        pDC->SetBkColor(RGB(37, 37, 38));
        pDC->SetTextColor(RGB(215, 220, 228));
        return (HBRUSH)m_brushPanel;
    case CTLCOLOR_EDIT:
    case CTLCOLOR_MSGBOX:
        if (bg != (COLORREF)-1)
        {
            pDC->SetBkColor(bg);
            return (HBRUSH)brushFor(bg);
        }
        pDC->SetBkColor(RGB(30, 30, 32));
        pDC->SetTextColor(RGB(220, 224, 230));
        return (HBRUSH)m_brushEdit;
    case CTLCOLOR_BTN:
        pDC->SetBkColor(RGB(45, 45, 48));
        pDC->SetTextColor(RGB(220, 224, 230));
        return (HBRUSH)m_brushGroup;
    }
    return CWnd::OnCtlColor(pDC, pWnd, nCtlColor);
}

BOOL DesignerPaneView::OnEraseBkgnd(CDC* pDC)
{
    // 整个客户区由 D2D 每帧全画，GDI 擦背景只会多一次填充 + 闪一下
    (void)pDC;
    return TRUE;
}

// 用 XamlRuntime 加载设计器自身界面 designer_layout.xaml，并递归创建控件
bool DesignerPaneView::loadLayout()
{
    wchar_t exePath[MAX_PATH] = {0};
    ::GetModuleFileNameW(::GetModuleHandleW(nullptr), exePath, MAX_PATH);
    CString exeDir(exePath);
    int slash = exeDir.ReverseFind(L'\\');
    if (slash >= 0) exeDir = exeDir.Left(slash);
    exeDir += L"\\";

    std::vector<std::wstring> candidates = {
        std::wstring(exeDir + L"designer_layout.xaml"),
        // 从输出目录（X64\Release）回到源码目录，换盘/换机仍可用，不再写死 G:\PyArx
        std::wstring(exeDir + L"..\\..\\UIDesign\\designer_layout.xaml"),
    };
    std::wstring used;
    std::wstring err;
    for (auto& c : candidates)
    {
        if (::GetFileAttributesW(c.c_str()) != INVALID_FILE_ATTRIBUTES)
        {
            used = c;
            break;
        }
    }
    if (used.empty() || !m_ui.loadFile(used.c_str(), err))
    {
        // 报错要把找过的地方说清楚，否则只剩一句"无法加载"，排不出是路径还是文件的问题
        CString tried;
        for (auto& c : candidates)
            tried += L"\n  " + CString(c.c_str());
        if (err.empty())
            err = L"文件不存在";
        AfxMessageBox(CString(L"无法加载设计器布局：") + err.c_str() + L"\n已查找：" + tried,
                      MB_ICONERROR);
        return false;
    }

    CRect rc;
    GetClientRect(&rc);
    m_ui.layout(0, 0, rc.Width(), rc.Height());

    // 递归创建控件（Window/Grid 等纯容器不建 HWND）
    XamlElement* root = m_ui.root();
    if (root == nullptr) return true;
    for (auto& child : root->children)
        createControlForElement(&child);

    buildMenu();
    // 加载 XML 控件目录（参照 wxFormBuilder objectinfo），驱动控件箱与属性面板
    std::vector<std::wstring> catCandidates = {
        std::wstring(exeDir + L"control_catalog.xml"),
        std::wstring(exeDir + L"..\\..\\PyMfcDesigner\\control_catalog.xml"),
    };
    for (auto& c : catCandidates)
    {
        if (::GetFileAttributesW(c.c_str()) != INVALID_FILE_ATTRIBUTES)
        {
            m_catalog.load(c.c_str());
            break;
        }
    }
    populateWidgetBox();
    setupXamlCodeView();   // 底部 XAML 代码视图
    setupSplitters();      // 面板分割条（画布左右边界可拖调整大小）
    return true;
}

// 按标签创建单个控件（自身界面）
void DesignerPaneView::createControlForElement(XamlElement* el)
{
    if (el == nullptr) return;

    // 只有主画布需要真实窗口（DesignCanvas 本身就是 D2D 直呈窗口），
    // 其余全部由 DesignerPaneView 用一块 D2D 表面画出来，不再创建任何子 HWND。
    // 必须递归下去找 canvasHost —— 它藏在 Grid 里，直接 return 会连画布一起丢掉。
    if (_wcsicmp(el->tag.c_str(), L"Canvas") != 0)
    {
        for (auto& child : el->children)
            createControlForElement(&child);
        return;
    }
    if (el->name == L"xamlCodeHost")
        return;   // 代码区由表面自绘

    CString tag(el->tag.c_str());
    CString name(el->name.c_str());
    CString text(el->text.c_str());
    CString icon = el->get(L"Icon").c_str();
    int index = (int)m_ctrlByNode.size();

    CRect rc(el->previewX, el->previewY,
             el->previewX + el->previewW, el->previewY + el->previewH);

    CWnd* w = nullptr;
    if (tag.CompareNoCase(L"Canvas") == 0)
    {
        // 主画布 canvasHost 用 DesignCanvas（自身即 D2D 直呈窗口）；
        // xamlCodeHost 已在函数开头 return，走不到这里。
        {
            auto* c = new DesignCanvas();
            c->create(this, nextCtrlId());
            c->MoveWindow(rc);
            // 绘制 jig：按下画布判定（有待放置控件才进入），拖动实时预览，松手按矩形创建
            c->onPickCreate = [this](const std::wstring& t) { pickCreateTag(t); };
            c->onDrawBegin = [this](CPoint) -> bool {
                return !m_pendingCreateTag.IsEmpty();
            };
            c->onDrawEnd = [this](CRect r) {
                if (m_pendingCreateTag.IsEmpty()) return;
                CRect nr = r; nr.NormalizeRect();
                int w = nr.Width(), h = nr.Height();
                // 位置直接取 XAML 的 Left/Top（画布客户坐标 = 文档坐标，不做偏移换算）
                int x = nr.left, y = nr.top;
                if (x < 0) x = 0;
                if (y < 0) y = 0;
                if (w < 8 || h < 8)          // 单击未拖动：按该 tag 的默认尺寸放置
                    uidesign::DefaultSize(std::wstring(m_pendingCreateTag.GetString()), w, h);
                // 直线这类图元拖成一条边时可能只有 1~2 px，节点尺寸至少留 1
                if (w < 1) w = 1;
                if (h < 1) h = 1;
                CString nm;
                nm.Format(L"%s%d", m_pendingCreateTag, m_nextId++);
                addControlAt(m_pendingCreateTag, nm, x, y, w, h);
                m_pendingCreateTag.Empty();
            };
            // 画布拖拽移动/缩放：开始前快照（可撤销），结束后重布局 + 刷新属性面板/状态栏
            c->onEditBegin = [this]() { pushUndo(); };
            // 画布上右击控件：与对象检查器同一套剪贴板动作，只是操作位置在画布
            c->onContextMenu = [this](XamlElement* n, CPoint scr) {
                XamlElement* root = m_doc.root();
                if (root == nullptr || n == nullptr) return;
                Path p;
                uidesign::Walk(root, [&](const Path& pp, XamlElement& el) {
                    if (p.empty() && &el == n) p = pp;
                });
                if (p.empty()) return;
                m_selected = p;
                selectNode(p);
                CPoint cli = scr;
                ScreenToClient(&cli);
                surfaceDocMenu(cli);
            };
            // Ctrl+拖动 = 复制：宿主负责压栈、唯一命名、选中副本；随后让副本接着跟手
            c->onCtrlDragCopy = [this](const Path& p, CPoint pressPt) {
                beginCopyDrag(p, pressPt);
            };
            c->onEditEnd = [this]() {
                XamlElement* root = m_doc.root();
                if (root != nullptr) m_doc.layout(0, 0, root->w, root->h);
                XamlElement* node = Resolve(m_doc.root(), m_selected);
                fillProperties(node);
                updateStatus();
                if (m_canvas != nullptr) m_canvas->Invalidate();
            };
            m_canvas = c;
            w = c;
        }
    }
    // Window/Grid/MenuBar/ToolBar/PropertyGrid/StatusBar/MenuItem：纯容器，不建 HWND

    if (w != nullptr)
    {
        m_ctrlByIndex[index] = w;
        m_ctrlByNode[el] = w;
        COLORREF bg = ParseColor(CString(el->get(L"Background").c_str()));
        COLORREF fg = ParseColor(CString(el->get(L"Foreground").c_str()));
        if (bg != (COLORREF)-1) m_bgByHwnd[w->m_hWnd] = bg;
        if (fg != (COLORREF)-1) m_fgByHwnd[w->m_hWnd] = fg;
    }

    // 递归子节点
    for (auto& child : el->children)
        createControlForElement(&child);
}

// 对象树 / 控件箱已由 D2D 表面每帧按 buildDocRows / buildWidgetRows 自绘，
// 不再有 CTreeCtrl 需要刷新；保留空实现供既有调用点复用。
void DesignerPaneView::rebuildTree() {}
void DesignerPaneView::populateWidgetBox() {}

void DesignerPaneView::applyLayoutToControls()
{
    if (m_ui.root() == nullptr) return;

    // 单表面模式：绝大多数节点已不是窗口，但 canvasHost 仍是一块真实子窗口，
    // 它的位置必须跟着 XAML 算出来的 preview 走，否则会盖住新画出来的邻居。
    XamlElement* root = m_ui.root();
    for (auto& kv : m_ctrlByNode)
    {
        XamlElement* el = kv.first;
        CRect rc(el->previewX, el->previewY,
                 el->previewX + el->previewW, el->previewY + el->previewH);
        kv.second->MoveWindow(rc);
    }
    Invalidate(FALSE);
}

// ===========================================================================
// 面板分割条（Splitter）拖动调整大小
// 分割条 Static 命中测试穿透到父窗口，故由 DesignerPaneView 捕获鼠标处理
// ===========================================================================
void DesignerPaneView::setupSplitters()
{
    m_splitters.clear();
    XamlElement* root = m_ui.root();
    if (root == nullptr) return;

    std::function<void(XamlElement&, XamlElement*)> walk;
    walk = [&](XamlElement& el, XamlElement* parent)
    {
        if ((el.tag == L"Splitter" || el.tag == L"Separator") && parent != nullptr)
        {
            // 仅处理竖分割条：属于含 5 列的主体 Grid，col1(左栏-画布)/col3(画布-右栏)
            if (parent->gridCols.size() >= 5)
            {
                SplitterInfo s;
                s.grid = parent;
                bool ok = false;
                if (el.col == 1)      { s.index = 0; s.sign = 1;  ok = true; } // 左栏-画布
                else if (el.col == 3) { s.index = 4; s.sign = -1; ok = true; } // 画布-右栏
                if (ok)
                {
                    s.el = &el;
                    m_splitters.push_back(s);   // 分割条不是窗口，按 preview 矩形命中
                }
            }
            // 主网格的行分隔条：主体区(*)/分隔条(6)/代码坞(160)。拖上→代码坞变高。
            else if (parent->gridRows.size() >= 6 && el.row == 3)
            {
                SplitterInfo s;
                s.grid = parent;
                s.vertical = true;
                s.index = 4;       // 代码坞所在行
                s.sign = -1;       // 向上拖（pt.y 变小）→ 高度变大
                s.el = &el;
                m_splitters.push_back(s);
            }
            // 右栏内部的行分隔条：对象检查器(34+150)/分隔条(6)/属性编辑器(34+*)。拖下→对象树变高。
            else if (parent->gridRows.size() >= 5 && el.row == 2)
            {
                SplitterInfo s;
                s.grid = parent;
                s.vertical = true;
                s.index = 1;       // 对象树所在行
                s.sign = 1;        // 向下拖（pt.y 变大）→ 对象树变高
                s.el = &el;
                m_splitters.push_back(s);
            }
        }
        for (auto& c : el.children)
            walk(c, &el);
    };
    walk(*root, nullptr);
}



void DesignerPaneView::splitterDrag(int idx, CPoint pt)
{
    SplitterInfo& s = m_splitters[idx];
    int delta = s.vertical ? (s.sign > 0 ? (pt.y - s.startPt.y) : (s.startPt.y - pt.y))
                           : (s.sign > 0 ? (pt.x - s.startPt.x) : (s.startPt.x - pt.x));
    int newW = s.startValue + delta;
    if (s.vertical)
    {
        CRect cli;
        GetClientRect(&cli);
        if (s.grid->gridRows.size() >= 6)
        {
            // 主体 ↔ 代码坞：高度钳制，上边主体至少留 200
            if (newW < 80) newW = 80;
            if (newW > cli.Height() - 200) newW = cli.Height() - 200;
        }
        else
        {
            // 右栏 对象检查器 ↔ 属性编辑器：给属性表至少留 120，对象树至少 70
            const int headerSum = 34 + 6 + 34;            // 两个标题条 + 分隔条自身
            int maxH = (int)s.grid->previewH - headerSum - 120;
            if (maxH < 70) maxH = 70;
            if (newW < 70) newW = 70;
            if (newW > maxH) newW = maxH;
        }
        s.grid->gridRows[s.index].second = std::to_wstring(newW);
    }
    else
    {
        if (newW < 60) newW = 60;
        s.grid->gridCols[s.index].second = std::to_wstring(newW);
    }
    // 节流：拖动时限制 relayout 频率，避免全量重排造成滞后
    DWORD now = ::GetTickCount();
    if (now - m_splitDragLastMove < 16) return;
    m_splitDragLastMove = now;
    relayoutUi();
    // 拖动中：局部重绘画布及其周围，清除画布移动后边缘残留的深色黑线
    CRect cr;
    if (m_canvas != nullptr)
    {
        m_canvas->GetWindowRect(&cr);
        ScreenToClient(&cr);
        cr.InflateRect(8, 8);
        RedrawWindow(&cr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);
    }
}

void DesignerPaneView::splitterEnd(int idx)
{
    m_dragSplit = -1;
    ::ReleaseCapture();
    relayoutUi();          // 释放后最终布局一次落定
    // 彻底重绘：擦除背景(OnEraseBkgnd) + 重绘所有子窗口(含画布)，清除拖动后边缘残留黑线
    RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);
}

void DesignerPaneView::relayoutUi()
{
    CRect rc;
    GetClientRect(&rc);
    if (m_ui.root() != nullptr && rc.Width() > 0 && rc.Height() > 0)
    {
        m_ui.layout(0, 0, rc.Width(), rc.Height());
        applyLayoutToControls();
    }
}

// 命中竖分割条：基于分割条窗口自身位置计算（相对 DesignerPaneView 客户区）

void DesignerPaneView::OnLButtonDown(UINT nFlags, CPoint pt)
{
    (void)nFlags;
    surfaceLButtonDown(pt);
}

// 双击的第二次按下系统发的是 WM_LBUTTONDBLCLK，不是 WM_LBUTTONDOWN：
// 不接这条消息，属性表的"双击值就地编辑"永远收不到第二下（表现为双击没反应）。
void DesignerPaneView::OnLButtonDblClk(UINT nFlags, CPoint pt)
{
    (void)nFlags;
    surfaceLButtonDown(pt);
}

BOOL DesignerPaneView::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
    CPoint pt;
    ::GetCursorPos(&pt);
    ScreenToClient(&pt);
    for (auto& r0 : m_surfSplitterRects)
    {
        RECT r = r0;
        if (r.right - r.left < 5) { r.left -= 2; r.right += 2; }
        else { r.top -= 2; r.bottom += 2; }
        if (pt.x >= r.left && pt.x < r.right && pt.y >= r.top && pt.y < r.bottom)
        {
            // 横向窄条 → 上下拖；竖向窄条 → 左右拖
            ::SetCursor(::LoadCursor(nullptr, (r.bottom - r.top) <= (r.right - r.left)
                                         ? IDC_SIZENS : IDC_SIZEWE));
            return TRUE;
        }
    }
    return CWnd::OnSetCursor(pWnd, nHitTest, message);
}

void DesignerPaneView::OnSize(UINT type, int cx, int cy)
{
    CWnd::OnSize(type, cx, cy);
    resizeTo(CRect(0, 0, cx, cy));
}

// 宿主可重复调用：停靠/浮动切换、Arx 侧栏改宽，都走同一条重排路径。
void DesignerPaneView::resizeTo(const CRect& rc)
{
    if (m_ui.root() == nullptr || rc.Width() <= 0 || rc.Height() <= 0)
        return;

    applyResponsiveLayout(rc.Width());
    m_ui.layout(0, 0, rc.Width(), rc.Height());
    applyLayoutToControls();
    // 尺寸变了以后整块重画一次：只让子窗口自己 Invalidate 的话，画布/代码坞缩小后
    // 腾出来的那块区域不会有人擦，上一帧的表格/控件像素就留在画布工作台上（残留画面）。
    RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
}

// 设计器面板在 CAD 侧栏里常被拖到 400~700 宽。两侧栏是固定列宽，
// 宽度不够时画布会被挤成一条，所以按档位收窄（只在档位变化时改文档模型）。
void DesignerPaneView::applyResponsiveLayout(int width)
{
    XamlElement* root = m_ui.root();
    if (root == nullptr) return;

    XamlElement* grid = nullptr;
    for (auto& child : root->children)
    {
        if (_wcsicmp(child.name.c_str(), L"rootGrid") == 0)
        {
            grid = &child;
            break;
        }
    }
    // 列布局：0=控件箱 1=Splitter 2=画布 3=Splitter 4=对象检查器+属性
    if (grid == nullptr || grid->gridCols.size() < 5) return;

    int left = 248, right = 308;
    if (width >= 1200)      { left = 248; right = 308; }
    else if (width >= 950)  { left = 210; right = 264; }
    else if (width >= 760)  { left = 184; right = 220; }
    else if (width >= 560)  { left = 160; right = 0;   }   // 右栏先让位给画布
    else                    { left = 148; right = 0;   }

    if (left == m_respLeft && right == m_respRight) return;
    m_respLeft = left;
    m_respRight = right;

    grid->gridCols[0].second = std::to_wstring(left);
    grid->gridCols[4].second = std::to_wstring(right);
}

// 以子窗口方式创建。用 AfxRegisterWndClass 注册带 CS_DBLCLKS 的类：
// 属性表就地编辑靠双击判定（m_surfClickTick），没有 CS_DBLCLKS 收不到双击语义。
BOOL DesignerPaneView::create(CWnd* parent, const CRect& rect, UINT id)
{
    LPCTSTR cls = AfxRegisterWndClass(CS_DBLCLKS, ::LoadCursor(nullptr, IDC_ARROW), nullptr, nullptr);
    return Create(cls, _T(""), WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                  rect, parent, id);
}

UINT DesignerPaneView::ensureAction(const CString& name)
{
    if (name.IsEmpty()) return 0;
    auto it = m_actionCmd.find(name);
    if (it != m_actionCmd.end()) return it->second;
    UINT id = m_nextCmd++;
    m_actionCmd[name] = id;
    m_cmdAction[id] = name;
    return id;
}

namespace
{
    // MenuBar 位于主 Grid 的 Row 0 内，需在整棵树中递归查找
    XamlElement* findMenuBar(XamlElement& el)
    {
        if (el.tag == L"MenuBar") return &el;
        for (auto& c : el.children)
        {
            if (XamlElement* r = findMenuBar(c)) return r;
        }
        return nullptr;
    }
}

void DesignerPaneView::buildMenu()
{
    XamlElement* root = m_ui.root();
    if (root == nullptr) return;
    XamlElement* bar = findMenuBar(*root);
    if (bar == nullptr) return;
    if (m_menu.m_hMenu != nullptr) m_menu.DestroyMenu();
    m_menu.CreateMenu();

    for (auto& top : bar->children)
    {
        CMenu popup;
        popup.CreatePopupMenu();
        for (auto& child : top.children)
        {
            UINT cmd = ensureAction(CString(child.name.c_str()));
            popup.AppendMenuW(MF_STRING, cmd, CString(child.text.c_str()));
        }
        m_menu.AppendMenuW(MF_POPUP, (UINT_PTR)popup.Detach(), CString(top.text.c_str()));
    }
    // 系统菜单只能挂在顶层框架上。作为子窗口（CDockablePane / CAdUiPalette）承载时，
    // SetMenu 会 ASSERT 且菜单栏根本无处显示；命令改由工具栏按钮经同一套 ensureAction 分发。
    if ((GetStyle() & WS_CHILD) == 0)
    {
        SetMenu(&m_menu);
        DrawMenuBar();
    }
}

DesignerPaneView::Action DesignerPaneView::actionOf(const CString& name)
{
    // 名字 -> 动作的静态表：新增按钮只往表里加一行，不再堆 if/else
    static const std::unordered_map<std::wstring, Action> kTable = {
        { L"btnOpen", Action::Open },          { L"mnuOpen", Action::Open },
        { L"btnSave", Action::Save },          { L"mnuSave", Action::Save },
        { L"mnuExit", Action::Exit },
        { L"btnNew", Action::New },            { L"mnuNew", Action::New },
        { L"btnAddButton", Action::AddButton },{ L"mnuAddButton", Action::AddButton },
        { L"btnAddLabel", Action::AddLabel },  { L"mnuAddLabel", Action::AddLabel },
        { L"btnApply", Action::Apply },
        { L"btnDelete", Action::Delete },      { L"mnuDelete", Action::Delete },
        { L"btnGrid", Action::Grid },          { L"mnuGrid", Action::Grid },
        { L"btnPreview", Action::Preview },    { L"mnuPreview", Action::Preview },
        { L"btnCheck", Action::Check },        { L"mnuCheck", Action::Check },
        { L"btnCodeToggle", Action::CodeToggle },
        { L"btnCodeCopy", Action::CodeCopy },
        { L"btnUndo", Action::Undo },          { L"mnuUndo", Action::Undo },
        { L"btnRedo", Action::Redo },          { L"mnuRedo", Action::Redo },
        { L"btnCut", Action::Cut },            { L"mnuCut", Action::Cut },
        { L"btnCopy", Action::Copy },          { L"mnuCopy", Action::Copy },
        { L"btnPaste", Action::Paste },        { L"mnuPaste", Action::Paste },
        { L"btnRuler", Action::Ruler },        { L"mnuRuler", Action::Ruler },
        { L"btnZoomIn", Action::ZoomIn },      { L"mnuZoomIn", Action::ZoomIn },
        { L"btnZoomOut", Action::ZoomOut },    { L"mnuZoomOut", Action::ZoomOut },
        { L"btnZoomFit", Action::ZoomFit },    { L"mnuZoomFit", Action::ZoomFit },
        { L"btnAlignLeft", Action::AlignLeft },       { L"mnuAlignLeft", Action::AlignLeft },
        { L"btnAlignHCenter", Action::AlignHCenter }, { L"mnuAlignHCenter", Action::AlignHCenter },
        { L"btnAlignRight", Action::AlignRight },     { L"mnuAlignRight", Action::AlignRight },
        { L"btnAlignTop", Action::AlignTop },         { L"mnuAlignTop", Action::AlignTop },
        { L"btnAlignVCenter", Action::AlignVCenter }, { L"mnuAlignVCenter", Action::AlignVCenter },
        { L"btnAlignBottom", Action::AlignBottom },   { L"mnuAlignBottom", Action::AlignBottom },
        { L"btnDistH", Action::DistributeH },          { L"mnuDistH", Action::DistributeH },
        { L"btnDistV", Action::DistributeV },          { L"mnuDistV", Action::DistributeV },
        { L"mnuAbout", Action::About },
    };
    auto it = kTable.find(std::wstring(name));
    return (it == kTable.end()) ? Action::None : it->second;
}

void DesignerPaneView::dispatchAction(Action a, const CString& name)
{
    // 临时诊断（PYARX_DESIGN_DEBUG=1）：记录动作派发，排查 Ctrl+C/V 没反应
    if (a == Action::Copy || a == Action::Cut || a == Action::Paste)
    {
        static const bool on = []() {
            char buf[8] = { 0 };
            return ::GetEnvironmentVariableA("PYARX_DESIGN_DEBUG", buf, sizeof(buf)) > 0;
        }();
        if (on)
        {
            std::FILE* f = nullptr;
            if (fopen_s(&f, "G:\\PyArx\\log\\paste_diag.log", "a") == 0 && f != nullptr)
            {
                fprintf(f, "[action] %d selLen=%d\n", (int)a, (int)m_selected.size());
                fclose(f);
            }
        }
    }
    switch (a)
    {
    case Action::Open: onOpen(); break;
    case Action::Save: onSave(); break;
    case Action::Exit: onExit(); break;
    case Action::New: onNew(); break;
    case Action::AddButton: addControl(L"Button"); break;
    case Action::AddLabel: addControl(L"TextBlock"); break;
    case Action::Apply: onApply(); break;
    case Action::Delete: onDelete(); break;
    case Action::Grid: toggleGrid(); break;
    case Action::Preview: previewCurrent(); break;
    case Action::Check: checkCurrent(); break;
    case Action::CodeToggle: toggleCodeDock(); break;
    case Action::CodeCopy: copyXamlToClipboard(); break;
    case Action::Undo: undo(); break;
    case Action::Redo: redo(); break;
    case Action::Cut: onCut(); break;
    case Action::Copy: onCopy(); break;
    case Action::Paste: onPaste(); break;
    case Action::Ruler: toggleRuler(); break;
    case Action::ZoomIn: zoomBy(1.25f); break;
    case Action::ZoomOut: zoomBy(0.8f); break;
    case Action::ZoomFit: zoomFit(); break;
    case Action::AlignLeft: onAlign(L"left"); break;
    case Action::AlignHCenter: onAlign(L"hcenter"); break;
    case Action::AlignRight: onAlign(L"right"); break;
    case Action::AlignTop: onAlign(L"top"); break;
    case Action::AlignVCenter: onAlign(L"vcenter"); break;
    case Action::AlignBottom: onAlign(L"bottom"); break;
    case Action::DistributeH: onDistribute(true); break;
    case Action::DistributeV: onDistribute(false); break;
    case Action::About: onAbout(); break;
    case Action::None:
    default:
    {
        // 兜底：btnAddXxx / btnXXX 这类"控件箱直加"按钮，按 catalog 的 tag 直接建
        CString t = TagFromToolName(name);
        if (!t.IsEmpty()) addControl(t);
        break;
    }
    }
}

void DesignerPaneView::OnDynamicCmd(UINT id)
{
    auto it = m_cmdAction.find(id);
    if (it == m_cmdAction.end()) return;
    dispatchAction(actionOf(it->second), it->second);
}

void DesignerPaneView::undoOrRedo(bool undo)
{
    dispatchAction(undo ? Action::Undo : Action::Redo, CString());
}

void DesignerPaneView::clipboardAction(UINT vk)
{
    if (vk == 'C' || vk == 'c') dispatchAction(Action::Copy, CString());
    else if (vk == 'X' || vk == 'x') dispatchAction(Action::Cut, CString());
    else dispatchAction(Action::Paste, CString());
}

void DesignerPaneView::onOpen()
{
    CFileDialog dlg(TRUE, _T("xaml"), nullptr, OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
        _T("XAML/XML (*.xaml;*.xml)|*.xaml;*.xml|所有文件 (*.*)|*.*||"), this);
    if (dlg.DoModal() != IDOK)
        return;
    openFile(dlg.GetPathName());
}

bool DesignerPaneView::openFile(const CString& path)
{
    std::wstring err;
    if (!m_doc.loadFile((LPCWSTR)path, err))
    {
        AfxMessageBox(CString(err.c_str()), MB_ICONERROR);
        return false;
    }
    // 布局编辑文档到画布
    XamlElement* root = m_doc.root();
    if (root != nullptr)
        m_doc.layout(0, 0, root->w, root->h);
    m_canvas->setDocument(&m_doc);
    m_selected.clear();
    m_docPath = path;
    rebuildTree();
    CString title;
    title.Format(_T("PyMfc UI Designer - %s"), (LPCWSTR)path);
    SetWindowText(title);
    updateStatus();
    return true;
}

void DesignerPaneView::onSave()
{
    if (m_doc.root() == nullptr)
        return;

    // 已有路径 → 原地保存，不弹对话框；新建（未命名）文档才弹出保存对话框选路径
    const bool isNewDoc = m_docPath.IsEmpty();
    CString path = m_docPath;
    if (isNewDoc)
    {
        CFileDialog dlg(FALSE, _T("xaml"), _T("ui.xaml"),
            OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY,
            _T("XAML/XML (*.xaml;*.xml)|*.xaml;*.xml|所有文件 (*.*)|*.*||"), this);
        if (dlg.DoModal() != IDOK)
            return;
        path = dlg.GetPathName();
        if (path.IsEmpty())
            return;
    }

    std::wstring err;
    if (!m_doc.save((LPCWSTR)path, err))
    {
        AfxMessageBox(CString(err.c_str()), MB_ICONERROR);
        return;
    }

    m_docPath = path;

    // 路径变了（新建→保存），状态栏与标题（updateStatus 里统一写）跟着换
    updateStatus();
    Invalidate(FALSE);

    // 只有"新建第一次保存"给一次确认；之后原地保存不再弹窗打扰
    if (isNewDoc)
        AfxMessageBox(_T("保存成功"), MB_ICONINFORMATION);
}

void DesignerPaneView::onExit()
{
    PostMessage(WM_CLOSE);
}

void DesignerPaneView::addControl(const CString& tag)
{
    int w = 100, h = 28;
    DefaultSize(tag.GetString(), w, h);
    CString name;
    name.Format(L"%s%d", (LPCWSTR)tag, m_nextId++);
    addControlAt(tag, name, 20, 20, w, h);
}

// 画布左侧竖向 Draw 工具条：点中图元后把它记成"待放置 tag"，
// 之后在画布上按下拖动就按拖出的矩形创建（与控件箱单击放置同一条 jig 路径）。
void DesignerPaneView::pickCreateTag(const std::wstring& tag)
{
    if (tag.empty()) return;
    m_pendingCreateTag = CString(tag.c_str());
    m_surfStatus[0].Format(L"拖动鼠标绘制 %s", tag.c_str());
    Invalidate(FALSE);
}

void DesignerPaneView::addControlAt(const CString& tag, const CString& name, int x, int y, int w, int h)
{    pushUndo();
    XamlElement* root = m_doc.root();
    if (root == nullptr)
    {
        // 新建空文档：根 Window
        std::wstring xaml =
            L"<Window xmlns:x=\"http://schemas.microsoft.com/winfx/2006/xaml\" "
            L"Name=\"MainWindow\" Width=\"640\" Height=\"420\"></Window>";
        std::wstring err;
        if (!m_doc.loadString(xaml.c_str(), err))
        {
            AfxMessageBox(CString(err.c_str()), MB_ICONERROR);
            return;
        }
        root = m_doc.root();
    }

    // 选中容器
    XamlElement* parent = Resolve(root, m_selected);
    if (parent == nullptr) parent = root;
    if (!IsContainer(parent->tag))
    {
        int idx;
        XamlElement* p = ParentOf(root, m_selected, idx);
        if (p != nullptr) parent = p;
    }

    // 画布上给的 x/y 是"画布绝对坐标"；写进容器子元素时必须换算成"相对父容器"，
    // 否则父容器有偏移时，新控件会整体漂到父容器之外（"容器中的对象没有做矩阵计算"）。
    if (parent != root)
    {
        x -= parent->previewX;
        y -= parent->previewY;
        if (x < 0) x = 0;
        if (y < 0) y = 0;
    }

    // 图元没有文本：别把 tag 名塞进 Content，否则 XAML 里多一条无意义的 Content="Rectangle"
    const std::wstring addText = uidesign::IsShapeTag(std::wstring(tag.GetString()))
                                 ? std::wstring() : std::wstring(tag.GetString());
    XamlElement* node = AddChild(parent, tag.GetString(), name.GetString(),
                                 addText, x, y, w, h);
    // 几何必须写进 XAML 的 Left/Top/Width/Height，否则只活在内存里，存盘重开就没了。
    // Grid 子节点按 Row/Column 占格，不写 Left/Top（写了会与网格冲突）。
    if (node != nullptr)
    {
        const bool gridParent = _wcsicmp(parent->tag.c_str(), L"Grid") == 0;
        if (gridParent)
        {
            node->hasX = false;
            node->hasY = false;
        }
        else
        {
            node->set(L"Left", std::to_wstring(x));
            node->set(L"Top", std::to_wstring(y));
        }
        node->set(L"Width", std::to_wstring(w));
        node->set(L"Height", std::to_wstring(h));
    }
    Path sel;
    if (node != nullptr)
        sel = parentPath(root, m_selected, parent, node);
    m_doc.layout(0, 0, root->w, root->h);
    m_canvas->setDocument(&m_doc);
    rebuildTree();
    selectNode(sel);
    updateStatus();
}

// 计算 child 在 parent 下的路径（parent 的路径 + child 的索引）
Path DesignerPaneView::parentPath(XamlElement* root, const Path& selPath, XamlElement* parent, XamlElement* child)
{
    Path p;
    // 找 parent 的路径
    uidesign::Walk(root, [&](const Path& pp, XamlElement& el) {
        if (p.empty() && &el == parent) p = pp;
    });
    for (int i = 0; i < (int)parent->children.size(); ++i)
        if (&parent->children[i] == child) { p.push_back(i); break; }
    return p;
}

CString DesignerPaneView::widgetTagFor(const CString& display)
{
    // 由 XML 控件目录驱动（参照 wxFormBuilder：display -> objectinfo.tag）
    const uidesign::CtrlTemplate* t = m_catalog.byDisplay(display);
    if (t != nullptr) return t->tag;
    return L"";
}





void DesignerPaneView::OnMouseMove(UINT nFlags, CPoint pt)
{
    surfaceMouseMove(pt, nFlags);
}

void DesignerPaneView::OnLButtonUp(UINT nFlags, CPoint pt)
{
    (void)nFlags;
    surfaceLButtonUp(pt);
}


void DesignerPaneView::OnDrawItem(int id, LPDRAWITEMSTRUCT dis)
{
    if (dis->CtlType == ODT_MENU)
    {
        CDC dc;
        dc.Attach(dis->hDC);
        bool hot = (dis->itemState & ODS_SELECTED) != 0;
        COLORREF bg = hot ? RGB(74, 105, 132) : RGB(50, 65, 75);
        dc.FillSolidRect(&dis->rcItem, bg);
        CString text = (LPCWSTR)dis->itemData;
        dc.SetTextColor(RGB(220, 220, 220));
        dc.SetBkMode(TRANSPARENT);
        dc.DrawText(text, &dis->rcItem, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        dc.Detach();
        return;
    }
    CWnd::OnDrawItem(id, dis);
}

void DesignerPaneView::OnMeasureItem(int id, LPMEASUREITEMSTRUCT mis)
{
    if (mis->CtlType == ODT_MENU)
    {
        mis->itemHeight = 22;
        mis->itemWidth = 120;
        return;
    }
    CWnd::OnMeasureItem(id, mis);
}

void DesignerPaneView::selectNode(const Path& p)
{
    m_selected = p;
    m_selSet.clear();
    m_docRevealSel = true;
    if (!p.empty()) m_selSet.push_back(p);
    if (m_canvas != nullptr) m_canvas->setSelected(p);
    fillProperties(Resolve(m_doc.root(), p));
    updateStatus();
    revealCodeForSelected();     // 点中控件 / 树里选行：代码坞自动滚到该节点的源码位置
}

void DesignerPaneView::selectMulti(const std::vector<Path>& v)
{
    m_selSet = v;
    m_selected = v.empty() ? Path() : v.front();
    m_docRevealSel = true;
    if (m_canvas != nullptr) m_canvas->setSelectionSet(v);
    fillProperties(Resolve(m_doc.root(), m_selected));
    updateStatus();
    revealCodeForSelected();
}

// Ctrl+A 全选：选中的是容器就选它的全部子节点，否则（选中叶子 / 无选中）选根窗体的全部子节点。
// 只选一层——设计器里"全选"指当前这一排控件，把整棵树的子孙全选上既看不见也没法用。
void DesignerPaneView::selectAllChildren()
{
    XamlElement* root = m_doc.root();
    if (root == nullptr) return;

    Path container;
    XamlElement* node = Resolve(root, m_selected);
    if (node != nullptr && node != root && uidesign::IsContainer(node->tag))
        container = m_selected;
    else if (node != nullptr && node != root)
    {
        // 叶子：取它所在的容器（与粘贴的落点口径一致）
        int idx = 0;
        XamlElement* par = uidesign::ParentOf(root, m_selected, idx);
        if (par != nullptr)
        {
            container = m_selected;
            container.pop_back();
        }
    }

    XamlElement* box = Resolve(root, container);
    if (box == nullptr) return;

    std::vector<Path> sel;
    for (int i = 0; i < (int)box->children.size(); ++i)
    {
        Path p = container;
        p.push_back(i);
        sel.push_back(p);
    }
    if (sel.empty())
    {
        clearSelection();
        notImpl(L"当前容器里没有可选的控件");
        return;
    }
    selectMulti(sel);
    Invalidate(FALSE);
}

// 取消选择：画布空白单击 / Esc 都走这里（对象检查器清空、属性表回到"无选中"）
void DesignerPaneView::clearSelection()
{
    m_selected = Path();
    m_selSet.clear();
    m_docRevealSel = false;
    if (m_canvas != nullptr)
    {
        m_canvas->setSelected(Path());
        m_canvas->clearSelection();
    }
    m_surfSelPropRow = -1;
    fillProperties(nullptr);
}

LRESULT DesignerPaneView::OnCanvasSelectMulti(WPARAM, LPARAM lParam)
{
    auto* v = reinterpret_cast<std::vector<Path>*>(lParam);
    if (v == nullptr) return 0;
    selectMulti(*v);
    delete v;
    return 0;
}

// 代码坞是"看"的：选中节点后把视口滚到它的开始标签那一行（不动光标、不进撤销栈）
void DesignerPaneView::revealCodeForSelected()
{
    if (m_codeEditor == nullptr || m_codeEditor->hwnd() == nullptr) return;
    XamlElement* node = Resolve(m_doc.root(), m_selected);
    if (node == nullptr) return;

    std::wstring text = m_codeEditor->text();
    if (text.empty()) return;

    // 用 "<Tag ... Name=\"X\"" 定位；没有 Name 时退化到 "<Tag" 的下一处
    std::wstring key = L"<" + node->tag;
    if (!node->name.empty()) key += L" ";
    std::size_t pos = text.find(key);
    if (!node->name.empty())
    {
        std::wstring nm = L"Name=\"" + node->name + L"\"";
        std::size_t p2 = text.find(nm, 0);
        while (p2 != std::wstring::npos)
        {
            // 找到 Name 前最近的开始标签，确认标签与节点一致
            std::size_t lt = text.rfind(L'<', p2);
            if (lt != std::wstring::npos && lt < p2)
            {
                std::size_t gt = text.find(L'>', p2);
                std::wstring frag = text.substr(lt, (gt == std::wstring::npos ? p2 : gt) - lt);
                if (frag.find(node->tag) != std::wstring::npos) { pos = lt; break; }
            }
            p2 = text.find(nm, p2 + 1);
        }
    }
    if (pos == std::wstring::npos) return;

    int line = 0;
    for (std::size_t i = 0; i < pos && i < text.size(); ++i)
        if (text[i] == L'\n') ++line;
    m_codeEditor->revealLine(line);
}



void DesignerPaneView::fillProperties(XamlElement* node)
{
    // 属性表由表面每帧按 buildPropRows 自绘，这里只需要触发重画
    (void)node;
    Invalidate(FALSE);
}

// ===== Property Editor 值列就地编辑 =====

BOOL DesignerPaneView::PreTranslateMessage(MSG* pMsg)
{
    if (pMsg != nullptr && pMsg->message == WM_KEYDOWN)
    {
        static const bool on = []() {
            char buf[8] = { 0 };
            return ::GetEnvironmentVariableA("PYARX_DESIGN_DEBUG", buf, sizeof(buf)) > 0;
        }();
        if (on)
        {
            std::FILE* f = nullptr;
            if (fopen_s(&f, "G:\\PyArx\\log\\paste_diag.log", "a") == 0 && f != nullptr)
            {
                fprintf(f, "[view-key] hwnd=%p vk=%02X ctrl=%d focus=%p\n",
                        (void*)pMsg->hwnd, (unsigned)pMsg->wParam,
                        (::GetKeyState(VK_CONTROL) & 0x8000) ? 1 : 0, (void*)::GetFocus());
                fclose(f);
            }
        }
    }
    // 只抢回会被 MFC 框架预翻译吃掉的那几个键：
    //   * 回车/Tab —— CFrameWnd 会当对话框导航处理，消息到不了控件（"换行换不了"）；
    //   * Ctrl+A/C/X/V/Z/Y —— 会先被框架加速键查走（"Ctrl+C 不灵"）。
    // 其余按键保持原样派发，让 TranslateMessage/输入法照常工作，否则中文输入法彻底进不来。
    if (pMsg != nullptr && pMsg->message == WM_KEYDOWN && m_codeEditor != nullptr &&
        m_codeEditor->hwnd() != nullptr &&
        (pMsg->hwnd == m_codeEditor->hwnd() || ::GetFocus() == m_codeEditor->hwnd()) &&
        !m_codeEditor->isImeComposing())
    {
        const WPARAM vk = pMsg->wParam;
        const bool ctrl = (::GetKeyState(VK_CONTROL) & 0x8000) != 0;
        const bool rescue = (vk == VK_RETURN || vk == VK_TAB) ||
                            (ctrl && (vk == 'A' || vk == 'C' || vk == 'X' || vk == 'V' ||
                                      vk == 'Z' || vk == 'Y' || vk == VK_BACK || vk == VK_DELETE));
        if (rescue)
        {
            m_codeEditor->handleKeyMessage(vk, pMsg->lParam);
            return TRUE;
        }
    }

    if (pMsg != nullptr && m_propEdit.GetSafeHwnd() != nullptr &&
        pMsg->hwnd == m_propEdit.m_hWnd && pMsg->message == WM_KEYDOWN)
    {
        // 内联编辑走表面模式的提交/取消
        if (pMsg->wParam == VK_RETURN) { commitPropEditSurface(); return TRUE; }
        if (pMsg->wParam == VK_ESCAPE) { cancelPropEditSurface(); return TRUE; }
    }
    // 属性内联编辑框已经隐藏（提交/取消后残留焦点）时，把焦点还给画布并让快捷键继续生效，
    // 否则 Ctrl+C/X/V、Delete 会一直落在一个看不见的编辑框里，表现就是"Ctrl+V 没反应"。
    if (pMsg != nullptr && m_propEdit.GetSafeHwnd() != nullptr &&
        pMsg->hwnd == m_propEdit.m_hWnd && !::IsWindowVisible(m_propEdit.m_hWnd))
    {
        if (m_canvas != nullptr && m_canvas->GetSafeHwnd() != nullptr)
            ::SetFocus(m_canvas->GetSafeHwnd());
        else
            ::SetFocus(m_hWnd);
    }
    // Delete 删除选中节点：焦点在本视图或其子窗口（画布 / 对象树 / 控件箱）时生效，
    // 属性编辑框内的 Delete 仍归编辑框自己（退格/删字符）。
    if (pMsg != nullptr && pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_DELETE &&
        handleDeleteKey(pMsg->hwnd))
        return TRUE;
    // Ctrl+Z / Ctrl+Y：设计器文档的撤销 / 重做（与工具栏 btnUndo / btnRedo 同一套实现）。
    // 焦点在代码编辑器或属性内联编辑框时让给它们自己处理。
    if (pMsg != nullptr && pMsg->message == WM_KEYDOWN &&
        (pMsg->wParam == 'Z' || pMsg->wParam == 'Y' || pMsg->wParam == 'z' || pMsg->wParam == 'y'))
    {
        const bool ctrl = (::GetKeyState(VK_CONTROL) & 0x8000) != 0;
        const bool inCode = (m_codeEditor != nullptr && m_codeEditor->hwnd() != nullptr &&
                             (pMsg->hwnd == m_codeEditor->hwnd() || ::GetFocus() == m_codeEditor->hwnd()));
        const bool inProp = (m_propEdit.GetSafeHwnd() != nullptr && pMsg->hwnd == m_propEdit.m_hWnd &&
                             ::IsWindowVisible(m_propEdit.m_hWnd));
        if (ctrl && !inCode && !inProp)
        {
            const bool undoIt = (pMsg->wParam == 'Z' || pMsg->wParam == 'z');
            dispatchAction(undoIt ? Action::Undo : Action::Redo, CString());
            return TRUE;
        }
    }
    // Ctrl+S 保存：与工具栏 btnSave / 文件菜单"保存"同一套实现
    if (pMsg != nullptr && pMsg->message == WM_KEYDOWN &&
        (pMsg->wParam == 'S' || pMsg->wParam == 's'))
    {
        const bool ctrl = (::GetKeyState(VK_CONTROL) & 0x8000) != 0;
        const bool inCode = (m_codeEditor != nullptr && m_codeEditor->hwnd() != nullptr &&
                             (pMsg->hwnd == m_codeEditor->hwnd() || ::GetFocus() == m_codeEditor->hwnd()));
        if (ctrl && !inCode)
        {
            dispatchAction(Action::Save, CString());
            return TRUE;
        }
    }
    // Ctrl+A 全选：画布 / 对象树层面一次选中一层控件（配合多选夹点、批量删除）。
    // 代码区与属性内联编辑框里的 Ctrl+A 是文本全选，让给它们自己。
    if (pMsg != nullptr && pMsg->message == WM_KEYDOWN &&
        (pMsg->wParam == 'A' || pMsg->wParam == 'a'))
    {
        const bool ctrl = (::GetKeyState(VK_CONTROL) & 0x8000) != 0;
        const bool inCode = (m_codeEditor != nullptr && m_codeEditor->hwnd() != nullptr &&
                             (pMsg->hwnd == m_codeEditor->hwnd() || ::GetFocus() == m_codeEditor->hwnd()));
        const bool inProp = (m_propEdit.GetSafeHwnd() != nullptr && pMsg->hwnd == m_propEdit.m_hWnd &&
                             ::IsWindowVisible(m_propEdit.m_hWnd));
        if (ctrl && !inCode && !inProp)
        {
            selectAllChildren();
            return TRUE;
        }
    }
    // 剪贴板键：Ctrl+C / Ctrl+X / Ctrl+V（画布与对象树共用同一套节点剪贴板动作）。
    // 焦点在代码编辑器或属性内联编辑框时让给它们自己处理。
    if (pMsg != nullptr && pMsg->message == WM_KEYDOWN &&
        (pMsg->wParam == 'C' || pMsg->wParam == 'X' || pMsg->wParam == 'V' ||
         pMsg->wParam == 'c' || pMsg->wParam == 'x' || pMsg->wParam == 'v'))
    {
        const bool ctrl = (::GetKeyState(VK_CONTROL) & 0x8000) != 0;
        const bool inCode = (m_codeEditor != nullptr && m_codeEditor->hwnd() != nullptr &&
                             (pMsg->hwnd == m_codeEditor->hwnd() || ::GetFocus() == m_codeEditor->hwnd()));
        const bool inProp = (m_propEdit.GetSafeHwnd() != nullptr && pMsg->hwnd == m_propEdit.m_hWnd &&
                             ::IsWindowVisible(m_propEdit.m_hWnd));
        if (ctrl && !inCode && !inProp)
        {
            const WPARAM vk = pMsg->wParam;
            if (vk == 'C' || vk == 'c') dispatchAction(Action::Copy, CString());
            else if (vk == 'X' || vk == 'x') dispatchAction(Action::Cut, CString());
            else dispatchAction(Action::Paste, CString());
            return TRUE;
        }
    }
    // Esc 取消选择（和画布空白单击一致）。代码区内的 Esc 仍归编辑器（关提示/退出输入法组合）
    if (pMsg != nullptr && pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_ESCAPE)
    {
        const bool inCode = (m_codeEditor != nullptr && m_codeEditor->hwnd() != nullptr &&
                             (pMsg->hwnd == m_codeEditor->hwnd() || ::GetFocus() == m_codeEditor->hwnd()));
        if (!inCode)
        {
            clearSelection();
            updateStatus();
            Invalidate(FALSE);
            return TRUE;
        }
    }
    return CWnd::PreTranslateMessage(pMsg);
}

bool DesignerPaneView::handleDeleteKey(HWND hwnd)
{
    // 焦点在代码编辑器里时，Delete 是删字符，不是删节点
    if (m_codeEditor != nullptr && m_codeEditor->hwnd() != nullptr &&
        (::GetFocus() == m_codeEditor->hwnd() || hwnd == m_codeEditor->hwnd() ||
         ::IsChild(m_codeEditor->hwnd(), hwnd)))
        return false;

    if (m_selected.empty()) return false;
    // 属性编辑框内的 Delete 是删字符，不是删节点
    if (m_propEdit.GetSafeHwnd() != nullptr && hwnd == m_propEdit.m_hWnd) return false;
    // hwnd 可以是本视图、本视图的子窗口，也可以是本视图的父（焦点在框架上时由 Frame 转发进来）
    if (hwnd != m_hWnd && !::IsChild(m_hWnd, hwnd) && !(hwnd != nullptr && ::IsChild(hwnd, m_hWnd)))
        return false;
    onDelete();
    return true;
}



void DesignerPaneView::applyPropVal(const CString& key, const CString& val)
{
    XamlElement* node = Resolve(m_doc.root(), m_selected);
    if (node == nullptr) return;

    pushUndo();
    long iv = _wtol(val);
    if (key == L"X" || key == L"Left") node->x = (int)iv;
    else if (key == L"Y" || key == L"Top") node->y = (int)iv;
    else if (key == L"Width") node->w = (iv > 0) ? (int)iv : 1;
    else if (key == L"Height") node->h = (iv > 0) ? (int)iv : 1;
    // Row/Column 必须写回 XAML 的 Grid.Row / Grid.Column，否则只改内存，存盘后重开就丢
    else if (key == L"Row") { node->row = (std::max)(0, (int)iv); node->set(L"Grid.Row", std::to_wstring(node->row)); }
    else if (key == L"Column") { node->col = (std::max)(0, (int)iv); node->set(L"Grid.Column", std::to_wstring(node->col)); }
    else if (key == L"RowSpan") { node->rowSpan = (iv > 0) ? (int)iv : 1; node->set(L"Grid.RowSpan", std::to_wstring(node->rowSpan)); }
    else if (key == L"ColumnSpan") { node->colSpan = (iv > 0) ? (int)iv : 1; node->set(L"Grid.ColumnSpan", std::to_wstring(node->colSpan)); }
    else if (key == L"Name") { node->name = val.GetString(); node->set(L"Name", node->name); }
    else if (key == L"Text" || key == L"Content" || key == L"Header")
    {
        node->text = val.GetString();
        node->set(key.GetString(), node->text);   // 按行上显示的真实属性名写回
    }
    else node->set(key.GetString(), val.GetString());

    // 写回几何属性：只有本来就按坐标定位的节点才写。
    // Grid 子节点只有 Row/Column，凭空补出 Left/Top 会让它下一轮重排从"格子内"变成"绝对坐标"。
    if (node->hasX) node->set(L"Left", std::to_wstring(node->x));
    if (node->hasY) node->set(L"Top", std::to_wstring(node->y));
    if (node->hasW) node->set(L"Width", std::to_wstring(node->w));
    if (node->hasH) node->set(L"Height", std::to_wstring(node->h));

    XamlElement* root = m_doc.root();
    // 属性改动后重算绑定：改 src.Text 会让所有 {Binding src.Text} 的目标同步刷新
    m_doc.applyBindings();
    if (root != nullptr) m_doc.layout(0, 0, root->w, root->h);
    if (m_canvas != nullptr) m_canvas->setDocument(&m_doc);
    rebuildTree();
    selectNode(m_selected);
    fillProperties(Resolve(m_doc.root(), m_selected));
    updateStatus();
    updateXamlView();
    Invalidate(FALSE);   // 没有子窗口会自己重绘，整窗表面要主动重画
}


// 从控件目录里查枚举候选（"Horizontal|Vertical" 形式），空表示自由输入
CString DesignerPaneView::propOptions(const CString& tag, const CString& key)
{
    const CtrlTemplate* t = m_catalog.byTag(tag);
    if (t == nullptr) return CString();
    for (auto& p : t->props)
    {
        if (p.name != key) continue;
        if (!p.options.IsEmpty()) return p.options;
        if (p.type == L"bool") return CString(L"True|False");   // 布尔属性按候选选，不让人手敲
        return CString();
    }
    return CString();
}





void DesignerPaneView::onApply()
{
    // 属性编辑已由表面内联编辑提交（commitPropEditSurface），这里只重算布局并重画
    pushUndo();
    XamlElement* root = m_doc.root();
    if (root != nullptr)
        m_doc.layout(0, 0, root->w, root->h);
    m_doc.applyBindings();
    if (m_canvas != nullptr) m_canvas->setDocument(&m_doc);
    updateStatus();
    Invalidate(FALSE);
}

LRESULT DesignerPaneView::OnCanvasSelect(WPARAM, LPARAM lParam)
{
    auto* p = reinterpret_cast<Path*>(lParam);
    if (p == nullptr) return 0;
    if (p->empty()) clearSelection();     // 画布空白单击：取消选择
    else selectNode(*p);
    delete p;
    return 0;
}


LRESULT DesignerPaneView::OnDocChanged(WPARAM, LPARAM)
{
    // 画布上直接改了文档（就地编辑文本框 / 下拉选中 / 勾选切换）：
    // 重排 + 重设画布 + 属性表 + 代码区 + 状态栏都要跟上
    refreshAfterDocChange();
    return 0;
}

void DesignerPaneView::onAbout()
{
    AfxMessageBox(_T("PyMfc UI Designer\n基于 PyMfcXaml 的可视化界面设计器\n输出 XAML 供 PyMfc 运行时加载"),
                  MB_ICONINFORMATION | MB_OK);
}

std::wstring DesignerPaneView::exeDir() const
{
    TCHAR buf[MAX_PATH] = { 0 };
    ::GetModuleFileName(nullptr, buf, MAX_PATH);
    std::wstring p(buf);
    const size_t s = p.find_last_of(L"\\/");
    return (s == std::wstring::npos) ? std::wstring() : p.substr(0, s);
}

// 当前文档落到临时文件：预览器/校验器都按"读文件"工作，不认内存文档
std::wstring DesignerPaneView::saveToTemp()
{
    if (m_doc.root() == nullptr)
    {
        m_surfStatus[0] = L"当前没有文档";
        Invalidate(FALSE);
        return std::wstring();
    }
    TCHAR dir[MAX_PATH] = { 0 };
    ::GetTempPath(MAX_PATH, dir);
    const std::wstring file = std::wstring(dir) + L"pyarx_designer_preview.xaml";
    std::wstring err;
    if (!m_doc.save(file.c_str(), err))
    {
        m_surfStatus[0] = CString(err.c_str());
        Invalidate(FALSE);
        return std::wstring();
    }
    return file;
}

void DesignerPaneView::previewCurrent()
{
    const std::wstring file = saveToTemp();
    if (file.empty()) return;
    // 预览器工程改名叫 Viewer（窗口类仍是 XamlViewer），两个名字都试；
    // 只认 XamlViewer.exe 会让"预览"按钮在真机上一个都不存在时彻底失效。
    std::wstring exe = exeDir() + L"\\XamlViewer.exe";
    if (::GetFileAttributesW(exe.c_str()) == INVALID_FILE_ATTRIBUTES)
    {
        const std::wstring alt = exeDir() + L"\\Viewer.exe";
        if (::GetFileAttributesW(alt.c_str()) == INVALID_FILE_ATTRIBUTES)
        {
            m_surfStatus[0] = L"预览需要 Viewer.exe / XamlViewer.exe（尚未构建）";
            Invalidate(FALSE);
            return;
        }
        exe = alt;
    }
    const std::wstring cmd = L"\"" + exe + L"\" \"" + file + L"\"";
    STARTUPINFOW si{};
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_SHOWNORMAL;
    PROCESS_INFORMATION pi{};
    if (::CreateProcessW(nullptr, (LPWSTR)cmd.c_str(), nullptr, nullptr, FALSE, 0, nullptr, nullptr,
                         &si, &pi))
    {
        ::CloseHandle(pi.hThread);
        ::CloseHandle(pi.hProcess);
        m_surfStatus[0] = L"已在 XamlViewer 中打开当前文档";
    }
    else
        m_surfStatus[0] = L"预览进程启动失败";
    Invalidate(FALSE);
}

void DesignerPaneView::checkCurrent()
{
    const std::wstring file = saveToTemp();
    if (file.empty()) return;
    std::wstring exe = exeDir() + L"\\XamlViewer.exe";
    if (::GetFileAttributesW(exe.c_str()) == INVALID_FILE_ATTRIBUTES)
    {
        const std::wstring alt = exeDir() + L"\\Viewer.exe";
        if (::GetFileAttributesW(alt.c_str()) == INVALID_FILE_ATTRIBUTES)
        {
            m_surfStatus[0] = L"校验需要 Viewer.exe / XamlViewer.exe（尚未构建）";
            Invalidate(FALSE);
            return;
        }
        exe = alt;
    }
    const std::wstring cmd = L"\"" + exe + L"\" --check \"" + file + L"\"";
    STARTUPINFOW si{};
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi{};
    if (!::CreateProcessW(nullptr, (LPWSTR)cmd.c_str(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
                          nullptr, nullptr, &si, &pi))
    {
        m_surfStatus[0] = L"校验进程启动失败";
        Invalidate(FALSE);
        return;
    }
    ::WaitForSingleObject(pi.hProcess, 12000);
    DWORD code = 1;
    ::GetExitCodeProcess(pi.hProcess, &code);
    ::CloseHandle(pi.hThread);
    ::CloseHandle(pi.hProcess);
    m_surfStatus[0] = (code == 0) ? L"校验通过：未发现语法或语义问题"
                                  : L"校验未通过：文档存在问题，用预览查看明细";
    Invalidate(FALSE);
}

// XAML 代码坞折叠：改自身界面根 Grid 的第 4 行轨道高度（行高来自 XAML，这里只翻档位）
void DesignerPaneView::toggleCodeDock()
{
    XamlElement* root = m_ui.root();
    if (root == nullptr || root->children.empty()) return;
    XamlElement* grid = &root->children[0];
    if (_wcsicmp(grid->tag.c_str(), L"Grid") != 0 || grid->gridRows.size() < 4) return;

    m_codeCollapsed = !m_codeCollapsed;
    grid->gridRows[3].second = m_codeCollapsed ? L"34" : L"236";
    uidesign::Walk(root, [&](const Path&, XamlElement& el) {
        if (el.name == L"btnCodeToggle")
            el.set(L"Text", m_codeCollapsed ? L"展开" : L"收起");
    });
    relayoutUi();
    Invalidate(FALSE);
}

void DesignerPaneView::copyXamlToClipboard()
{
    const std::wstring xml = m_doc.toXml();
    if (xml.empty())
    {
        m_surfStatus[0] = L"当前没有可复制的 XAML";
        Invalidate(FALSE);
        return;
    }
    // MFC 里 ::OpenClipboard() 会解析到需要 HWND 的重载，必须显式传窗口句柄
    if (!::OpenClipboard(m_hWnd)) return;
    ::EmptyClipboard();
    const size_t bytes = (xml.size() + 1) * sizeof(wchar_t);
    HGLOBAL h = ::GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (h != nullptr)
    {
        void* p = ::GlobalLock(h);
        if (p != nullptr)
        {
            memcpy(p, xml.c_str(), bytes);
            ::GlobalUnlock(h);
            ::SetClipboardData(CF_UNICODETEXT, h);
        }
    }
    ::CloseClipboard();
    m_surfStatus[0] = L"XAML 源码已复制到剪贴板";
    Invalidate(FALSE);
}

void DesignerPaneView::updateStatus()
{
    int count = CountNodes(m_doc.root());
    CString sel = L"就绪";
    int sx = 0, sy = 0, sw = 0, sh = 0;
    XamlElement* node = Resolve(m_doc.root(), m_selected);
    if (node != nullptr)
    {
        CString nameDisp = node->name.empty() ? CString(L"(unnamed)") : CString(node->name.c_str());
        sel.Format(L"Name: %s | Class: %s", (LPCWSTR)nameDisp, node->tag.c_str());
        sx = node->x; sy = node->y; sw = node->w; sh = node->h;
    }
    else
    {
        CString t;
        t.Format(L"就绪 | 节点: %d", count);
        sel = t;
    }
    m_surfStatus[0] = sel;
    {
        // 状态栏是 D2D 自绘，Win32 侧读不到文字；标题栏一并带上"当前选中"，
        // 既给用户即时反馈，也是验收脚本唯一能机器判定的出口。
        // 注意要写到顶层框架：DockPane 的窗格标题只在窗格内部可见，外面看着没变。
        CString t;
        if (m_docPath.IsEmpty())
            t.Format(_T("PyMfc UI Designer · %s"), (LPCWSTR)sel);
        else
            t.Format(_T("PyMfc UI Designer - %s · %s"), (LPCWSTR)m_docPath, (LPCWSTR)sel);
        CFrameWnd* frame = GetTopLevelFrame();
        CWnd* target = (frame != nullptr) ? static_cast<CWnd*>(frame) : this;
        target->SetWindowText(t);
    }
    {
        CString t; t.Format(L"X:%d Y:%d", sx, sy);   m_surfStatus[1] = t;
        CString u; u.Format(L"W:%d H:%d", sw, sh);   m_surfStatus[2] = u;
        if (m_canvas != nullptr)
            m_surfStatus[3].Format(L"%d%%", (int)(m_canvas->zoom() * 100.0f + 0.5f));
        else
            m_surfStatus[3] = L"100%";
    }
    updateXamlView();
}

// 底部 XAML 源码区：控件库的 CodeEditor（独立控件，自带窗口 + D2D 自绘 + 智能提示）。
// 位置要等第一帧算出 m_surfCodeRc 之后才准，所以这里只做"懒创建"，具体摆放交给 syncCodeEditor()。
void DesignerPaneView::setupXamlCodeView()
{
    if (m_codeEditor)
        return;
    m_codeEditor = std::make_unique<CodeEditor>();
    // 先建在 1×1 的角落里：矩形一算出来就搬过去，不会在屏幕中间闪一下
    if (!m_codeEditor->create(m_hWnd, 2004, 0, 0, 1, 1, L"xaml"))
        m_codeEditor.reset();
    else
    {
        // 焦点进 = 开始编辑；焦点出 = 落定源码。两件事都由控件回调驱动，
        // 设计器自己不再靠"点没点到代码区"去猜状态。
        m_codeEditor->setOnFocus([this]() {
            m_codeEditing = true;
            m_surfStatus[0] = L"源码编辑中（离开代码区即应用）";
        });
        m_codeEditor->setOnBlur([this]() { commitCodeEdit(); });
    }
}

// 编辑器内容变了 -> 重新解析回文档模型（解析失败就把错误放状态栏，不丢用户输入）
void DesignerPaneView::applyCodeText(const std::wstring& xml)
{
    if (!m_codeEditor)
        return;
    if (xml.empty())
        return;

    pushUndo();
    std::wstring err;
    if (m_doc.loadString(xml.c_str(), err))
    {
        m_selected.clear();
        refreshAfterDocChange();
        m_surfStatus[0] = L"源码已应用";
    }
    else
    {
        if (!m_undoStack.empty()) m_undoStack.pop_back();
        m_surfStatus[0] = CString(L"解析失败: ") + err.c_str();
    }
    Invalidate(FALSE);
}

// 把编辑器贴到代码区矩形上，并把文档文本同步进去（不触发 onTextChanged）。
// 关键：用户正在编辑时绝不回灌 —— 文档每次重绘都会调到这里，若拿 toXml() 覆盖编辑器，
// 打一个字就被打回去（"输不进字符/尖括号"就是这个）。只在失焦（m_codeEditing == false）后同步。
void DesignerPaneView::syncCodeEditor()
{
    if (!m_codeEditor || m_codeEditor->hwnd() == nullptr)
        return;
    if (m_surfCodeRc.right <= m_surfCodeRc.left || m_surfCodeRc.bottom <= m_surfCodeRc.top)
        return;

    RECT cur{};
    ::GetWindowRect(m_codeEditor->hwnd(), &cur);
    POINT tl{cur.left, cur.top}, br{cur.right, cur.bottom};
    ::ScreenToClient(m_hWnd, &tl);
    ::ScreenToClient(m_hWnd, &br);
    if (tl.x != m_surfCodeRc.left || tl.y != m_surfCodeRc.top ||
        br.x != m_surfCodeRc.right || br.y != m_surfCodeRc.bottom)
    {
        m_codeEditor->setBounds(m_surfCodeRc.left, m_surfCodeRc.top,
                                m_surfCodeRc.right - m_surfCodeRc.left,
                                m_surfCodeRc.bottom - m_surfCodeRc.top);
    }

    if (m_codeEditing || ::GetFocus() == m_codeEditor->hwnd())
        return;

    CString xml(m_doc.toXml().c_str());
    xml.Replace(L"\n", L"\r\n");
    const std::wstring want(xml);
    if (m_codeEditor->text() != want)
        m_codeEditor->setText(want);
}

void DesignerPaneView::updateXamlView()
{
    syncCodeEditor();
    Invalidate(FALSE);
}

// ===========================================================================
// 整窗单张 Direct2D 表面
// 整个设计器客户区由一块 Direct2D 表面绘制：工具栏、控件箱、对象树、
// 属性表、XAML 代码区、分隔条、状态栏全部自绘，不再创建子 HWND 控件。
// 唯一保留的真实窗口是主画布 —— DesignCanvas 自己就是一个 D2D 直呈窗口。
// ===========================================================================
void DesignerPaneView::OnPaint()
{
    CPaintDC dc(this);
    CRect client;
    GetClientRect(&client);
    if (client.Width() <= 0 || client.Height() <= 0) return;

    // 双缓冲：D2D 先画到内存位图，一次 BitBlt 上屏。
    // 直接画到窗口 DC 会让整窗逐帧刷新闪烁；BitBlt 的目标是 WM_PAINT 的 DC，
    // 叠加 WS_CLIPCHILDREN 后子窗口区域自动被裁掉，不会盖住真实控件。
    CDC mem;
    mem.CreateCompatibleDC(&dc);
    CBitmap bmp;
    bmp.CreateCompatibleBitmap(&dc, client.Width(), client.Height());
    CBitmap* oldBmp = mem.SelectObject(&bmp);

    paintSurface(mem.GetSafeHdc());

    // BitBlt 必须在 SelectObject 换回原位图之前，否则拷出来的是空位图
    dc.BitBlt(0, 0, client.Width(), client.Height(), &mem, 0, 0, SRCCOPY);
    mem.SelectObject(oldBmp);
}

