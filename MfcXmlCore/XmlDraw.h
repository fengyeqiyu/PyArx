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
#include "XmlMatrix4.h"

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
    // CSS 分角圆角：四个角半径不同（border-top-left-radius 等）。
    // 默认实现退化成统一半径（取最大值），有能力的画布（D2D）覆盖成真路径。
    virtual void fillRoundRectCorners(const RECT& rc, COLORREF face, COLORREF border,
                                      int tl, int tr, int br, int bl)
    {
        int m = (std::max)((std::max)(tl, tr), (std::max)(br, bl));
        fillRoundRect(rc, face, border, m);
    }
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

    // CSS text-decoration 的落点：文字基线在矩形内的 Y（像素，绝对坐标）。
    // HTML 的下划线永远贴着文字基线；写成"盒子底部减几像素"，一旦盒子变高、
    // 或文字改成垂直居中/顶对齐，整条线就会离开文字。算不出基线时返回 false。
    virtual bool textBaseline(const RECT& rc, const std::wstring& s, int ptSize,
                              const std::wstring& family, double& baselineY)
    {
        (void)rc; (void)s; (void)ptSize; (void)family; (void)baselineY;
        return false;
    }

    virtual void clipPush(const RECT& rc) = 0;
    virtual void clipPop() = 0;

    // 圆角裁剪（CSS border-radius 的"内容裁剪"语义）：HTML 里 `<img style="border-radius:50%">`
    // 画出来是圆形头像，方角裁剪只能把图裁成方框，四角一定会露出来。
    // 默认退化成就按矩形裁：没实现该能力的后端仍然画得出来，只是四角不圆。
    virtual void clipPushRound(const RECT& rc, int radius) { (void)radius; clipPush(rc); }
    virtual void clipPopRound() { clipPop(); }

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
    // 通用 4×4 变换（列向量约定 p' = M·p）。XML/CSS 的 transform、画布 3D 视图都走这一条：
    // 4×4 涵盖平移/缩放/旋转/斜切乃至 rotateX/Y，后端只按自己的维度取投影
    // （D2D 取 2D 子矩阵，GDI 不支持则保持原样）。
    virtual void pushTransform4(const xmlgeo::XmlMatrix4& /*mat*/) {}
    // 视图变换（缩放 + 平移）：后端自己保存矩阵，随后的 push/pop 在它之上叠加，
    // 不会再"恢复 Identity"把外层缩放抹掉。调用方（设计器画布等）用它代替直接 SetTransform。
    virtual void setViewTransform(float /*scale*/, float /*tx*/, float /*ty*/) {}

    // 位图（PNG）：工具栏图标、控件背景图都走这里（D2D 后端用 WIC 解码后 DrawBitmap）。
    // 给默认空实现而不是纯虚：自定义后端缺图标能力时也能编译，
    // 不会因为少一个图标能力整段绘制失败。
    virtual void drawImage(const RECT& rc, const std::wstring& path) { (void)rc; (void)path; }
    // 图片原始像素尺寸（object-fit 的 contain/cover/none 都要按原图比例算目标矩形）。
    // 返回 false 表示这个后端拿不到尺寸，调用方退回 fill（直接铺满）。
    virtual bool imageSize(const std::wstring& path, int& w, int& h)
    {
        (void)path; w = 0; h = 0; return false;
    }
    // 圆角裁图：按 mask 的圆角轮廓裁掉四角，图本身画在 dst（object-fit 时 dst 可能小于 mask）。
    // rx/ry 分开传：CSS `border-radius:50%` 在非正方形盒子上是椭圆（`ResolveRadiusXY` 给这两个值）。
    // 默认后端只能方角裁剪；D2D 后端用逐行轴对齐裁剪拼圆弧。
    virtual void drawImageClippedRound(const RECT& mask, int rx, int ry, const RECT& dst,
                                       const std::wstring& path)
    {
        if (rx <= 0 || ry <= 0) { drawImage(dst, path); return; }
        clipPushRound(mask, rx);
        drawImage(dst, path);
        clipPopRound();
    }

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

    // CSS letter-spacing（字距，px，可为负）。0 = 关闭，退回原绘制快路径。
    // 默认空实现：不支持该能力的后端就当没写。
    virtual void setLetterSpacing(double px) { (void)px; }
    // CSS text-decoration:underline。默认空实现（不支持的后端当没写）。
    virtual void setUnderline(bool on) { (void)on; }
    // CSS text-shadow（文字投影）。blur < 0 表示关闭；D2D 后端在 text() 里先画阴影层。
    virtual void setTextShadow(int ox, int oy, int blur, COLORREF color)
    {
        (void)ox; (void)oy; (void)blur; (void)color;
    }
    // CSS line-height（行距）。isMultiplier=true 时 px 是倍数（`line-height:1.5`），
    // 否则是绝对像素（`line-height:24px`）。0 = 自动（跟随字体默认行距）。
    virtual void setLineHeight(double px, bool isMultiplier) { (void)px; (void)isMultiplier; }
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

    const bool anySide = ap.borderSideW[0] >= 0 || ap.borderSideW[1] >= 0 ||
                         ap.borderSideW[2] >= 0 || ap.borderSideW[3] >= 0;
    const bool styleBorder = xmlapp::HasVisibleBorder(ap) && !anySide;
    const int th = ap.borderThickness > 0 ? ap.borderThickness : 1;
    // CSS border-style 的虚线/双线：HTML 里 `border:2px dashed #f00` 是最常见的
    // "虚线分割"写法，早先 borderStyle 只认 Sunken/Raised，dashed/dotted/double
    // 全被当 solid 画 —— 写了看不见差别，等于画错。圆角盒子仍退回 solid（虚线沿
    // 圆角取样偏差大，宁可按 solid 画也不糊成一圈）。
    const bool dashedB = styleBorder &&
        (_wcsicmp(ap.borderStyle.c_str(), L"Dashed") == 0 ||
         _wcsicmp(ap.borderStyle.c_str(), L"Dotted") == 0);
    const bool doubleB = styleBorder && _wcsicmp(ap.borderStyle.c_str(), L"Double") == 0;
    int cornerProbe[4] = {0, 0, 0, 0};
    xmlapp::ResolveCorners(rc, ap, cornerProbe);
    const bool rounded = !xmlapp::IsRectShape(ap) &&
                         (cornerProbe[0] > 0 || cornerProbe[1] > 0 ||
                          cornerProbe[2] > 0 || cornerProbe[3] > 0);
    const bool strokePattern = (dashedB || doubleB) && !rounded &&
                               !xmlapp::IsEllipseShape(ap);

    // CSS linear-gradient 背板：与 HTML 同一个写法，画布层已有渐变能力（D2D 走真渐变刷子，
    // 没有该能力的后端退化成首色纯填充）。注意角度换算：画布契约是 0=左→右、90=上→下，
    // 而 CSS 是 0=下→上、90=右→左、180=上→下，需要交换首尾色而不是硬套。
    COLORREF g1 = 0, g2 = 0;
    int gCss = 180;
    if (!ap.bgImage.empty() && xmlapp::ParseLinearGradient(ap.bgImage, g1, g2, gCss))
    {
        int css = ((gCss % 360) + 360) % 360;
        COLORREF from = g1, to = g2;
        int apiAngle = 90; // 默认上→下（CSS 180deg）
        if (css == 90) { apiAngle = 0; }
        else if (css == 270) { apiAngle = 0; from = g2; to = g1; }
        else if (css == 0) { apiAngle = 90; from = g2; to = g1; }
        else if (css > 45 && css <= 135) { apiAngle = 0; }
        else if (css > 225 && css < 315) { apiAngle = 0; from = g2; to = g1; }

        const COLORREF fc = xmlapp::Blend(from, fallbackBack, ap.opacity);
        const COLORREF tc = xmlapp::Blend(to, fallbackBack, ap.opacity);
        if (xmlapp::IsEllipseShape(ap) || xmlapp::IsRectShape(ap))
            cv.fillRectGradient(rc, fc, tc, apiAngle);
        else
        {
            // 与纯色圆角同一处病：渐变圆角的弧外也要先用父底色垫掉
            cv.fillRect(rc, fallbackBack);
            cv.fillRoundRectGradient(rc, fc, tc, apiAngle, xmlapp::ResolveRadius(rc, ap));
        }
        if (styleBorder && !strokePattern)
            cv.strokeRect(rc, border, th);
    }
    else if (xmlapp::IsEllipseShape(ap))
        cv.fillEllipse(rc, back, styleBorder ? border : back);
    else if (xmlapp::IsRectShape(ap))
    {
        cv.fillRect(rc, back);
        if (styleBorder && !strokePattern) cv.strokeRect(rc, border, th);
    }
    else
    {
        int c[4];
        xmlapp::ResolveCorners(rc, ap, c);
        // CSS 圆角盒子落在矩形窗口上，弧外那圈像素只能"用父底色填掉"。
        // 以前这里直接画圆角填充+描边，弧外留空 —— 窗口的擦除刷（底色刷）会从缺口漏出来，
        // 于是按钮四角是方角的浅色方块，弧线却缩在里面，看着像"描边圆了、背景没圆"。
        cv.fillRect(rc, fallbackBack);
        if (c[0] != c[1] || c[1] != c[2] || c[2] != c[3])
            cv.fillRoundRectCorners(rc, back, styleBorder ? border : back, c[0], c[1], c[2], c[3]);
        else if (styleBorder && !strokePattern && th > 1)
        {
            // 圆角盒子的 border-width 也要认：先按边框色铺整块圆角当"边带"，
            // 再用背色补内圈。以前圆角分支固定画 1px 描边，`border:2px` 在按钮/卡片上
            // 只剩一条细线，跟 HTML 的厚重感差一截。
            RECT in = rc;
            ::InflateRect(&in, -th, -th);
            cv.fillRoundRect(rc, border, border, c[0]);
            if (in.right > in.left && in.bottom > in.top)
            {
                int cr = c[0] - th;
                if (cr < 0) cr = 0;
                cv.fillRoundRect(in, back, back, cr);
            }
        }
        else
            cv.fillRoundRect(rc, back, styleBorder ? border : back, c[0]);
    }

    // border-style: dashed / dotted / double —— 按 CSS 的"虚线/双线"实际落笔。
    if (strokePattern)
    {
        if (doubleB)
        {
            // CSS double：两条实线，各占 th/3，中间留 th/3 空
            const int w1 = (std::max)(1, (th + 2) / 3);
            cv.strokeRect(rc, border, w1);
            if (th >= 3)
            {
                RECT b = rc;
                ::InflateRect(&b, -(th - w1), -(th - w1));
                if (b.right > b.left && b.bottom > b.top)
                    cv.strokeRect(b, border, w1);
            }
        }
        else
        {
            // dashed 段长 2*th、dotted 段长 th，四边各画一趟
            const int seg = (std::max)(1, th * ((_wcsicmp(ap.borderStyle.c_str(), L"Dotted") == 0) ? 1 : 2));
            auto band = [&](int x0, int y0, int run, bool horizontal) {
                for (int s = 0; s < run; s += seg * 2)
                {
                    const int len = (std::min)(seg, run - s);
                    if (len <= 0) break;
                    RECT r = horizontal ? RECT{ x0 + s, y0, x0 + s + len, y0 + th }
                                        : RECT{ x0, y0 + s, x0 + th, y0 + s + len };
                    cv.fillRect(r, border);
                }
            };
            RECT r = rc;
            band(r.left, r.top, r.right - r.left, true);
            band(r.left, r.bottom - th, r.right - r.left, true);
            band(r.left, r.top + th, r.bottom - r.top - th * 2, false);
            band(r.right - th, r.top + th, r.bottom - r.top - th * 2, false);
        }
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

    // CSS 单边边框（border-top/right/bottom/left）：HTML 里画一条分割线的标准写法。
    // 顺序 0=上 1=右 2=下 3=左；未声明的边沿用整圈 borderThickness（0 就不画）。
    if (anySide)
    {
        auto sideColor = [&](int i) -> COLORREF {
            COLORREF c = CLR_INVALID;
            if (!ap.borderSideColor[i].empty() && xmlapp::ParseColor(ap.borderSideColor[i], c))
                return c;
            return border;
        };
        for (int i = 0; i < 4; ++i)
        {
            const int w = ap.borderSideW[i] >= 0 ? ap.borderSideW[i]
                                                 : (ap.borderThickness > 0 ? ap.borderThickness : 0);
            if (w <= 0)
                continue;
            RECT band = rc;
            if (i == 0)      band.bottom = band.top + w;
            else if (i == 2) band.top = band.bottom - w;
            else if (i == 3) band.right = band.left + w;
            else             band.left = band.right - w;
            if (band.right <= band.left || band.bottom <= band.top)
                continue;
            cv.fillRect(band, sideColor(i));
        }
    }
}

