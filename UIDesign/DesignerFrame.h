#pragma once
#include "stdafx.h"
#include <afxcontrolbars.h>     // CFrameWndEx / CDockablePane / CDockingManager 全家桶
#include "DesignerDockPane.h"

namespace uidesign
{
    // 独立运行外壳：CFrameWndEx + CDockablePane。
    // 设计器不再是"自己当顶层框架"，而是停在框架里的一个可停靠面板，
    // 于是同一个 DesignerPaneView 可以原样搬到 AutoCAD 的 CAdUiPalette 里。
    class DesignerFrame : public CFrameWndEx
    {
    public:
        DesignerFrame() = default;
        ~DesignerFrame() override = default;

        BOOL createFrame(const CRect& rect = CRect(80, 60, 1440, 940));
        bool openFile(const CString& path) { return m_pane.view().openFile(path); }
        void newDocument() { m_pane.view().newDocument(); }
        DesignerDockPane& pane() { return m_pane; }

    protected:
        BOOL PreCreateWindow(CREATESTRUCT& cs) override;
        BOOL PreTranslateMessage(MSG* pMsg) override;
        afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
        afx_msg void OnSize(UINT nType, int cx, int cy);
        afx_msg BOOL OnEraseBkgnd(CDC* pDC);
        afx_msg void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp);
        afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
        DECLARE_MESSAGE_MAP()

    private:
        DesignerDockPane m_pane;
    };
}
