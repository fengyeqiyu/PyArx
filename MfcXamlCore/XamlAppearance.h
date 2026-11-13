#pragma once
// XamlAppearance —— 所有控件（标准 + 扩展 + Python 自定义控件）共用的外观基元。
//
// 这是"控件基类"的另一半（另一半是 XamlMfcControlBase 的回调表）：
//   形状 Shape      Rect / RoundRect / Ellipse / Capsule
//   后置背景 BackColor  —— 控件底板
//   前置背景 FrontColor —— 底板之上、内容之下的第二层（内衬/高亮）
//   前景色   Foreground —— 文字 / 图标
//   边框     BorderColor / BorderThickness / BorderStyle(None|Single|Sunken|Raised)
//   其它     Radius / Opacity / TextAlign / FontSize / FontFamily
//
// 放在运行时核心（不依赖 MFC），所以设计器画布、XamlViewer 预览、PyMfc 运行时
// 三处渲染共用同一套语义 —— XAML 定义外观，C++ 只负责画。
//
// 编码：本文件 UTF-8 with BOM。

#include "XamlCore.h"

#include <windows.h>

#include <algorithm>
#include <string>
#include <vector>

namespace xamlapp
{
    namespace util
    {
        inline std::string lower(const std::string& s)
        {
            std::string out = s;
            for (char& c : out)
                c = (char)tolower((unsigned char)c);
            return out;
        }

        inline std::string wideToUtf8(const std::wstring& w)
        {
            if (w.empty())
                return {};
            const int len = ::WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
            if (len <= 0)
                return {};
            std::string a(static_cast<size_t>(len), '\0');
            ::WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, a.data(), len, nullptr, nullptr);
            if (!a.empty() && a.back() == '\0')
                a.pop_back();
            return a;
        }

        inline bool strEq(const std::string& a, const std::string& b)
        {
            if (a.size() != b.size())
                return false;
            for (size_t i = 0; i < a.size(); ++i)
                if (tolower((unsigned char)a[i]) != tolower((unsigned char)b[i]))
                    return false;
            return true;
        }

