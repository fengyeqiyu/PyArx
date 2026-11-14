// XmlD2D.h —— Direct2D 后端（两种渲染目标，三端共用）
//
//   D2DHwndCanvas ：ID2D1HwndRenderTarget，给独占窗口用（预览器、设计器画布）
//   D2DDcCanvas   ：ID2D1DCRenderTarget，给"只有一个 HDC"的绘制路径用
//                   （PyMfc owner-draw、双缓冲内存 DC）
//
// 共同约定：
//   * SetDpi(96,96) → 1 DIP = 1 像素，坐标与 GDI 后端完全一致；
//   * 先试硬件，失败自动退 WARP 软件渲染（RDP / 老驱动 / 无 D3D）；
//   * EndDraw 返回 D2DERR_RECREATE_TARGET 时丢弃目标，下次 begin() 重建；
//   * 建不起来时 begin() 返回 false，调用方回退 GDI，不会白屏；
//   * 默认 ALIASED（与 GDI 像素对齐），设 XML_D2D_AA=1 切到 PER_PRIMITIVE。
#pragma once

#include "XmlDraw.h"
#include <d2d1.h>
#include <dwrite.h>
#include <dwrite_1.h>
#include <dwrite_2.h>
#include <wincodec.h>
#include <cstring>
#include <cmath>
#include <vector>
#include <algorithm>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "windowscodecs.lib")

inline ID2D1Factory* d2dFactory()
{
    static ID2D1Factory* f = nullptr;
    if (f == nullptr)
    {
        if (FAILED(::D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &f))) f = nullptr;
    }
    return f;
}

// 渲染目标类型：默认硬件；环境变量 PYARX_D2D_RT=software 时走 WARP（纯软件）。
// 本地 Python 直跑（进程里没有宿主 CWinApp）时，进程退出阶段显卡驱动的设备销毁会
// 在 atidxx64.dll 里空指针崩溃（0xC0000005），所以 PyMfcCore 自举时把这里设成 software。
inline D2D1_RENDER_TARGET_TYPE d2dRenderTargetType()
{
    static int cached = -1;
    if (cached < 0)
    {
        wchar_t buf[64] = {};
        const DWORD n = ::GetEnvironmentVariableW(L"PYARX_D2D_RT", buf, 64);
        cached = (n > 0 && ::_wcsicmp(buf, L"software") == 0)
                     ? static_cast<int>(D2D1_RENDER_TARGET_TYPE_SOFTWARE)
                     : static_cast<int>(D2D1_RENDER_TARGET_TYPE_HARDWARE);
    }
    return static_cast<D2D1_RENDER_TARGET_TYPE>(cached);
}

inline IDWriteFactory* dwriteFactory()
{
    static IDWriteFactory* f = nullptr;
    if (f == nullptr)
    {
        if (FAILED(::DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                                        reinterpret_cast<IUnknown**>(&f))))
            f = nullptr;
    }
    return f;
}

// WIC 解码工厂（PNG 图标 / 位图背景用，单例）
inline IWICImagingFactory* wicFactory()
{
    static IWICImagingFactory* f = nullptr;
    static bool tried = false;
    if (!tried)
    {
        tried = true;
        if (FAILED(::CoCreateInstance(CLSID_WICImagingFactory, nullptr,
                                      CLSCTX_INPROC_SERVER, __uuidof(IWICImagingFactory),
                                      reinterpret_cast<void**>(&f))))
            f = nullptr;
    }
    return f;
}

inline bool d2dAaEnabled()
{
    static int cached = -1;
    if (cached < 0)
    {
        // 默认开抗锯齿。曾经默认关（为了排查"接缝/描边重影"），代价是所有圆角、斜线、
        // 椭圆都变成 1px 台阶 —— 毛边比接缝显眼得多，所以改回默认开。
        // 需要复现旧行为时设 XML_D2D_AA=0（或 XMLVIEWER_AA=0）即可关掉。
        wchar_t v[8] = {};
        bool disable = (::GetEnvironmentVariableW(L"XML_D2D_AA", v, 8) >= 1 && v[0] == L'0');
        if (!disable)
        {
            wchar_t w[8] = {};
            disable = (::GetEnvironmentVariableW(L"XMLVIEWER_AA", w, 8) >= 1 && w[0] == L'0');
        }
        cached = disable ? 0 : 1;
    }
    return cached == 1;
}

