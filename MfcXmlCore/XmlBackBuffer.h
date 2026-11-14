// XmlBackBuffer.h —— 控件库统一提供的离屏绘制缓冲（纯 GDI + 32bpp DIB Section）。
//
// 为什么必须由库提供，而不是每个程序各写一遍：
//   1) DCRenderTarget / D2D 绑定的 DC 必须是 DIB Section。用 CreateCompatibleBitmap
//      （Display 兼容位图）在部分驱动上 BindDC 直接失败 —— 位图整块空白，BitBlt 上去
//      就是"整窗变黑、只剩真子窗口可见"，这类问题每个项目重写一遍就要重踩一遍。
//   2) 位图必须常驻复用，只在尺寸变化时重建；每帧 new/delete 一块 6MB 位图，
//      拖动时就是持续分配 + 缺页。
//
// 用法（任何 MFC 窗口的 OnPaint）：
//     XmlBackBuffer buf;
//     HDC mem = buf.ensure(dc.GetSafeHdc(), rc.Width(), rc.Height());  // 返回可直接画的 DC
//     if (mem != nullptr) { draw(mem); buf.present(dc.GetSafeHdc(), rc.Width(), rc.Height()); }
//
// 注意：不要用 WM_PRINTCLIENT / PrintWindow 传进来的重定向 DC 去建位图 —— 那里可能只有
// 1bpp，建出来是黑的还会被缓存。ensure() 内部统一用屏幕 DC 建，规避这一条。
#pragma once

#ifndef _INC_WINDOWS
#include <windows.h>
#endif

namespace xmlctl
{

class XmlBackBuffer
{
public:
    XmlBackBuffer() {}
    ~XmlBackBuffer() { release(); }

    XmlBackBuffer(const XmlBackBuffer&) = delete;
    XmlBackBuffer& operator=(const XmlBackBuffer&) = delete;

    // 保证缓冲与目标尺寸一致，返回内存 DC（可直接作为 D2D 的 BindDC 目标）；失败返回 nullptr
    HDC ensure(HDC clientDc, int cx, int cy)
    {
        if (cx <= 0 || cy <= 0)
            return nullptr;
        if (m_memDc == nullptr)
            m_memDc = ::CreateCompatibleDC(clientDc);
        if (m_memDc == nullptr)
            return nullptr;

        if (m_bmp != nullptr && m_w == cx && m_h == cy)
            return m_memDc;

        if (m_old != nullptr)
        {
            ::SelectObject(m_memDc, m_old);
            m_old = nullptr;
        }
        if (m_bmp != nullptr)
        {
            ::DeleteObject(m_bmp);
            m_bmp = nullptr;
        }
        m_w = m_h = 0;

        HDC sdc = ::GetDC(nullptr);
        BITMAPINFO bmi = {};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = cx;
        bmi.bmiHeader.biHeight = -cy;          // top-down：和 D2D 的坐标系一致
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;
        void* bits = nullptr;
        HBITMAP hbm = ::CreateDIBSection(sdc, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
        ::ReleaseDC(nullptr, sdc);
        if (hbm == nullptr)
            return nullptr;                    // 建不出来就不画，避免 BitBlt 一张空位图

        m_bmp = hbm;
        m_old = ::SelectObject(m_memDc, m_bmp);
        m_w = cx;
        m_h = cy;
        return m_memDc;
    }

    // 一次 BitBlt 上屏
    void present(HDC clientDc, int cx, int cy)
    {
        if (m_memDc == nullptr || m_bmp == nullptr || cx <= 0 || cy <= 0)
            return;
        ::BitBlt(clientDc, 0, 0, cx, cy, m_memDc, 0, 0, SRCCOPY);
    }

    HDC dc() const { return m_memDc; }
    int width() const { return m_w; }
    int height() const { return m_h; }

    // 尺寸已变（拖完收尾等）：丢掉旧内容，下次 ensure 重建
    void invalidate() { m_w = m_h = 0; }

    void release()
    {
        if (m_memDc != nullptr)
        {
            if (m_old != nullptr) ::SelectObject(m_memDc, m_old);
            ::DeleteDC(m_memDc);
            m_memDc = nullptr;
        }
        m_old = nullptr;
        if (m_bmp != nullptr)
        {
            ::DeleteObject(m_bmp);
            m_bmp = nullptr;
        }
        m_w = m_h = 0;
    }

private:
    HDC     m_memDc = nullptr;
    HBITMAP m_bmp = nullptr;
    HGDIOBJ m_old = nullptr;
    int     m_w = 0;
    int     m_h = 0;
};

} // namespace xmlctl
