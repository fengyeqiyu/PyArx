#pragma once
#include "stdafx.h"
#include <afxcontrolbars.h>
#include "DesignerPaneView.h"

namespace uidesign
{
    constexpr UINT kId_VIEW_DESIGNER = 0x8100;

    // CDockablePane 承载层。
    // 设计器内容（DesignerPaneView）完全不感知宿主：pane 只负责创建它、
    // 在 OnSize 里把它铺满客户区，标题栏/浮动/停靠/自动隐藏都交给 CDockablePane。
    class DesignerDockPane : public CDockablePane
    {
    public:
        DesignerDockPane() = default;
        ~DesignerDockPane() override = default;

        // parent 必须是 CFrameWndEx（MFC Feature Pack 的停靠宿主）
        BOOL createPane(CWnd* parent, UINT id, CRect rect, LPCTSTR caption = _T("界面设计器"));

        DesignerPaneView& view() { return m_view; }

        // 主内容面板不给关闭按钮：能浮动能自动隐藏，但不能被关成空框架
        BOOL CanBeClosed() const override { return FALSE; }
        // 设计器就是主窗体本体：不允许拖出去浮成独立窗口，也不允许自动隐藏
        BOOL CanFloat() const override { return FALSE; }
        BOOL CanAutoHide() const override { return FALSE; }

    protected:
        afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
        afx_msg void OnSize(UINT nType, int cx, int cy);
        afx_msg BOOL OnEraseBkgnd(CDC* pDC);
        afx_msg void OnSetFocus(CWnd* pOldWnd);
        // CDockablePane 默认右键会弹"停靠/选项卡式文档/自动隐藏/隐藏"系统菜单。
        // 设计器右键菜单由 DesignerPaneView 自己画，这里必须整条吞掉，否则两套菜单叠在一起。
        afx_msg void OnContextMenu(CWnd* pWnd, CPoint pos);
        DECLARE_MESSAGE_MAP()

    private:
        DesignerPaneView m_view;
        bool m_viewReady = false;
    };
}
