// XmlCore.Html.cpp —— 拆自 XmlCore.cpp 第 3686-5917 行。
#include "XmlCoreCommon.h"
#include "XmlCoreInternal.h"

// ===========================================================================
// HTML 词汇糖：让 XML 直接用 HTML 的标签名写界面
// ===========================================================================
// 映射表（左 = HTML 标签，右 = 已有原生控件）：
//   div/section/article/main/header/footer/nav/aside/form/figure/blockquote/
//   dl/menu/dialog/center/marquee → XStackPanel（高度按内容自动算，不撑满父容器）
//   p/span/strong/b/em/i/small/mark/label/h1..h6/output/time/dt/dd/cite/kbd/q/
//   abbr/u/s/ins/del/bdi/bdo/wbr/sub/sup/samp/var/data/acronym/big/tt/strike/
//   font/nobr/title → XLabel
//   code/pre → XLabel(等宽)   a → XLabel(蓝字+下划线, Url 留点击)   img/picture → XImage
//   hr → XDivider            progress/meter → XProgressBar
//   button → XButton         input(type) → XTextField/XCheckBox/XRadioButton/
//                                            XSlider/XSpinBox/XDatePicker/XButton
//   textarea → XTextArea     select(+option) → XComboBox / XListBox(multiple|size>1)
//   ul/ol(+li) → XListBox    fieldset(+legend)/details(+summary) → XGroupBox
//   table(+tr/td/th, colspan) → XGrid（行列定义按单元格数自动生成，td/th 逐格变 XLabel）
//   canvas/svg → XQCanvas    video/audio/iframe/embed/object/dialog → XPane
//   br → 8px 空行
// 不渲染（只描述文档结构/资源，识别后直接丢弃）：head/meta/link/base/style/script/
//   noscript/template/slot/source/track/param/col/colgroup/datalist/frameset/frame/applet
// 属性别名：id→Name、title/alt/aria-label→ToolTip、href→Url、src→Image、rows/cols→Height/Width、
//   min/max→Minimum/Maximum、disabled→Enabled=False、readonly→IsReadOnly、hidden→Visible=False、
//   maxlength→MaxLength、onclick→Click、onblur→LostFocus、
//   style="width/height/padding/gap" → 尺寸/内边距/间距，其余 CSS 忽略，
//   input type=hidden 不生成控件；required/pattern/step/data-*/aria-* 等一律清掉。
namespace htmlsugar
{
    std::wstring Lower(std::wstring s)
    {
        for (wchar_t& c : s)
            c = (wchar_t)towlower(c);
        return s;
    }

    // CSS 声明名归一：官方写法 kebab-case（border-radius），HTML/JS 侧等价写法是驼峰
    // （borderRadius）。只做 Lower 会把驼峰压成 borderradius、与 border-radius 不相等，
    // 表现就是 backgroundColor / fontSize / fontWeight / textAlign 这些"写了不生效"。
    // 这里先把驼峰展开成 kebab（同时把 '_' 视作 '-'），再转小写，两种写法进同一分支。
    std::wstring CssKeyNormalize(const std::wstring& raw)
    {
        std::wstring out;
        out.reserve(raw.size() + 4);
        for (size_t i = 0; i < raw.size(); ++i)
        {
            const wchar_t ch = raw[i];
            if (ch >= L'A' && ch <= L'Z')
            {
                if (i > 0 && !out.empty() && out.back() != L'-')
                    out += L'-';
                out += static_cast<wchar_t>(towlower(ch));
            }
            else
                out += (ch == L'_') ? L'-' : static_cast<wchar_t>(towlower(ch));
        }
        return out;
    }

    std::wstring Trim(const std::wstring& s)
    {
        const size_t b = s.find_first_not_of(L" \t\r\n");
        if (b == std::wstring::npos) return std::wstring();
        const size_t e = s.find_last_not_of(L" \t\r\n");
        return s.substr(b, e - b + 1);
    }

    std::wstring AttrOf(const XmlElement& e, const wchar_t* key)
    {
        return Trim(e.get(key));
    }

    // HTML 布尔属性：写了就算真（checked / disabled / selected / readonly / multiple /
    // hidden 等）。除了少数真/假字面量，"值等于属性名本身"也算真（hidden="hidden"）。
    bool Flag(const XmlElement& e, const wchar_t* key)
    {
        if (!e.has(key)) return false;
        const std::wstring v = Lower(Trim(e.get(key)));
        if (v.empty() || v == L"true" || v == L"1" || v == L"on" || v == L"yes")
            return true;
        return v == Lower(std::wstring(key));
    }

    void Put(XmlElement& e, const wchar_t* key, const std::wstring& v) { e.set(key, v); }

    void Drop(XmlElement& e, const wchar_t* key)
    {
        // 大小写敏感：只删 HTML 原始写法（checked/href…），不能误伤刚翻译出来的
        // 正名（Checked/Url…）——两者只差一个大小写，用 _wcsicmp 会把正名一起删掉。
        for (size_t i = 0; i < e.attrs.size(); ++i)
            if (wcscmp(e.attrs[i].first.c_str(), key) == 0)
            {
                e.attrs.erase(e.attrs.begin() + (std::ptrdiff_t)i);
                return;
            }
    }

    // 布局字段在解析阶段就落定了，这里补尺寸要同时写字段和属性
    void PutSize(XmlElement& e, int w, int h)
    {
        if (w > 0) { e.w = w; e.hasW = true; Put(e, L"Width", std::to_wstring(w)); }
        if (h > 0) { e.h = h; e.hasH = true; Put(e, L"Height", std::to_wstring(h)); }
    }

    // HTML 小写属性摘出来再翻译：checked / value 的正名与原名只差大小写，
    // 若直接 set("Checked")，set() 会就地改写小写那条（键名保持小写），
    // 随后清理 HTML 属性时又被一起删掉 —— 这里先摘走，再写正名。
    bool TakeAttr(XmlElement& e, const wchar_t* lowerName, std::wstring& out)
    {
        for (size_t i = 0; i < e.attrs.size(); ++i)
            if (_wcsicmp(e.attrs[i].first.c_str(), lowerName) == 0)
            {
                out = Trim(e.attrs[i].second);
                e.attrs.erase(e.attrs.begin() + (std::ptrdiff_t)i);
                return true;
            }
        return false;
    }

    bool TakeFlag(XmlElement& e, const wchar_t* lowerName)
    {
        std::wstring v;
        if (!TakeAttr(e, lowerName, v))
            return false;
        const std::wstring t = Lower(v);
        return t.empty() || t == L"true" || t == L"1" || t == lowerName ||
               t == L"on" || t == L"yes";
    }

    // class 里写 row / flex / inline 就是横排（HTML 靠 CSS，我们靠这两个记号）
    bool HasClass(XmlElement& e, const wchar_t* token)
    {
        if (!e.has(L"class"))
            return false;
        std::wstring text = Lower(Trim(e.get(L"class")));
        const std::wstring want = Lower(std::wstring(token));
        size_t start = 0;
        while (start <= text.size())
        {
            const size_t sp = text.find_first_of(L" \t,;", start);
            const std::wstring word = text.substr(start, sp == std::wstring::npos
                                                         ? std::wstring::npos : sp - start);
            if (word == want)
                return true;
            if (sp == std::wstring::npos)
                break;
            start = sp + 1;
        }
        return false;
    }

    // HTML 里样式是可以复用的：一段 <style>.card{...}</style> 配上若干 <div class="card">，
    // 写一遍就能给 N 个元素同一套外观。XML 侧此前只认 style="..."，每个元素都得抄一遍
    // —— 这就是"HTML 能做而 XML 明显更啰嗦"。这里补最小可用的 CSS 规则表：
    // 只支持"单选择器"（.class / tag.class / #id / tag / *，逗号并列），
    // 不支持后代/子代组合器与 @media（单选择器之外一律不匹配，宁缺勿错）。
    struct CssRule
    {
        std::wstring selector;      // 简单选择器（逗号串，已剥掉伪类后缀）
        std::wstring decls;         // { } 里的声明原样
        std::wstring pseudo;        // hover / focus / active；空 = 常态规则
        bool important = false;     // 声明里带 !important（CSS 里它压过一切常规声明，含行内）
    };

    // CSS 伪类 → 本系统的状态属性名（HTML 里一行 `button:hover{...}`，
    // 以前只能手写 hover-background / hover-color，还写不了 focus/active 的复用）。
    // 顺带认结构伪类（`:first-child` / `:last-child` / `:nth-child(...)`），
    // 它们是"表格斑马纹"这类 HTML 标准写法的基础，XML 侧此前只能手工给每行加 class。
    bool CssSplitPseudo(std::wstring& sel, std::wstring& pseudo)
    {
        static const wchar_t* kStates[] = {
            L":hover", L":focus", L":active",
            L":disabled", L":checked", L":selected",
            L":first-child", L":last-child", L":only-child", nullptr
        };
        const std::wstring low = Lower(sel);
        for (const wchar_t** p = kStates; *p != nullptr; ++p)
        {
            const size_t n = wcslen(*p);
            if (low.size() >= n && low.compare(low.size() - n, n, *p) == 0)
            {
                pseudo = std::wstring(*p + 1);          // 去掉冒号
                sel = Trim(sel.substr(0, sel.size() - n));
                return true;
            }
            // 伪类后面若还挂了别的记号（`:hover.x` 之类）就不认，宁缺勿错
        }
        if (low.size() > 12 && low.back() == L')')
        {
            const size_t fn = low.rfind(L":nth-child(");
            if (fn != std::wstring::npos && fn + 11 < low.size())
            {
                const std::wstring arg = Trim(sel.substr(fn + 11, sel.size() - fn - 12));
                if (!arg.empty())
                {
                    pseudo = L"nth-child(" + Lower(arg) + L")";
                    sel = Trim(sel.substr(0, fn));
                    return true;
                }
            }
        }
        return false;
    }

    // 伪元素（::before / ::after）：HTML 里用 `content:"★ "` 给元素加前缀/后缀文字，
    // XML 侧只能把这段字直接写进正文（改文案时要连正文一起动）。这里只接纯文本 content。
    bool CssSplitPseudoElement(std::wstring& sel, std::wstring& pseudo)
    {
        static const wchar_t* k[] = {
            L"::before", L":before", L"::after", L":after", nullptr
        };
        const std::wstring low = Lower(sel);
        for (const wchar_t** p = k; *p != nullptr; ++p)
        {
            const size_t n = wcslen(*p);
            if (low.size() >= n && low.compare(low.size() - n, n, *p) == 0)
            {
                pseudo = wcsstr(*p, L"before") != nullptr ? L"before" : L"after";
                sel = Trim(sel.substr(0, sel.size() - n));
                return !sel.empty();
            }
        }
        return false;
    }

    // 从 `content:"★ "` 里取纯文本（去掉两侧引号；`none`/`normal` 视为不生成）
    // `content:"★" attr(data-label)` —— HTML 里能把元素自己的属性值直接印进伪元素，
    // 属性改一次文字跟着变；XML 侧早先只能把同一段文字抄在每个元素上。
    std::wstring CssResolveContent(const std::wstring& v, const XmlElement& e)
    {
        std::wstring out;
        size_t i = 0;
        while (i < v.size())
        {
            while (i < v.size() && (iswspace(v[i]) || v[i] == L','))
                ++i;
            if (i >= v.size())
                break;
            if (v[i] == L'"' || v[i] == L'\'')
            {
                const wchar_t q = v[i++];
                while (i < v.size() && v[i] != q)
                    out += v[i++];
                if (i < v.size())
                    ++i;
                continue;
            }
            if (Lower(v.substr(i, 5)) == L"attr(")
            {
                const size_t close = v.find(L')', i);
                if (close == std::wstring::npos)
                    break;
                out += AttrOf(e, Trim(v.substr(i + 5, close - i - 5)).c_str());
                i = close + 1;
                continue;
            }
            // 其它 token（none / normal / counters(…) 等）直接跳过
            while (i < v.size() && v[i] != L'"' && v[i] != L'\'' && v[i] != L')')
                ++i;
            if (i < v.size() && v[i] == L')')
                ++i;
        }
        return out;
    }

    std::wstring CssContentText(const std::wstring& decls, const XmlElement& e)
    {
        size_t pos = 0;
        while (pos < decls.size())
        {
            const size_t semi = decls.find(L';', pos);
            const std::wstring one = decls.substr(pos, semi == std::wstring::npos
                                                       ? std::wstring::npos : semi - pos);
            pos = (semi == std::wstring::npos) ? decls.size() : semi + 1;
            const size_t colon = one.find(L':');
            if (colon == std::wstring::npos)
                continue;
            if (Lower(Trim(one.substr(0, colon))) != L"content")
                continue;
            std::wstring v = Trim(one.substr(colon + 1));
            const std::wstring low = Lower(v);
            if (low == L"none" || low == L"normal" || low.empty())
                return std::wstring();
            if (v.find(L'"') == std::wstring::npos && v.find(L'\'') == std::wstring::npos &&
                low.find(L"attr(") == std::wstring::npos)
                return v;                               // 老行为：无引号裸文本原样返回
            return CssResolveContent(v, e);
        }
        return std::wstring();
    }

    // 结构伪类是否命中：按元素在父的子序列里的位置判定（只看兄弟元素，不数文本）
    bool CssIsStructural(const std::wstring& pseudo)
    {
        return pseudo == L"first-child" || pseudo == L"last-child" ||
               pseudo == L"only-child" || pseudo.compare(0, 10, L"nth-child(") == 0;
    }

    // 属性态伪类（:disabled / :checked / :selected）：HTML 里靠控件自身状态生效，
    // 这里读元素上的同名属性（`disabled="1"`、`enabled="0"`、`checked="1"`）。
    bool CssAttrOn(const XmlElement& e, const std::wstring& name)
    {
        const std::wstring v = Lower(Trim(AttrOf(e, name.c_str())));
        if (v.empty())
            return false;                               // 属性缺省
        return !(v == L"0" || v == L"false" || v == L"no" || v == L"off");
    }

    bool CssIsAttrState(const std::wstring& pseudo)
    {
        return pseudo == L"disabled" || pseudo == L"checked" || pseudo == L"selected";
    }

    bool CssAttrStateMatches(const std::wstring& pseudo, const XmlElement& e)
    {
        if (pseudo == L"disabled")
        {
            if (CssAttrOn(e, L"disabled") || CssAttrOn(e, L"readonly"))
                return true;
            const std::wstring en = AttrOf(e, L"enabled");
            return !en.empty() && !CssAttrOn(e, L"enabled");
        }
        return CssAttrOn(e, pseudo);
    }

    bool CssStructuralMatches(const std::wstring& pseudo, const XmlElement& e,
                              const std::vector<const XmlElement*>& ancestors)
    {
        if (ancestors.empty())
            return false;
        const XmlElement& parent = *ancestors.back();
        size_t idx = parent.children.size();
        for (size_t i = 0; i < parent.children.size(); ++i)
            if (&parent.children[i] == &e)
            {
                idx = i;
                break;
            }
        if (idx == parent.children.size())
            return false;
        const size_t count = parent.children.size();
        if (pseudo == L"first-child")
            return idx == 0;
        if (pseudo == L"last-child")
            return idx + 1 == count;
        if (pseudo == L"only-child")
            return count == 1;
        // nth-child(odd|even|N)：位置从 1 开始数
        const std::wstring arg = pseudo.substr(10, pseudo.size() - 11);
        const size_t pos = idx + 1;
        if (arg == L"odd")
            return (pos % 2) == 1;
        if (arg == L"even")
            return (pos % 2) == 0;
        int n = 0;
        bool digits = !arg.empty();
        for (wchar_t ch : arg)
            if (ch < L'0' || ch > L'9')
            {
                digits = false;
                break;
            }
        if (digits)
        {
            for (wchar_t ch : arg)
                n = n * 10 + (ch - L'0');
            return n > 0 && pos == static_cast<size_t>(n);
        }
        // an+b 形式（2n+1 / 3n / n+2 / -n+3 / -2n+5）：存在 k>=0 使 pos == a*k+b
        const size_t np = arg.find(L'n');
        if (np == std::wstring::npos)
            return false;
        const std::wstring aText = Trim(arg.substr(0, np));
        int a = 1;
        if (aText == L"-")
            a = -1;
        else if (!aText.empty())
        {
            int sign = 1;
            size_t i = 0;
            if (aText[0] == L'-')
            {
                sign = -1;
                i = 1;
            }
            else if (aText[0] == L'+')
                i = 1;
            int val = 0;
            bool ok = i < aText.size();
            for (; i < aText.size(); ++i)
            {
                if (aText[i] < L'0' || aText[i] > L'9')
                {
                    ok = false;
                    break;
                }
                val = val * 10 + (aText[i] - L'0');
            }
            if (!ok)
                return false;
            a = sign * val;
        }
        int b = 0;
        const std::wstring rest = Trim(arg.substr(np + 1));
        if (!rest.empty())
        {
            if (rest[0] != L'+' && rest[0] != L'-')
                return false;
            int sign = (rest[0] == L'-') ? -1 : 1;
            int val = 0;
            bool ok = rest.size() > 1;
            for (size_t i = 1; i < rest.size(); ++i)
            {
                if (rest[i] < L'0' || rest[i] > L'9')
                {
                    ok = false;
                    break;
                }
                val = val * 10 + (rest[i] - L'0');
            }
            if (!ok)
                return false;
            b = sign * val;
        }
        const int p = static_cast<int>(pos);
        if (a == 0)
            return p == b;
        const int diff = p - b;
        return diff % a == 0 && diff / a >= 0;            // k = (pos-b)/a 必须是非负整数
    }

    // 去掉 /* … */ 注释（CSS 注释可能正好夹在选择器与花括号之间）
    std::wstring StripCssComments(const std::wstring& css)
    {
        std::wstring out;
        size_t pos = 0;
        while (pos < css.size())
        {
            const size_t b = css.find(L"/*", pos);
            if (b == std::wstring::npos)
            {
                out += css.substr(pos);
                break;
            }
            out += css.substr(pos, b - pos);
            const size_t e = css.find(L"*/", b + 2);
            if (e == std::wstring::npos)
                break;
            pos = e + 2;
        }
        return out;
    }

    // `@media (max-width:600px){ ... }`：HTML 里"窄屏换一套尺寸"的标准写法，
    // XML 侧此前只能靠宿主重新生成整棵树。这里按根元素的宽高做一次静态判定
    // （也就够了——窗口尺寸一变，宿主本来就会重生成）。
    bool CssMediaApplies(const std::wstring& cond, int vw, int vh)
    {
        const std::wstring low = Lower(cond);
        size_t pos = 0;
        while (pos < low.size())
        {
            const size_t colon = low.find(L':', pos);
            const size_t close = low.find(L')', colon == std::wstring::npos ? pos : colon);
            if (colon == std::wstring::npos || close == std::wstring::npos)
                break;
            const std::wstring name = Trim(low.substr(pos, colon - pos));
            std::wstring val = Trim(low.substr(colon + 1, close - colon - 1));
            // 去掉结尾的 px/em 等长度单位，只留数字
            while (!val.empty() && !(val.back() >= L'0' && val.back() <= L'9'))
                val.pop_back();
            const int num = val.empty() ? 0 : _wtoi(val.c_str());
            if (name.find(L"max-width") != std::wstring::npos && vw > 0 && vw > num)
                return false;
            if (name.find(L"min-width") != std::wstring::npos && vw > 0 && vw < num)
                return false;
            if (name.find(L"max-height") != std::wstring::npos && vh > 0 && vh > num)
                return false;
            if (name.find(L"min-height") != std::wstring::npos && vh > 0 && vh < num)
                return false;
            pos = close + 1;
        }
        return true;
    }