// Window.Icon：WPF 里它是 ImageSource，本仓库约定成 "res/xxx.png" 路径。
// 三端（设计器 / 预览器 / CAD 运行时）创建窗口后用它拿 HICON 调 WM_SETICON，
// 否则标题栏左上角永远是空白占位符。调用方负责 ::DestroyIcon 返回值。
inline HICON xmlIconFromFile(const std::wstring& path)
{
    if (path.empty() || wicFactory() == nullptr) return nullptr;

    // XML 里写 "res/new.png"，统一成反斜杠再交给 WIC
    std::wstring norm = path;
    for (auto& ch : norm)
        if (ch == L'/') ch = L'\\';

    IWICBitmapDecoder* dec = nullptr;
    IWICBitmapFrameDecode* frame = nullptr;
    IWICFormatConverter* conv = nullptr;
    HICON icon = nullptr;
    void* pixels = nullptr;
    HBITMAP hColor = nullptr;
    HBITMAP hMask = nullptr;
    do
    {
        if (FAILED(wicFactory()->CreateDecoderFromFilename(norm.c_str(), nullptr, GENERIC_READ,
                                                          WICDecodeMetadataCacheOnDemand, &dec)))
            break;
        if (FAILED(dec->GetFrame(0, &frame))) break;
        if (FAILED(wicFactory()->CreateFormatConverter(&conv))) break;
        if (FAILED(conv->Initialize(frame, GUID_WICPixelFormat32bppPBGRA,
                                    WICBitmapDitherTypeNone, nullptr, 0.0,
                                    WICBitmapPaletteTypeCustom)))
            break;

        UINT w = 0, hgt = 0;
        if (FAILED(conv->GetSize(&w, &hgt)) || w == 0 || hgt == 0) break;

        const UINT stride = w * 4;
        pixels = ::HeapAlloc(::GetProcessHeap(), 0, static_cast<SIZE_T>(stride) * hgt);
        if (pixels == nullptr) break;
        if (FAILED(conv->CopyPixels(nullptr, stride, stride * hgt,
                                    static_cast<BYTE*>(pixels))))
            break;

        BITMAPINFO bi = {};
        bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bi.bmiHeader.biWidth = static_cast<LONG>(w);
        bi.bmiHeader.biHeight = -static_cast<LONG>(hgt);  // 自顶向下，与 WIC 行序一致
        bi.bmiHeader.biPlanes = 1;
        bi.bmiHeader.biBitCount = 32;
        bi.bmiHeader.biCompression = BI_RGB;

        // 必须用 CreateDIBSection：CreateDIBitmap 出来的位图没有 alpha 语义，
        // CreateIconIndirect 会把它当单色掩码处理，彩色 PNG 变成灰白描边
        void* bits = nullptr;
        hColor = ::CreateDIBSection(nullptr, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
        if (hColor == nullptr || bits == nullptr) break;
        ::memcpy(bits, pixels, static_cast<size_t>(stride) * hgt);

        // 32bpp 有 alpha 时掩码不被使用，但 CreateIconIndirect 要求它存在
        hMask = ::CreateBitmap(static_cast<int>(w), static_cast<int>(hgt), 1, 1, nullptr);
        if (hColor == nullptr || hMask == nullptr) break;

        ICONINFO ii = {};
        ii.fIcon = TRUE;
        ii.hbmColor = hColor;
        ii.hbmMask = hMask;
        icon = ::CreateIconIndirect(&ii);
    } while (false);

    if (hColor != nullptr) ::DeleteObject(hColor);
    if (hMask != nullptr) ::DeleteObject(hMask);
    if (pixels != nullptr) ::HeapFree(::GetProcessHeap(), 0, pixels);
    if (conv != nullptr) conv->Release();
    if (frame != nullptr) frame->Release();
    if (dec != nullptr) dec->Release();
    return icon;
}

class D2DCanvasBase : public XmlCanvas
{
public:
    ~D2DCanvasBase() override { releaseTarget(); }

    bool isHardware() const { return m_hardware; }
    bool isAA() const { return d2dAaEnabled(); }
    HRESULT lastError() const { return m_hr; }
    // 底层渲染目标（Hwnd 版是其 HwndRenderTarget，DC 版是其 DCRenderTarget）：
    // 调用方要额外画位图（背景烘焙）时用它的 ID2D1RenderTarget 接口即可，两版通用。
    ID2D1RenderTarget* target() const { return m_rt; }

    bool begin(int width, int height) override
    {
        if (width <= 0 || height <= 0) return false;
        if (m_rt == nullptr || m_width != width || m_height != height)
        {
            releaseTarget();
            if (!createTarget(width, height)) return false;
            m_width = width;
            m_height = height;
        }
        m_rt->BeginDraw();
        m_rt->SetTransform(D2D1::Matrix3x2F::Identity());
        m_depth = 0;
        // P0 原语：每次 begin 都是全新一帧，残留的图层/变换必须清干净，
        // 否则上一次 Pop 漏了就永久带着一层旋转
        m_matrix = D2D1::Matrix3x2F::Identity();
        m_matrixStack.clear();
        m_layerStack.clear();
        // 上一帧漏了 popOpacity 时这里兜底：目标交还给原目标，离屏位图放掉
        if (m_opacityRt != nullptr) { m_opacityRt->Release(); m_opacityRt = nullptr; }
        m_opacityPrev = nullptr;
        m_opacityDepth = 0;
        return true;
    }

    void end() override
    {
        if (m_rt == nullptr) return;
        const HRESULT hr = m_rt->EndDraw();
        m_hr = hr;
        if (hr == static_cast<HRESULT>(D2DERR_RECREATE_TARGET) || FAILED(hr))
            releaseTarget();
    }

    void fillRect(const RECT& rc, COLORREF c) override
    {
        if (m_rt == nullptr) return;
        const D2D1_RECT_F r = rectf(rc);
        m_rt->FillRectangle(&r, brush(c));
    }

    // 点阵平铺：8x8 图块（左上角一个点，其余透明）+ 位图画刷，一次填充整块区域。
    // 头文件里默认实现是逐点 fillRect，这里换成图案填充，网格开销从上万次降到 1 次。
    void dotPattern(const RECT& rc, int cell, COLORREF c) override
    {
        if (m_rt == nullptr || cell < 2) return;
        if (m_dotBmp == nullptr || m_dotCell != cell || m_dotColor != c)
        {
            if (m_dotBrush != nullptr) { m_dotBrush->Release(); m_dotBrush = nullptr; }
            if (m_dotBmp != nullptr) { m_dotBmp->Release(); m_dotBmp = nullptr; }
            std::vector<unsigned int> px(static_cast<size_t>(cell) * cell, 0u);
            const unsigned int dot = 0xFF000000u | (static_cast<unsigned int>(GetBValue(c)) << 16)
                                   | (static_cast<unsigned int>(GetGValue(c)) << 8)
                                   | static_cast<unsigned int>(GetRValue(c));
            px[0] = dot;
            D2D1_BITMAP_PROPERTIES bp = D2D1::BitmapProperties(
                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE));
            if (FAILED(m_rt->CreateBitmap(D2D1::SizeU(static_cast<UINT32>(cell), static_cast<UINT32>(cell)),
                                          px.data(), static_cast<UINT32>(cell) * 4, bp, &m_dotBmp)))
            {
                m_dotBmp = nullptr;
                return;
            }
            m_dotCell = cell;
            m_dotColor = c;
        }
        if (m_dotBmp == nullptr) return;
        if (m_dotBrush == nullptr)
        {
            D2D1_BITMAP_BRUSH_PROPERTIES bbp = D2D1::BitmapBrushProperties(
                D2D1_EXTEND_MODE_WRAP, D2D1_EXTEND_MODE_WRAP,
                D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR);
            if (FAILED(m_rt->CreateBitmapBrush(m_dotBmp, bbp, D2D1::BrushProperties(),
                                               &m_dotBrush)))
            {
                m_dotBrush = nullptr;
                return;
            }
        }
        const D2D1_RECT_F r = rectf(rc);
        m_rt->FillRectangle(&r, m_dotBrush);
    }

    void fillRectAlpha(const RECT& rc, COLORREF c, int alpha) override
    {
        if (m_rt == nullptr) return;
        if (alpha >= 255)
        {
            fillRect(rc, c);
            return;
        }
        if (alpha <= 0)
            return;
        const D2D1_RECT_F r = rectf(rc);
        m_rt->FillRectangle(&r, brushAlpha(c, alpha));
    }

    void fillRoundRect(const RECT& rc, COLORREF face, COLORREF border, int radius) override
    {
        if (m_rt == nullptr) return;
        const float r = radius > 0 ? static_cast<float>(radius) : 1.0f;

        if (border == face)
        {
            // 不描边：整块填充，省一次 draw、也免掉边缘双重抗锯齿
            D2D1_ROUNDED_RECT rr = {rectf(rc), r, r};
            m_rt->FillRoundedRectangle(&rr, brush(face));
            return;
        }

        RECT in = rc;
        ::InflateRect(&in, -1, -1);
        if (in.right > in.left && in.bottom > in.top)
        {
            const float ir = r > 1.0f ? r - 1.0f : 1.0f;
            D2D1_ROUNDED_RECT inner = {rectf(in), ir, ir};
            m_rt->FillRoundedRectangle(&inner, brush(face));
        }

        // 1px 描边：D2D 描边是以路径为中心向两侧各扩一半，
        // 直接按 rc 描会外溢半像素、内侧又和填充重叠，圆弧看起来一侧虚一侧厚。
        // 这里把路径内缩 0.5px，笔画正好落在最外那一圈像素带里。
        D2D1_ROUNDED_RECT pen = {
            {static_cast<float>(rc.left) + 0.5f, static_cast<float>(rc.top) + 0.5f,
             static_cast<float>(rc.right) - 0.5f, static_cast<float>(rc.bottom) - 0.5f},
            r - 0.5f, r - 0.5f};
        if (pen.radiusX < 0.5f) pen.radiusX = 0.5f;
        if (pen.radiusY < 0.5f) pen.radiusY = 0.5f;
        m_rt->DrawRoundedRectangle(&pen, brush(border), 1.0f);
    }

    void fillEllipse(const RECT& rc, COLORREF c, COLORREF border) override
    {
        if (m_rt == nullptr) return;
        const D2D1_POINT_2F center = {static_cast<float>(rc.left + rc.right) / 2.0f,
                                      static_cast<float>(rc.top + rc.bottom) / 2.0f};
        const float rx = static_cast<float>(rc.right - rc.left) / 2.0f;
        const float ry = static_cast<float>(rc.bottom - rc.top) / 2.0f;
        // 无描边（border 与填充同色）时只填一次：先 DrawEllipse 描一圈再填内圆，
        // 描边压在路径两侧、内圆又缩 1px，边缘会露出 2px 参差的锯齿台阶。
        const bool hasBorder = (border != c) && rx > 1.5f && ry > 1.5f;
        if (hasBorder)
        {
            const D2D1_ELLIPSE outer = {center, rx, ry};
            m_rt->FillEllipse(&outer, brush(border));
            const D2D1_ELLIPSE inner = {center, rx - 1.0f, ry - 1.0f};
            if (inner.radiusX > 0.5f && inner.radiusY > 0.5f)
                m_rt->FillEllipse(&inner, brush(c));
            return;
        }
        const D2D1_ELLIPSE e = {center, rx, ry};
        m_rt->FillEllipse(&e, brush(c));
    }

    void strokeRect(const RECT& rc, COLORREF c, int thickness) override
    {
        const int t = thickness > 0 ? thickness : 1;
        RECT r = rc;
        for (int i = 0; i < t && r.right - r.left > 1 && r.bottom - r.top > 1; ++i)
        {
            fillRect({r.left, r.top, r.right, r.top + 1}, c);
            fillRect({r.left, r.bottom - 1, r.right, r.bottom}, c);
            fillRect({r.left, r.top, r.left + 1, r.bottom}, c);
            fillRect({r.right - 1, r.top, r.right, r.bottom}, c);
            ::InflateRect(&r, -1, -1);
        }
    }

    void strokeRoundRect(const RECT& rc, COLORREF c, int radius, int thickness) override
    {
        if (m_rt == nullptr) return;
        const float t = thickness > 0 ? static_cast<float>(thickness) : 1.0f;
        const float h = t * 0.5f;   // 内缩半个笔画，笔画整体落在框内侧
        float rr = radius > 0 ? static_cast<float>(radius) : 1.0f;
        if (rr < h) rr = h;
        D2D1_ROUNDED_RECT shape = {
            {static_cast<float>(rc.left) + h, static_cast<float>(rc.top) + h,
             static_cast<float>(rc.right) - h, static_cast<float>(rc.bottom) - h},
            rr, rr};
        m_rt->DrawRoundedRectangle(&shape, brush(c), t);
    }

    void strokeLine(const POINT& a, const POINT& b, COLORREF c, int thickness) override
    {
        if (m_rt == nullptr) return;
        m_rt->DrawLine(D2D1::Point2F(static_cast<float>(a.x), static_cast<float>(a.y)),
                       D2D1::Point2F(static_cast<float>(b.x), static_cast<float>(b.y)),
                       brush(c), static_cast<float>(thickness > 0 ? thickness : 1));
    }

    void frame3d(const RECT& rc, COLORREF base, bool sunken, int thickness) override
    {
        const COLORREF dark = RGB((GetRValue(base) * 55) / 100, (GetGValue(base) * 55) / 100,
                                  (GetBValue(base) * 55) / 100);
        const COLORREF light = RGB((std::min)(255, GetRValue(base) + 60),
                                   (std::min)(255, GetGValue(base) + 60),
                                   (std::min)(255, GetBValue(base) + 60));
        const COLORREF top = sunken ? dark : light;
        const COLORREF bottom = sunken ? light : dark;
        RECT r = rc;
        const int t = thickness > 0 ? thickness : 1;
        // 整像素条带：开抗锯齿时 1px 线段会变虚，条带保持锐利，边框与 GDI 一致
        for (int i = 0; i < t && r.right - r.left > 1 && r.bottom - r.top > 1; ++i)
        {
            fillRect({r.left, r.top, r.right, r.top + 1}, top);
            fillRect({r.left, r.top, r.left + 1, r.bottom}, top);
            fillRect({r.left, r.bottom - 1, r.right, r.bottom}, bottom);
            fillRect({r.right - 1, r.top, r.right, r.bottom}, bottom);
            ::InflateRect(&r, -1, -1);
        }
    }

    // WPF 对齐：字重 / 斜体 / 换行 / 垂直对齐
    void setTextStyle(const std::wstring& weight, const std::wstring& style,
                      const std::wstring& wrapping, const std::wstring& valign) override
    {
        m_tsWeight = weight;
        m_tsStyle = style;
        m_tsWrap = wrapping;
        m_tsVAlign = valign;
    }

    DWRITE_FONT_WEIGHT textWeightOf() const
    {
        if (_wcsicmp(m_tsWeight.c_str(), L"Bold") == 0) return DWRITE_FONT_WEIGHT_BOLD;
        if (_wcsicmp(m_tsWeight.c_str(), L"SemiBold") == 0) return DWRITE_FONT_WEIGHT_SEMI_BOLD;
        if (_wcsicmp(m_tsWeight.c_str(), L"Light") == 0) return DWRITE_FONT_WEIGHT_LIGHT;
        return DWRITE_FONT_WEIGHT_NORMAL;
    }

    DWRITE_FONT_STYLE textStyleOf() const
    {
        if (_wcsicmp(m_tsStyle.c_str(), L"Italic") == 0) return DWRITE_FONT_STYLE_ITALIC;
        if (_wcsicmp(m_tsStyle.c_str(), L"Oblique") == 0) return DWRITE_FONT_STYLE_OBLIQUE;
        return DWRITE_FONT_STYLE_NORMAL;
    }

    void text(const RECT& rc, const std::wstring& s, COLORREF fg,
              UINT align, int ptSize, const std::wstring& family) override
    {
        if (m_rt == nullptr || s.empty() || dwriteFactory() == nullptr) return;
        if (!ensureTextFormat(ptSize, family)) return;

        const DWRITE_TEXT_ALIGNMENT ta =
            ((align & DT_CENTER) != 0) ? DWRITE_TEXT_ALIGNMENT_CENTER :
            ((align & DT_RIGHT) != 0) ? DWRITE_TEXT_ALIGNMENT_TRAILING :
                                        DWRITE_TEXT_ALIGNMENT_LEADING;
        if (ta != m_align)
        {
            m_fmt->SetTextAlignment(ta);
            m_align = ta;
        }

        // 换行：默认单行（与 GDI 的 DT_SINGLELINE 一致），Wrap 时按宽度折行
        const DWRITE_WORD_WRAPPING ww =
            (_wcsicmp(m_tsWrap.c_str(), L"Wrap") == 0 ||
             _wcsicmp(m_tsWrap.c_str(), L"WrapWithOverflow") == 0)
                ? DWRITE_WORD_WRAPPING_WRAP : DWRITE_WORD_WRAPPING_NO_WRAP;
        if (ww != m_wrap)
        {
            m_fmt->SetWordWrapping(ww);
            m_wrap = ww;
        }

        // 垂直对齐：默认居中（与原行为一致）
        const DWRITE_PARAGRAPH_ALIGNMENT pa =
            (_wcsicmp(m_tsVAlign.c_str(), L"Top") == 0)    ? DWRITE_PARAGRAPH_ALIGNMENT_NEAR :
            (_wcsicmp(m_tsVAlign.c_str(), L"Bottom") == 0) ? DWRITE_PARAGRAPH_ALIGNMENT_FAR :
                                                             DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
        if (pa != m_valign)
        {
            m_fmt->SetParagraphAlignment(pa);
            m_valign = pa;
        }

        const D2D1_RECT_F r = rectf(rc);
        m_rt->DrawText(s.c_str(), static_cast<UINT32>(s.length()), m_fmt, r, brush(fg),
                       D2D1_DRAW_TEXT_OPTIONS_CLIP);
    }

    // 文字量宽：和 text() 走同一个 DWrite TextFormat，量出来的宽度就是画出来的宽度。
    // 光标定位/选区/逐段着色都靠它——用 GDI 量 DWrite 画是两把尺子，长行必然漂。
    float textWidth(const std::wstring& s, int ptSize, const std::wstring& family) override
    {
        if (s.empty()) return 0.0f;
        IDWriteFactory* dw = dwriteFactory();
        if (dw == nullptr || !ensureTextFormat(ptSize, family))
            return -1.0f;

        IDWriteTextLayout* layout = nullptr;
        if (FAILED(dw->CreateTextLayout(s.c_str(), static_cast<UINT32>(s.size()), m_fmt,
                                        1.0e6f, 1.0e6f, &layout)) ||
            layout == nullptr)
            return -1.0f;

        DWRITE_TEXT_METRICS tm = {};
        const float w = SUCCEEDED(layout->GetMetrics(&tm))
                            ? tm.widthIncludingTrailingWhitespace
                            : -1.0f;
        layout->Release();
        return w;
    }

    void clipPush(const RECT& rc) override
    {
        if (m_rt == nullptr) return;
        // D2D 的轴对齐裁剪会被当前 transform 变换（rendertarget 坐标系），
        // 所以这里原样传调用方自己的坐标，不要自己再乘一遍视图矩阵。
        m_rt->PushAxisAlignedClip(rectf(rc), D2D1_ANTIALIAS_MODE_ALIASED);
        ++m_depth;
    }

    void clipPop() override
    {
        if (m_rt == nullptr || m_depth <= 0) return;
        m_rt->PopAxisAlignedClip();
        --m_depth;
    }

    // =======================================================================
    // P0 原语：渐变 / 路径 / 柔和阴影 / 真 opacity 图层 / 几何变换
    // =======================================================================

    void fillRectGradient(const RECT& rc, COLORREF c1, COLORREF c2, int angleDeg) override
    {
        if (m_rt == nullptr) return;
        ID2D1LinearGradientBrush* br = gradient(c1, c2, angleDeg);
        if (br == nullptr) { fillRect(rc, c1); return; }
        const D2D1_POINT_2F a = gradientStart(rc, angleDeg);
        const D2D1_POINT_2F b = gradientEnd(rc, angleDeg);
        br->SetStartPoint(a);
        br->SetEndPoint(b);
        const D2D1_RECT_F r = rectf(rc);
        m_rt->FillRectangle(&r, br);
    }

    void fillRoundRectGradient(const RECT& rc, COLORREF c1, COLORREF c2, int angleDeg,
                               int radius) override
    {
        if (m_rt == nullptr) return;
        ID2D1LinearGradientBrush* br = gradient(c1, c2, angleDeg);
        if (br == nullptr) { fillRoundRect(rc, c1, c1, radius); return; }
        br->SetStartPoint(gradientStart(rc, angleDeg));
        br->SetEndPoint(gradientEnd(rc, angleDeg));
        const float r = radius > 0 ? static_cast<float>(radius) : 1.0f;
        D2D1_ROUNDED_RECT rr = {rectf(rc), r, r};
        m_rt->FillRoundedRectangle(&rr, br);
    }

    void fillPath(const std::vector<POINT>& pts, COLORREF c) override
    {
        if (m_rt == nullptr || pts.size() < 3) return;
        ID2D1PathGeometry* geo = pathOf(pts, true);
        if (geo == nullptr) return;
        m_rt->FillGeometry(geo, brush(c));
        geo->Release();
    }

    void strokePath(const std::vector<POINT>& pts, COLORREF c, int thickness,
                    bool closed) override
    {
        if (m_rt == nullptr || pts.size() < 2) return;
        ID2D1PathGeometry* geo = pathOf(pts, closed);
        if (geo == nullptr) return;
        // HTML 的 .link 是 stroke-linecap: round：D2D 默认平头，必须显式给圆头笔帽
        m_rt->DrawGeometry(geo, brush(c), static_cast<float>(thickness > 0 ? thickness : 1),
                           roundStrokeStyle());
        geo->Release();
    }

    // 圆头笔帽（进程内只建一次）
    ID2D1StrokeStyle* roundStrokeStyle()
    {
        if (m_roundStroke != nullptr) return m_roundStroke;
        ID2D1Factory* f = d2dFactory();
        if (f == nullptr) return nullptr;
        f->CreateStrokeStyle(D2D1::StrokeStyleProperties(
                                 D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE_ROUND,
                                 D2D1_LINE_JOIN_ROUND),
                             nullptr, 0, &m_roundStroke);
        return m_roundStroke;
    }

    void fillSoftShadow(const RECT& rc, int radius, COLORREF color, int alpha,
                        int blur, int dx, int dy) override
    {
        if (m_rt == nullptr || alpha <= 0) return;
        // 每多一层就往外扩 1px、alpha 摊薄一层：层数越多边缘越软。
        // steps 上限 8，再多也看不出差别，只是白画。
        int steps = blur > 0 ? (blur / 2 + 1) : 1;
        if (steps > 8) steps = 8;
        const float r = radius > 0 ? static_cast<float>(radius) : 1.0f;
        for (int k = steps; k >= 1; --k)
        {
            RECT layer = rc;
            ::OffsetRect(&layer, dx, dy);
            ::InflateRect(&layer, k - 1, k - 1);
            int a = static_cast<int>(static_cast<float>(alpha) * 1.7f / static_cast<float>(steps));
            if (a > alpha) a = alpha;
            if (a <= 0) continue;
            D2D1_ROUNDED_RECT rr = {rectf(layer), r + static_cast<float>(k - 1),
                                    r + static_cast<float>(k - 1)};
            m_rt->FillRoundedRectangle(&rr, brushAlpha(color, a));
        }
    }

    // 真 opacity 图层：DCRenderTarget 的 alpha 模式是 IGNORE，PushLayer + opacity 实测会退化成
    // "把目标整块乘 alpha"（结果正好等于 dest*alpha，图层里画的内容全丢），所以改成离屏合成：
    // 新建一张同尺寸的 bitmap 渲染目标，后续绘制都落到它上面，出栈时把整张位图按 alpha
    // 叠回原目标 —— 形状和文字一起淡出，这才是 WPF `Opacity` 的语义。
    void pushOpacity(int alpha) override
    {
        if (m_rt == nullptr) return;
        if (m_opacityDepth > 0)
        {
            ++m_opacityDepth;   // 暂不支持嵌套：内层直接透传，别把状态机弄乱
            return;
        }
        const int a = alpha < 0 ? 0 : (alpha > 255 ? 255 : alpha);
        const D2D1_SIZE_F sz = m_rt->GetSize();
        const D2D1_SIZE_U px = D2D1::SizeU(static_cast<UINT32>(sz.width),
                                            static_cast<UINT32>(sz.height));
        const D2D1_PIXEL_FORMAT fmt = D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,
                                                        D2D1_ALPHA_MODE_PREMULTIPLIED);
        ID2D1BitmapRenderTarget* brt = nullptr;
        if (FAILED(m_rt->CreateCompatibleRenderTarget(&sz, &px, &fmt,
                                                      D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS_NONE, &brt)) ||
            brt == nullptr)
            return;
        // 画笔/渐变笔刷都绑定在具体渲染目标上，换目标前必须整批放掉
        releaseBrushes();
        m_opacityPrev = m_rt;
        m_opacityRt = brt;
        m_opacityAlpha = a;
        m_rt = brt;
        brt->BeginDraw();
        brt->SetTransform(D2D1::Matrix3x2F::Identity());
        brt->Clear(D2D1::ColorF(0, 0, 0, 0));
        m_opacityDepth = 1;
    }

    void popOpacity() override
    {
        if (m_rt == nullptr) return;
        if (m_opacityDepth > 1)
        {
            --m_opacityDepth;
            return;
        }
        if (m_opacityDepth != 1 || m_opacityRt == nullptr || m_opacityPrev == nullptr)
        {
            m_opacityDepth = 0;
            return;
        }
        m_opacityRt->SetTransform(D2D1::Matrix3x2F::Identity());
        m_opacityRt->EndDraw();
        ID2D1Bitmap* bmp = nullptr;
        m_opacityRt->GetBitmap(&bmp);
        releaseBrushes();

        m_rt = m_opacityPrev;
        m_opacityPrev = nullptr;
        m_opacityDepth = 0;
        if (bmp != nullptr)
        {
            const D2D1_SIZE_F sz = m_rt->GetSize();
            const D2D1_RECT_F dst = D2D1::RectF(0, 0, sz.width, sz.height);
            m_rt->DrawBitmap(bmp, dst, static_cast<float>(m_opacityAlpha) / 255.0f,
                             D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
            bmp->Release();
        }
        m_opacityRt->Release();
        m_opacityRt = nullptr;
    }

    // 视图变换由后端持有：pushRotate / popTransform 都基于 m_matrix 叠加或回退，
    // 调用方不再对着 render target 直接 SetTransform（那样一 pop 就丢外层缩放）。
    void setViewTransform(float scale, float tx, float ty) override
    {
        m_matrix = D2D1::Matrix3x2F::Scale(scale, scale) *
                   D2D1::Matrix3x2F::Translation(tx, ty);
        m_matrixStack.clear();
        if (m_rt != nullptr) m_rt->SetTransform(m_matrix);
    }

    void pushRotate(int angleDeg, const POINT& center) override
    {
        if (m_rt == nullptr) return;
        m_matrixStack.push_back(m_matrix);
        m_matrix = D2D1::Matrix3x2F::Rotation(
            static_cast<float>(angleDeg % 360),
            D2D1::Point2F(static_cast<float>(center.x), static_cast<float>(center.y)))
            * m_matrix;
        m_rt->SetTransform(m_matrix);
    }

    void popTransform() override
    {
        if (m_rt == nullptr) return;
        m_matrix = m_matrixStack.empty() ? D2D1::Matrix3x2F::Identity() : m_matrixStack.back();
        if (!m_matrixStack.empty()) m_matrixStack.pop_back();
        m_rt->SetTransform(m_matrix);
    }

    // 位图（PNG）：工具栏图标、控件背景图都走这里（D2D 后端用 WIC 解码后 DrawBitmap）。
    void drawImage(const RECT& rc, const std::wstring& path) override
    {
        ID2D1Bitmap* bmp = image(path);
        if (bmp == nullptr || m_rt == nullptr) return;
        m_rt->DrawBitmap(bmp, rectf(rc), 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
    }

protected:
    virtual bool createTarget(int width, int height) = 0;

    static D2D1_RECT_F rectf(const RECT& rc)
    {
        return D2D1::RectF(static_cast<float>(rc.left), static_cast<float>(rc.top),
                           static_cast<float>(rc.right), static_cast<float>(rc.bottom));
    }

    virtual bool setTarget(ID2D1RenderTarget* rt, bool hardware)
    {
        if (rt == nullptr) return false;
        m_rt = rt;
        m_hardware = hardware;
        m_rt->SetDpi(96.0f, 96.0f);   // 1 DIP = 1 px，坐标与 GDI 一致
        m_rt->SetAntialiasMode(d2dAaEnabled() ? D2D1_ANTIALIAS_MODE_PER_PRIMITIVE
                                              : D2D1_ANTIALIAS_MODE_ALIASED);
        // 深色皮肤下 ClearType 的彩色边很明显（放大看是橙/蓝描边），改用灰度抗锯齿；
        // 需要彩色次像素渲染时设 XML_D2D_CT=1。
        wchar_t ct[8] = {};
        const bool clearType = (::GetEnvironmentVariableW(L"XML_D2D_CT", ct, 8) >= 1 && ct[0] == L'1');
        m_rt->SetTextAntialiasMode(clearType ? D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE
                                            : D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
        return true;
    }

    ID2D1SolidColorBrush* brush(COLORREF c)
    {
        return brushAlpha(c, 255);
    }

    // 半透明画笔：阴影、渐隐、半透明叠层都靠它。缓存键里必须带上 alpha，
    // 否则同一个颜色在半透明和不透明之间来回用会拿到错的画笔。
    ID2D1SolidColorBrush* brushAlpha(COLORREF c, int alpha)
    {
        if (m_rt == nullptr) return nullptr;
        const int a = alpha < 0 ? 0 : (alpha > 255 ? 255 : alpha);
        const DWORD key = (static_cast<DWORD>(c) << 8) | static_cast<DWORD>(a);
        for (auto& e : m_brushesAlpha)
        {
            if (e.first == key && e.second != nullptr)
                return e.second;
        }
        ID2D1SolidColorBrush* br = nullptr;
        m_rt->CreateSolidColorBrush(
            D2D1::ColorF(GetRValue(c) / 255.0f, GetGValue(c) / 255.0f, GetBValue(c) / 255.0f,
                         a / 255.0f), &br);
        if (br != nullptr)
        {
            if (m_brushesAlpha.size() >= 128) m_brushesAlpha.erase(m_brushesAlpha.begin());
            m_brushesAlpha.push_back(std::make_pair(key, br));
        }
        return br;
    }

    // ---- P0 原语依赖：渐变笔刷缓存 / 路径几何 / 图层池 / 变换矩阵 ----

    // 渐变笔刷按 (c1,c2,angle) 缓存，起止点每次按目标矩形重设（D2D 支持 SetStartPoint）。
    ID2D1LinearGradientBrush* gradient(COLORREF c1, COLORREF c2, int angleDeg)
    {
        if (m_rt == nullptr) return nullptr;
        const uint64_t key = (static_cast<uint64_t>(c1) << 32) |
                             static_cast<uint64_t>(c2 & 0xFFFFFF) << 8 |
                             static_cast<uint64_t>(angleDeg & 0xFF);
        for (auto& e : m_gradients)
        {
            if (e.first == key && e.second != nullptr)
                return e.second;
        }
        D2D1_GRADIENT_STOP stops[2] = {};
        stops[0].position = 0.0f;
        stops[0].color = D2D1::ColorF(GetRValue(c1) / 255.0f, GetGValue(c1) / 255.0f,
                                     GetBValue(c1) / 255.0f, 1.0f);
        stops[1].position = 1.0f;
        stops[1].color = D2D1::ColorF(GetRValue(c2) / 255.0f, GetGValue(c2) / 255.0f,
                                     GetBValue(c2) / 255.0f, 1.0f);
        ID2D1GradientStopCollection* coll = nullptr;
        if (FAILED(m_rt->CreateGradientStopCollection(stops, 2, &coll)) || coll == nullptr)
            return nullptr;
        D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES props = {};
        ID2D1LinearGradientBrush* br = nullptr;
        const HRESULT hr = m_rt->CreateLinearGradientBrush(props, coll, &br);
        coll->Release();
        if (FAILED(hr) || br == nullptr) return nullptr;
        if (m_gradients.size() >= 32)
        {
            m_gradients.front().second->Release();
            m_gradients.erase(m_gradients.begin());
        }
        m_gradients.push_back(std::make_pair(key, br));
        return br;
    }

    static D2D1_POINT_2F gradientStart(const RECT& rc, int angleDeg)
    {
        const float w = static_cast<float>(rc.right - rc.left);
        const float h = static_cast<float>(rc.bottom - rc.top);
        const float cx = static_cast<float>(rc.left) + w / 2.0f;
        const float cy = static_cast<float>(rc.top) + h / 2.0f;
        const float rad = static_cast<float>(angleDeg) * 3.14159265f / 180.0f;
        const float dx = (std::cos)(rad) * w / 2.0f;
        const float dy = (std::sin)(rad) * h / 2.0f;
        return D2D1::Point2F(cx - dx, cy - dy);
    }

    static D2D1_POINT_2F gradientEnd(const RECT& rc, int angleDeg)
    {
        const float w = static_cast<float>(rc.right - rc.left);
        const float h = static_cast<float>(rc.bottom - rc.top);
        const float cx = static_cast<float>(rc.left) + w / 2.0f;
        const float cy = static_cast<float>(rc.top) + h / 2.0f;
        const float rad = static_cast<float>(angleDeg) * 3.14159265f / 180.0f;
        const float dx = (std::cos)(rad) * w / 2.0f;
        const float dy = (std::sin)(rad) * h / 2.0f;
        return D2D1::Point2F(cx + dx, cy + dy);
    }

    ID2D1PathGeometry* pathOf(const std::vector<POINT>& pts, bool closed)
    {
        ID2D1Factory* f = d2dFactory();
        if (f == nullptr) return nullptr;
        ID2D1PathGeometry* geo = nullptr;
        ID2D1GeometrySink* sink = nullptr;
        if (FAILED(f->CreatePathGeometry(&geo)) || geo == nullptr) return nullptr;
        if (FAILED(geo->Open(&sink)) || sink == nullptr) { geo->Release(); return nullptr; }

        sink->BeginFigure(D2D1::Point2F(static_cast<float>(pts[0].x), static_cast<float>(pts[0].y)),
                          D2D1_FIGURE_BEGIN_FILLED);
        POINT prev = pts[0];
        for (size_t i = 1; i < pts.size(); ++i)
        {
            // 共线点跳过，避免 D2D 给零长度线段做退化处理
            if (pts[i].x == prev.x && pts[i].y == prev.y) continue;
            sink->AddLine(D2D1::Point2F(static_cast<float>(pts[i].x), static_cast<float>(pts[i].y)));
            prev = pts[i];
        }
        sink->EndFigure(closed ? D2D1_FIGURE_END_CLOSED : D2D1_FIGURE_END_OPEN);
        sink->Close();
        sink->Release();
        return geo;
    }

    // 画笔 / 半透明画笔 / 渐变笔刷都绑定在具体渲染目标上：换目标（离屏图层）前整批释放，
    // 之后按需重建，避免拿着别的目标建的笔刷去画
    void releaseBrushes()
    {
        for (auto& e : m_brushes)
            if (e.second != nullptr) e.second->Release();
        m_brushes.clear();
        for (auto& e : m_brushesAlpha)
            if (e.second != nullptr) e.second->Release();
        m_brushesAlpha.clear();
        for (auto& e : m_gradients)
            if (e.second != nullptr) e.second->Release();
        m_gradients.clear();
    }

    // 图层不能同时 Push 同一个 ID2D1Layer，按嵌套深度各建一个并复用
    ID2D1Layer* acquireLayer(int depth)
    {
        if (m_rt == nullptr || depth < 0) return nullptr;
        while (static_cast<int>(m_layers.size()) <= depth) m_layers.push_back(nullptr);
        if (m_layers[depth] == nullptr && FAILED(m_rt->CreateLayer(&m_layers[depth])))
            return nullptr;
        return m_layers[depth];
    }

    ID2D1Bitmap* image(const std::wstring& path)    {
        if (m_rt == nullptr || path.empty() || wicFactory() == nullptr) return nullptr;

        // XML 里写的是 "res/new.png"，拼出来是混用分隔符的路径；统一成反斜杠再交给 WIC
        std::wstring norm = path;
        for (auto& ch : norm)
            if (ch == L'/') ch = L'\\';

        for (auto& e : m_images)
            if (e.first == norm && e.second != nullptr)
                return e.second;

        IWICBitmapDecoder* dec = nullptr;
        IWICBitmapFrameDecode* frame = nullptr;
        IWICFormatConverter* conv = nullptr;
        ID2D1Bitmap* bmp = nullptr;
        do
        {
            if (FAILED(wicFactory()->CreateDecoderFromFilename(norm.c_str(), nullptr, GENERIC_READ,
                                                              WICDecodeMetadataCacheOnDemand, &dec)))
                break;
            if (FAILED(dec->GetFrame(0, &frame))) break;
            if (FAILED(wicFactory()->CreateFormatConverter(&conv))) break;
            if (FAILED(conv->Initialize(frame, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone,
                                        nullptr, 0.0, WICBitmapPaletteTypeCustom)))
                break;
            if (FAILED(m_rt->CreateBitmapFromWicBitmap(conv, nullptr, &bmp))) break;
        } while (false);

        if (conv != nullptr) conv->Release();
        if (frame != nullptr) frame->Release();
        if (dec != nullptr) dec->Release();

        if (bmp != nullptr)
        {
            if (m_images.size() >= 64)
            {
                m_images.front().second->Release();
                m_images.erase(m_images.begin());
            }
            m_images.push_back(std::make_pair(norm, bmp));
        }
        return bmp;
    }

    bool ensureTextFormat(int ptSize, const std::wstring& family)
    {
        IDWriteFactory* dw = dwriteFactory();
        if (dw == nullptr) return false;
        const int size = ptSize > 0 ? ptSize : xmlUiPointSize();
        const std::wstring face = family.empty() ? xmlUiFace() : family;
        const DWRITE_FONT_WEIGHT fw = textWeightOf();
        const DWRITE_FONT_STYLE fs = textStyleOf();
        // 缓存键必须含字重/斜体，否则 Bold 会复用之前建好的 Normal 格式
        if (m_fmt != nullptr && m_fmtSize == size && m_fmtFace == face &&
            m_fmtWeight == (int)fw && m_fmtStyle == (int)fs)
            return true;

        if (m_fmt != nullptr) { m_fmt->Release(); m_fmt = nullptr; }
        // DWrite 的字号单位是 DIP（1/96 英寸），而这里的 size 是磅值（1/72 英寸）：
        // 直接传会把 9pt 当成 9DIP 画，视觉上比 GDI 的 9pt 小 25%，必须换算。
        const float dip = static_cast<float>(size) * 96.0f / 72.0f;
        const HRESULT hr = dw->CreateTextFormat(face.c_str(), nullptr,
                                               fw, fs,
                                               DWRITE_FONT_STRETCH_NORMAL,
                                               dip, L"zh-cn", &m_fmt);
        if (FAILED(hr) || m_fmt == nullptr) { m_fmt = nullptr; m_hr = hr; return false; }

        // 换行与垂直对齐按段动态设置（text() 里做），这里只保证初始值合法
        m_fmt->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        m_fmt->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
        m_wrap = DWRITE_WORD_WRAPPING_NO_WRAP;
        m_valign = DWRITE_PARAGRAPH_ALIGNMENT_CENTER;

        // 超长文本用省略号收尾：原先只有 NO_WRAP + CLIP，会硬切，与 GDI 的
        // DT_END_ELLIPSIS 不一致（三端像素比对的已知差异之一）
        IDWriteTextFormat1* tf1 = nullptr;
        if (SUCCEEDED(m_fmt->QueryInterface(__uuidof(IDWriteTextFormat1),
                                            reinterpret_cast<void**>(&tf1))) && tf1 != nullptr)
        {
            DWRITE_TRIMMING trim = {};
            trim.granularity = DWRITE_TRIMMING_GRANULARITY_CHARACTER;
            trim.delimiter = 0;
            trim.delimiterCount = 0;
            tf1->SetTrimming(&trim, nullptr);   // 省略号由 DWrite 默认提供
            tf1->Release();
        }

        m_fmtSize = size;
        m_fmtFace = face;
        m_fmtWeight = (int)fw;
        m_fmtStyle = (int)fs;
        m_align = static_cast<DWRITE_TEXT_ALIGNMENT>(-1);   // 强制下一次设置对齐
        return true;
    }

    // 派生类持有的同一渲染目标的第二接口指针，必须在 Release 之前断开
    virtual void onReleaseTarget() {}

    void releaseTarget()
    {
        if (m_dotBrush != nullptr) { m_dotBrush->Release(); m_dotBrush = nullptr; }
        if (m_dotBmp != nullptr) { m_dotBmp->Release(); m_dotBmp = nullptr; }
        m_dotCell = 0;
        for (auto& e : m_brushes)
            if (e.second != nullptr) e.second->Release();
        m_brushes.clear();
        for (auto& e : m_brushesAlpha)
            if (e.second != nullptr) e.second->Release();
        m_brushesAlpha.clear();
        for (auto& e : m_gradients)
            if (e.second != nullptr) e.second->Release();
        m_gradients.clear();
        for (auto& l : m_layers)
            if (l != nullptr) l->Release();
        m_layers.clear();
        m_layerStack.clear();
        m_matrixStack.clear();
        m_matrix = D2D1::Matrix3x2F::Identity();
        // 位图绑定在渲染目标上，目标销毁前必须先放掉
        for (auto& e : m_images)
            if (e.second != nullptr) e.second->Release();
        m_images.clear();
        if (m_fmt != nullptr) { m_fmt->Release(); m_fmt = nullptr; }
        onReleaseTarget();
        // 外部目标（表面设备上下文）由 IXmlSurface 持有，画布只借用、不 Release。
        if (m_ownsTarget && m_rt != nullptr) { m_rt->Release(); }
        m_rt = nullptr;
        m_width = m_height = 0;
        m_depth = 0;
        m_align = static_cast<DWRITE_TEXT_ALIGNMENT>(-1);
    }

    ID2D1RenderTarget* m_rt = nullptr;
    ID2D1Bitmap*     m_dotBmp = nullptr;      // 点阵图块（cell x cell，左上角一个点）
    ID2D1BitmapBrush* m_dotBrush = nullptr;    // 平铺画刷
    int              m_dotCell = 0;
    COLORREF         m_dotColor = 0;
    bool m_ownsTarget = true;                            // 外部目标置 false，析构不释放
    ID2D1StrokeStyle* m_roundStroke = nullptr;          // 圆头笔帽（strokePath 用）
    std::vector<std::pair<COLORREF, ID2D1SolidColorBrush*>> m_brushes;
    std::vector<std::pair<DWORD, ID2D1SolidColorBrush*>> m_brushesAlpha;   // 键 = 颜色<<8 | alpha
    std::vector<std::pair<uint64_t, ID2D1LinearGradientBrush*>> m_gradients;  // 键 = c1/c2/angle
    std::vector<ID2D1Layer*> m_layers;                                // 按嵌套深度复用
    ID2D1RenderTarget* m_opacityPrev = nullptr;                       // 离屏图层前的目标
    ID2D1BitmapRenderTarget* m_opacityRt = nullptr;
    int m_opacityAlpha = 255;
    int m_opacityDepth = 0;
    std::vector<ID2D1Layer*> m_layerStack;
    D2D1_MATRIX_3X2_F m_matrix = D2D1::Matrix3x2F::Identity();
    std::vector<D2D1_MATRIX_3X2_F> m_matrixStack;
    std::vector<std::pair<std::wstring, ID2D1Bitmap*>> m_images;
    IDWriteTextFormat* m_fmt = nullptr;
    int m_width = 0;
    int m_height = 0;
    int m_depth = 0;
    int m_fmtSize = -1;
    DWRITE_TEXT_ALIGNMENT m_align = static_cast<DWRITE_TEXT_ALIGNMENT>(-1);
    std::wstring m_fmtFace;
    // 文字样式（WPF 对齐）：字重/斜体进格式缓存键，换行与垂直对齐按段动态设置
    std::wstring m_tsWeight;
    std::wstring m_tsStyle;
    std::wstring m_tsWrap;
    std::wstring m_tsVAlign;
    int m_fmtWeight = -1;
    int m_fmtStyle = -1;
    DWRITE_WORD_WRAPPING m_wrap = static_cast<DWRITE_WORD_WRAPPING>(-1);
    DWRITE_PARAGRAPH_ALIGNMENT m_valign = static_cast<DWRITE_PARAGRAPH_ALIGNMENT>(-1);
    bool m_hardware = false;
    HRESULT m_hr = S_OK;
};

// ---------------------------------------------------------------------------
// HWND 目标：自己拥有窗口画面（预览器 / 设计器画布）
// ---------------------------------------------------------------------------
class D2DHwndCanvas : public D2DCanvasBase
{
public:
    explicit D2DHwndCanvas(HWND hwnd) : m_hwnd(hwnd) {}

    ID2D1HwndRenderTarget* hwndTarget() const { return m_hrt; }

    // 窗口尺寸变了走 Resize（比重建渲染目标快得多，拖动分割条时尤其明显）
    bool resize(int w, int h)
    {
        if (w <= 0 || h <= 0) return false;
        if (m_hrt == nullptr) return false;
        if (m_width == w && m_height == h) return true;
        if (FAILED(m_hrt->Resize(D2D1::SizeU(static_cast<UINT32>(w), static_cast<UINT32>(h))))
            || m_hrt == nullptr)
        {
            releaseTarget();
            return false;
        }
        m_width = w;
        m_height = h;
        return true;
    }

protected:
    bool createTarget(int width, int height) override
    {
        ID2D1Factory* f = d2dFactory();
        if (f == nullptr || m_hwnd == nullptr) return false;

        D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
            d2dRenderTargetType(),
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE), 96.0f, 96.0f);
        D2D1_HWND_RENDER_TARGET_PROPERTIES hp = D2D1::HwndRenderTargetProperties(
            m_hwnd, D2D1::SizeU(static_cast<UINT32>(width), static_cast<UINT32>(height)),
            D2D1_PRESENT_OPTIONS_NONE);

        ID2D1HwndRenderTarget* rt = nullptr;
        HRESULT hr = f->CreateHwndRenderTarget(props, hp, &rt);
        bool hardware = SUCCEEDED(hr);
        if (FAILED(hr))
        {
            props.type = D2D1_RENDER_TARGET_TYPE_SOFTWARE;   // RDP / 老驱动：退 WARP
            hr = f->CreateHwndRenderTarget(props, hp, &rt);
            hardware = false;
        }
        m_hr = hr;
        if (FAILED(hr) || rt == nullptr) return false;
        m_hrt = rt;
        return setTarget(rt, hardware);
    }

    void onReleaseTarget() override { m_hrt = nullptr; }

private:
    HWND m_hwnd = nullptr;
    ID2D1HwndRenderTarget* m_hrt = nullptr;
};

