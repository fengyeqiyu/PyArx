// XmlDraw.h —— 三端共用的"声明 → 绘制命令"层
//
// 设计器画布、预览器、CAD 运行时（PyMfc owner-draw）都走这一份代码：
//   XmlAppearance（XML 属性语义）
//     → xmlDrawBackplate / xmlDrawText（"画什么"）
//       → XmlCanvas（"用什么画"：Direct2D 后端；GDI 实现已移除）
// 这样换渲染后端时三端一起换，不会出现"预览器好看、CAD 里不对"的漂移。
#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <algorithm>
#include "XmlCore.h"
#include "XmlAppearance.h"

struct XmlCanvas
{
    virtual ~XmlCanvas() = default;

    virtual bool begin(int width, int height) = 0;   // false = 后端不可用（此时不画，调用方自行填底）
    virtual void end() = 0;

    virtual void fillRect(const RECT& rc, COLORREF c) = 0;

    // 点阵平铺：整块区域一次填满（相当于 Qt 画刷的 pattern fill）。
    // 设计器工作台的网格点以前是逐点 fillRect，981x623 @8px 就是上万次调用；
    // D2D 后端重写为"一张 8x8 图块 + 位图画刷"，一次 FillRectangle 完成。
    virtual void dotPattern(const RECT& rc, int cell, COLORREF c)
    {
        if (cell < 2) return;
        for (int gy = rc.top + 8; gy < rc.bottom; gy += cell)
            for (int gx = rc.left + 8; gx < rc.right; gx += cell)
                fillRect(RECT{ gx, gy, gx + 1, gy + 1 }, c);
    }
    // 半透明填充（alpha 0..255）。阴影、渐隐、半透明叠层都靠它。
    // 默认退化成不透明填充：GDI 这类不支持逐像素混合的后端可以直接用，
    // 想要真效果的（D2D）覆写即可。
    virtual void fillRectAlpha(const RECT& rc, COLORREF c, int alpha)
    {
        (void)alpha;
        fillRect(rc, c);
    }
    // 圆角矩形：先铺边框色，再内缩铺面色（比"描边"更锐，避免半像素模糊）
    virtual void fillRoundRect(const RECT& rc, COLORREF face, COLORREF border, int radius) = 0;
    virtual void fillEllipse(const RECT& rc, COLORREF c, COLORREF border) = 0;
    virtual void strokeRect(const RECT& rc, COLORREF c, int thickness) = 0;
    // 圆角描边（焦点环用）。默认退化成直角描边，D2D 后端画真圆弧。
    virtual void strokeRoundRect(const RECT& rc, COLORREF c, int radius, int thickness)
    {
        strokeRect(rc, c, thickness);
    }
    virtual void strokeLine(const POINT& a, const POINT& b, COLORREF c, int thickness) = 0;
    virtual void frame3d(const RECT& rc, COLORREF base, bool sunken, int thickness) = 0;

    // ptSize <= 0 时用系统界面字体；align 用 DT_LEFT/DT_CENTER/DT_RIGHT
    virtual void text(const RECT& rc, const std::wstring& s, COLORREF fg,
                      UINT align, int ptSize, const std::wstring& family) = 0;

    // 文字量宽（逻辑像素，与 text() 用同一份字体与字号）。
    // 返回 < 0 表示后端没有测量能力，调用方自行退回别的手段（如 GDI 的 GetTextExtentPoint32）。
    // 光标定位、选区高亮、逐段着色的起点全靠它——用别的尺子量会和绘制错位。
    virtual float textWidth(const std::wstring& s, int ptSize, const std::wstring& family)
    {
        (void)s; (void)ptSize; (void)family;
        return -1.0f;
    }

    virtual void clipPush(const RECT& rc) = 0;
    virtual void clipPop() = 0;

    // =======================================================================
    // P0 绘制原语（渐变 / 阴影 / 路径 / 真 opacity 图层 / 几何变换）
    // 默认实现都退化成"最接近的不透明画法"，保证任何后端缺这些能力时
    // 仍然是"画得出来但没那么好看"，而不是整段绘制失败。
    // =======================================================================