// CSS box-shadow（外投影）：影子落在盒子外，控件自己的子窗口画布会把盒外部分裁掉，
// 所以只能在宿主整窗画布上落笔。逐圈画 1px 半透明环，越靠外越淡 —— 与 HTML 的
// blur 观感一致（不追求高斯，追求"盒下有一圈压暗带"这个语义）。
inline void xmlDrawBoxShadow(XmlCanvas& cv, const RECT& rc, const std::wstring& spec)
{
    if (spec.empty()) return;
    std::vector<xmlapp::BoxShadowSpec> list;
    xmlapp::ParseBoxShadowSpecList(spec, list);
    for (const auto& sp : list)
    {
        if (!sp.valid || sp.inset) continue;
        RECT base = rc;
        ::OffsetRect(&base, sp.ox, sp.oy);
        if (sp.spread != 0) ::InflateRect(&base, sp.spread, sp.spread);
        if (base.right <= base.left || base.bottom <= base.top) continue;
        const int blur = (std::max)(0, sp.blur);
        if (blur == 0)
        {
            // 无模糊：贴着盒子画一圈 1px 实边（盒外）
            RECT ring = base;
            ::InflateRect(&ring, 1, 1);
            const int a = (std::max)(1, sp.alpha);
            cv.fillRectAlpha(RECT{ ring.left, ring.top, ring.right, ring.top + 1 }, sp.color, a);
            cv.fillRectAlpha(RECT{ ring.left, ring.bottom - 1, ring.right, ring.bottom }, sp.color, a);
            cv.fillRectAlpha(RECT{ ring.left, ring.top + 1, ring.left + 1, ring.bottom - 1 }, sp.color, a);
            cv.fillRectAlpha(RECT{ ring.right - 1, ring.top + 1, ring.right, ring.bottom - 1 }, sp.color, a);
            continue;
        }
        const int denom = blur * 2 + 1;
        for (int k = blur; k >= 1; --k)
        {
            RECT r = base;
            ::InflateRect(&r, k, k);
            if (r.right <= r.left || r.bottom <= r.top) continue;
            const int a = (std::max)(1, sp.alpha * (blur - k + 1) / denom);
            cv.fillRectAlpha(RECT{ r.left, r.top, r.right, r.top + 1 }, sp.color, a);
            cv.fillRectAlpha(RECT{ r.left, r.bottom - 1, r.right, r.bottom }, sp.color, a);
            cv.fillRectAlpha(RECT{ r.left, r.top + 1, r.left + 1, r.bottom - 1 }, sp.color, a);
            cv.fillRectAlpha(RECT{ r.right - 1, r.top + 1, r.right, r.bottom - 1 }, sp.color, a);
        }
    }
}