// ---------------------------------------------------------------------------
// DC 目标：画到别人给的 HDC 上（PyMfc owner-draw / 内存 DC 双缓冲）
// ---------------------------------------------------------------------------
class D2DDcCanvas : public D2DCanvasBase
{
public:
    // 绑定目标 DC 与其设备矩形（通常 {0,0,宽,高}）
    bool bind(HDC dc, const RECT& targetRect)
    {
        const int w = targetRect.right - targetRect.left;
        const int h = targetRect.bottom - targetRect.top;
        if (m_dc == dc && m_rt != nullptr && m_width == w && m_height == h)
            return true;

        // 尺寸没变、只是换了 HDC（owner-draw 每次可能给不同的 HDC）：
        // 只重新 BindDC，保留渲染目标与笔刷/字体缓存。原先这里整个 releaseTarget()，
        // 等于每帧重建目标 + 重建全部笔刷和文字格式。
        if (m_dcRt != nullptr && m_width == w && m_height == h && w > 0 && h > 0)
        {
            const HRESULT hr = m_dcRt->BindDC(dc, &targetRect);
            if (SUCCEEDED(hr))
            {
                m_dc = dc;
                m_bindRect = targetRect;
                m_hr = hr;
                return true;
            }
        }

        m_dc = dc;
        m_bindRect = targetRect;
        releaseTarget();
        return true;
    }

protected:
    bool createTarget(int width, int height) override
    {
        ID2D1Factory* f = d2dFactory();
        if (f == nullptr || m_dc == nullptr) return false;

        D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
            d2dRenderTargetType(),
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE), 96.0f, 96.0f);

        ID2D1DCRenderTarget* rt = nullptr;
        const HRESULT hr = f->CreateDCRenderTarget(&props, &rt);
        m_hr = hr;
        if (FAILED(hr) || rt == nullptr) return false;

        RECT bindRc = m_bindRect;
        if (bindRc.right <= bindRc.left || bindRc.bottom <= bindRc.top)
            bindRc = {0, 0, width, height};

        const HRESULT hrBind = rt->BindDC(m_dc, &bindRc);
        m_hr = hrBind;
        if (FAILED(hrBind))
        {
            rt->Release();
            return false;
        }
        m_dcRt = rt;
        return setTarget(rt, true);
    }

    void onReleaseTarget() override { m_dcRt = nullptr; }