    // 线性渐变填充（angleDeg：0=左→右，90=上→下）。默认退化成单色填充。
    virtual void fillRectGradient(const RECT& rc, COLORREF c1, COLORREF /*c2*/,
                                  int /*angleDeg*/)
    {
        fillRect(rc, c1);
    }
    virtual void fillRoundRectGradient(const RECT& rc, COLORREF c1, COLORREF /*c2*/,
                                       int /*angleDeg*/, int radius)
    {
        fillRoundRect(rc, c1, c1, radius);
    }

    // 折线路径：pts 为绝对坐标（已含画布偏移）。fill 版按多边形填充，stroke 版描边。
    virtual void fillPath(const std::vector<POINT>& pts, COLORREF c)
    {
        if (pts.empty()) return;
        POINT lo = pts[0], hi = pts[0];
        for (const POINT& pt : pts)
        {
            lo.x = (std::min)(lo.x, pt.x); lo.y = (std::min)(lo.y, pt.y);
            hi.x = (std::max)(hi.x, pt.x); hi.y = (std::max)(hi.y, pt.y);
        }
        fillRect({lo.x, lo.y, hi.x + 1, hi.y + 1}, c);
    }
    virtual void strokePath(const std::vector<POINT>& /*pts*/, COLORREF /*c*/,
                            int /*thickness*/, bool /*closed*/) {}

    // 柔和阴影：先按 offset 铺一层放大 + 半透明的圆角矩形，blur 越大铺的层数越多
    // （每层 alpha 递减），得到"软边"效果；后端不支持 alpha 时只剩一层硬阴影。
    virtual void fillSoftShadow(const RECT& rc, int radius, COLORREF color, int alpha,
                                int blur, int dx, int dy)
    {
        RECT r = rc;
        ::OffsetRect(&r, dx, dy);
        fillRectAlpha(r, color, alpha);
        (void)radius; (void)blur;
    }

    // 真 opacity 图层：图层内所有绘制（形状 + 文字）按 alpha 统一混合，
    // 而不是逐条颜色与底色 Blend。成对调用。
    virtual void pushOpacity(int /*alpha*/) {}
    virtual void popOpacity() {}

    // 几何变换（绕 center 旋转 angleDeg）。成对调用；默认不平移不旋转。
    virtual void pushRotate(int /*angleDeg*/, const POINT& /*center*/) {}
    virtual void popTransform() {}

    // 位图（PNG）：工具栏图标、控件背景图都走这里（D2D 后端用 WIC 解码后 DrawBitmap）。
    // 给默认空实现而不是纯虚：自定义后端缺图标能力时也能编译，
    // 不会因为少一个图标能力整段绘制失败。
    virtual void drawImage(const RECT& rc, const std::wstring& path) { (void)rc; (void)path; }

    // 文字样式（WPF 对齐）：在 text() 之前调用，作用于随后的文本绘制。
    // weight  = Normal / Bold / SemiBold / Light
    // style   = Normal / Italic / Oblique
    // wrapping= NoWrap / Wrap / WrapWithOverflow
    // valign  = Top / Center / Bottom
    // 默认空实现：不实现该能力的后端行为退回原样（单行、居中、常规字重）。
    virtual void setTextStyle(const std::wstring& weight, const std::wstring& style,
                              const std::wstring& wrapping, const std::wstring& valign)
    {
        (void)weight; (void)style; (void)wrapping; (void)valign;
    }
};

// 系统界面字体名（DWrite 后端取它作为默认字体名）
inline std::wstring xmlUiFace()
{
    static std::wstring face;
    if (!face.empty()) return face;
    NONCLIENTMETRICSW ncm = {};
    ncm.cbSize = sizeof(ncm);
    if (::SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0))
        face = ncm.lfMessageFont.lfFaceName;
    if (face.empty()) face = L"Segoe UI";
    return face;
}

// 系统界面字体的磅值（ptSize <= 0 时用它）
inline int xmlUiPointSize()
{
    NONCLIENTMETRICSW ncm = {};
    ncm.cbSize = sizeof(ncm);
    int px = 12;
    if (::SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0) && ncm.lfMessageFont.lfHeight != 0)
        px = -ncm.lfMessageFont.lfHeight;
    HDC screen = ::GetDC(nullptr);
    const int dpi = screen != nullptr ? ::GetDeviceCaps(screen, LOGPIXELSY) : 96;
    if (screen != nullptr) ::ReleaseDC(nullptr, screen);
    const int pt = ::MulDiv(px, 72, dpi > 0 ? dpi : 96);
    // 界面基准字号比系统消息字体大一磅：12px 的 9pt 在现代高分屏上偏挤。
    // 这里只是"未声明字号时的默认值"，单个控件仍可用 XML 的 FontSize 覆盖
    //（XmlAppearance.fontSize > 0 时优先，见 xmlDrawText / 各控件绘制入口）。
    return pt > 0 ? pt + 1 : 10;
}