    // `@keyframes` 的帧表：HTML 把关键帧写在块里，元素上只写 `animation:pulse 1s`。
    // XML 早先只能把 `Keyframes="0:#f00;50:#0f0;100:#f00"` 抄在每个元素上，
    // 同一套动画给多个控件用就得逐个复制 —— 这里把块翻成帧表、按名字引用。
    using CssKeyframeTable = std::vector<std::pair<std::wstring, std::wstring>>;

    void CssDeclPairs(const std::wstring& decls,
                      std::vector<std::pair<std::wstring, std::wstring>>& out);

    // 帧选择器 `from/0%/50%/to` → 百分比整数（-1 表示不认识）
    int CssFramePct(const std::wstring& sel)
    {
        const std::wstring s = Lower(Trim(sel));
        if (s == L"from")
            return 0;
        if (s == L"to")
            return 100;
        if (s.find(L'%') == std::wstring::npos)
            return -1;                                  // 只认带 % 的写法，避免把别的东西当帧
        std::wstring num;
        for (const wchar_t ch : s)
            if ((ch >= L'0' && ch <= L'9') || ch == L'.')
                num += ch;
        if (num.empty())
            return -1;
        double v = _wtof(num.c_str());
        if (v < 0.0)
            v = 0.0;
        if (v > 100.0)
            v = 100.0;
        return static_cast<int>(v + 0.5);
    }

    // `@keyframes pulse{ 0%{background:#f00} 50%{background:#0f0} 100%{background:#f00} }`
    // → `pulse` = `0:#f00;50:#0f0;100:#f00`（宿主 startCssKeyframes 的帧表格式）。
    // 只取颜色（background/background-color/color）：宿主插值的是底色。
    void ParseKeyframes(const std::wstring& name, const std::wstring& inner,
                        CssKeyframeTable& table)
    {
        const std::wstring low = Lower(Trim(name));
        if (low.empty())
            return;
        std::wstring spec;
        size_t pos = 0;
        while (pos < inner.size())
        {
            const size_t lb = inner.find(L'{', pos);
            if (lb == std::wstring::npos)
                break;
            const size_t rb = inner.find(L'}', lb + 1);
            if (rb == std::wstring::npos)
                break;                                  // 帧块没闭合，整段忽略
            const std::wstring frameSel = Trim(inner.substr(pos, lb - pos));
            const std::wstring body = inner.substr(lb + 1, rb - lb - 1);
            pos = rb + 1;
            if (frameSel.empty() || body.empty())
                continue;
            std::vector<std::pair<std::wstring, std::wstring>> decls;
            CssDeclPairs(body, decls);
            std::wstring color;
            for (const auto& kv : decls)
            {
                const std::wstring p = Lower(kv.first);
                if (p != L"background" && p != L"background-color" && p != L"backgroundcolor" &&
                    p != L"color")
                    continue;
                if (kv.second.find(L"gradient(") != std::wstring::npos)
                    continue;                           // 渐变不参与插值
                color = Trim(kv.second);
                if (!color.empty())
                    break;
            }
            if (color.empty())
                continue;
            // `0%, 100% { … }` 一帧挂多个百分比
            size_t fpos = 0;
            while (fpos <= frameSel.size())
            {
                const size_t comma = frameSel.find(L',', fpos);
                const std::wstring one = Trim(frameSel.substr(fpos, comma == std::wstring::npos
                                                            ? std::wstring::npos : comma - fpos));
                fpos = comma == std::wstring::npos ? frameSel.size() + 1 : comma + 1;
                const int pct = CssFramePct(one);
                if (pct < 0)
                    continue;
                if (!spec.empty())
                    spec += L';';
                wchar_t buf[16];
                swprintf_s(buf, L"%d", pct);
                spec += buf;
                spec += L':';
                spec += color;
            }
        }
        if (spec.empty())
            return;
        for (auto& kv : table)
            if (kv.first == low)
            {
                kv.second = spec;                       // 同名后定义的赢（CSS 同款）
                return;
            }
        table.push_back({ low, spec });
    }

    void ParseCssRules(const std::wstring& raw, std::vector<CssRule>& out,
                       int vw = 0, int vh = 0, CssKeyframeTable* kf = nullptr)
    {
        const std::wstring css = StripCssComments(raw);
        size_t pos = 0;
        while (pos < css.size())
        {
            const size_t lb = css.find(L'{', pos);
            if (lb == std::wstring::npos)
                break;
            const std::wstring sel = Trim(css.substr(pos, lb - pos));
            if (!sel.empty() && sel[0] == L'@')
            {
                // 按花括号深度找块尾，内部再按规则解析（@media 可嵌套 @media）
                int depth = 1;
                size_t scan = lb + 1;
                while (scan < css.size() && depth > 0)
                {
                    if (css[scan] == L'{')
                        ++depth;
                    else if (css[scan] == L'}')
                        --depth;
                    ++scan;
                }
                if (depth != 0)
                    break;                              // 没闭合，整段忽略
                const std::wstring inner = css.substr(lb + 1, scan - lb - 2);
                pos = scan;
                const std::wstring cond = Lower(sel);
                if (cond.find(L"@media") == 0)
                {
                    if (CssMediaApplies(cond, vw, vh))
                        ParseCssRules(inner, out, vw, vh, kf);
                }
                else if (kf != nullptr && cond.find(L"keyframes") != std::wstring::npos)
                {
                    // `@keyframes name{…}` / `@-webkit-keyframes name{…}`：块转名字→帧表
                    const size_t kpos = cond.find(L"keyframes");
                    ParseKeyframes(Trim(sel.substr(kpos + 10)), inner, *kf);
                }
                // 其余 at-rule（@font-face/@supports 等）仍跳过
                continue;
            }
            const size_t rb = css.find(L'}', lb + 1);
            if (rb == std::wstring::npos)
                break;                                  // 块没闭合，整段忽略
            const std::wstring body = Trim(css.substr(lb + 1, rb - lb - 1));
            pos = rb + 1;
            if (sel.empty() || body.empty())
                continue;
            std::wstring pseudo;
            std::wstring base = sel;
            const bool important = Lower(body).find(L"!important") != std::wstring::npos;
            if (CssSplitPseudoElement(base, pseudo))
            {
                // 伪元素的 selector 里可能挂伪类（`.btn:hover::after`），先剥掉状态后缀再登记
                std::wstring inner;
                if (CssSplitPseudo(base, inner) && !inner.empty())
                    base = inner;
                out.push_back(CssRule{ base, body, pseudo, important });
                continue;
            }
            if (CssSplitPseudo(base, pseudo))
            {
                if (base.empty())
                    continue;                            // 纯 `:hover{}` 没有主体，忽略
                out.push_back(CssRule{ base, body, pseudo, important });
                continue;
            }
            out.push_back(CssRule{ sel, body, std::wstring(), important });
        }
    }

    // CSS 权重（只算本文件支持的简单选择器）：id > class > tag。
    // HTML 里 `div{width:100}` 写在 `.card{width:160}` 后面，class 照样赢；
    // 早先按源码顺序覆盖，等于把 CSS 优先级规则丢了。
    bool CssOneMatches(const std::wstring& sel, const XmlElement& e,
                       const std::vector<const XmlElement*>& ancestors);

    int CssSpecOf(const std::wstring& sel)
    {
        if (sel.empty() || sel[0] == L'*')
            return 0;
        int ids = 0, cls = 0, tags = 0;
        std::wstring tag;
        size_t i = 0;
        while (i < sel.size() && sel[i] != L'.' && sel[i] != L'#' && sel[i] != L'[')
            tag += sel[i++];
        while (i < sel.size())
        {
            const wchar_t sigil = sel[i++];
            if (sigil == L'[')
            {
                const size_t close = sel.find(L']', i);    // 属性选择器按 class 级权重
                ++cls;
                if (close == std::wstring::npos)
                    break;
                i = close + 1;
                continue;
            }
            while (i < sel.size() && sel[i] != L'.' && sel[i] != L'#' && sel[i] != L'[')
                ++i;
            if (sigil == L'#')
                ++ids;
            else
                ++cls;
        }
        if (!tag.empty())
            ++tags;
        return ids * 100 + cls * 10 + tags;
    }

    // 元素的 id 与 class 词表（class 大小写不敏感，按空白/逗号切）
    std::wstring CssIdOf(const XmlElement& e)
    {
        for (const auto& kv : e.attrs)
            if (Lower(kv.first) == L"id" && !kv.second.empty())
                return Lower(Trim(kv.second));
        return Lower(e.name);
    }

    void CssClassesOf(const XmlElement& e, std::vector<std::wstring>& out)
    {
        out.clear();
        for (const auto& kv : e.attrs)
        {
            if (Lower(kv.first) != L"class")
                continue;
            const std::wstring text = Lower(Trim(kv.second));
            size_t start = 0;
            while (start <= text.size())
            {
                const size_t sp = text.find_first_of(L" \t,;", start);
                const std::wstring word = Trim(text.substr(start, sp == std::wstring::npos
                                                                   ? std::wstring::npos : sp - start));
                if (!word.empty())
                    out.push_back(word);
                if (sp == std::wstring::npos)
                    break;
                start = sp + 1;
            }
        }
    }

    // 属性选择器：[type] / [type="text"] / [class~="btn"] / [id^="in"] / [id$="1"] / [id*="x"] / [lang|="en"]
    // HTML 里 `input[type="text"]{...}` 是"同一标签的不同形态分别给样式"的标准写法，
    // XML 侧只能给每个元素加 class 再写规则，明显更啰嗦。
    bool CssAttrMatches(const std::wstring& expr, const XmlElement& e)
    {
        const std::wstring body = Trim(expr);
        if (body.empty())
            return false;
        wchar_t op = 0;
        size_t eq = std::wstring::npos;
        for (size_t i = 0; i < body.size(); ++i)
        {
            if (body[i] == L'=')
            {
                eq = i;
                break;
            }
            if ((body[i] == L'~' || body[i] == L'^' || body[i] == L'$' ||
                 body[i] == L'*' || body[i] == L'|') &&
                i + 1 < body.size() && body[i + 1] == L'=')
            {
                op = body[i];
                eq = i + 1;
                break;
            }
        }
        const size_t nameEnd = (eq == std::wstring::npos) ? body.size()
                                                          : (op != 0 ? eq - 1 : eq);
        const std::wstring name = Lower(Trim(body.substr(0, nameEnd)));
        if (name.empty())
            return false;
        bool found = false;
        std::wstring val;
        for (const auto& kv : e.attrs)
            if (Lower(kv.first) == name)
            {
                found = true;
                val = kv.second;
                break;
            }
        if (eq == std::wstring::npos)
            return found;                               // 只查属性存在
        if (!found)
            return false;
        std::wstring want = Trim(body.substr(eq + 1));
        if (want.size() >= 2 && (want.front() == L'"' || want.front() == L'\'') &&
            want.back() == want.front())
            want = want.substr(1, want.size() - 2);
        val = Trim(val);
        if (op == 0)
            return val == want || Lower(val) == Lower(want);
        if (op == L'~')
        {
            std::wstring text = L' ' + val + L' ';
            return text.find(L' ' + want + L' ') != std::wstring::npos;
        }
        if (op == L'^')
            return val.size() >= want.size() && val.compare(0, want.size(), want) == 0;
        if (op == L'$')
            return val.size() >= want.size() &&
                   val.compare(val.size() - want.size(), want.size(), want) == 0;
        if (op == L'*')
            return want.empty() || val.find(want) != std::wstring::npos;
        if (op == L'|')
            return val == want || (val.size() > want.size() &&
                                   val.compare(0, want.size(), want) == 0 && val[want.size()] == L'-');
        return false;
    }

    // 单个复合选择器（不含组合器）是否命中：.a / tag.a / #id / tag / * / [attr=...] / :not(...)
    bool CssCompoundMatches(const std::wstring& rawSel, const XmlElement& e)
    {
        if (rawSel.empty())
            return false;
        // 先处理 :not(简单选择器)：HTML 里 `.btn:not(.primary)` 是"除掉某一类"的标准写法，
        // XML 侧只能靠额外 class 手工绕开。:not 里只认复合选择器，命中即整条不命中。
        std::wstring sel = rawSel;
        size_t notPos = 0;
        while ((notPos = sel.find(L":not(")) != std::wstring::npos)
        {
            const size_t close = sel.find(L')', notPos + 5);
            if (close == std::wstring::npos)
                return false;                           // 括号没闭合，宁缺勿错
            const std::wstring inner = Trim(sel.substr(notPos + 5, close - notPos - 5));
            if (inner.empty() || CssCompoundMatches(inner, e))
                return false;
            sel.erase(notPos, close - notPos + 1);
        }
        if (sel.empty())
            return false;
        if (sel[0] == L'*' && sel.size() == 1)
            return true;
        std::wstring tag;
        std::vector<std::wstring> need;
        bool anyPart = false;
        size_t i = 0;
        while (i < sel.size() && sel[i] != L'.' && sel[i] != L'#' && sel[i] != L'[')
        {
            if (sel[i] != L'*')
                tag += sel[i];
            ++i;
        }
        bool hasId = false;
        while (i < sel.size())
        {
            const wchar_t sigil = sel[i++];
            if (sigil == L'[')
            {
                const size_t close = sel.find(L']', i);
                if (close == std::wstring::npos)
                    return false;                       // 未闭合：宁缺勿错
                if (!CssAttrMatches(sel.substr(i, close - i), e))
                    return false;
                anyPart = true;
                i = close + 1;
                continue;
            }
            std::wstring token;
            while (i < sel.size() && sel[i] != L'.' && sel[i] != L'#' && sel[i] != L'[')
                token += sel[i++];
            if (sigil == L'#')
            {
                if (token.empty() || CssIdOf(e) != token)
                    return false;
                hasId = true;
                anyPart = true;
                continue;
            }
            if (token.empty())
                return false;
            need.push_back(token);
            anyPart = true;
        }
        if (!tag.empty() && Lower(e.tag) != tag)
            return false;
        if (need.empty())
            return !tag.empty() || hasId || anyPart;
        std::vector<std::wstring> have;
        CssClassesOf(e, have);
        for (const std::wstring& want : need)
        {
            bool hit = false;
            for (const std::wstring& c : have)
                if (c == want)
                {
                    hit = true;
                    break;
                }
            if (!hit)
                return false;
        }
        return true;
    }

    // 一条规则（可能是逗号并列）在当前元素上的权重：取命中项里最大的那个
    int CssRuleSpec(const CssRule& rule, const XmlElement& e,
                    const std::vector<const XmlElement*>& ancestors)
    {
        const std::wstring sel = Lower(rule.selector);
        int best = -1;
        size_t start = 0;
        while (start <= sel.size())
        {
            const size_t comma = sel.find(L',', start);
            const std::wstring one = Trim(sel.substr(start, comma == std::wstring::npos
                                                       ? std::wstring::npos : comma - start));
            if (CssOneMatches(one, e, ancestors))
                best = (std::max)(best, CssSpecOf(one));
            if (comma == std::wstring::npos)
                break;
            start = comma + 1;
        }
        return best;                                    // -1 = 不命中
    }

    // 兄弟组合器的左邻匹配：`E + F`（紧邻前一个兄弟）与 `E ~ F`（前面任一兄弟）。
    // 兄弟关系在 DOM 里是"同一父的子序列"，只能从父的 children 里往前找。
    bool CssSiblingMatches(const std::wstring& sel, const XmlElement& parent,
                           const XmlElement& self, bool adjacent)
    {
        size_t idx = parent.children.size();
        for (size_t i = 0; i < parent.children.size(); ++i)
            if (&parent.children[i] == &self)
            {
                idx = i;
                break;
            }
        if (idx == parent.children.size())
            return false;                               // 不是这个父的孩子
        size_t n = adjacent ? (idx > 0 ? 1 : 0) : idx;
        for (size_t k = 1; k <= n; ++k)
        {
            if (CssCompoundMatches(sel, parent.children[idx - k]))
                return true;
            if (adjacent)
                break;
        }
        return false;
    }

    // 选择器是否命中：支持复合选择器 + 后代（空格）、子代（>）、相邻兄弟（+）、
    // 后继兄弟（~）。HTML 里 `.card .title{...}` / `label + input{...}` 是最常见的
    // 层级复用写法，早先整类直接判不命中，作者只能把规则逐个套到每个元素上。
    bool CssOneMatches(const std::wstring& sel, const XmlElement& e,
                       const std::vector<const XmlElement*>& ancestors)
    {
        if (sel.empty())
            return false;
        // 切成复合选择器序列 + 它们之间的组合器
        std::vector<std::wstring> parts;
        std::vector<wchar_t> combs;                     // combs[i] = parts[i-1] 与 parts[i] 之间的组合器
        std::wstring cur;
        wchar_t pending = 0;                            // 0 = 无，' ' 后代，'>' 子代，'+' 紧邻，'~' 后继
        for (size_t i = 0; i < sel.size(); ++i)
        {
            const wchar_t ch = sel[i];
            if (ch == L' ' || ch == L'\t' || ch == L'>' || ch == L'+' || ch == L'~')
            {
                if (!cur.empty())
                {
                    parts.push_back(cur);
                    cur.clear();
                    combs.push_back(parts.size() >= 2 ? pending : L' ');
                }
                if (ch != L' ' && ch != L'\t')
                    pending = ch;                       // 组合器优先于空白
                else if (pending == 0 || pending == L' ' || pending == L'\t')
                    pending = L' ';
                continue;
            }
            cur += ch;
        }
        if (!cur.empty())
        {
            parts.push_back(cur);
            combs.push_back(parts.size() >= 2 ? pending : L' ');
        }
        if (parts.empty())
            return false;
        if (!CssCompoundMatches(parts.back(), e))
            return false;
        int ai = static_cast<int>(ancestors.size()) - 1;   // 最近的祖先
        for (int pi = static_cast<int>(parts.size()) - 2; pi >= 0; --pi)
        {
            if (ai < 0)
                return false;                       // 祖先用完了还有复合选择器没匹配
            const wchar_t comb = combs[static_cast<size_t>(pi) + 1];
            if (comb == L'>')
            {
                if (!CssCompoundMatches(parts[static_cast<size_t>(pi)], *ancestors[static_cast<size_t>(ai)]))
                    return false;
                --ai;
                continue;
            }
            if (comb == L'+' || comb == L'~')
            {
                // 左邻兄弟与当前元素同父：父是最近的祖先；匹配到兄弟后，
                // 更左的复合选择器继续在这个父及以上找（ai 停在父这一层）
                const XmlElement& parent = *ancestors[static_cast<size_t>(ai)];
                if (!CssSiblingMatches(parts[static_cast<size_t>(pi)], parent, e, comb == L'+'))
                    return false;
                continue;
            }
            bool found = false;
            while (ai >= 0)
            {
                if (CssCompoundMatches(parts[static_cast<size_t>(pi)], *ancestors[static_cast<size_t>(ai)]))
                {
                    found = true;
                    --ai;
                    break;
                }
                --ai;
            }
            if (!found)
                return false;
        }
        return true;
    }

