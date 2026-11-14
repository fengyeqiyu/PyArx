// XFrameWnd.h —— 无框/有框顶层窗口（控件库里的窗口 Frame）。
//
// 这是 Qt 里 QQuickWindow / ApplicationWindow + FramelessWindowHint 的等价物：
//   * FrameFrameless：客户区铺满整窗（非客户区压成 0），外观完全由内容窗口的 XML 像素决定；
//     四边 6px 交回系统缩放循环，标题栏交回系统移动循环，DWM 圆角/描边稳定后统一剥离；
//   * FrameNative：交给系统画标题栏/边框，内容窗口仍是唯一客户区子窗口（三端观感一致的兜底）。
//
// 放在 MfcXmlCore：外壳是渲染库能力，设计器 / 预览器 / 运行时共用同一实现，
// 各程序只做"调用者"（装配内容窗口、按业务重写关闭询问与快捷键）。
//
// 实现见 XFrameWnd.inl。MFC 的消息映射要求一个进程内只在一个编译单元里展开，
// 因此调用方在自己的"外壳装配 TU"里 #include "XFrameWnd.inl"，其它 TU 只用本头。
#pragma once

#ifndef _AFXWIN_H_
#include <afxwin.h>
#endif
#include "XFrameWindow.h"

namespace xmlframe
{

enum FrameMode : int
{
    FrameFrameless = 0,   // 无边框（自绘整窗外观）
    FrameNative = 1       // 有边框（系统标题栏/边框）
};

class XFrameWnd : public CWnd
{
public:
    XFrameWnd();
    virtual ~XFrameWnd();

    BOOL CreateFrame(LPCTSTR title, const CRect& rc, CWnd* parent = nullptr,
                     FrameMode mode = FrameFrameless);

    void SetContent(CWnd* wnd);           // 内容子窗口：设进来即铺满并显示
    HWND ContentHwnd() const;
    FrameMode Mode() const;

    // 标题栏拖动区：height<=0 表示没有可拖动区；rightInset 是右侧留给窗口按钮的宽度
    void SetCaptionDrag(int height, int rightInset = 0);
    void SetMinTrackSize(int w, int h);
    void SetFrameBackground(COLORREF color);

    // 系统循环通知（内容窗口据此冻结/解冻绘制）
    virtual void OnFrameSizeMoveBegin() {}
    virtual void OnFrameSizeMoveEnd() {}
    virtual void OnFrameSizeChanged(int cx, int cy) {}

    // 关闭前询问：返回 false 取消关闭
    virtual bool OnFrameClose() { return true; }

protected:
    // 外壳已建好、内容窗口尚不存在：派生类在此创建内容窗口并 SetContent
    virtual BOOL CreateContent() { return TRUE; }

    afx_msg int  OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp);
    afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
    afx_msg LRESULT OnNcHitTest(CPoint point);
    afx_msg void OnEnterSizeMove();
    afx_msg void OnExitSizeMove();
    afx_msg void OnClose();
    afx_msg void OnDestroy();
    afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
    afx_msg BOOL OnNcActivate(BOOL bActive);
    afx_msg LRESULT OnNcUahDraw(WPARAM wParam, LPARAM lParam);
    DECLARE_MESSAGE_MAP()

private:
    enum { kTimerSettle = 2 };

    void layoutContent();
    void stripFrame();
    static void frameLog(const wchar_t* fmt, ...);

    CWnd*    m_content = nullptr;
    FrameMode m_mode = FrameFrameless;
    bool     m_frameless = true;
    int      m_captionHeight = 0;
    int      m_captionRightInset = 0;
    int      m_minTrackW = 720;
    int      m_minTrackH = 480;
    COLORREF m_back = RGB(21, 24, 29);
};

} // namespace xmlframe