// ---------------------------------------------------------------------------
// 外观 → 绘制命令（与 XmlAppearance::DrawBackplate 逐条等价，三端唯一实现）
// ---------------------------------------------------------------------------
inline void xmlDrawBackplate(XmlCanvas& cv, const RECT& rc, const xmlapp::XmlAppearance& ap,
                              COLORREF fallbackBack, COLORREF fallbackBorder)
{
    COLORREF back = fallbackBack;
    if (!ap.backColor.empty())
    {
        COLORREF c = 0;
        if (xmlapp::ParseColor(ap.backColor, c))
            back = xmlapp::Blend(c, fallbackBack, ap.opacity);
    }
    COLORREF border = fallbackBorder;
    if (!ap.borderColor.empty())
    {
        COLORREF c = 0;
        if (xmlapp::ParseColor(ap.borderColor, c)) border = c;
    }

    const bool styleBorder = _wcsicmp(ap.borderStyle.c_str(), L"Single") == 0 ||
                             _wcsicmp(ap.borderStyle.c_str(), L"Sunken") == 0 ||
                             _wcsicmp(ap.borderStyle.c_str(), L"Raised") == 0;
    const int th = ap.borderThickness > 0 ? ap.borderThickness : 1;

    if (xmlapp::IsEllipseShape(ap))
        cv.fillEllipse(rc, back, styleBorder ? border : back);
    else if (xmlapp::IsRectShape(ap))
    {
        cv.fillRect(rc, back);
        if (styleBorder) cv.strokeRect(rc, border, th);
    }
    else
    {
        const int r = xmlapp::ResolveRadius(rc, ap);
        cv.fillRoundRect(rc, back, styleBorder ? border : back, r);
    }

    if (!ap.frontColor.empty())
    {
        COLORREF c = 0;
        if (xmlapp::ParseColor(ap.frontColor, c))
        {
            RECT inner = rc;
            const int inset = th + 1;
            ::InflateRect(&inner, -inset, -inset);
            if (inner.right > inner.left && inner.bottom > inner.top)
            {
                if (xmlapp::IsEllipseShape(ap))
                    cv.fillEllipse(inner, c, c);
                else if (xmlapp::IsRectShape(ap))
                    cv.fillRect(inner, c);
                else
                    cv.fillRoundRect(inner, c, c, (std::max)(1, xmlapp::ResolveRadius(inner, ap)));
            }
        }
    }

    if (_wcsicmp(ap.borderStyle.c_str(), L"Sunken") == 0)
        cv.frame3d(rc, border, true, (std::max)(1, ap.borderThickness));
    else if (_wcsicmp(ap.borderStyle.c_str(), L"Raised") == 0)
        cv.frame3d(rc, border, false, (std::max)(1, ap.borderThickness));
}

inline void xmlDrawText(XmlCanvas& cv, RECT rc, const std::wstring& s,
                         const xmlapp::XmlAppearance& ap, bool styled,
                         COLORREF defFg, UINT defAlign, int padLeft)
{
    if (s.empty() || rc.right <= rc.left || rc.bottom <= rc.top) return;

    COLORREF fg = defFg;
    if (styled && !ap.foreColor.empty())
    {
        COLORREF c = 0;
        if (xmlapp::ParseColor(ap.foreColor, c)) fg = c;
    }
    UINT align = styled ? xmlapp::TextFlag(ap) : defAlign;
    if (padLeft > 0 && (align & DT_LEFT) != 0) rc.left += padLeft;

    const int pt = (styled && ap.fontSize > 0) ? ap.fontSize : 0;
    if (styled)
        cv.setTextStyle(ap.fontWeight, ap.fontStyle, ap.textWrapping, ap.verticalAlign);
    cv.text(rc, s, fg, align, pt, styled ? ap.fontFamily : std::wstring());
}