    bool CssRuleMatches(const CssRule& rule, const XmlElement& e,
                        const std::vector<const XmlElement*>& ancestors)
    {
        const std::wstring sel = Lower(rule.selector);
        size_t start = 0;
        while (start <= sel.size())
        {
            const size_t comma = sel.find(L',', start);
            const std::wstring one = Trim(sel.substr(start, comma == std::wstring::npos
                                                       ? std::wstring::npos : comma - start));
            if (CssOneMatches(one, e, ancestors))
                return true;
            if (comma == std::wstring::npos)
                break;
            start = comma + 1;
        }
        return false;
    }

    // 一棵树里所有 <style> 的文字汇成规则表，节点本身剔掉（HTML 里它不产生界面）
    void HarvestCssRules(XmlElement& e, std::vector<CssRule>& rules, int vw, int vh,
                         CssKeyframeTable& kf)
    {
        std::vector<XmlElement> kept;
        kept.reserve(e.children.size());
        for (auto& c : e.children)
        {
            if (Lower(c.tag) == L"style")
            {
                ParseCssRules(c.text, rules, vw, vh, &kf);
                continue;
            }
            kept.push_back(std::move(c));
        }
        e.children = std::move(kept);
        for (auto& c : e.children)
            HarvestCssRules(c, rules, vw, vh, kf);
    }

    // 根元素的宽高就是这里的"视口"，@media 的一次性判定拿它当基准
    void HarvestCssRules(XmlElement& e, std::vector<CssRule>& rules, CssKeyframeTable& kf)
    {
        const std::wstring w = AttrOf(e, L"width");
        const std::wstring h = AttrOf(e, L"height");
        HarvestCssRules(e, rules, w.empty() ? 0 : _wtoi(w.c_str()),
                        h.empty() ? 0 : _wtoi(h.c_str()), kf);
    }

    // 伪类规则里的声明 → 状态属性名。只认背景/前景两族：
    // 圆角、阴影等"状态切换会改几何"的声明一律不接，免得 hover 时盒子跳动。
    std::wstring CssStateAttr(const std::wstring& pseudo, const std::wstring& prop)
    {
        std::wstring p;
        for (const wchar_t ch : prop)
            if (ch != L' ' && ch != L'\t' && ch != L'-' && ch != L'_')
                p += ch;
        const std::wstring state = (pseudo == L"hover") ? L"Hover"
                                  : (pseudo == L"focus") ? L"Focus"
                                  : (pseudo == L"active") ? L"Active"
                                  : (pseudo == L"disabled") ? L"Disabled" : L"";
        if (state.empty())
            return std::wstring();
        if (p == L"background" || p == L"backgroundcolor" || p == L"bg" || p == L"fill")
            return state + L"Background";
        if (p == L"color" || p == L"foreground" || p == L"textcolor" || p == L"fg")
            return state + L"Color";
        // 边框颜色不改几何（只换线色），HTML 里 `:hover{border-color:#fff}` 很常见；
        // 宽度/线型会影响布局，仍然不接（免得 hover 时盒子跳动）。
        if (p == L"bordercolor" || p == L"bordercolour" || p == L"stroke")
            return state + L"BorderColor";
        // 不透明度同样不改几何
        if (p == L"opacity" || p == L"alpha")
            return state + L"Opacity";
        // transform 只影响绘制（缩放/旋转），状态切换不会挤动兄弟，可以进状态
        if (p == L"transform")
            return state + L"Transform";
        // 投影同样只影响绘制：`:hover{box-shadow:0 8px 20px rgba(0,0,0,.45)}` 是 HTML 里
        // "悬停抬起"的标准写法（常态浅投影、悬停加深），不参与布局
        if (p == L"boxshadow" || p == L"shadow")
            return state + L"Shadow";
        return std::wstring();
    }

    // 把 "a: b; c: d" 拆成一条条 (属性, 值)
    void CssDeclPairs(const std::wstring& decls, std::vector<std::pair<std::wstring, std::wstring>>& out)
    {
        size_t pos = 0;
        while (pos < decls.size())
        {
            const size_t semi = decls.find(L';', pos);
            const std::wstring one = Trim(decls.substr(pos, semi == std::wstring::npos
                                                             ? std::wstring::npos : semi - pos));
            pos = (semi == std::wstring::npos) ? decls.size() : semi + 1;
            const size_t colon = one.find(L':');
            if (colon == std::wstring::npos)
                continue;
            std::wstring v = Trim(one.substr(colon + 1));
            // 去掉 !important（数值语义上它就是"更优先"，这里恒为最后写入）
            const size_t bang = v.find(L"!important");
            if (bang != std::wstring::npos)
                v = Trim(v.substr(0, bang));
            if (!v.empty())
                out.emplace_back(Lower(Trim(one.substr(0, colon))), v);
        }
    }

    // 带 class 的元素把命中的声明拼到自己的 style 前面；行内 style 放最后（HTML 同优先级规则）
    // ancestors 是当前元素的祖先链（root 在前、父在最后），后代/子代组合器靠它匹配
    void ApplyCssRules(XmlElement& e, const std::vector<CssRule>& rules,
                       std::vector<const XmlElement*> ancestors = {},
                       const CssKeyframeTable* kf = nullptr)
    {
        {
            // 命中规则按 (权重, 源码顺序) 升序套用：权重高的后写，同权重按先后
            std::vector<std::pair<std::pair<int, size_t>, const CssRule*>> hits;
            for (size_t i = 0; i < rules.size(); ++i)
            {
                const CssRule& rule = rules[i];
                const int spec = CssRuleSpec(rule, e, ancestors);
                if (spec < 0)
                    continue;
                hits.push_back({ { spec, i }, &rule });
            }
            std::sort(hits.begin(), hits.end(),
                      [](const std::pair<std::pair<int, size_t>, const CssRule*>& a,
                         const std::pair<std::pair<int, size_t>, const CssRule*>& b)
                      { return a.first < b.first; });
            // 作者自己写的状态属性不能被 class 规则盖掉（CSS 里行内优先）
            std::vector<std::wstring> authorAttrs;
            for (const auto& kv : e.attrs)
                authorAttrs.push_back(Lower(kv.first));
            std::wstring merged;
            std::wstring mergedImp;
            for (const auto& hit : hits)
            {
                const CssRule& rule = *hit.second;
                if (!rule.pseudo.empty() && CssIsStructural(rule.pseudo))
                {
                    // 结构伪类（:first-child/:nth-child(…)）按"是否命中"决定整条规则，
                    // 命中后声明按普通声明合进 style
                    if (!CssStructuralMatches(rule.pseudo, e, ancestors))
                        continue;
                }
                else if (rule.pseudo == L"before" || rule.pseudo == L"after")
                {
                    // 只认纯文本 content：拼到元素的文字上（正文或 Text 属性）
                    const std::wstring ct = CssContentText(rule.decls, e);
                    if (!ct.empty())
                    {
                        const bool before = rule.pseudo == L"before";
                        if (e.has(L"Text"))
                        {
                            const std::wstring cur = AttrOf(e, L"Text");
                            Put(e, L"Text", (before ? ct + cur : cur + ct).c_str());
                        }
                        else
                        {
                            e.text = before ? ct + e.text : e.text + ct;
                        }
                    }
                    continue;
                }
                else if (CssIsAttrState(rule.pseudo))
                {
                    // 属性态命中时声明按普通声明合进 style（静态状态，不需要绘制侧跟踪）
                    if (!CssAttrStateMatches(rule.pseudo, e))
                        continue;
                    // 但外观类声明（底色/文字色/边框色/不透明度）拆去状态属性：
                    // 几何（width/height/padding）只能静态合，绘制的颜色可以在运行时
                    // EnableWindow 开关时跟着变 —— 与 HTML 的 :disabled 行为一致。
                    std::vector<std::pair<std::wstring, std::wstring>> disDecls;
                    CssDeclPairs(rule.decls, disDecls);
                    std::wstring restDecls;
                    for (const auto& kv : disDecls)
                    {
                        const std::wstring attr = CssStateAttr(rule.pseudo, kv.first);
                        if (attr.empty())
                        {
                            restDecls += kv.first + L':' + kv.second + L';';
                            continue;
                        }
                        const std::wstring low = Lower(attr);
                        bool own = false;
                        for (const std::wstring& a : authorAttrs)
                            if (a == low)
                            {
                                own = true;
                                break;
                            }
                        if (!own)
                            Put(e, attr.c_str(), kv.second);
                    }
                    if (restDecls.empty())
                        continue;
                    if (!merged.empty() && merged.back() != L';')
                        merged += L';';
                    merged += restDecls;
                    continue;
                }
                else if (!rule.pseudo.empty())
                {
                    std::vector<std::pair<std::wstring, std::wstring>> decls;
                    CssDeclPairs(rule.decls, decls);
                    for (const auto& kv : decls)
                    {
                        const std::wstring attr = CssStateAttr(rule.pseudo, kv.first);
                        if (attr.empty())
                            continue;
                        const std::wstring low = Lower(attr);
                        bool own = false;
                        for (const std::wstring& a : authorAttrs)
                            if (a == low)
                            {
                                own = true;
                                break;
                            }
                        if (own)
                            continue;
                        Put(e, attr.c_str(), kv.second);
                    }
                    continue;
                }
                if (!merged.empty() && merged.back() != L';')
                    merged += L';';
                if (rule.important)
                {
                    if (!mergedImp.empty() && mergedImp.back() != L';')
                        mergedImp += L';';
                    mergedImp += rule.decls;
                }
                else
                {
                    merged += rule.decls;
                }
            }
            if (!merged.empty() || !mergedImp.empty())
            {
                const std::wstring own = AttrOf(e, L"style");
                const bool ownImportant = Lower(own).find(L"!important") != std::wstring::npos;
                if (!mergedImp.empty())
                {
                    // !important 压过常规声明（含行内），只有行内自带 !important 才排最后
                    merged += L';';
                    if (!ownImportant && !own.empty())
                    {
                        merged += own;
                        merged += L';';
                    }
                    merged += mergedImp;
                    if (ownImportant && !own.empty())
                        merged += L';' + own;
                }
                else if (!own.empty())
                {
                    merged += L';' + own;
                }
                Put(e, L"style", merged);
            }
            // `animation:pulse 1s` 只写名字，帧表在 @keyframes 块里：回查后落成 Keyframes 属性
            // 只写了行内 style（没有命中任何规则）时 merged 为空，这里退回读行内 style。
            const std::wstring animSrc = merged.empty() ? AttrOf(e, L"style") : merged;
            if (kf != nullptr && !kf->empty() && animSrc.find(L"animation") != std::wstring::npos &&
                !e.has(L"Keyframes"))
            {
                std::vector<std::pair<std::wstring, std::wstring>> decls;
                CssDeclPairs(animSrc, decls);
                bool done = false;
                for (const auto& kv : decls)
                {
                    if (done)
                        break;
                    const std::wstring p = Lower(kv.first);
                    if (p != L"animation" && p != L"animation-name" && p != L"animationname")
                        continue;
                    std::wstring cur;
                    for (size_t i = 0; i <= kv.second.size() && !done; ++i)
                    {
                        const wchar_t ch = i < kv.second.size() ? kv.second[i] : L' ';
                        if (!iswspace(ch) && ch != L',')
                        {
                            cur += ch;
                            continue;
                        }
                        const std::wstring tok = cur;
                        cur.clear();
                        const std::wstring lowTok = Lower(tok);
                        if (lowTok.empty())
                            continue;
                        // 时间/缓动/次数/方向这些 token 不是动画名
                        bool skip = lowTok.size() >= 2 && lowTok.back() == L's' &&
                                    iswdigit(lowTok[lowTok.size() - 2]);
                        for (const wchar_t c2 : lowTok)
                            if ((c2 >= L'0' && c2 <= L'9') || c2 == L'.' || c2 == L'-')
                                skip = true;
                        if (lowTok.find(L"ease") != std::wstring::npos || lowTok == L"linear" ||
                            lowTok == L"infinite" || lowTok == L"normal" || lowTok == L"forwards" ||
                            lowTok == L"backwards" || lowTok == L"both" || lowTok == L"alternate" ||
                            lowTok == L"reverse" || lowTok == L"none" || lowTok == L"auto" ||
                            lowTok == L"running" || lowTok == L"paused" ||
                            lowTok.find(L'(') != std::wstring::npos)
                            skip = true;
                        if (skip)
                            continue;
                        for (const auto& fm : *kf)
                            if (fm.first == lowTok)
                            {
                                Put(e, L"Keyframes", fm.second.c_str());
                                done = true;
                                break;
                            }
                    }
                }
            }
        }
        // 子元素匹配时要把自己算进祖先链
        ancestors.push_back(&e);
        for (auto& c : e.children)
            ApplyCssRules(c, rules, ancestors, kf);
        ancestors.pop_back();
    }

    // HTML 里只描述文档结构/资源、本身不产生界面的标签。
    // 必须先认出来再丢掉：否则它们会以"未知控件 <col>/<meta>"的身份把加载流程打断。
    bool IsDroppedTag(const std::wstring& t)
    {
        static const wchar_t* k[] = {
            L"head", L"meta", L"link", L"base", L"style", L"script", L"noscript",
            L"template", L"slot", L"source", L"track", L"param", L"col", L"colgroup",
            L"datalist", L"frameset", L"frame", L"applet", nullptr
        };
        for (const wchar_t** p = k; *p != nullptr; ++p)
            if (t == *p)
                return true;
        return false;
    }

    bool IsBlockContainer(const std::wstring& t)
    {
        return t == L"div" || t == L"section" || t == L"article" || t == L"main" ||
               t == L"header" || t == L"footer" || t == L"nav" || t == L"aside" ||
               t == L"form" || t == L"html" || t == L"body" || t == L"figure" ||
               t == L"figcaption" || t == L"blockquote" || t == L"address" ||
               t == L"thead" || t == L"tbody" || t == L"tfoot" ||
               t == L"dl" || t == L"menu" || t == L"dialog" || t == L"center" ||
               t == L"marquee" || t == L"fieldset-body";
    }

    bool IsTextNode(const std::wstring& t)
    {
        return t == L"p" || t == L"span" || t == L"strong" || t == L"b" || t == L"em" ||
               t == L"i" || t == L"small" || t == L"mark" || t == L"label" ||
               t == L"output" || t == L"time" || t == L"dt" || t == L"dd" ||
               t == L"cite" || t == L"h1" || t == L"h2" || t == L"h3" ||
               t == L"h4" || t == L"h5" || t == L"h6" || t == L"summary" || t == L"legend" ||
               t == L"kbd" || t == L"q" || t == L"abbr" || t == L"u" || t == L"s" ||
               t == L"ins" || t == L"del" || t == L"bdi" || t == L"bdo" || t == L"wbr" ||
               t == L"sub" || t == L"sup" || t == L"samp" || t == L"var" || t == L"data" ||
               t == L"acronym" || t == L"big" || t == L"tt" || t == L"strike" ||
               t == L"font" || t == L"nobr" || t == L"title";
    }

    bool IsHtmlTag(const std::wstring& t)
    {
        if (IsBlockContainer(t) || IsTextNode(t) || IsDroppedTag(t))
            return true;
        static const wchar_t* k[] = {
            L"input", L"textarea", L"select", L"option", L"optgroup", L"ul", L"ol", L"li",
            L"table", L"tr", L"td", L"th", L"caption", L"img", L"hr", L"a", L"button",
            L"progress", L"meter", L"fieldset", L"details", L"code", L"pre",
            L"br", L"canvas", L"svg", L"video", L"audio", L"iframe", L"embed", L"object",
            L"picture", nullptr
        };
        for (const wchar_t** p = k; *p != nullptr; ++p)
            if (t == *p)
                return true;
        return false;
    }

    // HTML 专有属性翻译完后要删掉，免得落到属性面板报"未知属性"。
    // 两类：已翻译过的原名，以及"认得但本系统没有对应"的（required/pattern/step…）。
    void DropHtmlOnly(XmlElement& e)
    {
        static const wchar_t* k[] = {
            L"id", L"class", L"type", L"value", L"checked", L"selected", L"multiple",
            L"disabled", L"readonly", L"maxlength", L"min", L"max", L"rows", L"cols",
            L"href", L"src", L"alt", L"colspan", L"rowspan", L"onclick", L"onblur",
            L"placeholder", L"for", L"style", L"hidden", L"aria-label",
            L"required", L"pattern", L"step", L"autocomplete", L"accept", L"size",
            L"minlength", L"form", L"formaction", L"enctype", L"method", L"action",
            L"target", L"rel", L"download", L"lang", L"dir", L"tabindex", L"accesskey",
            L"draggable", L"role", L"spellcheck", L"novalidate", L"autofocus", L"wrap",
            L"dirname", L"list", L"srcset", L"sizes", L"media", L"controls", L"loop",
            L"muted", L"poster", L"preload", L"open", L"reversed", L"start", L"scope",
            L"headers", L"abbr", L"datetime", L"high", L"low", L"optimum",
            L"allowfullscreen", L"frameborder", L"sandbox", L"name", nullptr
        };
        for (const wchar_t** p = k; *p != nullptr; ++p)
            Drop(e, *p);
        // data-* / aria-* 是"给脚本/无障碍看的"，前缀匹配整族删掉
        for (size_t i = 0; i < e.attrs.size(); )
        {
            const std::wstring& key = e.attrs[i].first;
            if (_wcsnicmp(key.c_str(), L"data-", 5) == 0 ||
                _wcsnicmp(key.c_str(), L"aria-", 5) == 0)
                e.attrs.erase(e.attrs.begin() + (std::ptrdiff_t)i);
            else
                ++i;
        }
    }

    // HTML 行内文本标签：宽按内容，不占满父容器（<XLabel> 保留 WPF TextBlock 的铺满语义）
    bool IsInlineTextTag(const std::wstring& t)
    {
        return t == L"label" || t == L"span" || t == L"strong" || t == L"b" ||
               t == L"em" || t == L"i" || t == L"small" || t == L"mark" ||
               t == L"a" || t == L"kbd" || t == L"q" || t == L"abbr" ||
               t == L"u" || t == L"s" || t == L"cite" || t == L"output" ||
               t == L"time" || t == L"code" || t == L"var" || t == L"samp" ||
               t == L"sub" || t == L"sup" || t == L"data" || t == L"legend";
    }

