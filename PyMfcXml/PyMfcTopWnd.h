// PyMfcTopWnd.h —— 顶层包装窗声明（MFC 侧独立 TU，NotUsing PCH，直接 include stdafx.h）。
//
// 回退到 MFC（用户要求）：基类重新用 CWnd，尺寸变化走 ON_WM_SIZE 消息映射。
// relayout 回调保留（宿主重排 MoveWindow + reflowXml 由 Wrap.cpp 注入）。

#pragma once

#include "stdafx.h"

#include <functional>

class PyMfcHost;

class PyMfcTopWnd : public CWnd
{
public:
    PyMfcHost* host = nullptr;
    std::function<void(int, int)> resize;
    std::function<void(int, int)> relayout;

    // 本对象是 PyMfcWindow 的成员，且窗口由 SubclassWindow 绑定：
    // 窗口销毁时 MFC 默认 PostNcDestroy 会 delete this，会删掉一块成员子对象 → 堆损坏。
    void PostNcDestroy() override;

protected:
    afx_msg void OnSize(UINT nType, int cx, int cy);
    DECLARE_MESSAGE_MAP()
};
