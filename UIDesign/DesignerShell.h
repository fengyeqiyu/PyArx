// DesignerShell.h —— 设计器外壳：只是 XFrameWnd 的装配者。
// 无框/命中/系统缩放循环/DWM 描边全部由控件库（xmlframe::XFrameWnd）负责，
// 这里只做三件业务相关的事：创建内容视图、把标题栏高度告诉外壳、转发快捷键与关闭询问。
#pragma once

#include "XFrameWnd.h"
#include "DesignerPaneView.h"

namespace uidesign
{

class DesignerShell : public xmlframe::XFrameWnd
{
public:
    // 创建外壳 + 内容视图（内容视图由外壳在 CreateContent 里创建）
    BOOL createShell(const CRect& rect);

    DesignerPaneView& view() { return m_view; }

protected:
    BOOL CreateContent() override;
    bool OnFrameClose() override;
    void OnFrameSizeMoveBegin() override;
    void OnFrameSizeMoveEnd() override;
    void OnFrameSizeChanged(int cx, int cy) override;
    BOOL PreTranslateMessage(MSG* pMsg) override;

private:
    DesignerPaneView m_view;
};

} // namespace uidesign