    // 文本自然宽估算：CJK 记 1 个字号，其余记 0.55 个字号。
    // 只是给"没写 Width 的行内标签"一个合理宽度，不追求像素级精确
    // （精确测量要 DC + 字体实例，代价远大于收益）。
    int ApproxTextWidth(const XmlElement& e, const std::wstring& text)
    {
        if (text.empty())
            return 0;
        int pt = 14;
        {
            const std::wstring fs = e.get(L"FontSize");
            if (!fs.empty())
            {
                const int v = (int)wcstol(fs.c_str(), nullptr, 10);
                if (v > 0)
                    pt = v;
            }
        }
        double px = 0.0;
        for (wchar_t c : text)
            px += (c < 0x2E80) ? pt * 0.55 : (double)pt;
        const int pad = e.padL + e.padR;
        return (int)(px + 0.5) + pad + 4;   // +4 留一点字间呼吸
    }

    // style="width:200px; height:30px; padding:8; gap:6" —— HTML 靠 CSS 表达几何，
    // 这里只搬"尺寸/内边距/间距"四件事：宽高、padding、gap。其余 CSS 一律忽略，
    // 我们不做样式引擎，但要能读懂作者最常用的那几行。
    void ApplyInlineStyle(XmlElement& e)
    {
        std::wstring style;
        if (!TakeAttr(e, L"style", style))
            return;
        // position:sticky 需要"先知道自己是 sticky"：CSS 允许 `top` 写在 `position` 之前，
        // 逐条解析时 top 那条会先撞上绝对定位分支。这里先扫一遍 style，把语义位提上来。
        {
            size_t p0 = 0;
            while (p0 < style.size())
            {
                const size_t semi0 = style.find(L';', p0);
                const std::wstring decl0 = Trim(style.substr(
                    p0, semi0 == std::wstring::npos ? std::wstring::npos : semi0 - p0));
                p0 = (semi0 == std::wstring::npos) ? style.size() : semi0 + 1;
                const size_t colon0 = decl0.find(L':');
                if (colon0 == std::wstring::npos)
                    continue;
                if (Lower(Trim(decl0.substr(0, colon0))) == L"position" &&
                    Lower(Trim(decl0.substr(colon0 + 1))) == L"sticky")
                {
                    e.sticky = true;
                    Put(e, L"Sticky", L"1");
                    break;
                }
            }
        }
        size_t start = 0;
        while (start < style.size())
        {
            const size_t semi = style.find(L';', start);
            const std::wstring decl = Trim(style.substr(
                start, semi == std::wstring::npos ? std::wstring::npos : semi - start));
            start = (semi == std::wstring::npos) ? style.size() : semi + 1;
            if (decl.empty())
                continue;
            const size_t colon = decl.find(L':');
            if (colon == std::wstring::npos)
                continue;
            const std::wstring key = CssKeyNormalize(Trim(decl.substr(0, colon)));
            const std::wstring val = Trim(decl.substr(colon + 1));
            const int px = (int)wcstol(val.c_str(), nullptr, 10);
            // CSS 百分比：width:100% / height:100% 就是占满容器（HTML 块级元素的默认行为）。
            // 旧实现直接 wcstol 会把 "100%" 读成 100px，这里必须先判 % 再判像素。
            const bool pct = val.find(L'%') != std::wstring::npos;
            if (pct && (key == L"width" || key == L"height"))
            {
                // 任意百分比都要留住（flex 容器排子项时按容器尺寸换算），100% 另外保留 fill 老语义
                if (key == L"width")
                {
                    e.pctW = static_cast<float>(_wtof(val.c_str()));
                    if (px >= 100)
                    {
                        e.fillW = true;
                        Put(e, L"Layout.fillWidth", L"1");
                    }
                }
                else
                {
                    e.pctH = static_cast<float>(_wtof(val.c_str()));
                    if (px >= 100)
                    {
                        e.fillH = true;
                        Put(e, L"Layout.fillHeight", L"1");
                    }
                }
            }
            else if (key == L"flex")
            {
                // CSS flex 简写：flex:1 → grow 1 / shrink 1 / basis 0；flex:2 0 auto → 三个分量
                std::vector<std::wstring> fv;
                {
                    std::wstring s = val;
                    for (size_t i = 0; i < s.size();)
                    {
                        while (i < s.size() && iswspace(s[i]))
                            ++i;
                        size_t j = i;
                        while (j < s.size() && !iswspace(s[j]))
                            ++j;
                        if (j > i)
                            fv.push_back(s.substr(i, j - i));
                        i = j;
                    }
                }
                if (!fv.empty())
                {
                    e.flexGrow = static_cast<float>(_wtof(fv[0].c_str()));
                    Put(e, L"Grow", fv[0]);
                    e.flexShrink = fv.size() > 1 ? static_cast<float>(_wtof(fv[1].c_str())) : 1.0f;
                    const std::wstring b = fv.size() > 2 ? fv[2] : L"0";
                    if (_wcsicmp(b.c_str(), L"auto") == 0)
                        e.flexBasis = -1;
                    else if (b.find(L'%') != std::wstring::npos)
                    {
                        // CSS 的 flex-basis 可以写百分比：按容器主轴尺寸换算，
                        // 早先 wcstol("50%") = 50 → 元素只有 50px 宽
                        e.flexBasisPct = static_cast<float>(_wtof(b.substr(0, b.find(L'%')).c_str()));
                        e.flexBasis = -1;
                    }
                    else
                        e.flexBasis = (int)wcstol(b.c_str(), nullptr, 10);
                    Put(e, L"flex-shrink", std::to_wstring((int)e.flexShrink));
                    Put(e, L"flex-basis", e.flexBasisPct >= 0.0f
                                              ? std::to_wstring((int)e.flexBasisPct) + L"%"
                                              : (e.flexBasis >= 0 ? std::to_wstring(e.flexBasis)
                                                                  : std::wstring(L"auto")));
                }
            }
            else if (key == L"flex-grow" || key == L"flexgrow")
            {
                e.flexGrow = static_cast<float>(_wtof(val.c_str()));
                Put(e, L"Grow", val);
            }
            else if (key == L"flex-shrink" || key == L"flexshrink" || key == L"shrink")
            {
                e.flexShrink = static_cast<float>(_wtof(val.c_str()));
                Put(e, L"flex-shrink", val);
            }
            else if (key == L"flex-basis" || key == L"flexbasis" || key == L"basis")
            {
                // auto / 百分比 / 像素三种写法：百分比要留给容器按主轴换算
                if (_wcsicmp(val.c_str(), L"auto") == 0)
                {
                    e.flexBasis = -1;
                    e.flexBasisPct = -1.0f;
                }
                else if (val.find(L'%') != std::wstring::npos)
                {
                    e.flexBasisPct = static_cast<float>(_wtof(val.substr(0, val.find(L'%')).c_str()));
                    e.flexBasis = -1;
                }
                else
                {
                    e.flexBasis = (int)wcstol(val.c_str(), nullptr, 10);
                    e.flexBasisPct = -1.0f;
                }
                Put(e, L"flex-basis", val);
            }
            else if (key == L"display")
            {
                // CSS display：flex / inline-flex → 这个元素就是 flex 容器，
                // 且主轴默认是 row（CSS 的 flex-direction 默认值）。此前 style 里的
                // display 完全被忽略，<div style="display:flex"> 只能靠 class="row" 私有约定横排。
                const std::wstring lowv = Lower(val);
                if (lowv.find(L"flex") != std::wstring::npos)
                {
                    e.flex = true;
                    Put(e, L"Display", L"flex");
                    e.flexDir = 0;                 // 0 = row（CSS 默认）
                    e.horizontal = true;
                    e.hasOrientation = true;
                    Put(e, L"Flex", L"row");
                }
                else
                {
                    e.flex = false;
                    Put(e, L"Display", lowv);
                    // CSS `display:none` —— HTML 里最常用的隐藏写法（比 hidden 属性更常见）。
                    // 语义与 hidden 一致：控件仍建出来（脚本可用），只是不可见、不占位。
                    if (lowv == L"none")
                        Put(e, L"Visible", L"False");
                }
            }
            else if (key == L"flex-direction" || key == L"flexdirection")
            {
                // 只认 flex-direction；CSS 的 direction 是文字方向，不能混进来
                const std::wstring lowv = Lower(val);
                const bool rev = lowv.find(L"reverse") != std::wstring::npos;
                if (lowv.find(L"column") != std::wstring::npos)
                {
                    e.flexDir = rev ? 3 : 2;
                    e.horizontal = false;
                }
                else
                {
                    e.flexDir = rev ? 1 : 0;
                    e.horizontal = true;
                }
                e.hasOrientation = true;
                Put(e, L"Flex", val);
            }
            else if ((key == L"width" || key == L"height") && px >= 0 && Lower(val) != L"auto")
            {
                // CSS 里 0 是合法尺寸（元素塌陷），旧实现 px > 0 会把 width:0 整条吞掉，
                // 于是"设成 0 收起来"这种写法在 XML 通道里根本表达不了。auto 则留给布局自己算。
                if (key == L"width")
                {
                    e.w = px;
                    e.hasW = true;
                    Put(e, L"Width", std::to_wstring(px));
                }
                else
                {
                    e.h = px;
                    e.hasH = true;
                    Put(e, L"Height", std::to_wstring(px));
                }
            }
            else if (key == L"padding" && px >= 0)
            {
                // CSS padding 简写与 margin 同规则：1 值=四边，2 值=上下/左右，
                // 3 值=上/左右/下，4 值=上右下左。早先只吃单值，
                // style="padding:4 16" 时右侧 16 会被整段忽略，卡片内容贴边。
                std::vector<int> pv;
                {
                    std::wstring cur;
                    for (wchar_t ch : val)
                    {
                        if (ch == L' ' || ch == L'\t')
                        {
                            if (!cur.empty())
                            {
                                pv.push_back((int)wcstol(cur.c_str(), nullptr, 10));
                                cur.clear();
                            }
                        }
                        else
                            cur.push_back(ch);
                    }
                    if (!cur.empty())
                        pv.push_back((int)wcstol(cur.c_str(), nullptr, 10));
                }
                int pT = px, pR = px, pB = px, pL = px;
                if (pv.size() == 2)
                {
                    pT = pB = pv[0];
                    pL = pR = pv[1];
                }
                else if (pv.size() == 3)
                {
                    pT = pv[0];
                    pL = pR = pv[1];
                    pB = pv[2];
                }
                else if (pv.size() >= 4)
                {
                    pT = pv[0];
                    pR = pv[1];
                    pB = pv[2];
                    pL = pv[3];
                }
                // Put 回属性必须与 ParsePadding 读回顺序一致：「左 上 右 下」（L T R B）。
                // 早先这里写「上 右 下 左」，style 写进节点后再从属性重解析时上下/左右整组翻转。
                Put(e, L"Padding", std::to_wstring(pL) + L" " + std::to_wstring(pT) + L" " +
                                     std::to_wstring(pR) + L" " + std::to_wstring(pB));
                e.padL = pL; e.padT = pT; e.padR = pR; e.padB = pB;
            }
            else if (key == L"padding-left" || key == L"padding-top" ||
                     key == L"padding-right" || key == L"padding-bottom")
            {
                // 单边写法（HTML 里常用来只压一条边，避免整块 padding 撑开）
                if (key == L"padding-left") e.padL = px;
                else if (key == L"padding-top") e.padT = px;
                else if (key == L"padding-right") e.padR = px;
                else e.padB = px;
                Put(e, L"Padding", std::to_wstring(e.padL) + L" " + std::to_wstring(e.padT) + L" " +
                                     std::to_wstring(e.padR) + L" " + std::to_wstring(e.padB));
            }
            // inset 简写已在下方专用分支解析（见 key == "inset"）：顺序同 margin 上 右
            // 下 左，left+right 撑宽、top+bottom 撑高均已可用。实测 220×120 容器内
            // `inset:0` 子项 220×120 铺满；`inset:10 20 5 40` 得 160×105、左偏 40、
            // 上偏 10（220-40-20 / 120-10-5）。单边 top/left/right/bottom 的写法走
            // 后面的通用别名分支。
            else if (key == L"gap")
            {
                // CSS `gap:<row-gap> <column-gap>`：一个值行列相同，两个值分别是行距/列距。
                // 早先只吃单值 —— 换行（flex-wrap）后行距被悄悄当成列距。
                std::vector<int> gv;
                {
                    std::wstring cur;
                    for (wchar_t ch : val)
                    {
                        if (ch == L' ' || ch == L'\t' || ch == L',')
                        {
                            if (!cur.empty())
                            {
                                gv.push_back((int)wcstol(cur.c_str(), nullptr, 10));
                                cur.clear();
                            }
                        }
                        else
                            cur.push_back(ch);
                    }
                    if (!cur.empty())
                        gv.push_back((int)wcstol(cur.c_str(), nullptr, 10));
                }
                int gRow = px, gCol = px;
                if (gv.size() >= 2)
                {
                    gRow = gv[0];
                    gCol = gv[1];
                }
                else if (gv.size() == 1)
                    gRow = gCol = gv[0];
                if (gRow < 0) gRow = 0;
                if (gCol < 0) gCol = 0;
                // gap 同时写 Spacing（流式容器）和 Gap（flex 容器）：作者写一次就够
                e.spacing = gRow;
                e.flexGapX = gCol;
                e.flexGapY = gRow;
                Put(e, L"Spacing", std::to_wstring(gRow));
                // Gap 一并写成 "row column"：控件层容器（XmlFlexPanel）认两值写法，
                // 读单值的旧消费方（wcstol）自然只取到行距，不会被多出的列距带偏。
                Put(e, L"Gap", std::to_wstring(gRow) + L" " + std::to_wstring(gCol));
                Put(e, L"RowGap", std::to_wstring(gRow));
                Put(e, L"ColumnGap", std::to_wstring(gCol));
            }
            // CSS Grid 的轨道声明：`grid-template-columns: 32px 1fr 1fr` / `repeat(3, 1fr)`。
            // 早先只有 XAML 的 <Grid.ColumnDefinitions> 能写轨道，HTML 写法整条静默失效。
            else if (key == L"grid-template-columns" || key == L"gridtemplatecolumns" ||
                     key == L"grid-template-rows" || key == L"gridtemplaterows")
            {
                const bool rowAxis = key.find(L"row") != std::wstring::npos;
                auto conv = [](std::wstring w) {
                    std::transform(w.begin(), w.end(), w.begin(), towlower);
                    if (w == L"auto") return w;
                    if (w.size() > 2 && w.compare(w.size() - 2, 2, L"fr") == 0)
                    {
                        const std::wstring n = w.substr(0, w.size() - 2);
                        return n == L"1" || n.empty() ? std::wstring(L"*") : n + L"*";
                    }
                    if (!w.empty() && w.back() == L'%')
                        return std::wstring(L"auto");   // 百分比轨道按内容宽，避免算成 0
                    return w;                               // "32px" / "48" 里的数字由 _wtoi 取
                };
                std::vector<std::pair<std::wstring, std::wstring>> tracks;
                std::wstring cur;
                auto flush = [&](const std::wstring& tok) {
                    if (tok.empty())
                        return;
                    const std::wstring t = Lower(tok);
                    if (t.size() > 7 && t.compare(0, 7, L"repeat(") == 0 && t.back() == L')')
                    {
                        const std::wstring inner = t.substr(7, t.size() - 8);
                        const size_t comma = inner.find(L',');
                        const int n = (std::max)(1, (std::min)(64, _wtoi(inner.substr(0, comma).c_str())));
                        const std::wstring body = conv(Trim(inner.substr(comma + 1)));
                        for (int i = 0; i < n; ++i)
                            tracks.emplace_back(rowAxis ? L"Height" : L"Width", body);
                        return;
                    }
                    tracks.emplace_back(rowAxis ? L"Height" : L"Width", conv(tok));
                };
                for (wchar_t ch : val)
                {
                    if (ch == L' ' || ch == L'\t')
                    {
                        flush(cur);
                        cur.clear();
                    }
                    else
                        cur.push_back(ch);
                }
                flush(cur);
                if (!tracks.empty())
                {
                    if (rowAxis)
                        e.gridRows = tracks;
                    else
                        e.gridCols = tracks;
                }
            }
            // CSS Grid 的落格：`grid-column: 2 / span 2`、`grid-row: 1 / 3`、`grid-column: 3`。
            // 之前只能写 XAML 的 Grid.Column + Grid.ColumnSpan，HTML 写法不生效。
            else if (key == L"grid-column" || key == L"grid-row" ||
                     key == L"grid-column-start" || key == L"grid-row-start" ||
                     key == L"grid-column-end" || key == L"grid-row-end")
            {
                const bool rowAxis = key.find(L"row") != std::wstring::npos;
                auto num = [](const std::wstring& s) {
                    const int v = (int)wcstol(s.c_str(), nullptr, 10);
                    return v > 0 ? v : 0;
                };
                const int curStart = num(e.get(rowAxis ? L"Grid.Row" : L"Grid.Column"));
                const bool isEnd = key.find(L"end") != std::wstring::npos;
                const bool isStart = key.find(L"start") != std::wstring::npos;
                if (!isEnd && !isStart)
                {
                    // 简写：start [ / end ]，end 可以是 "span n" 或终止线号
                    const size_t slash = val.find(L'/');
                    const std::wstring a = Trim(slash == std::wstring::npos ? val : val.substr(0, slash));
                    const std::wstring b = slash == std::wstring::npos ? std::wstring() : Trim(val.substr(slash + 1));
                    int start = num(a);
                    int span = 1;
                    if (!b.empty())
                    {
                        const std::wstring lb = Lower(b);
                        if (lb.compare(0, 5, L"span ") == 0)
                            span = (std::max)(1, num(Trim(b.substr(5))));
                        else if (num(b) > 0 && start > 0)
                            span = (std::max)(1, num(b) - start);
                    }
                    if (start > 0)
                    {
                        e.set(rowAxis ? L"Grid.Row" : L"Grid.Column", std::to_wstring(start - 1));
                        e.set(rowAxis ? L"Grid.RowSpan" : L"Grid.ColumnSpan", std::to_wstring(span));
                        if (rowAxis)
                        {
                            e.row = start - 1;
                            e.rowSpan = span;
                            e.hasGridRow = true;
                        }
                        else
                        {
                            e.col = start - 1;
                            e.colSpan = span;
                            e.hasGridCol = true;
                        }
                    }
                }
                else if (isStart)
                {
                    const int start = num(val);
                    if (start > 0)
                    {
                        e.set(rowAxis ? L"Grid.Row" : L"Grid.Column", std::to_wstring(start - 1));
                        if (rowAxis)
                        {
                            e.row = start - 1;
                            e.hasGridRow = true;
                        }
                        else
                        {
                            e.col = start - 1;
                            e.hasGridCol = true;
                        }
                    }
                }
                else
                {
                    const int end = num(val);
                    if (end > 0)
                    {
                        const int startLine = curStart + 1;
                        const int span = (std::max)(1, end - startLine);
                        e.set(rowAxis ? L"Grid.RowSpan" : L"Grid.ColumnSpan", std::to_wstring(span));
                        if (rowAxis)
                            e.rowSpan = span;
                        else
                            e.colSpan = span;
                    }
                }
            }
            // 单边 margin（HTML 里最常见的 `margin-left:auto` / `margin-top:8`）：
            // 早先只认 `margin` 简写，单边写法整条被丢掉。auto 记为标记，数值按像素。
            else if (key == L"margin-left" || key == L"margin-top" ||
                     key == L"margin-right" || key == L"margin-bottom")
            {
                const bool isAuto = _wcsicmp(val.c_str(), L"auto") == 0;
                const int px2 = isAuto ? 0 : (int)wcstol(val.c_str(), nullptr, 10);
                if (key == L"margin-left")
                {
                    e.marginL = px2; e.marginLAuto = isAuto;
                    Put(e, L"MarginLeft", isAuto ? std::wstring(L"auto") : std::to_wstring(px2));
                }
                else if (key == L"margin-top")
                {
                    e.marginT = px2; e.marginTAuto = isAuto;
                    Put(e, L"MarginTop", isAuto ? std::wstring(L"auto") : std::to_wstring(px2));
                }
                else if (key == L"margin-right")
                {
                    e.marginR = px2; e.marginRAuto = isAuto;
                    Put(e, L"MarginRight", isAuto ? std::wstring(L"auto") : std::to_wstring(px2));
                }
                else
                {
                    e.marginB = px2; e.marginBAuto = isAuto;
                    Put(e, L"MarginBottom", isAuto ? std::wstring(L"auto") : std::to_wstring(px2));
                }
            }
            else if (key == L"margin")
            {
                // CSS margin 简写：1 值=四边，2 值=上下/左右，3 值=上/左右/下，4 值=上右下左。
                // auto 单独记标记（数值按 0），供 flex 容器按 CSS 规则吃掉剩余空间。
                std::vector<int> mv;
                std::vector<bool> mvAuto;
                std::wstring cur;
                auto flushTok = [&]() {
                    if (cur.empty())
                        return;
                    if (_wcsicmp(cur.c_str(), L"auto") == 0)
                    {
                        mv.push_back(0);
                        mvAuto.push_back(true);
                    }
                    else
                    {
                        mv.push_back((int)wcstol(cur.c_str(), nullptr, 10));
                        mvAuto.push_back(false);
                    }
                    cur.clear();
                };
                for (wchar_t ch : val)
                {
                    if (ch == L' ' || ch == L'\t')
                        flushTok();
                    else
                        cur.push_back(ch);
                }
                flushTok();
                int mT = 0, mR = 0, mB = 0, mL = 0;
                bool aT = false, aR = false, aB = false, aL = false;
                if (mv.size() == 1)
                {
                    mT = mR = mB = mL = mv[0];
                    aT = aR = aB = aL = mvAuto[0];
                }
                else if (mv.size() == 2)
                {
                    mT = mB = mv[0];
                    mL = mR = mv[1];
                    aT = aB = mvAuto[0];
                    aL = aR = mvAuto[1];
                }
                else if (mv.size() == 3)
                {
                    mT = mv[0];
                    mL = mR = mv[1];
                    mB = mv[2];
                    aT = mvAuto[0];
                    aL = aR = mvAuto[1];
                    aB = mvAuto[2];
                }
                else if (mv.size() >= 4)
                {
                    mT = mv[0];
                    mR = mv[1];
                    mB = mv[2];
                    mL = mv[3];
                    aT = mvAuto[0];
                    aR = mvAuto[1];
                    aB = mvAuto[2];
                    aL = mvAuto[3];
                }
                if (!mv.empty())
                {
                    e.marginT = mT; e.marginR = mR; e.marginB = mB; e.marginL = mL;
                    e.marginTAuto = aT; e.marginRAuto = aR; e.marginBAuto = aB; e.marginLAuto = aL;
                    Put(e, L"MarginLeft", aL ? std::wstring(L"auto") : std::to_wstring(mL));
                    Put(e, L"MarginTop", aT ? std::wstring(L"auto") : std::to_wstring(mT));
                    Put(e, L"MarginRight", aR ? std::wstring(L"auto") : std::to_wstring(mR));
                    Put(e, L"MarginBottom", aB ? std::wstring(L"auto") : std::to_wstring(mB));
                    // Margin 属性的既有解析顺序是 左 上 右 下，跟 CSS 相反，转一次再写
                    Put(e, L"Margin", std::to_wstring(mL) + L" " + std::to_wstring(mT) + L" " +
                                      std::to_wstring(mR) + L" " + std::to_wstring(mB));
                }
            }
            // 其余 CSS 键只做「别名 → 既有属性」的翻译，不引入新控件实现
            else if (!val.empty())
            {
                // flex 对齐与间距：CSS 写法 → 容器既有属性（XmlFlexPanel 已实现，纯映射）
                if (key == L"justify-content" || key == L"justifycontent")
                    Put(e, L"Justify", val);
                else if (key == L"align-items" || key == L"alignitems")
                    Put(e, L"Align", val);
                else if (key == L"align-content" || key == L"aligncontent")
                    Put(e, L"AlignContent", val);
                else if (key == L"align-self" || key == L"alignself")
                {
                    // 同时写 AlignSelf（规范名，读取时优先）与 Self（旧名兼容）
                    Put(e, L"AlignSelf", val);
                    Put(e, L"Self", val);
                }
                else if (key == L"flex-wrap" || key == L"flexwrap")
                    Put(e, L"Wrap", val);
                // CSS `flex-flow` 简写 = flex-direction + flex-wrap 一句话，
                // HTML 里 `flex-flow: row wrap` / `flex-flow: column nowrap` 很常见；
                // 拆成方向块与换行块，各自落到已有语义上（纯映射，不加新实现）。
                else if (key == L"flex-flow" || key == L"flexflow")
                {
                    std::vector<std::wstring> toks;
                    {
                        std::wstring cur;
                        for (wchar_t c : val)
                        {
                            if (iswspace(c))
                            {
                                if (!cur.empty()) { toks.push_back(cur); cur.clear(); }
                            }
                            else cur.push_back(c);
                        }
                        if (!cur.empty()) toks.push_back(cur);
                    }
                    for (const auto& t : toks)
                    {
                        const std::wstring lt = Lower(t);
                        if (lt.find(L"row") != std::wstring::npos ||
                            lt.find(L"column") != std::wstring::npos)
                        {
                            const bool rev = lt.find(L"reverse") != std::wstring::npos;
                            if (lt.find(L"column") != std::wstring::npos)
                            {
                                e.flexDir = rev ? 3 : 2;
                                e.horizontal = false;
                            }
                            else
                            {
                                e.flexDir = rev ? 1 : 0;
                                e.horizontal = true;
                            }
                            e.hasOrientation = true;
                            Put(e, L"Flex", lt);
                        }
                        else if (lt.find(L"wrap") != std::wstring::npos ||
                                 lt == L"nowrap")
                        {
                            Put(e, L"Wrap", lt);
                            e.flexWrap = lt != L"nowrap";
                        }
                    }
                }
                else if (key == L"row-gap" || key == L"rowgap")
                    Put(e, L"RowGap", val);
                else if (key == L"column-gap" || key == L"columngap")
                    Put(e, L"ColumnGap", val);
                else if (key == L"transform")
            {
                // CSS transform 整串下沉到外观层（translate/rotate/scale/skew/matrix 列表），
                // 由绘制端按元素盒子解析成 4×4 矩阵。这里不做语法校验：
                // 语法不认识时绘制端整条放弃，控件保持原样，比"半懂不懂地变形"安全。
                Put(e, L"Transform", val);
            }
            else if (key == L"border-top-left-radius" || key == L"border-top-right-radius" ||
                     key == L"border-bottom-right-radius" || key == L"border-bottom-left-radius")
            {
                // CSS 分角圆角（HTML 里"只圆左上角"的卡片/气泡很常见），落成同名 PascalCase 属性
                std::wstring prop = L"Border";
                if (key == L"border-top-left-radius") prop += L"TopLeft";
                else if (key == L"border-top-right-radius") prop += L"TopRight";
                else if (key == L"border-bottom-right-radius") prop += L"BottomRight";
                else prop += L"BottomLeft";
                prop += L"Radius";
                Put(e, prop.c_str(), val);
            }
            else if (key == L"border-radius" || key == L"radius")
                    Put(e, L"Radius", val);
                // CSS box-shadow：HTML 里 `box-shadow:0 4 10 rgba(0,0,0,.55)` 是最常见的
                // 卡片/浮层写法。XML 通道此前的状态是"完全不认"——style 写了也读不回来，
                // 绘制端更拿不到参数。这里先落成 BoxShadow 属性（值保持原串，含 rgba），
                // 由外观层解析成 offsetX/offsetY/blur/spread/color 交给绘制端。
                // inset 与 none 也原样保留：语义是"不画外投影"，判断归绘制端。
                else if (key == L"box-shadow" || key == L"boxshadow")
                    Put(e, L"BoxShadow", val);
                // CSS `border` 简写：HTML 里 `border:1px solid #5F6976` 是最常见的写法，
                // 而 XML 早先必须写 BorderThickness + BorderColor 两个属性（还要知道别名）。
                // 这里把简写拆成 宽/样式/颜色 三块，各落到已有的外观属性上，纯映射不加实现。
                else if (key.size() > 7 && key.compare(0, 7, L"border-") == 0 &&
                         (key.find(L"top") != std::wstring::npos ||
                          key.find(L"right") != std::wstring::npos ||
                          key.find(L"bottom") != std::wstring::npos ||
                          key.find(L"left") != std::wstring::npos) &&
                         key.find(L"radius") == std::wstring::npos)
                {
                    // CSS 单边边框：border-top / border-right / border-bottom / border-left
                    // 及其 -width / -color / -style 变体。HTML 画分割线最常用的一句
                    // `border-bottom:1px solid #333`，XML 早先只能整圈框住（做不到单边）。
                    // 键名驼峰化后交给外观层：border-bottom-width → BorderBottomWidth。
                    std::wstring prop = L"Border";
                    bool upper = true;
                    for (size_t i = 7; i < key.size(); ++i)
                    {
                        const wchar_t c = key[i];
                        if (c == L'-')
                        {
                            upper = true;
                            continue;
                        }
                        prop.push_back(upper ? static_cast<wchar_t>(towupper(c)) : c);
                        upper = false;
                    }
                    Put(e, prop.c_str(), val);
                }
                else if (key == L"border" || key == L"border-width" ||
                         key == L"border-style" || key == L"border-color")
                {
                    auto isColorTok = [](const std::wstring& t) -> bool {
                        if (t.empty())
                            return false;
                        if (t[0] == L'#')
                            return true;
                        if (_wcsicmp(t.c_str(), L"transparent") == 0)
                            return true;
                        if (t.find(L"rgb") != std::wstring::npos)
                            return true;
                        for (wchar_t c : t)
                            if (c >= L'0' && c <= L'9')
                                return false;   // 含数字 → 尺寸，不是颜色名
                        return true;            // 纯字母 → 颜色关键字（red/none 之外的样式词另行判断）
                    };
                    auto isStyleTok = [](const std::wstring& t) -> bool {
                        static const wchar_t* k[] = { L"solid", L"dashed", L"dotted", L"double",
                                                      L"groove", L"ridge", L"inset", L"outset" };
                        for (const wchar_t* s : k)
                            if (_wcsicmp(t.c_str(), s) == 0)
                                return true;
                        return false;
                    };
                    // 允许带 px 的宽度
                    auto widthOf = [](const std::wstring& t) -> int {
                        std::wstring n;
                        for (wchar_t c : t)
                        {
                            if ((c >= L'0' && c <= L'9') || c == L'-')
                                n.push_back(c);
                            else
                                break;
                        }
                        return n.empty() ? -1 : (int)wcstol(n.c_str(), nullptr, 10);
                    };

                    if (key == L"border-width")
                    {
                        const int w = widthOf(val);
                        if (w >= 0)
                            Put(e, L"BorderThickness", std::to_wstring(w));
                    }
                    else if (key == L"border-color")
                        Put(e, L"BorderColor", val);
                    else if (key == L"border-style")
                        Put(e, L"BorderStyle", val);
                    else // border 简写：空格分词后按 token 性质归类
                    {
                        std::vector<std::wstring> toks;
                        std::wstring cur;
                        for (wchar_t ch : val)
                        {
                            if (ch == L' ' || ch == L'\t')
                            {
                                if (!cur.empty()) { toks.push_back(cur); cur.clear(); }
                            }
                            else
                                cur.push_back(ch);
                        }
                        if (!cur.empty())
                            toks.push_back(cur);
                        for (const std::wstring& t : toks)
                        {
                            if (_wcsicmp(t.c_str(), L"none") == 0)
                            {
                                // border:none / border:0 none —— 宽度归零 + 样式 None
                                Put(e, L"BorderThickness", L"0");
                                Put(e, L"BorderStyle", L"None");
                                continue;
                            }
                            if (isStyleTok(t))
                            {
                                Put(e, L"BorderStyle", t);
                                continue;
                            }
                            if (isColorTok(t))
                            {
                                Put(e, L"BorderColor", t);
                                continue;
                            }
                            const int w = widthOf(t);
                            if (w >= 0)
                                Put(e, L"BorderThickness", std::to_wstring(w));
                        }
                    }
                }
                else if (key == L"color" || key == L"foreground")
                    Put(e, L"Foreground", val);
                // CSS outline：焦点框的标准写法（不占盒模型、画在盒外、可负偏移）。
                // XML 早先完全没有这条通路，焦点环只能靠宿主画、宽/色/偏移都改不了。
                else if (key == L"outline" || key == L"outline-width" ||
                         key == L"outline-style" || key == L"outline-color" ||
                         key == L"outline-offset")
                {
                    if (key == L"outline-width")       Put(e, L"OutlineWidth", val);
                    else if (key == L"outline-style")  Put(e, L"OutlineStyle", val);
                    else if (key == L"outline-color")  Put(e, L"OutlineColor", val);
                    else if (key == L"outline-offset") Put(e, L"OutlineOffset", val);
                    else                               Put(e, L"Outline", val);   // 简写下沉，外观层拆 token
                }
                else if (key == L"overflow" || key == L"overflow-x" || key == L"overflow-y")
                    Put(e, L"Overflow", val);
                else if (key == L"text-shadow" || key == L"textshadow")
                    Put(e, L"TextShadow", val);
                else if (key == L"pointer-events" || key == L"pointerevents")
                    Put(e, L"PointerEvents", val);   // none = 点击穿透（HTML 同款）
                else if (key == L"aspect-ratio" || key == L"aspectratio")
                    Put(e, L"AspectRatio", val);     // 16/9、2/1、1.5
                else if (key == L"transition" || key == L"transition-duration")
                    // CSS transition 简写/时长：`all .2s ease` / `background-color 150ms`
                    Put(e, L"TransitionDuration", val);
                else if (key == L"transition-timing-function")
                    Put(e, L"TransitionTimingFunction", val);
                else if (key == L"animation" || key == L"animation-duration" ||
                         key == L"animation-iteration-count" || key == L"animation-timing-function")
                {
                    // CSS animation：`pulse 1s ease infinite`。
                    // HTML 的关键帧写在 @keyframes 块里；XML 没有块级语法，用同名元素属性
                    // Keyframes="0:#ff0000;50:#00ff00;100:#ff0000" 给帧表（一处声明、一处引用）。
                    // 这里只翻译参数，真正播放交给宿主（同 transition 的分工）。
                    if (key == L"animation-duration")
                        Put(e, L"AnimationDuration", val);
                    else if (key == L"animation-iteration-count")
                        Put(e, L"AnimationIteration", val);
                    else if (key == L"animation-timing-function")
                        Put(e, L"AnimationTimingFunction", val);
                    else
                        Put(e, L"Animation", val);        // 简写下沉，加载端拆 token
                }
                else if (key == L"keyframes")
                    Put(e, L"Keyframes", val);
                else if (key == L"background" || key == L"background-color" ||
                         key == L"background-image")
                {
                    // CSS `background:linear-gradient(180deg,#A,#B)` —— 与 HTML 完全同一个写法。
                    // 渐变整串交给外观层（BgImage），绘制端逐行插值；纯色仍走 BackColor。
                    if (val.find(L"gradient(") != std::wstring::npos)
                        Put(e, L"BgImage", val);
                    else
                    {
                        // CSS 半透明底色：rgba(r,g,b,a) 与 #RRGGBBAA。
                        // HTML 里 `background:rgba(0,0,0,.5)` 只让背景半透明、文字不透明，
                        // 所以拆成「不透明底色 + BgAlpha」，绘制端按 alpha 与父底色混合。
                        std::wstring solid = val;
                        int alpha255 = -1;
                        int r = -1, g = -1, b = -1;
                        double fa = -1.0;
                        const std::wstring t = Trim(val);
                        if (swscanf_s(t.c_str(), L"rgba(%d,%d,%d,%lf)", &r, &g, &b, &fa) == 4 ||
                            swscanf_s(t.c_str(), L"rgba( %d , %d , %d , %lf )", &r, &g, &b, &fa) == 4)
                        {
                            if (fa < 0.0) fa = 0.0;
                            if (fa > 1.0) fa = 1.0;
                            alpha255 = (int)(fa * 255.0 + 0.5);
                            wchar_t buf[32];
                            swprintf_s(buf, L"#%02X%02X%02X", r & 0xFF, g & 0xFF, b & 0xFF);
                            solid = buf;
                        }
                        else if (t.size() == 9 && t[0] == L'#')
                        {
                            // #RRGGBBAA（CSS 标准：alpha 在末尾）
                            const unsigned long v = wcstoul(t.c_str() + 1, nullptr, 16);
                            const unsigned long rgb = (v >> 8) & 0xFFFFFF;
                            alpha255 = (int)(v & 0xFF);
                            wchar_t buf[32];
                            swprintf_s(buf, L"#%06X", rgb);
                            solid = buf;
                        }
                        else if (t.size() == 4 && t[0] == L'#')
                        {
                            // CSS 短写 #RGB：等价于把每位重复一遍（#fff = #ffffff）
                            solid.clear();
                            solid.push_back(L'#');
                            for (int i = 1; i <= 3; ++i)
                            {
                                solid.push_back(t[i]);
                                solid.push_back(t[i]);
                            }
                        }
                        else if (t.size() == 5 && t[0] == L'#')
                        {
                            // CSS 短写 #RGBA：末位 alpha 展开成 #RRGGBB + BgAlpha
                            wchar_t buf[32];
                            swprintf_s(buf, L"#%c%c%c%c%c%c", t[1], t[1], t[2], t[2], t[3], t[3]);
                            solid = buf;
                            const int hi = (t[4] >= L'a') ? (t[4] - L'a' + 10)
                                             : (t[4] >= L'A') ? (t[4] - L'A' + 10) : (t[4] - L'0');
                            alpha255 = hi * 17;
                        }
                        // 两个正名都写一份：Background 给旧控件属性、BackColor 给外观契约
                        // （XmlAppearance 读的是 BackColor，只写 Background 时容器画不出底色）
                        Put(e, L"Background", solid);
                        Put(e, L"BackColor", solid);
                        if (alpha255 >= 0)
                            Put(e, L"BgAlpha", std::to_wstring(alpha255));
                    }
                }
                else if (key == L"font-size")
                    Put(e, L"FontSize", val);
                // CSS text-transform：HTML 里一行就能让标签/按钮全大写或首字母大写，
                // XML 只能改文案本身（改数据不改渲染），属于"语言便利度"缺口。
                else if (key == L"text-transform" || key == L"texttransform")
                    Put(e, L"TextTransform", val);
                // CSS :hover 的 XML 等价：XML 单元素没有伪类，用 hover-background / hover-color。
                // HTML 的 `button:hover{...}` 一行就有"鼠标移入就变"，XML 早先一点都表达不了。
                else if (key == L"hover-background" || key == L"hoverbackground")
                    Put(e, L"HoverBackground", val);
                else if (key == L"hover-color" || key == L"hovercolor")
                    Put(e, L"HoverColor", val);
                // CSS :focus 的 XML 等价：focus-background / focus-color（同上，单元素无伪类）
                else if (key == L"focus-background" || key == L"focusbackground")
                    Put(e, L"FocusBackground", val);
                else if (key == L"focus-color" || key == L"focuscolor")
                    Put(e, L"FocusColor", val);
                // CSS :active 的 XML 等价：active-background / active-color（按下态）
                else if (key == L"active-background" || key == L"activebackground")
                    Put(e, L"ActiveBackground", val);
                else if (key == L"active-color" || key == L"activecolor")
                    Put(e, L"ActiveColor", val);
                // CSS 字体族 / 斜体：绘制端已有 FontFamily / FontStyle，纯映射即可。
                // 但 HTML 写的是"回退列表 + 引号"：font-family:"Segoe UI", Arial, sans-serif。
                // 整串直传会匹配失败 → 静默回落默认字体，所以这里取列表首个具体字体并去引号；
                // 只写通用族（sans-serif 之类）时留空 = 用系统默认，与浏览器行为一致。
                else if (key == L"font-family" || key == L"fontfamily")
                {
                    std::wstring first = val;
                    const size_t comma = first.find(L',');
                    if (comma != std::wstring::npos)
                        first = first.substr(0, comma);
                    first = Trim(first);
                    if (!first.empty() && (first.front() == L'"' || first.front() == L'\''))
                    {
                        const wchar_t q = first.front();
                        first.erase(0, 1);
                        if (!first.empty() && first.back() == q)
                            first.pop_back();
                    }
                    const std::wstring low = Lower(first);
                    if (low == L"sans-serif" || low == L"serif" || low == L"monospace" ||
                        low == L"system-ui" || low == L"cursive" || low == L"fantasy")
                        first.clear();
                    Put(e, L"FontFamily", first);
                }
                else if (key == L"font-style" || key == L"fontstyle")
                    Put(e, L"FontStyle",
                        (Lower(val) == L"italic" ? L"Italic"
                         : (Lower(val) == L"oblique" ? L"Oblique" : L"Normal")));
                else if (key == L"font-weight")
                    Put(e, L"FontWeight", val);
                else if (key == L"font")
                {
                    // CSS `font` 简写：`font: [style] [weight] size[/line-height] family`。
                    // HTML 里一句就能同时给出字号/字重/行高/字体族，XML 要写四五个属性，
                    // 是"语言便利度"上最典型的一处差距。
                    std::vector<std::wstring> tk;
                    {
                        std::wstring cur;
                        for (wchar_t c : val)
                        {
                            if (iswspace(c))
                            {
                                if (!cur.empty()) { tk.push_back(cur); cur.clear(); }
                            }
                            else
                                cur.push_back(c);
                        }
                        if (!cur.empty())
                            tk.push_back(cur);
                    }
                    std::wstring fam;
                    for (size_t i = 0; i < tk.size(); ++i)
                    {
                        const std::wstring low = Lower(tk[i]);
                        // 尺寸项：数字开头（可带 /行高）
                        const bool isNum = !low.empty() && (iswdigit(low[0]) || low[0] == L'.');
                        if (low == L"bold" || low == L"bolder")
                        {
                            Put(e, L"FontWeight", L"bold");
                            continue;
                        }
                        if (low == L"italic" || low == L"oblique")
                        {
                            Put(e, L"FontStyle", low == L"italic" ? L"Italic" : L"Oblique");
                            continue;
                        }
                        if (low == L"normal" && fam.empty() && tk.size() > 1)
                            continue;   // 前置 normal = 常规字重/直立，无需落属性
                        if (isNum)
                        {
                            const size_t slash = low.find(L'/');
                            const std::wstring sz = slash == std::wstring::npos ? low : low.substr(0, slash);
                            Put(e, L"FontSize", sz);
                            if (slash != std::wstring::npos)
                            {
                                const std::wstring lh = low.substr(slash + 1);
                                if (!lh.empty())
                                    Put(e, L"LineHeight", lh);
                            }
                            continue;
                        }
                        if (!fam.empty())
                            fam += L" ";
                        fam += tk[i];
                    }
                    if (!fam.empty())
                        Put(e, L"FontFamily", fam);
                }
                // CSS text-align：HTML 值域 left/right/center/justify（start/end 在 LTR 下等同 left/right）。
                // justify 是两端对齐，宿主按 DWRITE_TEXT_ALIGNMENT_JUSTIFIED 画（需配 text-wrapping:wrap）。
                else if (key == L"text-align")
                {
                    std::wstring v = val;
                    const std::wstring low = Lower(val);
                    if (low == L"start" || low == L"left") v = L"Left";
                    else if (low == L"end" || low == L"right") v = L"Right";
                    else if (low == L"center") v = L"Center";
                    else if (low == L"justify") v = L"Justify";
                    Put(e, L"TextAlign", v);
                }
                // CSS text-decoration / text-decoration-line：HTML 里 `<a>` / `<del>` 的标准写法。
                // 值是 token 串，underline 与 line-through 可同时存在，原样落属性交给外观层判。
                else if (key == L"text-decoration" || key == L"textdecoration" ||
                         key == L"text-decoration-line")
                {
                    std::wstring low = Lower(val);
                    std::wstring deco;
                    if (low.find(L"underline") != std::wstring::npos)
                        deco = L"underline";
                    if (low.find(L"line-through") != std::wstring::npos)
                        deco = deco.empty() ? std::wstring(L"line-through")
                                            : deco + L" line-through";
                    Put(e, L"TextDecoration", deco.empty() ? std::wstring(L"none") : deco);
                }
                // HTML 的文字换行 / 垂直对齐：white-space、vertical-align 是 CSS 的写法，
                // 落到既有的 TextWrapping / VerticalAlignment（纯映射，绘制端已有实现）
                else if (key == L"white-space" || key == L"whitespace")
                    Put(e, L"TextWrapping",
                        (_wcsicmp(val.c_str(), L"nowrap") == 0 ? L"NoWrap" : L"Wrap"));
                else if (key == L"vertical-align")
                {
                    // 绘制端认的是 Top / Center / Bottom，CSS 值全是小写，这里归一化
                    const std::wstring lowv = Lower(val);
                    Put(e, L"VerticalAlignment",
                        lowv == L"top" ? L"Top" : (lowv == L"bottom" ? L"Bottom" : L"Center"));
                }
                // CSS 半透明：opacity:0.5 是 0~1 的比例（映射成 0~255 交给外观层）
                else if (key == L"opacity")
                    Put(e, L"Opacity", val);
                // CSS 盒模型：border-box 让 width 变成"含 padding 的外框宽"
                else if (key == L"box-sizing" || key == L"boxsizing")
                    Put(e, L"BoxSizing", val);
                // CSS 字距：绘制端用 DWrite TextLayout 的 SetCharacterSpacing 实现
                else if (key == L"letter-spacing" || key == L"letterspacing")
                    Put(e, L"LetterSpacing", val);
                // CSS 尺寸上下限：min-width/max-width/min-height/max-height。
                // 浏览器靠它们保证窄屏下卡片不被 flex-shrink 压塌，XML 早先完全没有对应写法。
                else if (key == L"min-width")
                    Put(e, L"MinWidth", val);
                else if (key == L"max-width")
                    Put(e, L"MaxWidth", val);
                else if (key == L"min-height")
                    Put(e, L"MinHeight", val);
                else if (key == L"max-height")
                    Put(e, L"MaxHeight", val);
                else if (key == L"line-height" || key == L"lineheight")
                    Put(e, L"LineHeight", val);
                else if (key == L"text-indent" || key == L"textindent")
                    Put(e, L"TextIndent", val);
                else if (key == L"text-overflow" || key == L"textoverflow")
                    Put(e, L"TextOverflow", val);
                else if (key == L"top" || key == L"left" || key == L"right" || key == L"bottom")
                {
                    // CSS position:sticky：`top` 是"吸顶距离"，不是绝对定位的 y。
                    // 元素自己的落位仍来自 Top 属性（本引擎里等价于 CSS 的流位置）；
                    // 属性没写 Top 时，按 CSS 语义把 top 当落位（吸顶线就等于落位）。
                    if (e.sticky && key == L"top")
                    {
                        if (e.has(L"Top"))
                        {
                            e.stickyTop = px;
                            Put(e, L"StickyTop", std::to_wstring(px));
                        }
                        else
                            Put(e, L"Top", val);
                        continue;
                    }
                    // CSS 定位关键字：top/left 直接对上 Top/Left；right/bottom 由宿主按父尺寸换算
                    Put(e, key == L"top" ? L"Top" : key == L"left" ? L"Left"
                                                                    : (key == L"right" ? L"Right" : L"Bottom"),
                        val);
                }
                else if (key == L"inset")
                {
                    // CSS `inset` 简写（1~4 个值，顺序同 margin：上 右 下 左）。
                    // HTML 里 `inset:0` 是"撑满父"最常用的一句；XML 侧不能照搬 Right/Bottom
                    // —— 宿主把它们当右下锚定，元素会被压成 0 尺寸。所以全 0 时翻译成
                    // 等价且更稳的 Left/Top + 100% 宽高；其余值仍按四边落属性（带锚定风险）。
                    std::vector<std::wstring> tk;
                    {
                        std::wstring cur;
                        for (wchar_t c : val)
                        {
                            if (iswspace(c))
                            {
                                if (!cur.empty()) { tk.push_back(cur); cur.clear(); }
                            }
                            else
                                cur.push_back(c);
                        }
                        if (!cur.empty())
                            tk.push_back(cur);
                    }
                    if (!tk.empty())
                    {
                        const std::wstring vT = tk[0];
                        const std::wstring vR = tk.size() > 1 ? tk[1] : vT;
                        const std::wstring vB = tk.size() > 2 ? tk[2] : vT;
                        const std::wstring vL = tk.size() > 3 ? tk[3] : (tk.size() == 1 ? vT : vR);
                        auto zero = [](const std::wstring& s) {
                            return s == L"0" || s == L"0px" || s == L"0.0px";
                        };
                        if (zero(vT) && zero(vR) && zero(vB) && zero(vL))
                        {
                            // `inset:0` = 四边贴齐父，等价于撑满父内框。
                            // XML 的 Right/Bottom 是右下锚定（元素会被压成 0 尺寸），
                            // 所以内部翻译成「撑满父」这条机制（fillParent = 父内框铺满）。
                            e.fillParent = true;
                            e.fillMargin = 0;
                        }
                        else
                        {
                            Put(e, L"Top", vT);
                            Put(e, L"Right", vR);
                            Put(e, L"Bottom", vB);
                            Put(e, L"Left", vL);
                        }
                    }
                }
                else if (key == L"z-index" || key == L"zindex")
                {
                    // CSS 的 z-index 是标准写法，控件层一直只认属性名 ZIndex/Level：
                    // HTML 里 style="z-index:5" 完全不生效且不报错（静默失效），
                    // XML 侧却只能退回 WPF 味的 ZIndex="5"。这里把 CSS 写法翻译成同一份属性。
                    const int z = static_cast<int>(wcstol(val.c_str(), nullptr, 10));
                    Put(e, L"ZIndex", std::to_wstring(z));
                }
                else if (key == L"cursor")
                {
                    // CSS `cursor:pointer` 是"这块可点"的最直观表达，HTML 里几乎每个按钮都写。
                    // XML 侧此前既没有同名属性、也没有任何映射，写了等于白写。
                    Put(e, L"Cursor", val);
                }
                else if (key == L"visibility")
                {
                    // visibility:hidden 在 HTML 里是"留着位置但不画"，而 Visible="False"
                    // 是"整块拿掉"——两条语义不能串。这里只记 Visibility，由加载端
                    // 单独做"藏窗口但不动几何"，布局照常按盒尺寸占位。
                    Put(e, L"Visibility", Lower(val));
                }
                else if (key == L"position")
                {
                    // CSS position：fixed 在 overflow 容器里意味着"不随内容滚动"。
                    // XML 侧此前没有任何 position 关键字（绝对定位靠 left/top 隐式触发），
                    // 连 fixed 这么基础的一条都表达不了。这里只存语义，几何仍由 left/top 给。
                    const std::wstring lowv = Lower(val);
                    Put(e, L"Position", lowv);
                    // position:sticky —— 元素跟着内容滚，滚到距容器顶 stickyTop 就吸住。
                    // XML 侧此前完全没有这个能力（只能靠 fixed 一滚到底不动）。
                    if (lowv == L"sticky")
                    {
                        e.sticky = true;
                        Put(e, L"Sticky", L"1");
                    }
                }
                else if (key == L"object-fit")
                {
                    // CSS object-fit：图片在给定盒子里的缩放模式。XML 侧此前完全没有
                    // （原生 SS_BITMAP 只会按原图尺寸居中+裁剪，写了 width/height 也不缩放）。
                    Put(e, L"ObjectFit", Lower(val));
                }
                else if (key == L"object-position")
                {
                    // CSS object-position：图片在盒子里的对齐位置（contain/cover/none 才有余量）。
                    Put(e, L"ObjectPosition", val);
                }
            }
        }
        // position:sticky 的 top 后处理：CSS 允许 `top` 写在 `position` 前面，
        // 而 key == top 的分支（更早）会把它落成绝对定位的 Top 属性 ——
        // 那样元素会被直接钉死在 y=top。这里统一改判：sticky 的 Top 只当吸顶距离，并从属性里摘掉。
    }