private:
    HDC m_dc = nullptr;
    ID2D1DCRenderTarget* m_dcRt = nullptr;
    RECT m_bindRect = {0, 0, 0, 0};
};

// ---------------------------------------------------------------------------
// 外部目标画布：绑定到表面已建好的 ID2D1RenderTarget（设备上下文）。
// 不拥有目标、不做 BeginDraw/EndDraw：帧边界由 IXmlSurface::BeginFrame/EndFrame 统一管，
// 节点只在一帧内复用这套原语（fillRect/clipPush/…）。供 S2 场景节点绘制使用。
// ---------------------------------------------------------------------------
class D2DCtxCanvas : public D2DCanvasBase
{
public:
    bool attach(ID2D1RenderTarget* rt, int width, int height)
    {
        if (rt == nullptr || width <= 0 || height <= 0) return false;
        m_ownsTarget = false;
        if (m_rt != nullptr && m_rt != rt) releaseTarget();
        m_rt = rt;
        m_width = width;
        m_height = height;
        setTarget(rt, true);
        return true;
    }

    // 帧已由表面 BeginFrame 开启：这里只复位变换/裁剪/图层栈，不再 BeginDraw。
    bool begin(int width, int height) override
    {
        if (m_rt == nullptr || width <= 0 || height <= 0) return false;
        m_width = width;
        m_height = height;
        m_rt->SetTransform(D2D1::Matrix3x2F::Identity());
        m_depth = 0;
        m_matrix = D2D1::Matrix3x2F::Identity();
        m_matrixStack.clear();
        m_layerStack.clear();
        if (m_opacityRt != nullptr) { m_opacityRt->Release(); m_opacityRt = nullptr; }
        m_opacityPrev = nullptr;
        m_opacityDepth = 0;
        return true;
    }

    // 不 EndDraw：结束动作交还表面 EndFrame。
    void end() override {}

protected:
    bool createTarget(int, int) override { return false; }
    void onReleaseTarget() override {}
};
