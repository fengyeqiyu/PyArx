// PyMfcHostApi.h —— 宿主跨世界 POD 接口（第 78 轮 B1 切片）。
//
// 非 MFC 绑定 TU（XmlBindings.cpp / RegisterPyMfcControls.cpp / PyMfcTopWnd.cpp）
// 只依赖本头；实现类 PyMfcHost（MFC-present，见 PyMfcHost.h）继续继承 CWnd 并
// 实现本接口。本头红线：签名/成员只允许 HWND/RECT/POINT/COLORREF/UINT/WPARAM/
// LPARAM/const wchar_t*/bool/int/LONG，不得出现 CWnd*/CString/CRect/CPoint/CSize/
// CDC/CBitmap 等 MFC 包装类型。
//
// 操作集合口径：按第 78 轮三 TU 只读审计的真实命中集合落；审计未命中的宿主方法
// 仍留在 PyMfcHost.h（MFC-present），待其消费 TU 脱离 MFC 世界后再迁入本头，
// 不照抄整个 PyMfcHost 类。

#pragma once

#include <windows.h>

// 宿主对外暴露的跨世界操作面。PyMfcHost 实现本接口；非 MFC 侧只持本接口指针，
// 不接触 PyMfcHost 的 CWnd 基类与 MFC 头链。
class IPyMfcHostApi
{
public:
    virtual ~IPyMfcHostApi() = default;

    // 取宿主顶层窗句柄（POD HWND；跨世界替代 MFC GetSafeHwnd 入口）。
    virtual HWND hostHwnd() const = 0;
};