    // style 里的 flex 声明先在解析期落成本地属性。
    // 原因：ApplyInlineStyle 的翻译发生在 HTML 糖化过程中，而糖化会把 <div> 重建成 <XFlexPanel>，
    // 布局阶段读到的是重建后的节点 —— 于是「属性写法生效、style 写法不生效」。
    // 在糖化之前写一次，糖化复制属性时会一并带过去，两条路径读到同一份值。
    void ApplyStyleFlexProps(XmlElement& e)
    {
        const std::wstring style = AttrOf(e, L"style");
        size_t start = 0;
        while (!style.empty() && start < style.size())
        {
            const size_t semi = style.find(L';', start);
            const std::wstring decl = Trim(style.substr(
                start, semi == std::wstring::npos ? std::wstring::npos : semi - start));
            start = (semi == std::wstring::npos) ? style.size() : semi + 1;
            if (decl.empty())
                continue;
            const size_t colon = decl.find(L':');
            if (colon == std::wstring::npos)
                continue;
            const std::wstring key = CssKeyNormalize(Trim(decl.substr(0, colon)));
            const std::wstring val = Trim(decl.substr(colon + 1));
            if (val.empty())
                continue;
            if (key == L"display")
            {
                if (_wcsicmp(val.c_str(), L"flex") == 0)
                    Put(e, L"Display", L"flex");
                else
                {
                    Put(e, L"Display", Lower(val));
                    // display:none 在控件路径也必须能隐藏（上面那层解析已经翻成 Visible="False"，
                    // 这里保持一致，避免两条解析链给出两种结果）。
                    if (_wcsicmp(val.c_str(), L"none") == 0)
                        Put(e, L"Visible", L"False");
                }
            }
            else if (key == L"flex-direction" || key == L"flexdirection")
                Put(e, L"FlexDirection", val);
            else if (key == L"flex-wrap" || key == L"flexwrap")
                Put(e, L"Wrap", val);
            else if (key == L"justify-content" || key == L"justifycontent")
                Put(e, L"Justify", val);
            else if (key == L"align-items" || key == L"alignitems")
                Put(e, L"Align", val);
            else if (key == L"align-self" || key == L"alignself")
            {
                Put(e, L"AlignSelf", val);
                Put(e, L"Self", val);
                // 交叉轴对齐同时改字段：flexSelf 在解析早期就固化了，
                // 只写属性不重算字段的话，布局仍然按旧值走。
                e.flexSelf = (_wcsicmp(val.c_str(), L"auto") == 0) ? -1
                           : (_wcsicmp(val.c_str(), L"start") == 0 ||
                              _wcsicmp(val.c_str(), L"flex-start") == 0) ? 0
                           : (_wcsicmp(val.c_str(), L"center") == 0 ||
                              _wcsicmp(val.c_str(), L"middle") == 0) ? 1
                           : (_wcsicmp(val.c_str(), L"end") == 0 ||
                              _wcsicmp(val.c_str(), L"flex-end") == 0) ? 2
                           : (_wcsicmp(val.c_str(), L"baseline") == 0) ? 4
                           : (_wcsicmp(val.c_str(), L"stretch") == 0) ? 3
                                                                     : -1;
            }
            else if (key == L"gap")
                Put(e, L"Gap", val);
            else if (key == L"row-gap" || key == L"rowgap")
                Put(e, L"RowGap", val);
            else if (key == L"column-gap" || key == L"columngap")
                Put(e, L"ColumnGap", val);
            else if (key == L"order")
                Put(e, L"Order", val);
            else if (key == L"flex")
                Put(e, L"Flex", val);
            else if (key == L"flex-grow" || key == L"flexgrow")
                Put(e, L"Grow", val);
            else if (key == L"flex-shrink" || key == L"flexshrink")
                Put(e, L"Shrink", val);
            else if (key == L"flex-basis" || key == L"flexbasis")
                Put(e, L"Basis", val);
        }
        for (auto& c : e.children)
            ApplyStyleFlexProps(c);
    }