// CSS outline：不占盒模型，画在盒子外沿（outline-offset 可正可负）。
// HTML 里焦点框就这一行 `outline:2px solid #0af`；XML 早先画不出盒外带。
// 注意：outline 全落在盒子外侧，**不能**在控件自己的画布上画（子窗口会把盒外部分裁掉），
// 必须由宿主在整窗画布上落笔（见 PyMfcHost::drawPaintNodes）。
inline void xmlDrawOutline(XmlCanvas& cv, const RECT& rc, const xmlapp::XmlAppearance& ap)
{
    if (ap.outlineWidth <= 0) return;

    COLORREF oc = RGB(0, 0, 0);                     // CSS 缺省 = currentColor
    COLORREF tmp = 0;
    if (!ap.outlineColor.empty())
    {
        if (xmlapp::ParseColor(ap.outlineColor, tmp)) oc = tmp;
    }
    else if (!ap.foreColor.empty() && xmlapp::ParseColor(ap.foreColor, tmp))
        oc = tmp;

    const int w = ap.outlineWidth;
    const int off = ap.outlineOffset;
    RECT outer = rc;
    ::InflateRect(&outer, off + w, off + w);
    if (outer.right <= outer.left || outer.bottom <= outer.top) return;

    const bool dashed = _wcsicmp(ap.outlineStyle.c_str(), L"Dashed") == 0;
    const bool dotted = _wcsicmp(ap.outlineStyle.c_str(), L"Dotted") == 0;
    const int seg = (std::max)(1, w * (dashed ? 3 : 1));
    auto bandPx = [&](int x0, int y0, int run, bool horizontal) {
        if (!dashed && !dotted)
        {
            RECT r = horizontal ? RECT{ x0, y0, x0 + run, y0 + w }
                                : RECT{ x0, y0, x0 + w, y0 + run };
            cv.fillRect(r, oc);
            return;
        }
        for (int s = 0; s < run; s += seg * 2)
        {
            const int len = (std::min)(seg, run - s);
            if (len <= 0) break;
            RECT r = horizontal ? RECT{ x0 + s, y0, x0 + s + len, y0 + w }
                                : RECT{ x0, y0 + s, x0 + w, y0 + s + len };
            cv.fillRect(r, oc);
        }
    };
    bandPx(outer.left, outer.top, outer.right - outer.left, true);                        // 上
    bandPx(outer.left, outer.bottom - w, outer.right - outer.left, true);                 // 下
    bandPx(outer.left, outer.top + w, outer.bottom - outer.top - 2 * w, false);           // 左
    bandPx(outer.right - w, outer.top + w, outer.bottom - outer.top - 2 * w, false);      // 右
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
    {
        cv.setTextStyle(ap.fontWeight, ap.fontStyle, ap.textWrapping, ap.verticalAlign);
        cv.setLetterSpacing(ap.letterSpacing);
        cv.setUnderline(ap.underline);
    }
    // CSS text-shadow：交给画布（D2D 在后端里先画阴影层再画正文，不会出现两层互相覆盖）。
    // 不看 styled：阴影是元素样式，声明了就生效；未声明时显式关掉，避免上一次绘制的状态残留。
    {
        xmlapp::TextShadowSpec ts;
        if (!ap.textShadow.empty() && xmlapp::ParseTextShadow(ap.textShadow, ts))
            cv.setTextShadow(ts.ox, ts.oy, ts.blur, ts.color);
        else
            cv.setTextShadow(0, 0, -1, 0);
    }
    cv.text(rc, s, fg, align, pt, styled ? ap.fontFamily : std::wstring());
}
