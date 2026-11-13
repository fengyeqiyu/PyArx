#include "stdafx.h"
#include "DesignerDockPane.h"

using namespace uidesign;

BEGIN_MESSAGE_MAP(DesignerDockPane, CDockablePane)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_ERASEBKGND()
    ON_WM_SETFOCUS()
    ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()

BOOL DesignerDockPane::createPane(CWnd* parent, UINT id, CRect rect, LPCTSTR caption)
{
    // CBRS_LEFT：默认贴左停靠。不要 CBRS_HIDE_INPLACE（会自动隐藏成侧边条），
    // 控制条风格里也去掉 FLOAT / AUTOHIDE / CLOSE / RESIZE：
    // 设计器是一个整体，代码坞属于主窗体的一部分，不能拖出去浮成独立窗口。
    DWORD style = WS_CHILD | WS_VISIBLE | CBRS_LEFT;
    // 第 4 个参数 bHasGripper 传 FALSE：不再生成"界面设计器"那条浅蓝标题条（也不带拖拽把手）。
    // 设计器内容直接从客户区顶端画起，标题成本地框架的一部分。
    return Create(caption, parent, rect, FALSE, id, style,
                  AFX_CBRS_REGULAR_TABS, AFX_DEFAULT_DOCKING_PANE_STYLE);
}

// 设计器是主窗体的一部分，不允许浮成独立窗口、自动隐藏或关闭（声明在头文件里）

int DesignerDockPane::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CDockablePane::OnCreate(lpCreateStruct) == -1)
        return -1;

    CRect rc;
    GetClientRect(&rc);
    // CDockablePane 在停靠/浮动切换时会重建自身窗口，OnCreate 会被再次调用。
    // 视图必须只创建一次，否则旧 HWND 变成孤儿窗口留在 pane 里 —— 表现就是同一份界面叠两层。
    if (m_view.GetSafeHwnd() == nullptr)
    {
        if (!m_view.create(this, rc, kId_VIEW_DESIGNER + 1))
            return -1;
    }
    else
    {
        m_view.SetWindowPos(nullptr, 0, 0, rc.Width(), rc.Height(),
                            SWP_NOZORDER | SWP_NOACTIVATE);
    }

    m_viewReady = true;
    return 0;
}

void DesignerDockPane::OnSize(UINT nType, int cx, int cy)
{
    CDockablePane::OnSize(nType, cx, cy);
    if (m_viewReady && m_view.GetSafeHwnd() != nullptr && cx > 0 && cy > 0)
        m_view.SetWindowPos(nullptr, 0, 0, cx, cy, SWP_NOZORDER | SWP_NOACTIVATE);
}

BOOL DesignerDockPane::OnEraseBkgnd(CDC* pDC)
{
    // 内容视图每帧全画，pane 底色只负责"还没画到时"不闪白
    CRect rc;
    GetClientRect(&rc);
    pDC->FillSolidRect(&rc, RGB(21, 24, 29));
    return TRUE;
}

void DesignerDockPane::OnSetFocus(CWnd* pOldWnd)
{
    CDockablePane::OnSetFocus(pOldWnd);
    if (m_viewReady && m_view.GetSafeHwnd() != nullptr)
        m_view.SetFocus();
}

void DesignerDockPane::OnContextMenu(CWnd* pWnd, CPoint pos)
{
    // 吞掉，交给内容视图的一整套自绘右键菜单（对象树/属性表/画布各有自己的语境菜单）
    (void)pWnd;
    (void)pos;
}