    void CommonAliases(XmlElement& e)
    {
        // 属性式 `Padding="8 16"`：作者写的简写按 CSS 口径（1=四边、2=上下/左右、
        // 3=上左右下、4=上右下左）展开，再落成规范四值「左 上 右 下」。
        // 必须放在 ApplyInlineStyle 之前：style 那一路自己就会 Put 回规范四值，
        // 放在后面会把规范串再按 CSS 口径解一遍，上下/左右整组翻转（实测过）。
        {
            const std::wstring raw = e.get(L"Padding");
            if (!raw.empty())
            {
                std::vector<int> pv;
                std::wstring cur;
                for (wchar_t ch : raw)
                {
                    if (ch == L' ' || ch == L'\t' || ch == L',')
                    {
                        if (!cur.empty())
                        {
                            pv.push_back((int)wcstol(cur.c_str(), nullptr, 10));
                            cur.clear();
                        }
                    }
                    else
                        cur.push_back(ch);
                }
                if (!cur.empty())
                    pv.push_back((int)wcstol(cur.c_str(), nullptr, 10));
                if (!pv.empty() && pv.size() <= 4)
                {
                    int pT = pv[0], pR = pv[0], pB = pv[0], pL = pv[0];
                    if (pv.size() == 2)
                    {
                        pT = pB = pv[0];
                        pL = pR = pv[1];
                    }
                    else if (pv.size() == 3)
                    {
                        pT = pv[0];
                        pL = pR = pv[1];
                        pB = pv[2];
                    }
                    else if (pv.size() >= 4)
                    {
                        pT = pv[0];
                        pR = pv[1];
                        pB = pv[2];
                        pL = pv[3];
                    }
                    Put(e, L"Padding", std::to_wstring(pL) + L" " + std::to_wstring(pT) + L" " +
                                         std::to_wstring(pR) + L" " + std::to_wstring(pB));
                    e.padL = pL; e.padT = pT; e.padR = pR; e.padB = pB;
                }
            }
        }
        ApplyInlineStyle(e);
        // HTML 的 `bg` / `bgcolor`（老式写法，仍常见）就是背景色：翻成 BackColor。
        // 只靠外观层的 keyIs("bg") 在容器路径上到不了（div 会先被改写成布局容器），
        // 所以在这里一次性改写成规范名，后续所有路径都认。
        {
            std::wstring bgv;
            if (TakeAttr(e, L"bg", bgv) || TakeAttr(e, L"bgcolor", bgv))
                if (!bgv.empty() && !e.has(L"BackColor"))
                    Put(e, L"BackColor", bgv);
        }
        if (e.has(L"id"))
        {
            // Name 是节点的名字字段（加载器读的是 XmlElement::name），属性也要留一份给序列化
            e.name = AttrOf(e, L"id");
            Put(e, L"Name", e.name);
        }
        if (e.has(L"title"))
            Put(e, L"ToolTip", AttrOf(e, L"title"));
        if (e.has(L"alt"))
            Put(e, L"ToolTip", AttrOf(e, L"alt"));
        if (e.has(L"aria-label"))
            Put(e, L"ToolTip", AttrOf(e, L"aria-label"));
        // hidden 是"不显示"而不是"不存在"：控件仍然建出来（脚本还能用），只是不可见
        if (Flag(e, L"hidden"))
            Put(e, L"Visible", L"False");
        if (e.has(L"placeholder"))
        {
            // 先摘走再写 Placeholder：set() 大小写不敏感，直接在原位改写会被 DropHtmlOnly 删掉
            std::wstring ph;
            if (TakeAttr(e, L"placeholder", ph))
                Put(e, L"Placeholder", ph);
        }
        if (Flag(e, L"disabled"))
            Put(e, L"Enabled", L"False");
        if (Flag(e, L"readonly"))
            Put(e, L"IsReadOnly", L"True");
        if (e.has(L"maxlength"))
            Put(e, L"MaxLength", AttrOf(e, L"maxlength"));
        if (e.has(L"href"))
            Put(e, L"Url", AttrOf(e, L"href"));
        if (e.has(L"src"))
            Put(e, L"Image", AttrOf(e, L"src"));
        // 事件：HTML 的 onxxx 对应本系统的驼峰事件名
        if (e.has(L"onclick")) { Put(e, L"Click", AttrOf(e, L"onclick")); }
        if (e.has(L"onblur")) { Put(e, L"LostFocus", AttrOf(e, L"onblur")); }
        if (e.has(L"rows"))
            PutSize(e, 0, (std::max)(2, _wtoi(AttrOf(e, L"rows").c_str())) * 22 + 8);
        if (e.has(L"cols"))
            PutSize(e, (std::max)(4, _wtoi(AttrOf(e, L"cols").c_str())) * 8 + 16, 0);
    }