        inline bool keyIs(const std::string& key, const char* name)
        {
            return strEq(key, std::string(name));
        }
    }

    struct XamlAppearance
    {
        std::wstring shape;         // 形状
        std::wstring backColor;     // 后置背景
        std::wstring frontColor;    // 前置背景
        std::wstring foreColor;     // 前景（文字/图标）
        std::wstring borderColor;
        std::wstring borderStyle;   // None / Single / Sunken / Raised
        std::wstring textAlign;     // Left / Center / Right
        std::wstring verticalAlign; // Top / Center / Bottom（WPF VerticalAlignment 对齐）
        std::wstring textWrapping;  // NoWrap / Wrap / WrapWithOverflow（WPF TextWrapping）
        std::wstring fontWeight;    // Normal / Bold / SemiBold / Light
        std::wstring fontStyle;     // Normal / Italic / Oblique
        std::wstring fontFamily;
        std::wstring backImage;     // 后置背景图（保留）
        std::wstring frontImage;    // 前置背景图（保留）
        int borderThickness = 0;    // 0 表示未声明
        int radius = 0;             // 0 表示按形状默认
        int fontSize = 0;           // 0 表示未声明
        int opacity = 255;          // 0..255

        bool empty() const
        {
            return shape.empty() && backColor.empty() && frontColor.empty() &&
                   foreColor.empty() && borderColor.empty() && borderStyle.empty() &&
                   textAlign.empty() && verticalAlign.empty() && textWrapping.empty() &&
                   fontWeight.empty() && fontStyle.empty() &&
                   fontFamily.empty() && borderThickness == 0 &&
                   radius == 0 && fontSize == 0 && opacity == 255;
        }
    };

    // 颜色解析：支持 #RRGGBB、#AARRGGBB、RGB(r,g,b)
    inline bool ParseColor(const std::wstring& s, COLORREF& out, int* alpha = nullptr)
    {
        std::wstring v = s;
        size_t b = v.find_first_not_of(L" \t\r\n");
        if (b == std::wstring::npos)
            return false;
        v = v.substr(b);
        size_t e = v.find_last_not_of(L" \t\r\n");
        if (e != std::wstring::npos)
            v = v.substr(0, e + 1);
        if (v.empty())
            return false;

        if (v[0] == L'#')
        {
            const size_t n = v.size() - 1;
            // 8 位 #AARRGGBB 的数值有可能是 0xFFxxxxxx，超过 LONG_MAX：
            // 用有符号 long 接会溢出成 LONG_MAX，颜色整体变成白色。
            if (n == 8)   // #AARRGGBB
            {
                const unsigned long a = wcstoul(v.c_str() + 1, nullptr, 16);
                if (alpha != nullptr)
                    *alpha = (int)((a >> 24) & 0xFF);
                out = RGB((a >> 16) & 0xFF, (a >> 8) & 0xFF, a & 0xFF);
                return true;
            }
            if (n == 6)
            {
                const unsigned long c = wcstoul(v.c_str() + 1, nullptr, 16);
                out = RGB((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF);
                return true;
            }
            return false;
        }
        if (v.find(L"RGB") == 0 || v.find(L"rgb") == 0)
            return xamlParseColor(v, out);
        return false;
    }

    inline bool IsAppearanceProperty(const std::wstring& name)
    {
        const std::string key = util::lower(util::wideToUtf8(name));
        static const char* k[] = {
            "shape", "backcolor", "background", "frontcolor", "forecolor", "foreground",
            "bordercolor", "bordersize", "borderstyle", "borderthickness", "borderwidth",
            "radius", "cornerradius", "opacity", "textalign", "fontsize", "fontfamily",
            "fontname", "backimage", "frontimage",
            "verticalalignment", "textwrapping", "fontweight", "fontstyle"
        };
        for (auto kk : k)
            if (util::keyIs(key, kk))
                return true;
        return false;
    }

    // 写外观属性：命中返回 true（同一语义可有多个别名）
    inline bool SetProperty(XamlAppearance& ap, const std::wstring& name, const std::wstring& value)
    {
        const std::string key = util::lower(util::wideToUtf8(name));
        if (util::keyIs(key, "shape")) { ap.shape = value; return true; }
        if (util::keyIs(key, "backcolor") || util::keyIs(key, "background")) { ap.backColor = value; return true; }
        if (util::keyIs(key, "frontcolor")) { ap.frontColor = value; return true; }
        if (util::keyIs(key, "forecolor") || util::keyIs(key, "foreground")) { ap.foreColor = value; return true; }
        if (util::keyIs(key, "bordercolor")) { ap.borderColor = value; return true; }
        if (util::keyIs(key, "borderstyle")) { ap.borderStyle = value; return true; }
        if (util::keyIs(key, "borderthickness") || util::keyIs(key, "borderwidth") ||
            util::keyIs(key, "bordersize"))
        { ap.borderThickness = _wtoi(value.c_str()); return true; }
        if (util::keyIs(key, "radius") || util::keyIs(key, "cornerradius")) { ap.radius = _wtoi(value.c_str()); return true; }
        if (util::keyIs(key, "opacity")) { ap.opacity = _wtoi(value.c_str()); return true; }
        if (util::keyIs(key, "textalign")) { ap.textAlign = value; return true; }
        if (util::keyIs(key, "verticalalignment") || util::keyIs(key, "valign")) { ap.verticalAlign = value; return true; }
        if (util::keyIs(key, "textwrapping") || util::keyIs(key, "wrap")) { ap.textWrapping = value; return true; }
        if (util::keyIs(key, "fontweight")) { ap.fontWeight = value; return true; }
        if (util::keyIs(key, "fontstyle")) { ap.fontStyle = value; return true; }
        if (util::keyIs(key, "fontsize")) { ap.fontSize = _wtoi(value.c_str()); return true; }
        if (util::keyIs(key, "fontfamily") || util::keyIs(key, "fontname")) { ap.fontFamily = value; return true; }
        if (util::keyIs(key, "backimage")) { ap.backImage = value; return true; }
        if (util::keyIs(key, "frontimage")) { ap.frontImage = value; return true; }
        return false;
    }

    inline std::wstring GetProperty(const XamlAppearance& ap, const std::wstring& name)
    {
        const std::string key = util::lower(util::wideToUtf8(name));
        if (util::keyIs(key, "shape")) return ap.shape;
        if (util::keyIs(key, "backcolor") || util::keyIs(key, "background")) return ap.backColor;
        if (util::keyIs(key, "frontcolor")) return ap.frontColor;
        if (util::keyIs(key, "forecolor") || util::keyIs(key, "foreground")) return ap.foreColor;
        if (util::keyIs(key, "bordercolor")) return ap.borderColor;
        if (util::keyIs(key, "borderstyle")) return ap.borderStyle;
        if (util::keyIs(key, "borderthickness") || util::keyIs(key, "borderwidth") ||
            util::keyIs(key, "bordersize"))
            return ap.borderThickness == 0 ? std::wstring() : std::to_wstring(ap.borderThickness);
        if (util::keyIs(key, "radius") || util::keyIs(key, "cornerradius"))
            return ap.radius == 0 ? std::wstring() : std::to_wstring(ap.radius);
        if (util::keyIs(key, "opacity")) return std::to_wstring(ap.opacity);
        if (util::keyIs(key, "textalign")) return ap.textAlign;
        if (util::keyIs(key, "verticalalignment") || util::keyIs(key, "valign")) return ap.verticalAlign;
        if (util::keyIs(key, "textwrapping") || util::keyIs(key, "wrap")) return ap.textWrapping;
        if (util::keyIs(key, "fontweight")) return ap.fontWeight;
        if (util::keyIs(key, "fontstyle")) return ap.fontStyle;
        if (util::keyIs(key, "fontsize"))
            return ap.fontSize == 0 ? std::wstring() : std::to_wstring(ap.fontSize);
        if (util::keyIs(key, "fontfamily") || util::keyIs(key, "fontname")) return ap.fontFamily;
        if (util::keyIs(key, "backimage")) return ap.backImage;
        if (util::keyIs(key, "frontimage")) return ap.frontImage;
        return {};
    }

    // 统一属性元数据（属性面板 / 控件 properties() 都用这份，保证"所有控件一样"）
    inline std::vector<XamlProperty> Properties()
    {
        return {
            {L"Shape", XamlPropType::Enum, true, true, L"Rect|RoundRect|Ellipse|Capsule"},
            {L"BackColor", XamlPropType::Color, true, true},
            {L"FrontColor", XamlPropType::Color, true, true},
            {L"Foreground", XamlPropType::Color, true, true},
            {L"BorderColor", XamlPropType::Color, true, true},
            {L"BorderStyle", XamlPropType::Enum, true, true, L"None|Single|Sunken|Raised"},
            {L"BorderThickness", XamlPropType::Int, true, true},
            {L"Radius", XamlPropType::Int, true, true},
            {L"Opacity", XamlPropType::Int, true, true},
            {L"TextAlign", XamlPropType::Enum, true, true, L"Left|Center|Right"},
            {L"VerticalAlignment", XamlPropType::Enum, true, true, L"Top|Center|Bottom"},
            {L"TextWrapping", XamlPropType::Enum, true, true, L"NoWrap|Wrap|WrapWithOverflow"},
            {L"FontWeight", XamlPropType::Enum, true, true, L"Normal|Bold|SemiBold|Light"},
            {L"FontStyle", XamlPropType::Enum, true, true, L"Normal|Italic|Oblique"},
            {L"FontSize", XamlPropType::Int, true, true},
            {L"FontFamily", XamlPropType::String, true, true},
        };
    }

    inline UINT TextFlag(const XamlAppearance& ap)
    {
        if (_wcsicmp(ap.textAlign.c_str(), L"Center") == 0) return DT_CENTER;
        if (_wcsicmp(ap.textAlign.c_str(), L"Right") == 0) return DT_RIGHT;
        return DT_LEFT;
    }

    // 与底色混合（Opacity 生效）
    inline COLORREF Blend(COLORREF fg, COLORREF bg, int opacity)
    {
        if (opacity >= 255) return fg;
        if (opacity <= 0) return bg;
        const int r = (GetRValue(fg) * opacity + GetRValue(bg) * (255 - opacity)) / 255;
        const int g = (GetGValue(fg) * opacity + GetGValue(bg) * (255 - opacity)) / 255;
        const int b = (GetBValue(fg) * opacity + GetBValue(bg) * (255 - opacity)) / 255;
        return RGB(r, g, b);
    }

    inline int ResolveRadius(const RECT& rc, const XamlAppearance& ap)
    {
        const int h = rc.bottom - rc.top;
        if (_wcsicmp(ap.shape.c_str(), L"Capsule") == 0)
            return h / 2;
        if (ap.radius > 0)
            return ap.radius;
        return 8;  // 圆角默认
    }

    inline bool IsRectShape(const XamlAppearance& ap)
    {
        return _wcsicmp(ap.shape.c_str(), L"Rect") == 0;
    }

    inline bool IsEllipseShape(const XamlAppearance& ap)
    {
        return _wcsicmp(ap.shape.c_str(), L"Ellipse") == 0;
    }

    // 按形状填充（可选描边）
    inline void FillShaped(HDC hdc, const RECT& rc, const XamlAppearance& ap,
                           COLORREF fill, COLORREF border, int thickness)
    {
        if (hdc == nullptr)
            return;
        const int th = thickness > 0 ? thickness : 1;
        HBRUSH brush = ::CreateSolidBrush(fill);
        HPEN pen = border == CLR_INVALID
                       ? static_cast<HPEN>(::GetStockObject(NULL_PEN))
                       : ::CreatePen(PS_SOLID, th, border);
        HGDIOBJ oldBrush = ::SelectObject(hdc, brush);
        HGDIOBJ oldPen = ::SelectObject(hdc, pen);

        if (IsEllipseShape(ap))
            ::Ellipse(hdc, rc.left, rc.top, rc.right, rc.bottom);
        else if (IsRectShape(ap))
            ::Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
        else
        {
            const int r = ResolveRadius(rc, ap) * 2;
            ::RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, r, r);
        }

        ::SelectObject(hdc, oldBrush);
        ::SelectObject(hdc, oldPen);
        ::DeleteObject(brush);
        if (border != CLR_INVALID)
            ::DeleteObject(pen);
    }

    // 3D 边框（Sunken / Raised）
    inline void Draw3dFrame(HDC hdc, const RECT& rc, COLORREF base, bool sunken, int thick)
    {
        if (hdc == nullptr)
            return;
        const COLORREF dark = RGB((GetRValue(base) * 55) / 100, (GetGValue(base) * 55) / 100,
                                  (GetBValue(base) * 55) / 100);
        const COLORREF light = RGB((std::min)(255, GetRValue(base) + 60), (std::min)(255, GetGValue(base) + 60),
                                   (std::min)(255, GetBValue(base) + 60));
        const COLORREF top = sunken ? dark : light;
        const COLORREF bottom = sunken ? light : dark;
        RECT r = rc;
        for (int i = 0; i < thick && r.right - r.left > 1 && r.bottom - r.top > 1; ++i)
        {
            HPEN pTop = ::CreatePen(PS_SOLID, 1, top);
            HPEN pBottom = ::CreatePen(PS_SOLID, 1, bottom);
            HGDIOBJ old = ::SelectObject(hdc, pTop);
            ::MoveToEx(hdc, r.left, r.top, nullptr);
            ::LineTo(hdc, r.right - 1, r.top);
            ::MoveToEx(hdc, r.left, r.top, nullptr);
            ::LineTo(hdc, r.left, r.bottom - 1);
            ::SelectObject(hdc, pBottom);
            ::MoveToEx(hdc, r.left, r.bottom - 1, nullptr);
            ::LineTo(hdc, r.right - 1, r.bottom - 1);
            ::MoveToEx(hdc, r.right - 1, r.top, nullptr);
            ::LineTo(hdc, r.right - 1, r.bottom - 1);
            ::SelectObject(hdc, old);
            ::DeleteObject(pTop);
            ::DeleteObject(pBottom);
            ::InflateRect(&r, -1, -1);
        }
    }

    // 后置背景 + 形状 + 边框 + 前置背景（一条龙；文字由调用方随后自己画）
    inline void DrawBackplate(HDC hdc, const RECT& rc, const XamlAppearance& ap,
                              COLORREF fallbackBack, COLORREF fallbackBorder)
    {
        if (hdc == nullptr)
            return;
        COLORREF back = fallbackBack;
        if (!ap.backColor.empty())
        {
            COLORREF c = 0;
            if (ParseColor(ap.backColor, c))
                back = Blend(c, fallbackBack, ap.opacity);
        }
        COLORREF border = fallbackBorder;
        if (!ap.borderColor.empty())
        {
            COLORREF c = 0;
            if (ParseColor(ap.borderColor, c))
                border = c;
        }

        // 只有 XAML 明确写了边框样式（Single/Sunken/Raised）才描边。
        // 不能用"borderStyle 非空"或 fallbackBorder 来兜底，否则未声明边框的控件
        // （TextBlock 这类）会凭空多出一圈深色圆角框。
        const bool styleBorder =
            _wcsicmp(ap.borderStyle.c_str(), L"Single") == 0 ||
            _wcsicmp(ap.borderStyle.c_str(), L"Sunken") == 0 ||
            _wcsicmp(ap.borderStyle.c_str(), L"Raised") == 0;
        const COLORREF stroke = styleBorder ? border : CLR_INVALID;
        FillShaped(hdc, rc, ap, back, stroke,
                   ap.borderThickness > 0 ? ap.borderThickness : 1);

        // 前置背景层：底板之上、内容之下的覆盖块
        if (!ap.frontColor.empty())
        {
            COLORREF c = 0;
            if (ParseColor(ap.frontColor, c))
            {
                const int inset = (ap.borderThickness > 0 ? ap.borderThickness : 1) + 1;
                RECT inner = rc;
                ::InflateRect(&inner, -inset, -inset);
                if (inner.right > inner.left && inner.bottom > inner.top)
                    FillShaped(hdc, inner, ap, c, CLR_INVALID, 0);
            }
        }

        // 3D 风格覆盖一圈明暗
        if (_wcsicmp(ap.borderStyle.c_str(), L"Sunken") == 0 ||
            _wcsicmp(ap.borderStyle.c_str(), L"Raised") == 0)
        {
            Draw3dFrame(hdc, rc, border, _wcsicmp(ap.borderStyle.c_str(), L"Sunken") == 0,
                        (std::max)(1, ap.borderThickness));
        }
    }
}
