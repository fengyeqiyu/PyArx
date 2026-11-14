#include "stdafx.h"
#include "DesignerShell.h"
#include "XFrameWnd.inl"      // 外壳实现：本 TU 是 UIDesign 唯一展开消息映射的地方

using namespace uidesign;

BOOL DesignerShell::createShell(const CRect& rect)
{
    if (!CreateFrame(_T("PyMfc UI Designer"), rect))
        return FALSE;

    // 顶栏高度随后按内容视图的布局给出（XML rootGrid 的第一行）
    SetCaptionDrag(m_view.titleBarHeight(), 3 * 46);   // 右侧三个窗口按钮
    return TRUE;
}

BOOL DesignerShell::CreateContent()
{
    CRect rc;
    GetClientRect(&rc);
    if (!m_view.create(this, rc, 0x8201))
        return FALSE;

    SetContent(&m_view);        // 交给外壳管理：尺寸随窗口走，唯一内容子窗口

    // 内容视图已经按 XML 布局好顶栏高度，把拖动区告诉外壳
    SetCaptionDrag(m_view.titleBarHeight(), 3 * 46);
    return TRUE;
}

bool DesignerShell::OnFrameClose()
{
    // 有未保存修改时弹"保存 / 不保存 / 取消"
    return m_view.confirmClose();
}

void DesignerShell::OnFrameSizeMoveBegin()
{
    m_view.beginSizeMovePreview();
}

void DesignerShell::OnFrameSizeMoveEnd()
{
    m_view.endSizeMovePreview();
}

void DesignerShell::OnFrameSizeChanged(int cx, int cy)
{
    // 内容视图收到的 WM_SIZE 会自己判定"正在拖 → 只擦底色，松手一次重排重画"，
    // 外壳这里不需要额外动作。
    (void)cx; (void)cy;
}

BOOL DesignerShell::PreTranslateMessage(MSG* pMsg)
{
    if (pMsg != nullptr && pMsg->message == WM_KEYDOWN)
    {
        HWND vh = m_view.GetSafeHwnd();
        const bool focusInside = (vh != nullptr && (pMsg->hwnd == vh || ::IsChild(vh, pMsg->hwnd)));

        if (pMsg->wParam == VK_DELETE && m_view.handleDeleteKey(pMsg->hwnd))
            return TRUE;

        if ((::GetKeyState(VK_CONTROL) & 0x8000) != 0 && !focusInside)
        {
            const UINT ch = pMsg->wParam;
            if (ch == 'Z' || ch == 'z' || ch == 'Y' || ch == 'y')
            {
                m_view.undoOrRedo(ch == 'Z' || ch == 'z');
                return TRUE;
            }
            if (ch == 'A' || ch == 'a')
            {
                m_view.selectAllChildren();
                return TRUE;
            }
            if (ch == 'C' || ch == 'c' || ch == 'X' || ch == 'x' || ch == 'V' || ch == 'v')
            {
                m_view.clipboardAction(ch);
                return TRUE;
            }
        }
    }
    return xmlframe::XFrameWnd::PreTranslateMessage(pMsg);
}