    void ConvertNode(XmlElement& e);

    void ConvertChildren(XmlElement& e)
    {
        // 结构性标签（<meta>/<col>/<datalist>…）先剔掉，不渲染也不递归
        std::vector<XmlElement> kept;
        kept.reserve(e.children.size());
        for (auto& c : e.children)
        {
            if (IsDroppedTag(Lower(c.tag)))
                continue;
            kept.push_back(std::move(c));
        }
        e.children = std::move(kept);
        for (auto& c : e.children)
            ConvertNode(c);

        // flex 容器里的行内文本子项：宽度交给容器（CSS 里 flex item 不按内容定宽，
        // align-items:stretch 就拉满整行）。之前把内容宽写成 Width 属性，导致
        // <div style="display:flex;flex-direction:column"> 的卡片标题被裁成"图纸"。
        if (e.flex || _wcsicmp(e.get(L"Display").c_str(), L"flex") == 0)
        {
            for (auto& c : e.children)
            {
                if (!c.autoTextW)
                    continue;
                for (auto it = c.attrs.begin(); it != c.attrs.end(); ++it)
                {
                    if (_wcsicmp(it->first.c_str(), L"Width") == 0)
                    {
                        c.attrs.erase(it);
                        break;
                    }
                }
                c.hasW = false;
                c.autoTextW = false;
            }
        }
    }

    // select / ul / ol：子项汇成 Items="a|b|c"，<option selected> / <li selected> 给 SelectedIndex
    // numbering: 0 = 原样（<select> 的选项），1 = 圆点（<ul>），2 = 序号（<ol>）。
    // HTML 的列表在浏览器里每行都带 marker，XML 侧早先只把条目塞进列表框，
    // 一列光秃秃的文字 —— 与 HTML 观感差一截，而 marker 只是一次字符串拼接。
    void CollectItems(XmlElement& e, const wchar_t* itemTag, int numbering = 0)
    {
        std::wstring items;
        int selected = -1;
        int index = 0;
        std::vector<XmlElement> rest;
        for (auto& c : e.children)
        {
            const std::wstring t = Lower(c.tag);
            if (t != itemTag)
            {
                rest.push_back(std::move(c));
                continue;
            }
            std::wstring text = c.has(L"value") ? AttrOf(c, L"value") : Trim(c.text);
            if (text.empty())
                continue;
            if (!items.empty())
                items += L'|';
            if (numbering == 1)
                items += L"• ";
            else if (numbering == 2)
            {
                items += std::to_wstring(index + 1);
                items += L". ";
            }
            items += text;
            if (Flag(c, L"selected"))
                selected = index;
            ++index;
        }
        e.children = std::move(rest);
        if (!items.empty())
            Put(e, L"Items", items);
        if (selected >= 0)
            Put(e, L"SelectedIndex", std::to_wstring(selected));
    }

    // table：行列按单元格数生成星号轨道，td/th 逐格变成 Grid 里的 XLabel
    void ConvertTable(XmlElement& e)
    {
        std::vector<XmlElement> trs;
        for (const auto& c : e.children)
        {
            const std::wstring t = Lower(c.tag);
            if (t == L"thead" || t == L"tbody" || t == L"tfoot")
            {
                for (const auto& r : c.children)
                    if (Lower(r.tag) == L"tr")
                        trs.push_back(r);
            }
            else if (t == L"tr")
            {
                trs.push_back(c);
            }
        }

        int cols = 1;
        for (const auto& tr : trs)
        {
            int n = 0;
            for (const auto& c : tr.children)
            {
                const std::wstring t = Lower(c.tag);
                if (t == L"td" || t == L"th")
                    n += (std::max)(1, _wtoi(AttrOf(c, L"colspan").c_str()));
            }
            cols = (std::max)(cols, n);
        }
        const int rows = (std::max)(1, (int)trs.size());

        e.tag = L"XGrid";
        e.children.clear();
        e.gridRows.clear();
        e.gridCols.clear();
        // 行高按内容给：单元格标签 24 + 上下各 4 的留白
        PutSize(e, 0, rows * 32 + 8);

        XmlElement defsR;
        defsR.tag = L"Grid.RowDefinitions";
        XmlElement defsC;
        defsC.tag = L"Grid.ColumnDefinitions";
        for (int i = 0; i < rows; ++i)
        {
            e.gridRows.emplace_back(L"Height", L"*");
            XmlElement d;
            d.tag = L"RowDefinition";
            d.set(L"Height", L"*");
            defsR.children.push_back(std::move(d));
        }
        for (int j = 0; j < cols; ++j)
        {
            e.gridCols.emplace_back(L"Width", L"*");
            XmlElement d;
            d.tag = L"ColumnDefinition";
            d.set(L"Width", L"*");
            defsC.children.push_back(std::move(d));
        }
        e.children.push_back(std::move(defsR));
        e.children.push_back(std::move(defsC));

        int r = 0;
        for (auto& tr : trs)
        {
            int col = 0;
            for (auto& c : tr.children)
            {
                const std::wstring t = Lower(c.tag);
                if (t != L"td" && t != L"th")
                    continue;
                const int span = (std::max)(1, _wtoi(AttrOf(c, L"colspan").c_str()));

                XmlElement cell;
                cell.tag = L"XLabel";
                cell.row = r;
                cell.col = col;
                cell.set(L"Grid.Row", std::to_wstring(r));
                cell.set(L"Grid.Column", std::to_wstring(col));
                if (span > 1)
                {
                    cell.colSpan = span;
                    cell.set(L"Grid.ColumnSpan", std::to_wstring(span));
                }
                // 单元格留点边距，表格才不糊成一片
                cell.marginL = cell.marginT = cell.marginR = cell.marginB = 4;
                cell.set(L"Margin", L"4");

                if (!c.children.empty())
                {
                    cell.tag = L"XStackPanel";
                    cell.children = c.children;   // <td><b>x</b></td>
                }
                else
                {
                    cell.text = Trim(c.text);
                    if (!cell.text.empty())
                        cell.set(L"Text", cell.text);
                }
                e.children.push_back(std::move(cell));
                ConvertNode(e.children.back());
                col += span;
            }
            ++r;
        }
    }

    void ConvertInput(XmlElement& e, bool htmlChecked, bool hasHtmlValue,
                      const std::wstring& htmlValue)
    {
        const std::wstring type = Lower(AttrOf(e, L"type"));
        const std::wstring value = hasHtmlValue ? htmlValue : AttrOf(e, L"value");

        if (type == L"hidden")
        {
            // HTML 语义：不渲染。借 "Resources" 这个加载器/布局都跳过的标签占位
            e.tag = L"XResources";
            e.children.clear();
            return;
        }
        if (type == L"checkbox" || type == L"radio")
        {
            // HTML 的 name 只管表单提交/同组，落到我们这边会撞上元素的 Name 字段，
            // 摘掉它，让控件照常拿到自动名字（radiobutton_1000 之类）
            std::wstring group;
            if (TakeAttr(e, L"name", group) && !e.has(L"id") && e.name == group)
                e.name.clear();
            e.tag = (type == L"checkbox") ? L"XCheckBox" : L"XRadioButton";
            const std::wstring text = !value.empty() ? value : Trim(e.text);
            if (!text.empty())
                Put(e, L"Text", text);
            Put(e, L"Checked", (htmlChecked || Flag(e, L"defaultchecked")) ? L"True" : L"False");
            return;
        }
        if (type == L"range" || type == L"number")
        {
            e.tag = (type == L"range") ? L"XSlider" : L"XSpinBox";
            if (e.has(L"min")) Put(e, L"Minimum", AttrOf(e, L"min"));
            if (e.has(L"max")) Put(e, L"Maximum", AttrOf(e, L"max"));
            if (!value.empty()) Put(e, L"Value", value);
            return;
        }
        if (type == L"date" || type == L"datetime-local" || type == L"datetime" ||
            type == L"month" || type == L"week")
        {
            e.tag = L"XDatePicker";
            if (!value.empty())
                Put(e, L"Value", value);
            return;
        }
        if (type == L"time")
        {
            // 没有时间选择控件：单行输入，值形如 10:30
            e.tag = L"XTextField";
            if (!value.empty())
                Put(e, L"Text", value);
            return;
        }
        if (type == L"submit" || type == L"button" || type == L"reset" || type == L"file" ||
            type == L"image")
        {
            e.tag = L"XButton";
            std::wstring label = value;
            if (label.empty())
            {
                std::wstring alt;
                if (TakeAttr(e, L"alt", alt))
                    label = alt;
            }
            Put(e, L"Text", !label.empty() ? label
                                           : (type == L"file" ? L"选择文件…" : L"确定"));
            return;
        }
        if (type == L"color")
        {
            // 无原生取色控件：做成一个显示色值的按钮，点击可接 Python 挑色
            e.tag = L"XButton";
            Put(e, L"Text", !value.empty() ? value : L"#000000");
            Put(e, L"ToolTip", L"选择颜色");
            return;
        }
        if (type == L"password")
        {
            e.tag = L"XTextField";
            Put(e, L"Password", L"True");
            if (!value.empty())
                Put(e, L"Text", value);
            return;
        }
        // 其余（text/search/email/tel/url/…）都落到单行输入框
        e.tag = L"XTextField";
        // 标记"这个 XTextField 是 HTML 原生 <input> 变来的"：
        // 标签名在这里已经被改写，默认值表就再也没法区分"原生的"和"老写法"，
        // 于是尺寸默认值只能按老写法 160x26 走。留一个标记，默认值表据此走 HTML 语义。
        Put(e, L"HtmlInput", L"1");
        const std::wstring body = !value.empty() ? value : Trim(e.text);
        if (!body.empty())
            Put(e, L"Text", body);
    }

