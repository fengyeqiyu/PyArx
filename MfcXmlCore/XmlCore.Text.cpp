// XmlCore.Engine.cpp —— 拆自 XmlCore.cpp 第 115-3494 行。
#include "XmlCoreCommon.h"
#include "XmlCoreInternal.h"

// ===========================================================================
// 工具
// ===========================================================================
namespace xmlcore_impl
{
    // 文本基线
    // 盒顶到文字基线 = 内边距 + ascent（文字垂直居中的控件再让出居中偏移）。
    // 字号/字体族/字重/斜体都参与度量，按「字体族|字号|字重|斜体」缓存；
    // 设计器画布与运行时窗口都调这一处，两边算出的基线必然一致。
    // ===================================================================
    struct TextMetrics
    {
        int ascent = 0;
        int descent = 0;
    };

    TextMetrics TextMetricsOf(const XmlElement& el)
    {
        int pt = _wtoi(el.get(L"FontSize").c_str());
        if (pt <= 0)
            pt = 9;
        std::wstring face = el.get(L"FontFamily");
        if (face.empty())
            face = L"Microsoft YaHei UI";
        int weight = FW_NORMAL;
        const std::wstring fw = el.get(L"FontWeight");
        if (_wcsicmp(fw.c_str(), L"Bold") == 0 ||
            _wcsicmp(el.get(L"Bold").c_str(), L"true") == 0)
            weight = FW_BOLD;
        else if (_wcsicmp(fw.c_str(), L"SemiBold") == 0)
            weight = FW_SEMIBOLD;
        else if (_wcsicmp(fw.c_str(), L"Light") == 0)
            weight = FW_LIGHT;
        // 斜体：FontStyle="Italic" 或 Italic="true"
        const bool italic = _wcsicmp(el.get(L"FontStyle").c_str(), L"Italic") == 0 ||
                            _wcsicmp(el.get(L"Italic").c_str(), L"true") == 0;

        static std::map<std::wstring, TextMetrics> s_cache;
        std::wstring key = face + L"|" + std::to_wstring(pt) + L"|" +
                           std::to_wstring(weight) + (italic ? L"|i" : L"|n");
        auto it = s_cache.find(key);
        if (it != s_cache.end())
            return it->second;

        TextMetrics m;
        HDC dc = ::GetDC(nullptr);
        if (dc != nullptr)
        {
            const int hpx = -::MulDiv(pt, ::GetDeviceCaps(dc, LOGPIXELSY), 72);
            HFONT f = ::CreateFontW(hpx, 0, 0, 0, weight, italic ? TRUE : FALSE, FALSE, FALSE,
                                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                    CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                                    face.c_str());
            if (f != nullptr)
            {
                HFONT old = static_cast<HFONT>(::SelectObject(dc, f));
                TEXTMETRICW tm = {};
                ::GetTextMetricsW(dc, &tm);
                m.ascent = static_cast<int>(tm.tmAscent);
                m.descent = static_cast<int>(tm.tmDescent);
                ::SelectObject(dc, old);
                ::DeleteObject(f);
            }
            ::ReleaseDC(nullptr, dc);
        }
        if (m.ascent <= 0)
            m.ascent = (std::max)(1, pt * 4 / 3);     // 量不到就按 4/3 估，至少不是 0
        if (m.descent <= 0)
            m.descent = (std::max)(1, pt / 3);
        s_cache[key] = m;
        return m;
    }

    // 文字在控件盒里垂直居中绘制的类型：按钮族、勾选族、输入框族。
    // 这类控件的基线在盒子中线附近，不能按"贴顶 + ascent"算。
    bool TextVerticallyCentered(const std::wstring& kind)
    {
        static const wchar_t* k[] = {
            L"button", L"roundbutton", L"toolbutton", L"delaybutton", L"togglebutton",
            L"iconbutton", L"checkbox", L"radio", L"edit", L"textfield", L"combo",
            L"choice", L"datepicker", L"spinbox", L"pushbutton"
        };
        for (const wchar_t* t : k)
            if (_wcsicmp(kind.c_str(), t) == 0)
                return true;
        return false;
    }

    // 元素盒顶到其文字基线的距离；boxH<=0 时按元素自身声明或该类缺省高度算
    int BaselineFromTop(const XmlElement& el, int boxH)
    {
        const std::wstring kind = KindOf(el.tag);
        const TextMetrics m = TextMetricsOf(el);
        const int h = boxH > 0 ? boxH
                               : (el.hasH ? el.h : (el.previewH > 0 ? el.previewH : DefaultHeight(kind)));
        if (TextVerticallyCentered(kind))
        {
            const int textH = m.ascent + m.descent;
            const int innerH = (std::max)(0, h - el.padT - el.padB);
            return el.padT + (std::max)(0, (innerH - textH) / 2) + m.ascent;
        }
        return el.padT + m.ascent;
    }

    // ===================================================================
}
