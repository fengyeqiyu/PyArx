#pragma once
// XmlAppearance —— 所有控件（标准 + 扩展 + Python 自定义控件）共用的外观基元。
//
// 这是"控件基类"的另一半（另一半是 XmlMfcControlBase 的回调表）：
//   形状 Shape      Rect / RoundRect / Ellipse / Capsule
//   后置背景 BackColor  —— 控件底板
//   前置背景 FrontColor —— 底板之上、内容之下的第二层（内衬/高亮）
//   前景色   Foreground —— 文字 / 图标
//   边框     BorderColor / BorderThickness / BorderStyle(None|Single|Sunken|Raised)
//   其它     Radius / Opacity / TextAlign / FontSize / FontFamily
//
// 放在运行时核心（不依赖 MFC），所以设计器画布、XmlViewer 预览、PyMfc 运行时
// 三处渲染共用同一套语义 —— XML 定义外观，C++ 只负责画。
//
// 编码：本文件 UTF-8 with BOM。

#include "XmlCore.h"

#include <windows.h>

#include <algorithm>
#include <string>
#include <vector>

// HTML `text-align:justify` 的等价位。DrawText 的 DT_* 里没有"两端对齐"，所以
// 用一个不冲突的高位标志随 align 一起传下去：D2D 后端认它 → DWRITE_TEXT_ALIGNMENT_JUSTIFIED，
// 其它后端不认识就退化成左对齐（和浏览器在无法两端对齐时的退化一致）。
static const UINT kXmlTextJustify = 0x80000000u;