    void ConvertNode(XmlElement& e)
    {
        const std::wstring t = Lower(e.tag);
        if (e.tag.empty())
            return;
        if (IsDroppedTag(t))
        {
            // 结构性标签落在根上（极少见）：变成不渲染的占位，别让它当未知控件
            e.tag = L"XResources";
            e.children.clear();
            return;
        }
        if (!IsHtmlTag(t))
        {
            // CSS 的 style 对任何元素都生效（HTML 也是这么来的），不限于 HTML 标签名。
            // 写在 <XLabel> / <row> 上的 style 照样解析，解析完摘掉属性免报"未知属性"。
            ApplyInlineStyle(e);
            Drop(e, L"style");
            ConvertChildren(e);    // XAML 写法：原样递归，不动
            return;
        }

        const bool wasHtml = true;
        (void)wasHtml;

        // 撞命名的两个 HTML 属性先摘走（checked / value），翻译时用摘下来的值
        const bool htmlChecked = TakeFlag(e, L"checked");
        std::wstring htmlValue;
        const bool hasHtmlValue = TakeAttr(e, L"value", htmlValue);

        if (t == L"html")
        {
            // <html> 只是文档壳：把 <body> 拆掉，尺寸/内边距/间距提到根上，
            // 这样 style="width:420;padding:12" 写在 body 也照样决定窗口大小。
            ConvertChildren(e);
            if (!e.children.empty())
            {
                XmlElement& body = e.children.front();
                if (body.tag == L"XStackPanel" || body.tag == L"XFlexPanel")
                {
                    for (const auto& a : body.attrs)
                    {
                        bool exists = false;
                        for (const auto& ea : e.attrs)
                            if (_wcsicmp(ea.first.c_str(), a.first.c_str()) == 0)
                            {
                                exists = true;
                                break;
                            }
                        if (!exists)
                            e.attrs.emplace_back(a.first, a.second);
                    }
                    if (!e.hasW && body.hasW) { e.w = body.w; e.hasW = true; }
                    if (!e.hasH && body.hasH)
                    {
                        e.h = body.h;
                        e.hasH = true;
                        e.autoH = body.autoH;
                    }
                    if (e.padL == 0 && e.padT == 0)
                    {
                        e.padL = body.padL; e.padT = body.padT;
                        e.padR = body.padR; e.padB = body.padB;
                    }
                    // 布局字段也要跟着走：根是按字段排的，只搬属性不搬字段等于没搬
                    if (e.spacing == 0 && body.spacing > 0)
                        e.spacing = body.spacing;
                    if (!e.hasOrientation && body.hasOrientation)
                    {
                        e.horizontal = body.horizontal;
                        e.hasOrientation = true;
                    }
                    // 先把 body 的子节点搬到局部变量，再赋给 e：body 是 e.children 里的引用，
                    // 直接 e.children = std::move(body.children) 会在释放自身时先析构 body。
                    std::vector<XmlElement> kids = std::move(body.children);
                    e.children = std::move(kids);
                }
            }
            e.tag = L"XStackPanel";
            CommonAliases(e);
            DropHtmlOnly(e);
            return;
        }
        if (t == L"input")
        {
            ConvertInput(e, htmlChecked, hasHtmlValue, htmlValue);
            CommonAliases(e);
        }
        else if (t == L"textarea")
        {
            e.tag = L"XTextArea";
            Put(e, L"HtmlTextarea", L"1");
            const std::wstring body = Trim(e.text);
            if (!body.empty() && !e.has(L"Text"))
                Put(e, L"Text", body);
            CommonAliases(e);
        }
        else if (t == L"select")
        {
            // <optgroup> 只是分组标签：把里面的 <option> 提到 select 直接子级
            std::vector<XmlElement> flat;
            for (auto& c : e.children)
            {
                if (Lower(c.tag) == L"optgroup")
                {
                    for (auto& gc : c.children)
                        flat.push_back(std::move(gc));
                }
                else
                    flat.push_back(std::move(c));
            }
            e.children = std::move(flat);
            // size>1 就是"展开的列表框"，与 multiple 一样落到 XListBox
            std::wstring sizeAttr;
            const int listRows = TakeAttr(e, L"size", sizeAttr) ? _wtoi(sizeAttr.c_str()) : 0;
            e.tag = (Flag(e, L"multiple") || listRows > 1) ? L"XListBox" : L"XComboBox";
            Put(e, L"HtmlSelect", L"1");
            CollectItems(e, L"option");
            CommonAliases(e);
        }
        else if (t == L"ul" || t == L"ol")
        {
            e.tag = L"XListBox";
            CollectItems(e, L"li", (t == L"ol") ? 2 : 1);
            CommonAliases(e);
        }
        else if (t == L"table")
        {
            ConvertTable(e);
            CommonAliases(e);
            return;                // ConvertTable 内部已递归单元格
        }
        else if (t == L"optgroup" || t == L"option" || t == L"li" ||
                 t == L"tr" || t == L"td" || t == L"th" || t == L"caption" ||
                 t == L"thead" || t == L"tbody" || t == L"tfoot")
        {
            // 结构化子节点：父级已经消费掉，落到这里说明是孤立使用，按文本/容器处理。
            // 必须继续往下递归：<thead> 孤儿时里面还有 <tr><td>，不递归就成了未知控件。
            if (e.children.empty())
            {
                e.tag = L"XLabel";
                const std::wstring body = Trim(e.text);
                if (!body.empty() && !e.has(L"Text"))
                    Put(e, L"Text", body);
            }
            else
            {
                e.tag = L"XStackPanel";
            }
            CommonAliases(e);
            ConvertChildren(e);
        }
        else if (t == L"img")
        {
            e.tag = L"XImage";
            CommonAliases(e);
        }
        else if (t == L"hr")
        {
            e.tag = L"XDivider";
            CommonAliases(e);
        }
        else if (t == L"a")
        {
            // HTML 的 <a href="…">文字</a> 早先只换控件不搬内容：文字留在 e.text 没落 Text、
            // href 也没进 Url，嵌套 <span>/<b> 更不会转换（加载时报"未知控件 <span>"）。
            // 落 XLabel 而不是 Hyperlink 预览节点：HTML 的链接就是"蓝色带下划线的文字"，
            // XLabel 走完整的外观通道（color / text-decoration / 字号 / 折行），
            // Hyperlink 预览节点只用固定 accent 色画字、下划线不生效（实测 diff=0）。
            // href 仍映射到 Url 属性，留给点击行为接线。
            e.tag = L"XLabel";
            const std::wstring body = Trim(e.text);
            if (!body.empty() && !e.has(L"Text"))
                Put(e, L"Text", body);
            // 浏览器默认链接样式：蓝色 + 下划线（作者显式写 color/text-decoration 仍优先）
            if (!e.has(L"Foreground") && !e.has(L"Color"))
                Put(e, L"Foreground", L"#0066cc");
            if (!e.has(L"TextDecoration"))
                Put(e, L"TextDecoration", L"underline");
            ConvertChildren(e);
            CommonAliases(e);
        }
        else if (t == L"br")
        {
            // 换行就是一段竖直空隙：给个 8px 高的空标签
            e.tag = L"XLabel";
            PutSize(e, 0, 8);
            CommonAliases(e);
        }
        else if (t == L"picture")
        {
            // <picture> 只是给 <img> 套多分辨率：真正要画的是里面那个 img
            for (auto& c : e.children)
                if (Lower(c.tag) == L"img")
                {
                    e.tag = L"XImage";
                    std::wstring src;
                    if (TakeAttr(c, L"src", src))
                        Put(e, L"Image", src);
                    std::wstring alt;
                    if (TakeAttr(c, L"alt", alt))
                        Put(e, L"ToolTip", alt);
                    break;
                }
            e.children.clear();          // <source> 等分辨率候选一律丢弃
            CommonAliases(e);
        }
        else if (t == L"canvas" || t == L"svg")
        {
            // 自绘画布：HTML 的 canvas/svg 正好对应 QCanvas（可挂 Python 绘制）。
            // 内部节点（<circle>/fallback 文本）不是控件，清空子树，交给画布自绘。
            e.tag = L"XQCanvas";
            e.children.clear();
            CommonAliases(e);
        }
        else if (t == L"video" || t == L"audio" || t == L"iframe" || t == L"embed" ||
                 t == L"object")
        {
            // 没有原生等价：占位成一个可放内容的 Pane，子元素照旧往里塞
            e.tag = L"XPane";
            CommonAliases(e);
        }
        else if (t == L"button")
        {
            e.tag = L"XButton";
            // 标记"这个 XButton 是 HTML 原生 <button> 变来的"：默认值表（圆角/浅灰底/灰框/居中）
            // 靠标签名区分原生与老写法，而标签名在上一步已被改写 —— 不留标记，
            // Util/Expr 里 `node.tag == L"button"` 分支永远不命中，默认外观整段是死代码。
            Put(e, L"HtmlButton", L"1");
            CommonAliases(e);
            // HTML 的 <button> 宽高都按内容：宽度 = 文字宽 + 左右默认 padding（CSS 约 10px/侧）+ 边框。
            // XML 早先不写 Width 就取 NaturalWidth(button)=100，短文案（"确定"/"取消"）
            // 在 HTML 里是紧凑按钮，在这里被拉成 100px 的大块 —— 观感同级化的直接障碍。
            // 显式 width（属性或 style="width:..."）优先，不覆盖；<XButton> 不受影响（那是老界面写法）。
            if (!e.get(L"Width").size())
            {
                std::wstring body = Trim(e.text);
                if (body.empty())
                    body = e.get(L"Text");
                const int tw = ApproxTextWidth(e, body);
                if (tw > 0)
                {
                    const int bw = (std::max)(48, tw + 20);
                    e.w = bw;
                    e.hasW = true;
                    e.autoW = true;   // 内容自然宽：网格单元格可拉伸，flex row 仍按内容宽
                    e.sugarW = true;  // 跨几何重算保留（autoW 会被 Expr 的重算清掉）
                    // 不能打 autoTextW 标记：flex 容器会把 autoTextW 的伪 Width 剥掉（那是 <label> 的语义），
                    // 而 HTML 里按钮在 flex row 中本来就是内容宽，不该被 align-items:stretch 拉满。
                    Put(e, L"Width", std::to_wstring(bw));
                }
            }
        }
        else if (t == L"progress" || t == L"meter")
        {
            e.tag = L"XProgressBar";
            if (e.has(L"min")) Put(e, L"Minimum", AttrOf(e, L"min"));
            if (e.has(L"max")) Put(e, L"Maximum", AttrOf(e, L"max"));
            if (hasHtmlValue && !htmlValue.empty())
                Put(e, L"Value", htmlValue);
            CommonAliases(e);
        }
        else if (t == L"code" || t == L"pre")
        {
            // 早先落成 XCodeEditor，但该控件从未注册，<code>/<pre> 会在加载时直接报
            // "未知控件 <XCodeEditor>"。浏览器里它们是等宽文本（code 行内、pre 块级保留折行），
            // 这里落 XLabel + monospace 族，宽度按内容（等价于 inline）。
            e.tag = L"XLabel";
            const std::wstring body = Trim(e.text);
            if (!body.empty() && !e.has(L"Text"))
                Put(e, L"Text", body);
            if (!e.has(L"FontFamily"))
                Put(e, L"FontFamily", L"Consolas");
            if (t == L"pre" && !e.has(L"TextWrapping"))
                Put(e, L"TextWrapping", L"Wrap");
            if (!e.get(L"Width").size())
            {
                const std::wstring shown = !body.empty() ? body : e.get(L"Text");
                const int tw = ApproxTextWidth(e, shown);
                if (tw > 0)
                {
                    e.w = tw;
                    e.hasW = true;
                    e.autoTextW = true;
                    Put(e, L"Width", std::to_wstring(tw));
                }
            }
            CommonAliases(e);
        }
        else if (t == L"fieldset" || t == L"details")
        {
            // legend / summary 变成分组标题
            e.tag = L"XGroupBox";
            for (const auto& c : e.children)
            {
                const std::wstring ct = Lower(c.tag);
                if (ct == L"legend" || ct == L"summary")
                {
                    const std::wstring body = Trim(c.text);
                    if (!body.empty())
                        Put(e, L"Header", body);
                    break;
                }
            }
            std::vector<XmlElement> kept;
            for (auto& c : e.children)
            {
                const std::wstring ct = Lower(c.tag);
                if (ct == L"legend" || ct == L"summary")
                    continue;
                kept.push_back(std::move(c));
            }
            e.children = std::move(kept);
            CommonAliases(e);
            ConvertChildren(e);
        }
        else if (IsTextNode(t))
        {
            e.tag = L"XLabel";
            // 同上：<label> 在这里被改写成 XLabel，默认值表的 `node.tag == L"label"`
            // 分支（内容定宽、不涂背板）必须靠标记才能命中。
            if (t == L"label")
                Put(e, L"HtmlLabel", L"1");
            const std::wstring body = Trim(e.text);
            if (!body.empty() && !e.has(L"Text"))
                Put(e, L"Text", body);
            // HTML 语义标签的默认样式：浏览器里 <b>/<del>/<u>/<h2> 不写 CSS 也有效果，
            // XML 早先要自己补 FontWeight / TextDecoration / FontSize，属于典型"更啰嗦"。
            // 只补作者没写的属性（显式 style="text-decoration:none" 仍然赢）。
            {
                const auto def = [&e](const wchar_t* k, const wchar_t* v) {
                    if (!e.has(k))
                        Put(e, k, v);
                };
                if (t == L"strong" || t == L"b")
                    def(L"FontWeight", L"Bold");
                else if (t == L"em" || t == L"i" || t == L"cite" || t == L"var")
                    def(L"FontStyle", L"Italic");
                else if (t == L"u" || t == L"ins")
                    def(L"TextDecoration", L"underline");
                else if (t == L"del" || t == L"s" || t == L"strike")
                    def(L"TextDecoration", L"line-through");
                else if (t == L"small")
                    def(L"FontSize", L"13");
                else if (t == L"mark")
                    def(L"Background", L"#ffff00");   // 浏览器 <mark> 默认黄色高亮
                else if (t == L"code" || t == L"kbd" || t == L"samp" || t == L"tt" ||
                         t == L"pre")
                    def(L"FontFamily", L"Consolas");  // 等宽族，对应浏览器 monospace 默认
                else if (t.size() == 2 && t[0] == L'h' && t[1] >= L'1' && t[1] <= L'6')
                {
                    // h1..h6 的浏览器默认：32/24/19/16/13/11px，全部加粗
                    static const wchar_t* kSize[] = {L"32", L"24", L"19", L"16", L"13", L"11"};
                    def(L"FontSize", kSize[t[1] - L'1']);
                    def(L"FontWeight", L"Bold");
                }
            }
            CommonAliases(e);
            ConvertChildren(e);   // <p>里套 <b> 这类
            // <label>/<span>/<strong>… 是行内元素：宽度由内容决定（HTML 语义）。
            // <XLabel> 不动 —— 老界面大量靠它铺满一行，改了会打散既有排版。
            if (IsInlineTextTag(t) && !e.get(L"Width").size())
            {
                const std::wstring shown = !body.empty() ? body : e.get(L"Text");
                const int tw = ApproxTextWidth(e, shown);
                if (tw > 0)
                {
                    // 必须写进属性：RecomputeGeometry 会按 Width 属性重算 hasW，
                    // 只落字段的话下一轮几何重算又会把它当成"没写宽度"重新铺满
                    e.w = tw;
                    e.hasW = true;
                    e.autoTextW = true;   // 标记：这是按内容测的，父若是 flex 容器会被剥掉
                    Put(e, L"Width", std::to_wstring(tw));
                }
            }
        }
        else // IsBlockContainer
        {
            if (e.children.empty() && !Trim(e.text).empty())
            {
                // <div>纯文本</div> 这种块里只有文字：直接当文本块
                e.tag = L"XLabel";
                if (!e.has(L"Text"))
                    Put(e, L"Text", Trim(e.text));
            }
            else if (_wcsicmp(e.get(L"Display").c_str(), L"grid") == 0 ||
                     _wcsicmp(e.get(L"Display").c_str(), L"inline-grid") == 0)
            {
                // CSS `display:grid`：块级容器改成网格容器，轨道来自
                // style 的 grid-template-*（ApplyInlineStyle 已落进 gridRows/gridCols）。
                e.tag = L"XGrid";
                e.flex = false;
                if (!e.has(L"Background") && !e.has(L"BackColor"))
                    Put(e, L"Background", L"transparent");
                if (e.gridCols.empty() && e.gridRows.empty())
                {
                    // 没写轨道时给一份单列兜底，避免所有子项叠在 (0,0)
                    e.gridRows.emplace_back(L"Height", L"auto");
                }
            }
            else
            {
                // div/section/article… 是块级容器：按 HTML 语义它就是一个 flex 容器
                // （display:block 时主轴竖排、display:flex 时主轴横排）。
                // 落成控件层容器 XFlexPanel，子项由容器自己排：
                // 显式 Width/Height 不会被交叉轴 stretch 拉满、gap/align-self 也都生效。
                e.tag = L"XFlexPanel";
                Put(e, L"Display", L"flex");
                // HTML 的 div 默认透明（不刷背板）。不声明就在绘制端垫父表面色，
                // 否则容器会拿主题底色 m_bg 铺一大块，跟页面底色差一层灰。
                if (!e.has(L"Background") && !e.has(L"BackColor"))
                    Put(e, L"Background", L"transparent");
                e.flex = true;
                e.flexDir = 2;                 // 默认竖排（block 流），class/style 可改横排
                e.horizontal = false;
                e.hasOrientation = true;
                Put(e, L"Flex", L"column");
                if (HasClass(e, L"row") || HasClass(e, L"inline") || HasClass(e, L"flex-row"))
                {
                    e.flexDir = 0;
                    e.horizontal = true;
                    e.hasOrientation = true;
                    Put(e, L"Flex", L"row");
                    Put(e, L"Orientation", L"Horizontal");
                }
                else if (HasClass(e, L"column"))
                {
                    e.flexDir = 2;
                    e.horizontal = false;
                    e.hasOrientation = true;
                    Put(e, L"Flex", L"column");
                    Put(e, L"Orientation", L"Vertical");
                }
            }
            CommonAliases(e);
            ConvertChildren(e);

            // HTML 里块容器的高度是"内容高度"，不是"撑满父容器"。
            // 子节点已经转好了，此时算自然高最准；只落字段不写属性，
            // 免得导出的 XML 里多出一堆假的 Height="…"。
            if (!e.hasH)
            {
                const int nh = xmlNaturalHeight(e);
                if (nh > 0)
                {
                    e.h = nh;
                    e.hasH = true;
                    e.autoH = true;      // 重算几何时保留这个"内容高度"
                }
            }
        }

        DropHtmlOnly(e);
    }
} // namespace htmlsugar

namespace xmlcore_impl
{
    // CSS 继承：color / font-family / font-size / font-weight / font-style / text-align /
    // line-height / letter-spacing / white-space / text-transform / text-indent 在浏览器里
    // 都是「继承属性」，写在父元素上子元素自动生效。XML 通道此前每条属性只作用于声明那一层，
    // 所以 <div style="text-align:right;color:#f00"><label Text="X"/></div> 里的文字永远
    // 左对齐、永远黑字 —— HTML 一行能表达的，XML 必须逐个子控件重写。
    const wchar_t* const kInheritableText[] = {
        L"TextAlign", L"FontSize", L"FontFamily", L"FontWeight", L"FontStyle",
        L"LineHeight", L"LetterSpacing", L"TextTransform", L"TextWrapping", L"TextIndent",
        // text-decoration 严格说不是"继承属性"，但 CSS 规定它会传播到所有在流后代——
        // 浏览器里 <div style="text-decoration:underline">标题</div> 的子树文字全带下划线。
        L"TextDecoration"
    };

    void ApplyTextInheritance(XmlElement& el,
                              std::vector<std::pair<std::wstring, std::wstring>>& inherited)
    {
        for (const wchar_t* k : kInheritableText)
        {
            const std::wstring v = el.get(k);
            if (v.empty())
                continue;
            // text-decoration:none 显式声明不叠加（也不往下传）
            if (_wcsicmp(k, L"TextDecoration") == 0 && _wcsicmp(v.c_str(), L"none") == 0)
            {
                inherited.erase(std::remove_if(inherited.begin(), inherited.end(),
                                               [](const std::pair<std::wstring, std::wstring>& kv) {
                                                   return _wcsicmp(kv.first.c_str(), L"TextDecoration") == 0;
                                               }),
                                inherited.end());
                continue;
            }
            bool replaced = false;
            for (auto& kv : inherited)
                if (_wcsicmp(kv.first.c_str(), k) == 0)
                {
                    kv.second = v;
                    replaced = true;
                    break;
                }
            if (!replaced)
                inherited.emplace_back(k, v);
        }
        // 颜色在属性层有两个名字（Foreground / Color，外观层等价），合并成一条继承项
        {
            const std::wstring fg = !el.get(L"Foreground").empty() ? el.get(L"Foreground")
                                                                  : el.get(L"Color");
            if (!fg.empty())
            {
                bool replaced = false;
                for (auto& kv : inherited)
                    if (_wcsicmp(kv.first.c_str(), L"color") == 0)
                    {
                        kv.second = fg;
                        replaced = true;
                        break;
                    }
                if (!replaced)
                    inherited.emplace_back(L"color", fg);
            }
        }
        for (auto& c : el.children)
        {
            for (const auto& kv : inherited)
            {
                if (_wcsicmp(kv.first.c_str(), L"color") == 0)
                {
                    // 子元素自己写了任一颜色别名就不再继承
                    if (c.get(L"Foreground").empty() && c.get(L"Color").empty())
                        c.set(L"Foreground", kv.second);
                    continue;
                }
                if (c.get(kv.first.c_str()).empty())
                    c.set(kv.first.c_str(), kv.second);
            }
            ApplyTextInheritance(c, inherited);
        }
    }

    // XML 老写法 <XStackPanel Display="flex" Flex="column"> 同样落成控件层 flex 容器：
    // 否则只有 HTML 写法在缩放时跟排，两种写法行为不一致（HTML 能、XML 不能）。
    // 只动显式声明 Display="flex" 的面板，未声明 flex 的老界面一个都不碰。
    void NormalizeFlexTags(XmlElement& el)
    {
        for (auto& c : el.children)
            NormalizeFlexTags(c);
        const bool isFlex = el.flex || _wcsicmp(el.get(L"Display").c_str(), L"flex") == 0;
        if (!isFlex)
            return;
        if (_wcsicmp(el.tag.c_str(), L"XStackPanel") == 0)
            el.tag = L"XFlexPanel";
    }
} // namespace

void xmlApplyHtmlSugar(XmlElement& root)
{
    // CSS 规则表：先在整棵树上收 <style> 的文字，再把命中的 .class 声明并进各节点的 style。
    // 必须在 ConvertNode 之前做 —— 糖化之后 <div> 已经变成 <XFlexPanel>，选择器的 tag 就对不上了。
    std::vector<htmlsugar::CssRule> cssRules;
    htmlsugar::CssKeyframeTable cssKeyframes;
    htmlsugar::HarvestCssRules(root, cssRules, cssKeyframes);
    if (!cssRules.empty() || !cssKeyframes.empty())
        htmlsugar::ApplyCssRules(root, cssRules, {}, &cssKeyframes);

    htmlsugar::ConvertNode(root);
    // CSS 继承属性（color / font-* / text-align / line-height …）自上而下下发：
    // 写一次父元素，子控件自动生效。默认开；PYARX_CSS_INHERIT=0 可关（回逐控件写法）。
    {
        std::vector<std::pair<std::wstring, std::wstring>> seed;
        ApplyTextInheritance(root, seed);
    }
    // <XStackPanel Display="flex"> 与 <div style="display:flex"> 等价，同样落成控件层 flex 容器：
    // 缩放时两版行为一致，才是"XML 与 HTML 同级"。默认开；PYARX_FLEX_NORM=0 可关（回老路径）。
    wchar_t normEnv[8] = {};
    const bool hasNormEnv = ::GetEnvironmentVariableW(L"PYARX_FLEX_NORM", normEnv, 8) != 0;
    const bool normOff = hasNormEnv &&
                         (normEnv[0] == L'0' || normEnv[0] == L'f' || normEnv[0] == L'F' ||
                          normEnv[0] == L'n' || normEnv[0] == L'N');
    if (!normOff)
        NormalizeFlexTags(root);
}