namespace xmlapp
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

    struct XmlAppearance
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
        // CSS box-shadow 原始串（`0 4 10 rgba(0,0,0,0.55)`）。
        // HTML 里卡片/浮层的投影就靠这一条声明；XML 早先完全不认（回读都是空）。
        // 存原文交给绘制端解析 offsetX/offsetY/blur/spread/color + alpha。
        std::wstring boxShadow;
        // CSS linear-gradient(...) 原始串（`background:linear-gradient(180deg, #313842, #1E2226)`）。
        // GDI 背板层没有渐变刷子，DrawBackplate 会把它拆成起止色 + 角度后逐行填充。
        std::wstring bgImage;
        std::wstring frontImage;    // 前置背景图（保留）
        // CSS letter-spacing（字距，单位 px）。绘制端用 DWrite TextLayout 的
        // SetCharacterSpacing 实现；0 表示未声明，走原来的 DrawText 快路径。
        double letterSpacing = 0.0;
        std::wstring lineHeight;      // CSS line-height 原文（`1.5` 倍数 / `24px` 绝对 / `150%`）
        std::wstring textIndent;      // CSS text-indent 原文（`2em` / `24px`）；首行缩进
        std::wstring textOverflow;    // CSS text-overflow 原文（`ellipsis`）；单行溢出打点
        // CSS text-shadow 原文（`0 2px 6px rgba(0,0,0,.6)`）：文字投影。
        // HTML 里标题/大字的标配（深色底上尤其常见），XML 早先没有 —— 只能把字改成图。
        std::wstring textShadow;
        // CSS pointer-events 原文（`none` / `auto`）：none = 这层不吃鼠标，点击穿透到下面。
        // HTML 里"覆盖层/提示层不挡点击"的标准写法，XML 早先只能靠 Visible=False 整块拿掉。
        std::wstring pointerEvents;
        // CSS transform 原文（`rotate(3deg) scale(1.2)` / `matrix(...)`）。
        // 存原文而不存矩阵：translate 的百分比要按元素盒子折算，盒子尺寸只有绘制那步才有。
        std::wstring transform;
        int borderThickness = 0;    // 0 表示未声明
        // CSS 单边边框（border-top / border-bottom / border-left / border-right）。
        // HTML 里"只要一条分割线"是最常见的写法（border-bottom:1px solid #333），
        // XML 早先只能整圈描边，做不到单边。顺序固定：0=上 1=右 2=下 3=左；
        // -1 = 这一边没有声明（沿用整圈 borderThickness），0 = 显式无边框（none / 0）。
        int borderSideW[4] = { -1, -1, -1, -1 };
        std::wstring borderSideColor[4];   // 空 = 沿用整圈 borderColor
        std::wstring borderSideStyle[4];   // 空 = 沿用整圈 borderStyle
        // CSS outline（`outline:2px solid #0af` / `outline-offset:2px`）：焦点框的标准写法。
        // 与 border 的差别是它不参与盒模型（不挤内容、可画到盒外、可负偏移），
        // XML 早先完全没有这条通路，焦点环只能靠宿主画、改不了宽/色/偏移。
        int outlineWidth = -1;          // -1 = 未声明；0 = none（显式关掉）
        int outlineOffset = 0;          // CSS outline-offset，正数向外扩
        std::wstring outlineColor;      // 空 = 跟随文字色（CSS currentColor）
        std::wstring outlineStyle;      // solid / dashed / dotted / double / none
        int radius = 0;             // 0 表示按形状默认
        int radiusPct = -1;         // CSS border-radius:N% 的百分比圆角（-1 = 未声明）
        // CSS 分角圆角：border-top-left-radius / top-right / bottom-right / bottom-left。
        // HTML 里「只有左上角圆角」是常规写法（卡片、气泡、选项卡），单一半径做不到。
        // -1 = 未声明（回落到统一 radius）。
        int radiusTL = -1;
        int radiusTR = -1;
        int radiusBR = -1;
        int radiusBL = -1;
        // CSS overflow：hidden / auto / scroll 要把超出盒子的子项裁掉（可见 / 裁剪 / 滚动）。
        // HTML 里「卡片圆角 + 内部图片溢出」全靠这一条；XML 早先写 `overflow:hidden` 是空声明。
        // 空 = 未声明（HTML 默认 visible，行为与现状一致）。
        std::wstring overflow;
        int fontSize = 0;           // 0 表示未声明
        int opacity = 255;          // 0..255（整块：背景 + 边框一起混合）
        // CSS `background:rgba(r,g,b,a)` / `#RRGGBBAA` 的 alpha：只作用于背景填充，
        // 文字与边框保持不透明（与 HTML 一致）。255 表示不透明。
        int bgAlpha = 255;
        // CSS text-decoration:underline（下划线）。画在文字基线下方，颜色跟随文字色。
        bool underline = false;
        // CSS text-decoration:line-through（删除线）。画在文字中部，颜色跟随文字色。
        // HTML 里 <del> 默认就是这条线；XML 早先只有下划线，删除线只能自己画图。
        bool lineThrough = false;
        // CSS text-transform（uppercase / lowercase / capitalize）。HTML 里"标签全大写"
        // 只需一个声明，XML 早先只能把文字本身写成大写（改文案就改数据，语义不可复用）。
        std::wstring textTransform;
        // CSS :hover 的 XML 等价写法。XML 单元素挂属性，没有伪类，所以写成
        // HoverBackground / HoverColor（style 里 hover-background / hover-color）。
        // HTML 里 `button:hover{background:#...}` 一行就有的交互反馈，XML 早先完全没有。
        std::wstring hoverBackColor;
        std::wstring hoverForeColor;
        // CSS :focus 的 XML 等价写法（同上：单元素没有伪类）。
        // HTML 里 `input:focus{border-color:#08f;color:#000}` 一行到位，
        // XML 早先只能靠宿主画的焦点环，改不了底色与字色。
        std::wstring focusBackColor;
        std::wstring focusForeColor;
        // CSS :active 的 XML 等价写法。HTML `button:active{...}` 是"按下去就变"，
        // XML 早先只有宿主主题里的固定按下色，改不了。
        std::wstring activeBackColor;
        std::wstring activeForeColor;
        // 状态化的边框颜色：HTML 里 `:hover{border-color:#fff}` / `:focus{border-color:#08f}`
        // 不改几何、只换线色，是最常见的"可交互感"。宽/线型仍不进状态，避免盒子跳动。
        std::wstring hoverBorderColor;
        std::wstring focusBorderColor;
        std::wstring activeBorderColor;
        // 状态化不透明度：HTML 里 `:hover{opacity:.85}` / `:disabled{opacity:.6}` 是最轻量的
        // 交互反馈（不改几何、不改配色）。-1 = 未声明（沿用常态 opacity）。
        int hoverOpacity = -1;
        int focusOpacity = -1;
        int activeOpacity = -1;
        // 状态化 transform：HTML 里 `:hover{transform:scale(1.04)}` 是最常见的"轻微放大"反馈。
        // transform 只影响绘制、不影响布局（与 HTML 一致），所以放进状态是安全的。
        std::wstring hoverTransform;
        std::wstring focusTransform;
        std::wstring activeTransform;
        // 状态化 box-shadow：HTML 里卡片/按钮"悬停抬起"的标准写法是
        // `:hover{box-shadow:0 8px 20px rgba(0,0,0,.45)}`（常态只有浅投影，悬停加厚）。
        // 投影不参与布局，放进状态不会挤动兄弟。
        std::wstring hoverShadow;
        std::wstring focusShadow;
        std::wstring activeShadow;
        // 状态化 :disabled（`:disabled{background:#ccc;color:#999;opacity:.5}`）：
        // HTML 里禁用态外观一行 CSS 就能定制；XML 早先 :disabled 规则整条被丢，
        // 只能靠 Enabled=False 的固定置灰（改不了配色）。
        std::wstring disabledBackColor;
        std::wstring disabledForeColor;
        std::wstring disabledBorderColor;
        int disabledOpacity = -1;
        // CSS transition：状态（hover/active/focus）切换时在时长内插值，而不是硬切。
        // HTML 里 `transition:background-color .2s` 一行就有，XML 早先只能瞬间跳色。
        int transitionMs = -1;            // -1 = 未声明（不接管），0 = 明确无过渡
        std::wstring transitionEase;     // linear / ease / ease-in / ease-out / ease-in-out / cubic-bezier(...)

        // 外观全字段相等判定：供宿主做幂等短路（同一套外观重复下发时不再重建刷子/重绘）。
        // 必须是"全字段"比较——漏一个字段就会出现"改了却没生效"的假幂等。
        bool sameAs(const XmlAppearance& o) const
        {
            return shape == o.shape && backColor == o.backColor &&
                   frontColor == o.frontColor && foreColor == o.foreColor &&
                   borderColor == o.borderColor && borderStyle == o.borderStyle &&
                   textAlign == o.textAlign && verticalAlign == o.verticalAlign &&
                   textWrapping == o.textWrapping && fontWeight == o.fontWeight &&
                   fontStyle == o.fontStyle && fontFamily == o.fontFamily &&
                   backImage == o.backImage && frontImage == o.frontImage &&
                   transform == o.transform && boxShadow == o.boxShadow && bgImage == o.bgImage &&
                   hoverShadow == o.hoverShadow && focusShadow == o.focusShadow &&
                   activeShadow == o.activeShadow &&
                   disabledBackColor == o.disabledBackColor &&
                   disabledForeColor == o.disabledForeColor &&
                   disabledBorderColor == o.disabledBorderColor &&
                   disabledOpacity == o.disabledOpacity &&
                   letterSpacing == o.letterSpacing && lineHeight == o.lineHeight &&
                   textIndent == o.textIndent && textOverflow == o.textOverflow &&
                   textShadow == o.textShadow &&
                   borderThickness == o.borderThickness && radius == o.radius &&
                   radiusPct == o.radiusPct && radiusTL == o.radiusTL &&
                   radiusTR == o.radiusTR && radiusBR == o.radiusBR && radiusBL == o.radiusBL &&
                   fontSize == o.fontSize &&
                   opacity == o.opacity && bgAlpha == o.bgAlpha &&
                   underline == o.underline && lineThrough == o.lineThrough &&
                   textTransform == o.textTransform &&
                   hoverBackColor == o.hoverBackColor && hoverForeColor == o.hoverForeColor &&
                   focusBackColor == o.focusBackColor && focusForeColor == o.focusForeColor &&
                   activeBackColor == o.activeBackColor && activeForeColor == o.activeForeColor &&
                   hoverBorderColor == o.hoverBorderColor &&
                   focusBorderColor == o.focusBorderColor &&
                   activeBorderColor == o.activeBorderColor &&
                   hoverOpacity == o.hoverOpacity && focusOpacity == o.focusOpacity &&
                   activeOpacity == o.activeOpacity &&
                   hoverTransform == o.hoverTransform && focusTransform == o.focusTransform &&
                   activeTransform == o.activeTransform &&
                   borderSideW[0] == o.borderSideW[0] && borderSideW[1] == o.borderSideW[1] &&
                   borderSideW[2] == o.borderSideW[2] && borderSideW[3] == o.borderSideW[3] &&
                   borderSideColor[0] == o.borderSideColor[0] &&
                   borderSideColor[1] == o.borderSideColor[1] &&
                   borderSideColor[2] == o.borderSideColor[2] &&
                   borderSideColor[3] == o.borderSideColor[3] &&
                   borderSideStyle[0] == o.borderSideStyle[0] &&
                   borderSideStyle[1] == o.borderSideStyle[1] &&
                   borderSideStyle[2] == o.borderSideStyle[2] &&
                   borderSideStyle[3] == o.borderSideStyle[3] &&
                   outlineWidth == o.outlineWidth && outlineOffset == o.outlineOffset &&
                   outlineColor == o.outlineColor && outlineStyle == o.outlineStyle &&
                   overflow == o.overflow && pointerEvents == o.pointerEvents &&
                   transitionMs == o.transitionMs && transitionEase == o.transitionEase;
        }

        bool empty() const
        {
            return shape.empty() && backColor.empty() && frontColor.empty() &&
                   foreColor.empty() && borderColor.empty() && borderStyle.empty() &&
                   textAlign.empty() && verticalAlign.empty() && textWrapping.empty() &&
                   fontWeight.empty() && fontStyle.empty() &&
                   fontFamily.empty() && borderThickness == 0 &&
                   radius == 0 && radiusPct < 0 && radiusTL < 0 && radiusTR < 0 &&
                   radiusBR < 0 && radiusBL < 0 && fontSize == 0 && opacity == 255 &&
                   bgAlpha == 255 && !underline && !lineThrough && textTransform.empty() &&
                   hoverBackColor.empty() && hoverForeColor.empty() &&
                   focusBackColor.empty() && focusForeColor.empty() &&
                   activeBackColor.empty() && activeForeColor.empty() &&
                   hoverBorderColor.empty() && focusBorderColor.empty() &&
                   activeBorderColor.empty() &&
                   hoverOpacity < 0 && focusOpacity < 0 && activeOpacity < 0 &&
                   hoverTransform.empty() && focusTransform.empty() && activeTransform.empty() &&
                   hoverShadow.empty() && focusShadow.empty() && activeShadow.empty() &&
                   disabledBackColor.empty() && disabledForeColor.empty() &&
                   disabledBorderColor.empty() && disabledOpacity < 0 &&
                   letterSpacing == 0.0 && lineHeight.empty() && textIndent.empty() &&
                   textOverflow.empty() && transform.empty() && boxShadow.empty() && bgImage.empty() &&
                   textShadow.empty() &&
                   borderSideW[0] < 0 && borderSideW[1] < 0 && borderSideW[2] < 0 && borderSideW[3] < 0 &&
                   outlineWidth < 0 && outlineOffset == 0 && outlineColor.empty() &&
                   outlineStyle.empty() && overflow.empty() && pointerEvents.empty() &&
                   transitionMs < 0 && transitionEase.empty();
        }
    };

    // CSS 状态化投影的取值规则（focus → hover → 常态），与状态选色一致。
    // 绘制端统一走这里，免得每个绘制分支各写一遍 if。
    inline const std::wstring& ShadowOf(const XmlAppearance& ap, bool hovered, bool focused)
    {
        if (focused && !ap.focusShadow.empty())
            return ap.focusShadow;
        if (hovered && !ap.hoverShadow.empty())
            return ap.hoverShadow;
        return ap.boxShadow;
    }

    // CSS text-transform 的落地点：绘制端拿到文字后按声明转换（不改数据，只改渲染）。
    // capitalize 按"词首"处理（空白/标点后的第一个字母），与浏览器一致。
    inline std::wstring TransformText(const std::wstring& s, const std::wstring& how)
    {
        if (how.empty() || s.empty())
            return s;
        std::wstring r = s;
        if (_wcsicmp(how.c_str(), L"uppercase") == 0)
        {
            for (auto& c : r) c = static_cast<wchar_t>(towupper(c));
        }
        else if (_wcsicmp(how.c_str(), L"lowercase") == 0)
        {
            for (auto& c : r) c = static_cast<wchar_t>(towlower(c));
        }
        else if (_wcsicmp(how.c_str(), L"capitalize") == 0)
        {
            bool start = true;
            for (auto& c : r)
            {
                if (iswspace(c) || iswpunct(c) || iswdigit(c))
                    start = true;
                else
                {
                    if (start) c = static_cast<wchar_t>(towupper(c));
                    start = false;
                }
            }
        }
        return r;
    }

    // CSS box-shadow 解析说明（保留结论，避免后来者再走一遍）：
    // XML 的每个控件都在自己的窗口 DC 上自绘（owner-draw），窗口矩形就等于元素盒子，
    // 而 CSS 的 box-shadow 画在盒子之外 —— 超出客户区的部分直接被裁掉，看得见的只剩
    // "把背板压暗一层"。真正要落地必须等"绘制表面"层（父表面统一绘制，与真父子同一批改造）。
    // 因此这里不做 box-shadow 声明，宁可"没有这个属性"，也不要"写了却静默画错"。

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
            // CSS 短写：`#fff` = #ffffff、`#f00f` = #ff0000ff。HTML 里这三四位写法极常用
            // （设计稿、简写配色），此前直接返回 false → `background:#fff` 静默画不出底色。
            if (n == 3 || n == 4)
            {
                auto hexv = [](wchar_t c) -> int {
                    if (c >= L'0' && c <= L'9') return c - L'0';
                    if (c >= L'a' && c <= L'f') return c - L'a' + 10;
                    if (c >= L'A' && c <= L'F') return c - L'A' + 10;
                    return 0;
                };
                out = RGB(hexv(v[1]) * 17, hexv(v[2]) * 17, hexv(v[3]) * 17);
                if (n == 4 && alpha != nullptr)
                    *alpha = hexv(v[4]) * 17;
                return true;
            }
            return false;
        }
        if (v.find(L"RGB") == 0 || v.find(L"rgb") == 0)
            return xmlParseColor(v, out);
        return false;
    }

    // CSS text-shadow：`0 2px 6px rgba(0,0,0,.6)`。取第一个阴影层
    //（多层 text-shadow 在桌面绘制里叠加意义有限，按 HTML 首个层先落地）。
    struct TextShadowSpec
    {
        int ox = 0;
        int oy = 0;
        int blur = 0;
        COLORREF color = RGB(0, 0, 0);
        int alpha = 255;
    };

    inline bool ParseTextShadow(const std::wstring& s, TextShadowSpec& out)
    {
        std::wstring v = s;
        size_t b = v.find_first_not_of(L" \t\r\n");
        if (b == std::wstring::npos)
            return false;
        v = v.substr(b);
        std::wstring cur;
        std::vector<std::wstring> toks;
        for (wchar_t c : v)
        {
            if (c == L' ' || c == L'\t')
            {
                if (!cur.empty()) { toks.push_back(cur); cur.clear(); }
            }
            else
                cur.push_back(c);
        }
        if (!cur.empty())
            toks.push_back(cur);

        int nums[3] = { 0, 0, 0 };
        int nNum = 0;
        bool hasColor = false;
        for (const std::wstring& t : toks)
        {
            if (_wcsicmp(t.c_str(), L"none") == 0)
                return false;
            COLORREF c = 0;
            int a = 255;
            if (!t.empty() && (t[0] == L'#' || t.find(L"rgb") != std::wstring::npos))
            {
                if (ParseColor(t, c, &a))
                {
                    out.color = c;
                    out.alpha = a;
                    hasColor = true;
                    continue;
                }
            }
            if (nNum < 3)
                nums[nNum++] = _wtoi(t.c_str());
        }
        if (nNum == 0)
            return false;
        out.ox = nums[0];
        out.oy = nNum > 1 ? nums[1] : 0;
        out.blur = nNum > 2 ? nums[2] : 0;
        (void)hasColor;
        return true;
    }

    // CSS font-weight 归一：浏览器里 100~900 与关键字都合法，绘制端只认
    // Normal/Bold/SemiBold/Light 四个枚举。不做归一就会出现"写了 font-weight:600
    // 看着像没生效"——HTML 能写而 XML 静默失效，属于语言便利度的硬缺口。
    inline std::wstring NormalizeFontWeight(const std::wstring& v)
    {
        if (v.empty()) return v;
        bool numeric = true;
        for (wchar_t c : v)
            if (c < L'0' || c > L'9') { numeric = false; break; }
        if (numeric)
        {
            const int n = _wtoi(v.c_str());
            if (n >= 700) return L"Bold";
            if (n >= 500) return L"SemiBold";
            if (n <= 350) return L"Light";
            return L"Normal";
        }
        if (_wcsicmp(v.c_str(), L"bold") == 0 || _wcsicmp(v.c_str(), L"bolder") == 0) return L"Bold";
        if (_wcsicmp(v.c_str(), L"semibold") == 0) return L"SemiBold";
        if (_wcsicmp(v.c_str(), L"light") == 0 || _wcsicmp(v.c_str(), L"lighter") == 0) return L"Light";
        if (_wcsicmp(v.c_str(), L"normal") == 0 || _wcsicmp(v.c_str(), L"regular") == 0) return L"Normal";
        return v;   // 未知值原样交给绘制端（不吞作者的意图）
    }

    // CSS font-style 归一：italic / oblique / normal（大小写不敏感）
    inline std::wstring NormalizeFontStyle(const std::wstring& v)
    {
        if (v.empty()) return v;
        if (_wcsicmp(v.c_str(), L"italic") == 0) return L"Italic";
        if (_wcsicmp(v.c_str(), L"oblique") == 0) return L"Oblique";
        if (_wcsicmp(v.c_str(), L"normal") == 0) return L"Normal";
        return v;
    }

    inline bool IsAppearanceProperty(const std::wstring& name)
    {
        const std::string key = util::lower(util::wideToUtf8(name));
        static const char* k[] = {
            "shape", "backcolor", "background", "bg", "bgcolor",
            "frontcolor", "forecolor", "foreground", "color",
            "bordercolor", "bordersize", "borderstyle", "borderthickness", "borderwidth",
            "radius", "cornerradius", "bordertopleftradius", "bordertoprightradius",
            "borderbottomrightradius", "borderbottomleftradius",
            "opacity", "textalign", "fontsize", "fontfamily",
            "fontname", "backimage", "frontimage", "letterspacing", "bgimage", "backgroundimage",
            "bgalpha", "verticalalignment", "textwrapping", "fontweight", "fontstyle",
            "textdecoration", "texttransform", "lineheight", "textindent", "textoverflow", "transform",
            "boxshadow", "shadow", "textshadow", "overflow", "outline", "outlinewidth",
            "outlinecolor", "outlinestyle", "outlineoffset", "pointerevents",
            "transition", "transitionduration", "transitionproperty", "transitiontimingfunction",
            "hoverbackground", "hovercolor", "hoverforecolor", "hoverforeground",
            "focusbackground", "focuscolor", "focusforecolor", "focusforeground",
            "activebackground", "activecolor", "activeforecolor", "activeforeground",
            "hoverbordercolor", "focusbordercolor", "activebordercolor",
            "hoveropacity", "focusopacity", "activeopacity",
            "hovertransform", "focustransform", "activetransform",
            "hovershadow", "focusshadow", "activeshadow",
            "disabledbackground", "disabledcolor", "disabledforecolor",
            "disabledforeground", "disabledbordercolor", "disabledopacity"
        };
        for (auto kk : k)
            if (util::keyIs(key, kk))
                return true;
        return false;
    }

    // 写外观属性：命中返回 true（同一语义可有多个别名）
    inline bool SetProperty(XmlAppearance& ap, const std::wstring& name, const std::wstring& value)
    {
        const std::string key = util::lower(util::wideToUtf8(name));
        if (util::keyIs(key, "shape")) { ap.shape = value; return true; }
        // `bg` / `bgcolor` 是 HTML/CSS 里最常用的短写（与 background 同义），此前只认全名
        if (util::keyIs(key, "backcolor") || util::keyIs(key, "background") ||
            util::keyIs(key, "bg") || util::keyIs(key, "bgcolor"))
        { ap.backColor = value; return true; }
        // `border="2px solid #f00"` 属性式简写：与 style 里的 border 简写等价。
        // 早先只有 style 路能解析，属性路整条丢 —— XML 侧最顺手的写法反而不生效。
        if (util::keyIs(key, "border"))
        {
            std::wstring v = value;
            for (wchar_t& ch : v) if (ch == L',' || ch == L'/') ch = L' ';
            size_t pos = 0;
            while (pos < v.size())
            {
                const size_t sp = v.find(L' ', pos);
                const std::wstring tok = v.substr(pos, sp == std::wstring::npos
                                                            ? std::wstring::npos : sp - pos);
                pos = (sp == std::wstring::npos) ? v.size() : sp + 1;
                if (tok.empty())
                    continue;
                if (iswdigit(tok[0]))
                {
                    ap.borderThickness = _wtoi(tok.c_str());
                    continue;
                }
                if (tok[0] == L'#' || iswalpha(tok[0]) != 0)
                {
                    if (_wcsicmp(tok.c_str(), L"solid") == 0 || _wcsicmp(tok.c_str(), L"none") == 0 ||
                        _wcsicmp(tok.c_str(), L"dashed") == 0 || _wcsicmp(tok.c_str(), L"dotted") == 0 ||
                        _wcsicmp(tok.c_str(), L"double") == 0 || _wcsicmp(tok.c_str(), L"inset") == 0 ||
                        _wcsicmp(tok.c_str(), L"outset") == 0 || _wcsicmp(tok.c_str(), L"groove") == 0 ||
                        _wcsicmp(tok.c_str(), L"ridge") == 0 || _wcsicmp(tok.c_str(), L"hidden") == 0)
                    {
                        // 线型复用 border-style 的规范化（solid → Single）
                        SetProperty(ap, L"borderstyle", tok);
                    }
                    else
                        ap.borderColor = tok;   // 颜色 token（#rgb / 颜色名）
                }
            }
            return true;
        }
        // CSS linear-gradient(...)：原始串存进 bgImage，绘制时再拆起止色与角度
        if (util::keyIs(key, "bgimage") || util::keyIs(key, "backgroundimage"))
        { ap.bgImage = value; return true; }
        if (util::keyIs(key, "frontcolor")) { ap.frontColor = value; return true; }
        if (util::keyIs(key, "forecolor") || util::keyIs(key, "foreground") ||
            util::keyIs(key, "color")) { ap.foreColor = value; return true; }
        if (util::keyIs(key, "bordercolor")) { ap.borderColor = value; return true; }
        // CSS 的 border-style 关键字与 Win32 外观契约不是一套：契约原本只认
        // None|Single|Sunken|Raised，于是 `border:2px dashed #f00` 被压成 Single，
        // 浏览器里明明是虚线、XML 里一律实线（"能写、写对了、画出来不是那回事"）。
        // 现在把 CSS 线型词原样保留（固体的 solid 仍归一到 Single，老 GDI 路径行为不变），
        // 由绘制端自己决定画法：容器位图刷按 CSS 语义画 dashed/dotted/double 与明暗半道。
        if (util::keyIs(key, "borderstyle"))
        {
            std::wstring st;
            st.reserve(value.size());
            for (wchar_t c : value)
                if (c != L' ' && c != L'\t' && c != L'\r' && c != L'\n')
                    st.push_back(c);
            if (st.empty() || _wcsicmp(st.c_str(), L"none") == 0 ||
                _wcsicmp(st.c_str(), L"hidden") == 0)
                ap.borderStyle = L"None";
            else if (_wcsicmp(st.c_str(), L"solid") == 0 || _wcsicmp(st.c_str(), L"single") == 0)
                ap.borderStyle = L"Single";      // 实线 = 老的 Single，保持历史绘制一致
            else if (_wcsicmp(st.c_str(), L"dashed") == 0 || _wcsicmp(st.c_str(), L"dotted") == 0 ||
                     _wcsicmp(st.c_str(), L"double") == 0 || _wcsicmp(st.c_str(), L"groove") == 0 ||
                     _wcsicmp(st.c_str(), L"ridge") == 0 || _wcsicmp(st.c_str(), L"inset") == 0 ||
                     _wcsicmp(st.c_str(), L"outset") == 0 ||
                     _wcsicmp(st.c_str(), L"sunken") == 0 || _wcsicmp(st.c_str(), L"raised") == 0)
            {
                // 首字母大写存一份，取值回读稳定（Dashed/Dotted/Double/Groove/...）
                std::wstring canon = st;
                canon[0] = static_cast<wchar_t>(::towupper(canon[0]));
                for (size_t i = 1; i < canon.size(); ++i)
                    canon[i] = static_cast<wchar_t>(::towlower(canon[i]));
                ap.borderStyle = canon;
            }
            else
                ap.borderStyle = L"Single";
            return true;
        }
        if (util::keyIs(key, "borderthickness") || util::keyIs(key, "borderwidth") ||
            util::keyIs(key, "bordersize"))
        { ap.borderThickness = _wtoi(value.c_str()); return true; }
        // CSS 单边边框：`border-bottom:1px solid #333` 是 HTML 里画分割线的标准写法。
        // 支持 border-<side>（简写）、border-<side>-width/color/style 六种键。
        {
            static const char* kSides[4] = { "top", "right", "bottom", "left" };
            for (int si = 0; si < 4; ++si)
            {
                const std::string base = std::string("border") + kSides[si];
                const std::string kw = base + "width";
                const std::string kc = base + "color";
                const std::string ks = base + "style";
                if (!(key == base || key == kw || key == kc || key == ks))
                    continue;
                if (key == kw)
                {
                    ap.borderSideW[si] = _wtoi(value.c_str());
                    return true;
                }
                if (key == kc) { ap.borderSideColor[si] = value; return true; }
                if (key == ks) { ap.borderSideStyle[si] = value; return true; }
                // 简写：拆 token 按性质归类（宽度 / 颜色 / 线型）。none / 0 视为显式无边。
                int w = -1;
                std::wstring col, st;
                std::wstring cur;
                std::wstring v = value;
                v.push_back(L' ');
                for (wchar_t c : v)
                {
                    if (c == L' ' || c == L'\t' || c == L',')
                    {
                        if (cur.empty())
                            continue;
                        if (_wcsicmp(cur.c_str(), L"none") == 0)
                            w = 0;
                        else if (_wcsicmp(cur.c_str(), L"solid") == 0 ||
                                 _wcsicmp(cur.c_str(), L"dashed") == 0 ||
                                 _wcsicmp(cur.c_str(), L"dotted") == 0 ||
                                 _wcsicmp(cur.c_str(), L"double") == 0)
                            st = cur;
                        else if (!cur.empty() && (cur[0] == L'#' || cur.find(L"rgb") != std::wstring::npos))
                            col = cur;
                        else if (!cur.empty() && !iswdigit(cur[0]))
                        {
                            // 纯字母：可能是线型也可能是颜色名（red/blue…），先按线型词表判
                            std::wstring low = cur;
                            for (auto& ch : low) ch = static_cast<wchar_t>(towlower(ch));
                            if (low == L"groove" || low == L"ridge" || low == L"inset" || low == L"outset")
                                st = cur;
                            else
                                col = cur;
                        }
                        else
                            w = _wtoi(cur.c_str());
                        cur.clear();
                    }
                    else
                        cur.push_back(c);
                }
                if (w >= 0)
                    ap.borderSideW[si] = w;
                if (!col.empty())
                    ap.borderSideColor[si] = col;
                if (!st.empty())
                    ap.borderSideStyle[si] = st;
                return true;
            }
        }
        // CSS overflow：hidden/auto/scroll = 裁剪到盒子（圆角一并裁），visible/空 = 不裁。
        if (util::keyIs(key, "overflow") || util::keyIs(key, "overflowx") ||
            util::keyIs(key, "overflowy"))
        {
            ap.overflow = value;
            return true;
        }
        // CSS outline：`outline:2px solid #0af` / `outline-offset:2px`。
        // 语义与 border 不同 —— 不占盒模型、画在盒外（或按 offset 内外挪），
        // 所以存成独立字段，绘制端在背板之后画一圈"盒外带"。
        if (util::keyIs(key, "outline") || util::keyIs(key, "outlinewidth") ||
            util::keyIs(key, "outlinecolor") || util::keyIs(key, "outlinestyle") ||
            util::keyIs(key, "outlineoffset"))
        {
            if (util::keyIs(key, "outlineoffset"))
            {
                ap.outlineOffset = _wtoi(value.c_str());
                return true;
            }
            if (util::keyIs(key, "outlinewidth"))
            {
                ap.outlineWidth = _wtoi(value.c_str());
                return true;
            }
            if (util::keyIs(key, "outlinecolor")) { ap.outlineColor = value; return true; }
            if (util::keyIs(key, "outlinestyle"))
            {
                std::wstring st;
                for (wchar_t c : value)
                    if (c != L' ' && c != L'\t' && c != L'\r' && c != L'\n')
                        st.push_back(c);
                if (st.empty() || _wcsicmp(st.c_str(), L"none") == 0)
                {
                    ap.outlineStyle = L"None";
                    ap.outlineWidth = 0;
                }
                else if (_wcsicmp(st.c_str(), L"auto") == 0)
                    ap.outlineStyle = L"Solid";
                else
                {
                    std::wstring canon = st;
                    canon[0] = static_cast<wchar_t>(::towupper(canon[0]));
                    for (size_t i = 1; i < canon.size(); ++i)
                        canon[i] = static_cast<wchar_t>(::towlower(canon[i]));
                    ap.outlineStyle = canon;
                }
                return true;
            }
            // 简写：拆 token（宽度 / 颜色 / 线型），归类规则与单边 border 一致。
            int w = -1;
            std::wstring col, st;
            std::wstring cur;
            std::wstring v = value;
            v.push_back(L' ');
            for (wchar_t c : v)
            {
                if (c == L' ' || c == L'\t' || c == L',')
                {
                    if (cur.empty())
                        continue;
                    if (_wcsicmp(cur.c_str(), L"none") == 0)
                        w = 0;
                    else if (_wcsicmp(cur.c_str(), L"solid") == 0 ||
                             _wcsicmp(cur.c_str(), L"dashed") == 0 ||
                             _wcsicmp(cur.c_str(), L"dotted") == 0 ||
                             _wcsicmp(cur.c_str(), L"double") == 0 ||
                             _wcsicmp(cur.c_str(), L"auto") == 0)
                        st = cur;
                    else if (cur[0] == L'#' || cur.find(L"rgb") != std::wstring::npos)
                        col = cur;
                    else if (!iswdigit(cur[0]))
                        col = cur;
                    else
                        w = _wtoi(cur.c_str());
                    cur.clear();
                }
                else
                    cur.push_back(c);
            }
            if (w >= 0)
                ap.outlineWidth = w;
            else if (ap.outlineWidth < 0)
                ap.outlineWidth = 1;        // CSS 省略宽度的 outline 默认 1px（medium）
            if (!col.empty())
                ap.outlineColor = col;
            if (!st.empty())
            {
                std::wstring canon = st;
                canon[0] = static_cast<wchar_t>(::towupper(canon[0]));
                for (size_t i = 1; i < canon.size(); ++i)
                    canon[i] = static_cast<wchar_t>(::towlower(canon[i]));
                ap.outlineStyle = canon;
            }
            return true;
        }
        // CSS 分角圆角（border-top-left-radius 等）：HTML 里"只圆左上角"的卡片/气泡很常见，
        // 单一半径表达不了。声明了就优先于统一 radius，没声明的角回落统一值。
        if (util::keyIs(key, "bordertopleftradius") || util::keyIs(key, "bordertoprightradius") ||
            util::keyIs(key, "borderbottomrightradius") || util::keyIs(key, "borderbottomleftradius"))
        {
            int v = _wtoi(value.c_str());
            if (v < 0) v = 0;
            if (util::keyIs(key, "bordertopleftradius")) ap.radiusTL = v;
            else if (util::keyIs(key, "bordertoprightradius")) ap.radiusTR = v;
            else if (util::keyIs(key, "borderbottomrightradius")) ap.radiusBR = v;
            else ap.radiusBL = v;
            return true;
        }
        if (util::keyIs(key, "radius") || util::keyIs(key, "cornerradius")) {
            // CSS 的 `border-radius:50%` 是按盒子短边算圆的，不能当成 50px。
            // 遇到百分比记到 radiusPct，由 ResolveRadius 在有矩形尺寸时换算。
            std::wstring v = value;
            while (!v.empty() && (v.back() == L' ' || v.back() == L'\t')) v.pop_back();
            if (!v.empty() && v.back() == L'%') {
                ap.radiusPct = _wtoi(v.c_str());
                ap.radius = 0;
            } else {
                ap.radius = _wtoi(v.c_str());
                ap.radiusPct = -1;
            }
            return true;
        }
        // CSS text-decoration / text-decoration-line：下划线 + 删除线（本通道能画的两种线型）。
        // 按 token 判：`text-decoration:underline dotted` 也算有下划线，
        // `line-through` 与 `line-through wavy` 同理；`none` 两条线都不画。
        if (util::keyIs(key, "textdecoration"))
        {
            std::wstring low = value;
            for (auto& c : low)
                if (c >= L'A' && c <= L'Z') c = static_cast<wchar_t>(c - L'A' + L'a');
            const bool none = low == L"none" || low.empty();
            ap.underline = !none && low.find(L"underline") != std::wstring::npos;
            ap.lineThrough = !none && low.find(L"line-through") != std::wstring::npos;
            return true;
        }
        // CSS text-transform：只认三个合法值（其它值当没写，避免"静默画错"）
        if (util::keyIs(key, "texttransform"))
        {
            std::wstring low = value;
            for (auto& c : low)
                if (c >= L'A' && c <= L'Z') c = static_cast<wchar_t>(c - L'A' + L'a');
            if (low == L"uppercase") ap.textTransform = L"uppercase";
            else if (low == L"lowercase") ap.textTransform = L"lowercase";
            else if (low == L"capitalize") ap.textTransform = L"capitalize";
            else ap.textTransform.clear();
            return true;
        }
        // CSS :hover 的两个等价属性（XML 没有伪类，只能一条属性带一个 hover 值）
        if (util::keyIs(key, "hoverbackground")) { ap.hoverBackColor = value; return true; }
        if (util::keyIs(key, "hovercolor") || util::keyIs(key, "hoverforecolor") ||
            util::keyIs(key, "hoverforeground"))
        { ap.hoverForeColor = value; return true; }
        // CSS :focus 的两个等价属性
        if (util::keyIs(key, "focusbackground")) { ap.focusBackColor = value; return true; }
        if (util::keyIs(key, "focuscolor") || util::keyIs(key, "focusforecolor") ||
            util::keyIs(key, "focusforeground"))
        { ap.focusForeColor = value; return true; }
        // CSS :active 的两个等价属性（按下态）
        if (util::keyIs(key, "activebackground")) { ap.activeBackColor = value; return true; }
        if (util::keyIs(key, "activecolor") || util::keyIs(key, "activeforecolor") ||
            util::keyIs(key, "activeforeground"))
        { ap.activeForeColor = value; return true; }
        // 状态化边框颜色（CSS :hover/:focus/:active 的 border-color）
        if (util::keyIs(key, "hoverbordercolor")) { ap.hoverBorderColor = value; return true; }
        if (util::keyIs(key, "focusbordercolor")) { ap.focusBorderColor = value; return true; }
        if (util::keyIs(key, "activebordercolor")) { ap.activeBorderColor = value; return true; }
        // 状态化不透明度（CSS `:hover{opacity:.85}`）：0~1 浮点或 0~255 整数都接
        if (util::keyIs(key, "hoveropacity") || util::keyIs(key, "focusopacity") ||
            util::keyIs(key, "activeopacity"))
        {
            const double v = _wtof(value.c_str());
            int o = -1;
            if (value.find(L'.') != std::wstring::npos || (v > 0.0 && v <= 1.0))
                o = static_cast<int>(v * 255.0 + 0.5);
            else
                o = static_cast<int>(v);
            if (util::keyIs(key, "hoveropacity")) ap.hoverOpacity = o;
            else if (util::keyIs(key, "focusopacity")) ap.focusOpacity = o;
            else ap.activeOpacity = o;
            return true;
        }
        // 状态化 transform（`:hover{transform:scale(1.04)}`）：只换绘制用的变换串
        if (util::keyIs(key, "hovertransform")) { ap.hoverTransform = value; return true; }
        if (util::keyIs(key, "focustransform")) { ap.focusTransform = value; return true; }
        // 状态化 :disabled（禁用态外观可定制）
        if (util::keyIs(key, "disabledbackground")) { ap.disabledBackColor = value; return true; }
        if (util::keyIs(key, "disabledcolor") || util::keyIs(key, "disabledforecolor") ||
            util::keyIs(key, "disabledforeground"))
        {
            ap.disabledForeColor = value;
            return true;
        }
        if (util::keyIs(key, "disabledbordercolor")) { ap.disabledBorderColor = value; return true; }
        if (util::keyIs(key, "disabledopacity"))
        {
            int o = -1;
            std::wstring v = value;
            if (v.find(L'.') != std::wstring::npos)
                o = static_cast<int>(_wtof(v.c_str()) * 255.0f + 0.5f);
            else
                o = _wtoi(v.c_str());
            if (o >= 0 && o <= 255) ap.disabledOpacity = o;
            return true;
        }
        // 状态化 box-shadow（`:hover{box-shadow:0 8px 20px rgba(0,0,0,.45)}`）：只换投影串
        if (util::keyIs(key, "hovershadow")) { ap.hoverShadow = value; return true; }
        if (util::keyIs(key, "focusshadow")) { ap.focusShadow = value; return true; }
        if (util::keyIs(key, "activeshadow")) { ap.activeShadow = value; return true; }
        if (util::keyIs(key, "activetransform")) { ap.activeTransform = value; return true; }
        // CSS 的 opacity 是 0~1 的浮点（opacity:0.5 = 半透明），老 XML 写法是 0~255 整数。
        // 以前一律 _wtoi，style="opacity:0.5" 被读成 0 → 控件彻底看不见（真 bug）。
        if (util::keyIs(key, "opacity"))
        {
            const double v = _wtof(value.c_str());
            if (value.find(L'.') != std::wstring::npos || (v > 0.0 && v <= 1.0))
                ap.opacity = static_cast<int>(v * 255.0 + 0.5);   // CSS 比例 → 0~255
            else
                ap.opacity = static_cast<int>(v);                 // 已是 0~255
            return true;
        }
        if (util::keyIs(key, "textalign")) { ap.textAlign = value; return true; }
        if (util::keyIs(key, "verticalalignment") || util::keyIs(key, "valign")) { ap.verticalAlign = value; return true; }
        if (util::keyIs(key, "textwrapping") || util::keyIs(key, "wrap")) { ap.textWrapping = value; return true; }
        if (util::keyIs(key, "fontweight")) { ap.fontWeight = NormalizeFontWeight(value); return true; }
        if (util::keyIs(key, "fontstyle")) { ap.fontStyle = NormalizeFontStyle(value); return true; }
        if (util::keyIs(key, "fontsize")) { ap.fontSize = _wtoi(value.c_str()); return true; }
        if (util::keyIs(key, "fontfamily") || util::keyIs(key, "fontname")) { ap.fontFamily = value; return true; }
        // CSS letter-spacing：负值合法（收紧字距），单位 px
        if (util::keyIs(key, "letterspacing"))
        {
            ap.letterSpacing = _wtof(value.c_str());
            return true;
        }
        // CSS box-shadow：原文保留（`0 4 10 rgba(0,0,0,0.55)`），颜色可能是 rgba，
        // 解析成 offset/blur/alpha 由绘制端做（这里不做归一，回读才能与原文一致）。
        if (util::keyIs(key, "boxshadow") || util::keyIs(key, "shadow"))
        {
            ap.boxShadow = value;
            return true;
        }
        // CSS line-height：原文保留（`1.5` 倍数 / `24px` / `150%`），由绘制端按字号换算
        if (util::keyIs(key, "lineheight"))
        {
            ap.lineHeight = value;
            return true;
        }
        // CSS text-indent：原文保留（`2em` / `24px`），绘制端按字号换算
        if (util::keyIs(key, "textindent"))
        {
            ap.textIndent = value;
            return true;
        }
        // CSS text-overflow：`ellipsis` 时绘制端把溢出部分换成 "…"（浏览器同款）
        if (util::keyIs(key, "textoverflow"))
        {
            ap.textOverflow = value;
            return true;
        }
        // CSS pointer-events：`none` 表示这层不吃鼠标（点击穿透到下面的元素）
        if (util::keyIs(key, "pointerevents"))
        {
            ap.pointerEvents = value;
            return true;
        }
        // CSS transition-duration / transition-timing-function / transition（简写）
        if (util::keyIs(key, "transitionduration") || util::keyIs(key, "transition"))
        {
            // 简写里可能带属性名和缓动："all 200ms ease" / "background-color .2s linear"
            int best = -1;
            {
                std::wstring tok;
                for (size_t i = 0; i <= value.size(); ++i)
                {
                    const wchar_t c = (i < value.size() ? value[i] : L' ');
                    if (c == L' ' || c == L',' || c == L';')
                    {
                        const double v = _wtof(tok.c_str());
                        const bool isMs = tok.find(L"ms") != std::wstring::npos;
                        const bool isS = !isMs && tok.find(L's') != std::wstring::npos;
                        if (v > 0.0 || (v == 0.0 && !tok.empty() && (c == L',' || tok[0] == L'0')))
                            best = isMs ? static_cast<int>(v) : (isS ? static_cast<int>(v * 1000.0 + 0.5)
                                                                   : static_cast<int>(v));
                        tok.clear();
                    }
                    else
                        tok.push_back(c);
                }
            }
            if (best >= 0)
                ap.transitionMs = best;
            return true;
        }
        if (util::keyIs(key, "transitiontimingfunction") || util::keyIs(key, "transitionproperty"))
        {
            if (util::keyIs(key, "transitiontimingfunction"))
            {
                std::wstring ez;
                for (wchar_t c : value)
                    ez.push_back(static_cast<wchar_t>(towlower(c)));
                ap.transitionEase = ez;
            }
            return true;
        }
        // CSS text-shadow：文字投影（原串下沉，绘制端按 ox/oy/blur/color 解析）
        if (util::keyIs(key, "textshadow"))
        {
            ap.textShadow = value;
            return true;
        }
        // CSS transform：原样存整串（translate/rotate/scale/skew/matrix 列表），
        // 由绘制端按元素盒子尺寸解析成 4×4 矩阵。存原文而不是预先算好矩阵，
        // 是因为百分比要按盒子换算，盒子尺寸只有绘制那一步才知道。
        if (util::keyIs(key, "transform"))
        {
            ap.transform = value;
            return true;
        }
        if (util::keyIs(key, "backimage")) { ap.backImage = value; return true; }
        if (util::keyIs(key, "frontimage")) { ap.frontImage = value; return true; }
        // CSS 背景 alpha（只影响背景填充）：0~1 比例或 0~255 整数都接受
        if (util::keyIs(key, "bgalpha"))
        {
            const double v = _wtof(value.c_str());
            if (value.find(L'.') != std::wstring::npos || (v > 0.0 && v <= 1.0))
                ap.bgAlpha = static_cast<int>(v * 255.0 + 0.5);
            else
                ap.bgAlpha = static_cast<int>(v);
            if (ap.bgAlpha < 0) ap.bgAlpha = 0;
            if (ap.bgAlpha > 255) ap.bgAlpha = 255;
            return true;
        }
        return false;
    }

    inline std::wstring GetProperty(const XmlAppearance& ap, const std::wstring& name)
    {
        const std::string key = util::lower(util::wideToUtf8(name));
        if (util::keyIs(key, "shape")) return ap.shape;
        if (util::keyIs(key, "backcolor") || util::keyIs(key, "background") ||
            util::keyIs(key, "bg") || util::keyIs(key, "bgcolor")) return ap.backColor;
        if (util::keyIs(key, "bgimage") || util::keyIs(key, "backgroundimage")) return ap.bgImage;
        if (util::keyIs(key, "frontcolor")) return ap.frontColor;
        if (util::keyIs(key, "forecolor") || util::keyIs(key, "foreground") ||
            util::keyIs(key, "color")) return ap.foreColor;
        if (util::keyIs(key, "bordercolor")) return ap.borderColor;
        if (util::keyIs(key, "borderstyle")) return ap.borderStyle;
        if (util::keyIs(key, "borderthickness") || util::keyIs(key, "borderwidth") ||
            util::keyIs(key, "bordersize"))
            return ap.borderThickness == 0 ? std::wstring() : std::to_wstring(ap.borderThickness);
        if (util::keyIs(key, "radius") || util::keyIs(key, "cornerradius"))
            return ap.radiusPct >= 0 ? (std::to_wstring(ap.radiusPct) + L"%")
                                     : (ap.radius == 0 ? std::wstring() : std::to_wstring(ap.radius));
        if (util::keyIs(key, "opacity")) return std::to_wstring(ap.opacity);
        if (util::keyIs(key, "bgalpha")) return std::to_wstring(ap.bgAlpha);
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
        if (util::keyIs(key, "boxshadow") || util::keyIs(key, "shadow")) return ap.boxShadow;
        if (util::keyIs(key, "lineheight")) return ap.lineHeight;
        if (util::keyIs(key, "textindent")) return ap.textIndent;
        if (util::keyIs(key, "textoverflow")) return ap.textOverflow;
        if (util::keyIs(key, "letterspacing"))
        {
            // CSS 允许小数与负值（letter-spacing:-.5px 是正文紧缩的常用写法），
            // 回读必须保留小数，否则契约上看是"写了等于没写"。
            if (ap.letterSpacing == 0.0)
                return std::wstring();
            wchar_t buf[32];
            swprintf_s(buf, L"%.2f", ap.letterSpacing);
            std::wstring s = buf;
            while (s.size() > 1 && s.back() == L'0') s.pop_back();
            if (!s.empty() && s.back() == L'.') s.pop_back();
            return s;
        }
        if (util::keyIs(key, "textdecoration"))
        {
            std::wstring out;
            if (ap.underline) out = L"underline";
            if (ap.lineThrough) out = out.empty() ? std::wstring(L"line-through")
                                                  : out + L" line-through";
            return out;
        }
        if (util::keyIs(key, "texttransform")) return ap.textTransform;
        if (util::keyIs(key, "hoverbackground")) return ap.hoverBackColor;
        if (util::keyIs(key, "hovercolor") || util::keyIs(key, "hoverforecolor") ||
            util::keyIs(key, "hoverforeground"))
            return ap.hoverForeColor;
        if (util::keyIs(key, "focusbackground")) return ap.focusBackColor;
        if (util::keyIs(key, "focuscolor") || util::keyIs(key, "focusforecolor") ||
            util::keyIs(key, "focusforeground"))
            return ap.focusForeColor;
        if (util::keyIs(key, "activebackground")) return ap.activeBackColor;
        if (util::keyIs(key, "activecolor") || util::keyIs(key, "activeforecolor") ||
            util::keyIs(key, "activeforeground"))
            return ap.activeForeColor;
        if (util::keyIs(key, "hoverbordercolor")) return ap.hoverBorderColor;
        if (util::keyIs(key, "focusbordercolor")) return ap.focusBorderColor;
        if (util::keyIs(key, "activebordercolor")) return ap.activeBorderColor;
        if (util::keyIs(key, "hoveropacity"))
            return ap.hoverOpacity < 0 ? std::wstring() : std::to_wstring(ap.hoverOpacity);
        if (util::keyIs(key, "focusopacity"))
            return ap.focusOpacity < 0 ? std::wstring() : std::to_wstring(ap.focusOpacity);
        if (util::keyIs(key, "activeopacity"))
            return ap.activeOpacity < 0 ? std::wstring() : std::to_wstring(ap.activeOpacity);
        if (util::keyIs(key, "hovertransform")) return ap.hoverTransform;
        if (util::keyIs(key, "focustransform")) return ap.focusTransform;
        if (util::keyIs(key, "activetransform")) return ap.activeTransform;
        if (util::keyIs(key, "hovershadow")) return ap.hoverShadow;
        if (util::keyIs(key, "focusshadow")) return ap.focusShadow;
        if (util::keyIs(key, "activeshadow")) return ap.activeShadow;
        if (util::keyIs(key, "disabledbackground")) return ap.disabledBackColor;
        if (util::keyIs(key, "disabledcolor") || util::keyIs(key, "disabledforecolor") ||
            util::keyIs(key, "disabledforeground")) return ap.disabledForeColor;
        if (util::keyIs(key, "disabledbordercolor")) return ap.disabledBorderColor;
        if (util::keyIs(key, "disabledopacity"))
            return ap.disabledOpacity < 0 ? std::wstring() : std::to_wstring(ap.disabledOpacity);
        return {};
    }

    // 统一属性元数据（属性面板 / 控件 properties() 都用这份，保证"所有控件一样"）
    inline std::vector<XmlProperty> Properties()
    {
        return {
            {L"Shape", XmlPropType::Enum, true, true, L"Rect|RoundRect|Ellipse|Capsule"},
            {L"BackColor", XmlPropType::Color, true, true},
            // CSS linear-gradient(...) 串（与 HTML 同一个写法）。类型按 String，便于原样回读。
            {L"BgImage", XmlPropType::String, true, true},
            {L"FrontColor", XmlPropType::Color, true, true},
            {L"Foreground", XmlPropType::Color, true, true},
            {L"BorderColor", XmlPropType::Color, true, true},
            {L"BorderStyle", XmlPropType::Enum, true, true, L"None|Single|Sunken|Raised"},
            {L"BorderThickness", XmlPropType::Int, true, true},
            {L"Radius", XmlPropType::Int, true, true},
            {L"Opacity", XmlPropType::Int, true, true},
            // CSS 背景 alpha（rgba 的 a / #RRGGBBAA），只让背景半透明
            {L"BgAlpha", XmlPropType::Int, true, true},
            {L"TextAlign", XmlPropType::Enum, true, true, L"Left|Center|Right|Justify"},
            {L"VerticalAlignment", XmlPropType::Enum, true, true, L"Top|Center|Bottom"},
            {L"TextWrapping", XmlPropType::Enum, true, true, L"NoWrap|Wrap|WrapWithOverflow"},
            {L"FontWeight", XmlPropType::Enum, true, true, L"Normal|Bold|SemiBold|Light"},
            {L"FontStyle", XmlPropType::Enum, true, true, L"Normal|Italic|Oblique"},
            {L"FontSize", XmlPropType::Int, true, true},
            {L"FontFamily", XmlPropType::String, true, true},
            {L"LetterSpacing", XmlPropType::Int, true, true},
            // CSS text-decoration（目前只支持 underline，对应 HTML <a> 与下划线文本）
            {L"TextDecoration", XmlPropType::String, true, true},
            {L"TextTransform", XmlPropType::String, true, true},
            {L"HoverBackground", XmlPropType::String, true, true},
            {L"HoverColor", XmlPropType::String, true, true},
            {L"HoverBorderColor", XmlPropType::String, true, true},
            {L"FocusBorderColor", XmlPropType::String, true, true},
            {L"ActiveBorderColor", XmlPropType::String, true, true},
            {L"HoverOpacity", XmlPropType::Int, true, true},
            {L"FocusOpacity", XmlPropType::Int, true, true},
            {L"ActiveOpacity", XmlPropType::Int, true, true},
            {L"HoverTransform", XmlPropType::String, true, true},
            {L"FocusTransform", XmlPropType::String, true, true},
            {L"ActiveTransform", XmlPropType::String, true, true},
            {L"HoverShadow", XmlPropType::String, true, true},
            {L"FocusShadow", XmlPropType::String, true, true},
            {L"ActiveShadow", XmlPropType::String, true, true},
            {L"DisabledBackground", XmlPropType::String, true, true},
            {L"DisabledColor", XmlPropType::String, true, true},
            {L"DisabledBorderColor", XmlPropType::String, true, true},
            {L"DisabledOpacity", XmlPropType::Int, true, true},
        };
    }

    // CSS text-overflow：`ellipsis` 时单行溢出用 "…" 收尾（默认 clip，与浏览器一致）。
    // 返回值落到 DT_END_ELLIPSIS，由各画布后端负责"量宽 + 截断 + 补点"。
    inline bool TextOverflowEllipsis(const XmlAppearance& ap)
    {
        if (ap.textOverflow.empty()) return false;
        std::wstring v = ap.textOverflow;
        for (auto& c : v) c = static_cast<wchar_t>(towlower(c));
        return v.find(L"ellipsis") != std::wstring::npos;
    }

    inline UINT TextFlag(const XmlAppearance& ap)
    {
        UINT flag = DT_LEFT;
        if (_wcsicmp(ap.textAlign.c_str(), L"Center") == 0) flag = DT_CENTER;
        else if (_wcsicmp(ap.textAlign.c_str(), L"Right") == 0) flag = DT_RIGHT;
        // HTML text-align:justify —— 两端对齐。DT_ 里没有这一档，用自定义高位标志带下去。
        else if (_wcsicmp(ap.textAlign.c_str(), L"Justify") == 0) flag = DT_LEFT | kXmlTextJustify;
        if (TextOverflowEllipsis(ap))
            flag |= DT_END_ELLIPSIS;
        return flag;
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

    // CSS line-height 解析：`1.5`（倍数）/ `24px`（绝对）/ `150%`（百分比）。
    // 返回 false 表示未声明或无效值（此时走字体默认行距，和浏览器 auto 一致）。
    inline bool ResolveLineHeight(const std::wstring& raw, double& outValue, bool& outMultiplier)
    {
        std::wstring v = raw;
        while (!v.empty() && (v.front() == L' ' || v.front() == L'\t')) v.erase(v.begin());
        while (!v.empty() && (v.back() == L' ' || v.back() == L'\t')) v.pop_back();
        if (v.empty())
            return false;
        if (v.back() == L'%')
        {
            const double pct = _wtof(v.c_str());
            if (pct <= 0.0)
                return false;
            outValue = pct / 100.0;
            outMultiplier = true;
            return true;
        }
        bool px = false;
        if (v.size() > 2 && _wcsicmp(v.c_str() + v.size() - 2, L"px") == 0)
        {
            v.resize(v.size() - 2);
            px = true;
        }
        const double num = _wtof(v.c_str());
        if (num <= 0.0)
            return false;
        outValue = num;
        // CSS 规定无单位数字就是倍数；带 px 是绝对像素。小数字（<3）当倍数最符合常见写法。
        outMultiplier = !px && num < 3.0;
        return true;
    }

    // CSS text-indent 解析：`2em`（相对字号）/ `24px` / 裸数字按像素。返回像素宽度。
    inline double ResolveTextIndent(const std::wstring& raw, int fontPx)
    {
        std::wstring v = raw;
        while (!v.empty() && (v.front() == L' ' || v.front() == L'\t')) v.erase(v.begin());
        while (!v.empty() && (v.back() == L' ' || v.back() == L'\t')) v.pop_back();
        if (v.empty())
            return 0.0;
        const int fs = fontPx > 0 ? fontPx : 12;
        if (v.size() > 2 && _wcsicmp(v.c_str() + v.size() - 2, L"em") == 0)
        {
            v.resize(v.size() - 2);
            return _wtof(v.c_str()) * fs;
        }
        if (v.size() > 2 && _wcsicmp(v.c_str() + v.size() - 2, L"px") == 0)
            v.resize(v.size() - 2);
        const double num = _wtof(v.c_str());
        return num > 0.0 ? num : 0.0;
    }

    inline int ResolveRadius(const RECT& rc, const XmlAppearance& ap)
    {
        const int h = rc.bottom - rc.top;
        const int w = rc.right - rc.left;
        if (_wcsicmp(ap.shape.c_str(), L"Capsule") == 0)
            return h / 2;
        // CSS `border-radius:50%` → 正圆（按盒子短边半径），和浏览器一致
        if (ap.radiusPct >= 0)
        {
            const int s = w < h ? w : h;
            return s * ap.radiusPct / 100;
        }
        if (ap.radius > 0)
            return ap.radius;
        return 8;  // 圆角默认
    }

    // CSS 口径的"两个半径"版本：`border-radius:50%` 在非正方形盒子上是椭圆（rx=w*50%、ry=h*50%），
    // 单一半径只能给正圆，长条盒子里的头像/图片会圆得不像浏览器。px 值仍是统一半径。
    inline void ResolveRadiusXY(const RECT& rc, const XmlAppearance& ap, int& rx, int& ry)
    {
        const int w = rc.right - rc.left;
        const int h = rc.bottom - rc.top;
        if (ap.radiusPct >= 0)
        {
            rx = w * ap.radiusPct / 100;
            ry = h * ap.radiusPct / 100;
        }
        else
        {
            rx = ry = ResolveRadius(rc, ap);
        }
        if (rx > w / 2) rx = w / 2;
        if (ry > h / 2) ry = h / 2;
        if (rx < 0) rx = 0;
        if (ry < 0) ry = 0;
    }

    inline void ResolveCorners(const RECT& rc, const XmlAppearance& ap, int out[4])
    {
        int base = ResolveRadius(rc, ap);
        // 声明了任一分角时，未声明的角按 CSS 回落"统一 border-radius"；
        // 统一 radius 也没显式声明就是 0（HTML 默认无圆角）。
        // 这里的 8px 是本通道历史默认值，只在"完全没写圆角"时保留，不能跟着分角走。
        const bool anyCorner = ap.radiusTL >= 0 || ap.radiusTR >= 0 ||
                               ap.radiusBR >= 0 || ap.radiusBL >= 0;
        if (anyCorner)
        {
            const bool explicitRadius = ap.radiusPct >= 0 || ap.radius > 0 ||
                                        _wcsicmp(ap.shape.c_str(), L"Capsule") == 0;
            if (!explicitRadius)
                base = 0;
        }
        out[0] = ap.radiusTL >= 0 ? ap.radiusTL : base;   // 左上
        out[1] = ap.radiusTR >= 0 ? ap.radiusTR : base;   // 右上
        out[2] = ap.radiusBR >= 0 ? ap.radiusBR : base;   // 右下
        out[3] = ap.radiusBL >= 0 ? ap.radiusBL : base;   // 左下
        const int lim = (rc.bottom - rc.top) / 2 < (rc.right - rc.left) / 2
                            ? (rc.bottom - rc.top) / 2 : (rc.right - rc.left) / 2;
        for (int i = 0; i < 4; ++i)
            if (out[i] > lim) out[i] = lim;
    }

    inline bool IsRectShape(const XmlAppearance& ap)
    {
        return _wcsicmp(ap.shape.c_str(), L"Rect") == 0;
    }

    inline bool IsEllipseShape(const XmlAppearance& ap)
    {
        return _wcsicmp(ap.shape.c_str(), L"Ellipse") == 0;
    }

    // 解析 CSS `linear-gradient(<angle>deg, #c1, #c2[, ...])`。
    // 只取首尾两个色标（中间的中间色在 GDI 背板层意义不大），角度缺省按 180deg（从上到下）。
    // 成功返回 true 并给出起止色；bgImage 不是渐变时返回 false，调用方保持原纯色路径。
    inline bool ParseLinearGradient(const std::wstring& raw, COLORREF& from, COLORREF& to,
                                    int& angleDeg)
    {
        const std::size_t g = raw.find(L"gradient(");
        if (g == std::wstring::npos)
            return false;
        const std::size_t open = raw.find(L'(', g);
        const std::size_t close = raw.rfind(L')');
        if (open == std::wstring::npos || close == std::wstring::npos || close <= open)
            return false;
        std::wstring body = raw.substr(open + 1, close - open - 1);

        // 先摘角度（`180deg` / `to bottom` 之类）
        angleDeg = 180;
        const std::size_t deg = body.find(L"deg");
        if (deg != std::wstring::npos)
        {
            std::wstring num;
            for (std::size_t i = deg; i > 0; --i)
            {
                const wchar_t c = body[i - 1];
                if ((c >= L'0' && c <= L'9') || c == L'-')
                    num.insert(num.begin(), c);
                else
                    break;
            }
            if (!num.empty())
                angleDeg = (int)wcstol(num.c_str(), nullptr, 10);
            body = body.substr(deg + 3);
        }
        else
        {
            const std::size_t to = body.find(L"to ");
            if (to != std::wstring::npos)
            {
                const std::wstring dir = body.substr(to, 48);
                if (dir.find(L"right") != std::wstring::npos)
                    angleDeg = 90;
                else if (dir.find(L"top") != std::wstring::npos)
                    angleDeg = 0;
                else
                    angleDeg = 180;
                const std::size_t comma = body.find(L',', to);
                body = comma == std::wstring::npos ? std::wstring() : body.substr(comma + 1);
            }
        }

        // 剩下的按逗号取色标
        std::vector<std::wstring> stops;
        std::wstring cur;
        for (wchar_t c : body)
        {
            if (c == L',')
            {
                if (!cur.empty()) { stops.push_back(cur); cur.clear(); }
            }
            else if (c != L' ' && c != L'\t')
                cur.push_back(c);
        }
        if (!cur.empty())
            stops.push_back(cur);
        if (stops.size() < 2)
            return false;
        // 色标可能带位置后缀（`#fff 20%`），只在带百分号时截掉；
        // 纯十六进制色不能剥尾位数字，否则 `#000000` 会被削成 `#`。
        for (auto& s : stops)
        {
            const std::size_t p = s.find(L'%');
            if (p == std::wstring::npos)
                continue;
            s = s.substr(0, p);
            while (!s.empty() && (s.back() == L' ' ||
                                  (s.back() >= L'0' && s.back() <= L'9')))
                s.pop_back();
        }
        return ParseColor(stops.front(), from) && ParseColor(stops.back(), to);
    }

    // GDI 逐行填充线性渐变（背板层没有渐变刷子，只能按行走）。
    // 只处理 4 个正交方向，其余角度就近取（斜向用水平近似，视觉上仍是同色系过渡）。
    inline void FillRectGradientRows(HDC hdc, const RECT& rc, COLORREF from, COLORREF to,
                                      int angleDeg, const XmlAppearance& ap)
    {
        if (hdc == nullptr || rc.right <= rc.left || rc.bottom <= rc.top)
            return;
        const bool vertical = (angleDeg % 180) != 90;
        const int span = vertical ? (rc.bottom - rc.top) : (rc.right - rc.left);
        if (span <= 0)
            return;

        // 圆角/形状裁剪：非矩形时先把绘制区裁成圆角矩形，避免渐变糊到直角上
        HRGN clip = nullptr;
        HRGN oldClip = nullptr;
        if (!IsRectShape(ap))
        {
            const int r = ResolveRadius(rc, ap) * 2;
            clip = ::CreateRoundRectRgn(rc.left, rc.top, rc.right, rc.bottom, r, r);
            oldClip = ::CreateRectRgn(0, 0, 0, 0);
            if (::SelectClipRgn(hdc, clip) == ERROR)
            {
                ::DeleteObject(clip);
                ::DeleteObject(oldClip);
                clip = nullptr;
                oldClip = nullptr;
            }
        }

        const int fr = GetRValue(from), fg = GetGValue(from), fb = GetBValue(from);
        const int tr = GetRValue(to), tg = GetGValue(to), tb = GetBValue(to);
        for (int i = 0; i < span; ++i)
        {
            const int t = (span > 1) ? (i * 255 / (span - 1)) : 0;
            const COLORREF c = RGB(fr + (tr - fr) * t / 255,
                                   fg + (tg - fg) * t / 255,
                                   fb + (tb - fb) * t / 255);
            RECT row = rc;
            if (vertical)
            {
                row.top = rc.top + i;
                row.bottom = row.top + 1;
            }
            else
            {
                row.left = rc.left + i;
                row.right = row.left + 1;
            }
            HBRUSH b = ::CreateSolidBrush(c);
            ::FillRect(hdc, &row, b);
            ::DeleteObject(b);
        }

        if (clip != nullptr)
        {
            ::SelectClipRgn(hdc, oldClip);
            ::DeleteObject(clip);
            ::DeleteObject(oldClip);
        }
    }

    // 按形状填充（可选描边）
    inline void FillShaped(HDC hdc, const RECT& rc, const XmlAppearance& ap,
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

    // CSS box-shadow 拆分结果：inset 区分内外投影；偏移 / blur / spread / 颜色 / alpha 齐全。
    // 外投走 DrawBoxShadow 老路径（盒外暗带）；内投由容器位图刷工厂在盒内合成。
    struct BoxShadowSpec
    {
        bool inset = false;
        bool valid = false;
        int ox = 0, oy = 0, blur = 0, spread = 0;
        COLORREF color = RGB(0, 0, 0);
        int alpha = 255;
    };

    // CSS box-shadow 解析：`0 4 10 rgba(0,0,0,0.55)` / `0 2 6 #000000` / `2px 2px` 都认。
    // inset 关键字标记内投影；返回 false 表示空串 / none / 无可画项。
    inline bool ParseBoxShadowSpec(const std::wstring& spec, BoxShadowSpec& out)
    {
        out = BoxShadowSpec();
        if (spec.empty())
            return false;
        std::wstring s = spec;
        for (auto& c : s)
            if (c == L'\t' || c == L'\n' || c == L'\r')
                c = L' ';
        const std::string lowered = util::lower(util::wideToUtf8(s));
        if (lowered.find("none") != std::string::npos)
            return false;
        out.inset = lowered.find("inset") != std::string::npos;

        int nums[4] = {0, 0, 0, 0};
        int nNum = 0;
        bool hasColor = false;
        size_t i = 0;
        while (i < s.size())
        {
            while (i < s.size() && (s[i] == L' ' || s[i] == L'\t')) ++i;
            if (i >= s.size())
                break;
            const size_t start = i;
            while (i < s.size() && s[i] != L' ') ++i;
            const std::wstring tok = s.substr(start, i - start);
            if (tok.empty())
                continue;
            const std::string ltok = util::lower(util::wideToUtf8(tok));
            // 关键字 token（inset/outset/none）不占偏移/blur 槽位；逗号保留在 rgba(...) 内。
            if (ltok == "inset" || ltok == "outset" || ltok == "none")
                continue;
            // 颜色 token：rgba(...) / #hex / 颜色名
            if (ltok.find("rgba") != std::string::npos || ltok.find("rgb") != std::string::npos ||
                tok[0] == L'#' || (!iswdigit(tok[0]) && tok[0] != L'-' && tok[0] != L'.'))
            {
                if (tok[0] == L'#')
                {
                    // #RRGGBBAA：alpha 在末尾两位（CSS Color 4 写法）
                    if (tok.size() == 9)
                    {
                        out.alpha = static_cast<int>(wcstol(tok.substr(7, 2).c_str(), nullptr, 16));
                        ParseColor(tok.substr(0, 7), out.color);
                    }
                    else
                        ParseColor(tok, out.color);
                    hasColor = true;
                    continue;
                }
                // rgba(r,g,b,a)
                const size_t lp = ltok.find('(');
                const size_t rp = ltok.rfind(')');
                if (lp != std::string::npos && rp != std::string::npos && rp > lp)
                {
                    std::wstring inner = tok.substr(lp + 1, rp - lp - 1);
                    int comp[4] = {0, 0, 0, 255};
                    int cn = 0;
                    size_t p = 0;
                    while (p <= inner.size() && cn < 4)
                    {
                        size_t q = p;
                        while (q < inner.size() && inner[q] != L',') ++q;
                        std::wstring part = inner.substr(p, q - p);
                        while (!part.empty() && (part.front() == L' ')) part.erase(part.begin());
                        while (!part.empty() && (part.back() == L' ' || part.back() == L'%')) part.pop_back();
                        if (cn == 3)
                        {
                            double a = _wtof(part.c_str());
                            if (part.find(L'%') == std::wstring::npos && a <= 1.0)
                                a *= 255.0;   // CSS 的 0..1 与 0..100% 两种写法
                            comp[3] = (std::min)(255, (std::max)(0, static_cast<int>(a)));
                        }
                        else
                            comp[cn] = _wtoi(part.c_str());
                        ++cn;
                        p = q + 1;
                        if (q >= inner.size())
                            break;
                    }
                    out.color = RGB((std::min)(255, comp[0]), (std::min)(255, comp[1]),
                                    (std::min)(255, comp[2]));
                    out.alpha = comp[3];
                    hasColor = true;
                    continue;
                }
                if (ParseColor(tok, out.color))
                {
                    hasColor = true;
                    continue;
                }
            }
            // 长度 token：`10` / `10px` / `-4px`
            if (nNum < 4)
            {
                std::wstring num = tok;
                const size_t px = num.find(L"px");
                if (px != std::wstring::npos)
                    num = num.substr(0, px);
                nums[nNum++] = _wtoi(num.c_str());
            }
        }
        out.ox = nums[0];
        out.oy = nums[1];
        out.blur = nums[2] < 0 ? 0 : nums[2];
        out.spread = nums[3];
        out.valid = hasColor || nNum > 0;
        return out.valid;
    }

    // CSS box-shadow 允许逗号分隔的多条阴影：
    //   `0 2 4 rgba(0,0,0,.2), 0 0 0 1px #fff` / `inset 0 0 0 2 #000, 0 4 10 #000`
    // 老实现把整串当一条扫，数字只取前 4 个、颜色被最后一条覆盖 —— 多条阴影会退化成
    // 一条语义错位的阴影。这里按"顶层逗号"切分（跳过 rgba(...) 括号内的逗号）。
    inline void ParseBoxShadowSpecList(const std::wstring& spec,
                                       std::vector<BoxShadowSpec>& out)
    {
        out.clear();
        if (spec.empty())
            return;
        std::wstring cur;
        int depth = 0;
        for (size_t i = 0; i < spec.size(); ++i)
        {
            const wchar_t c = spec[i];
            if (c == L'(')
                ++depth;
            else if (c == L')')
            {
                if (depth > 0)
                    --depth;
            }
            if (c == L',' && depth == 0)
            {
                BoxShadowSpec sp;
                if (ParseBoxShadowSpec(cur, sp))
                    out.push_back(sp);
                cur.clear();
                continue;
            }
            cur.push_back(c);
        }
        BoxShadowSpec sp;
        if (ParseBoxShadowSpec(cur, sp))
            out.push_back(sp);
    }

    // 外投解析老接口：inset 一律当"无可画外投影"返回 false（盒外断言盯着这一点）。
    // 多条外投时取"最显著"的一条（blur+spread 最大，同值取 alpha 大的）作为代表。
    inline bool ParseBoxShadow(const std::wstring& spec, int& ox, int& oy, int& blur,
                               int& spread, COLORREF& color, int& alpha)
    {
        std::vector<BoxShadowSpec> list;
        ParseBoxShadowSpecList(spec, list);
        const BoxShadowSpec* best = nullptr;
        for (const BoxShadowSpec& sp : list)
        {
            if (sp.inset)
                continue;
            if (best == nullptr || (sp.blur + sp.spread) > (best->blur + best->spread) ||
                ((sp.blur + sp.spread) == (best->blur + best->spread) && sp.alpha > best->alpha))
                best = &sp;
        }
        if (best == nullptr)
            return false;
        ox = best->ox;
        oy = best->oy;
        blur = best->blur;
        spread = best->spread;
        color = best->color;
        alpha = best->alpha;
        return true;
    }

    // box-shadow 的 GDI 近似：没有高斯模糊，用"由外向内、逐圈加深的实心矩形"模拟，
    // 颜色预先与页面底色混合（GDI 无 alpha 通道）。阴影画在盒子底下：
    // 调用方必须"先阴影后盒子"，盒子自身会把中心那块盖掉，只剩外圈可见。
    inline void DrawBoxShadow(HDC hdc, const RECT& rc, const std::wstring& spec,
                              COLORREF base)
    {
        if (hdc == nullptr)
            return;
        std::vector<BoxShadowSpec> list;
        ParseBoxShadowSpecList(spec, list);
        // CSS 的多条阴影是"先写的在上、后写的在下"，所以倒序画：
        // 最后一条先铺底，第一条最后画、盖在最上面。
        for (size_t idx = list.size(); idx-- > 0;)
        {
            const BoxShadowSpec& sp = list[idx];
            if (sp.inset || sp.alpha <= 0)
                continue;
            RECT s = rc;
            ::OffsetRect(&s, sp.ox, sp.oy);
            ::InflateRect(&s, sp.spread, sp.spread);
            // 模糊圈数：blur 越大圈越多；每圈 alpha 由外(浅)到内(深)
            const int rings = (std::max)(1, (sp.blur + 1) / 2);
            for (int i = rings; i >= 1; --i)
            {
                RECT r = s;
                ::InflateRect(&r, i, i);
                const int a = static_cast<int>(static_cast<double>(sp.alpha) *
                                               (static_cast<double>(rings - i + 1) / (rings + 1.0)));
                if (a <= 0)
                    continue;
                HBRUSH br = ::CreateSolidBrush(Blend(sp.color, base, (std::min)(255, a)));
                ::FillRect(hdc, &r, br);
                ::DeleteObject(br);
            }
        }
    }

    // 边框该不该画：CSS `border:1px solid #f00` 解析出来的 BorderStyle 是 "solid"，
    // 而旧实现只认 XML 老写法的 Single/Sunken/Raised —— 于是 style 简写"解析成功、画不出来"，
    // 浏览器里一条红框在 XML 里完全消失（真 bug）。这里把两套词表合并：
    // 线型词（single/solid/dashed/dotted/double/groove/ridge/inset/outset）都算有边框，
    // none/0 不算；另外"显式给了宽度和颜色"也算（老 XML 的 BorderThickness + BorderColor 写法）。
    inline bool HasVisibleBorder(const XmlAppearance& ap)
    {
        static const wchar_t* const kVisible[] = {
            L"Single", L"Solid", L"Dashed", L"Dotted", L"Double",
            L"Groove", L"Ridge", L"Inset", L"Outset", L"Sunken", L"Raised"
        };
        for (const wchar_t* s : kVisible)
            if (_wcsicmp(ap.borderStyle.c_str(), s) == 0)
                return true;
        if (_wcsicmp(ap.borderStyle.c_str(), L"None") == 0 || ap.borderStyle == L"0")
            return false;
        return ap.borderThickness > 0 && !ap.borderColor.empty();
    }

    // 后置背景 + 形状 + 边框 + 前置背景（一条龙；文字由调用方随后自己画）
    inline void DrawBackplate(HDC hdc, const RECT& rc, const XmlAppearance& ap,
                              COLORREF fallbackBack, COLORREF fallbackBorder)
    {
        if (hdc == nullptr)
            return;
        COLORREF back = fallbackBack;
        if (!ap.backColor.empty())
        {
            COLORREF c = 0;
            if (ParseColor(ap.backColor, c))
                // CSS `background:rgba(r,g,b,a)` 只让背景半透明（文字/边框不透明），
                // 所以 bgAlpha 单独参与背景混合；opacity 是整块（含边框）的近似。
                back = Blend(c, fallbackBack, (std::min)(ap.opacity, ap.bgAlpha));
        }
        COLORREF border = fallbackBorder;
        if (!ap.borderColor.empty())
        {
            COLORREF c = 0;
            if (ParseColor(ap.borderColor, c))
                border = c;
        }

        // CSS 单边边框：任一边显式声明后，整圈描边退位，由下面逐边落像素。
        // HTML 的 `border-bottom:1px solid #333` 只要一条线；此前 XML 只能整圈框住。
        const bool anySide = ap.borderSideW[0] >= 0 || ap.borderSideW[1] >= 0 ||
                             ap.borderSideW[2] >= 0 || ap.borderSideW[3] >= 0;

        // 只有 XML 明确写了边框样式（Single/Sunken/Raised）才描边。
        // 不能用"borderStyle 非空"或 fallbackBorder 来兜底，否则未声明边框的控件
        // （TextBlock 这类）会凭空多出一圈深色圆角框。
        const bool styleBorder = HasVisibleBorder(ap) && !anySide;
        const COLORREF stroke = styleBorder ? border : CLR_INVALID;

        // CSS box-shadow：先铺外投影（盒子之外的暗带），盒子底色随后把它中心盖掉，
        // 与 HTML 的"投影只在盒外可见"一致。page/控件背板走同一份 DrawBackplate，
        // 所以容器、按钮、卡片都能写一条 box-shadow 就用上。
        if (!ap.boxShadow.empty())
            DrawBoxShadow(hdc, rc, ap.boxShadow, fallbackBack);

        // CSS linear-gradient 优先于纯色背板：先逐行铺渐变，再叠一圈描边
        // （描边用 FillShaped 只画边框不足以复用，这里保留原有"先填充后描边"的顺序）。
        COLORREF g1 = 0, g2 = 0;
        int gAng = 180;
        if (!ap.bgImage.empty() && ParseLinearGradient(ap.bgImage, g1, g2, gAng))
        {
            FillRectGradientRows(hdc, rc, Blend(g1, fallbackBack, ap.opacity),
                                 Blend(g2, fallbackBack, ap.opacity), gAng, ap);
            if (stroke != CLR_INVALID)
            {
                const int th = ap.borderThickness > 0 ? ap.borderThickness : 1;
                HPEN pen = ::CreatePen(PS_SOLID, th, stroke);
                HGDIOBJ oldPen = ::SelectObject(hdc, pen);
                HGDIOBJ oldBrush = ::SelectObject(hdc, ::GetStockObject(NULL_BRUSH));
                if (IsRectShape(ap))
                    ::Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
                else
                {
                    const int r = ResolveRadius(rc, ap) * 2;
                    ::RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, r, r);
                }
                ::SelectObject(hdc, oldBrush);
                ::SelectObject(hdc, oldPen);
                ::DeleteObject(pen);
            }
        }
        else
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

        // 单边边框：顺序 上/右/下/左。未声明的那一边沿用整圈 borderThickness（0 就不画）。
        // 简化口径：单边声明时按矩形边落像素，不参与圆角切角（圆角 + 单边列在台账待办）。
        if (anySide)
        {
            auto sideColor = [&](int i) -> COLORREF {
                COLORREF c = CLR_INVALID;
                if (!ap.borderSideColor[i].empty() && ParseColor(ap.borderSideColor[i], c))
                    return c;
                return border;
            };
            const int rectW = rc.right - rc.left;
            const int rectH = rc.bottom - rc.top;
            for (int i = 0; i < 4; ++i)
            {
                const int th = ap.borderSideW[i] >= 0 ? ap.borderSideW[i]
                                                     : (ap.borderThickness > 0 ? ap.borderThickness : 0);
                if (th <= 0)
                    continue;
                RECT band = rc;
                if (i == 0)      band.bottom = band.top + th;
                else if (i == 2) band.top = band.bottom - th;
                else if (i == 3) band.right = band.left + th;
                else             band.left = band.right - th;
                if ((i == 0 || i == 2) && rectH <= 0) continue;
                if ((i == 1 || i == 3) && rectW <= 0) continue;
                HBRUSH br = ::CreateSolidBrush(sideColor(i));
                if (br != nullptr)
                {
                    ::FillRect(hdc, &band, br);
                    ::DeleteObject(br);
                }
            }
            // 单边已经有肉眼可见的分割线，老 3D 框不再重复叠（避免 Sunken + 单边同时写时糊成一片）
        }
    }
}
