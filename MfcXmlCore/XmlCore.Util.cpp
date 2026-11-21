// XmlCore.Engine.cpp —— 拆自 XmlCore.cpp 第 115-3494 行。
#include "XmlCoreCommon.h"
#include "XmlCoreInternal.h"

// ===========================================================================
// 工具
// ===========================================================================
namespace xmlcore_impl
{
    std::wstring XmlEscape(const std::wstring& s)
    {
        std::wstring out;
        for (wchar_t c : s)
        {
            switch (c)
            {
            case L'&': out += L"&amp;"; break;
            case L'<': out += L"&lt;"; break;
            case L'>': out += L"&gt;"; break;
            case L'"': out += L"&quot;"; break;
            case L'\'': out += L"&apos;"; break;
            default: out += c; break;
            }
        }
        return out;
    }

    std::wstring Attr(IXMLDOMElement* element, const wchar_t* name)
    {
        VARIANT value;
        VariantInit(&value);
        BSTR key = ::SysAllocString(name);
        element->getAttribute(key, &value);
        ::SysFreeString(key);
        std::wstring result;
        if (value.vt == VT_BSTR && value.bstrVal != nullptr)
            result = value.bstrVal;
        ::VariantClear(&value);
        return result;
    }

    int IntAttr(const std::wstring& s, int fallback)
    {
        if (s.empty())
            return fallback;
        return _wtoi(s.c_str());
    }

    // CSS 定位边值：`12` / `12px` / `10%`。百分比按父内框（base）换算，
    // 与 width/height 的百分比口径一致，避免 `inset:0 5%` 被 wcstol 读成 5px。
    int CssEdgePx(const std::wstring& s, int base, int fallback)
    {
        if (s.empty())
            return fallback;
        if (s.find(L'%') != std::wstring::npos)
            return (std::max)(0, static_cast<int>(base * _wtof(s.c_str()) / 100.0));
        return _wtoi(s.c_str());
    }

    // WPF 惯用写法 → 内部属性名的别名归一化。
    // 目的：让按 WPF 常识写的 XML 直接生效，而不是静默失效（我们踩过的坑）。
    // 只做"名字层"的映射，不改任何语义；值域必须与内部名一致。
    std::wstring WpfAlias(const std::wstring& a)
    {
        static const struct { const wchar_t* from; const wchar_t* to; } k[] = {
            { L"Canvas.Left",    L"Left" },         // 我们用 Left/Top（画布定位）
            { L"Canvas.Top",     L"Top" },
            { L"BorderBrush",    L"BorderColor" },   // WPF 叫 Brush，我们叫 Color
            // HorizontalAlignment 的值域 Left/Center/Right 与 TextAlign 完全一致，可直接借用；
            // Stretch 在 TextAlign 语义下无意义，会被当作未识别值（--check 会报）。
            { L"HorizontalAlignment", L"TextAlign" },
        };
        for (const auto& e : k)
            if (_wcsicmp(a.c_str(), e.from) == 0)
                return e.to;
        return a;
    }

    // ---------------- 布局缺省值（由 catalog 注入，避免 C++ 硬编码） ----------------
    std::wstring KindOf(const std::wstring& raw);   // 前置声明（定义在布局段）


    LayoutDefaults& layoutDefaults()
    {
        static LayoutDefaults d;
        return d;
    }

    void defaultSizeFor(const std::wstring& tag, int& w, int& h)
    {
        const std::wstring key = KindOf(tag);
        for (const auto& it : layoutDefaults().sizes)
        {
            if (it.first == key)
            {
                w = it.second.first;
                h = it.second.second;
                return;
            }
        }
        w = 120;   // 最终兜底：catalog 未注册该标签
        h = 28;
    }

    // 从属性名去掉 XML 命名空间前缀 {uri}Name -> Name
    std::wstring LocalName(const std::wstring& k)
    {
        const size_t pos = k.rfind(L'}');
        return pos == std::wstring::npos ? k : k.substr(pos + 1);
    }

    void ParseThickness(const std::wstring& text, XmlElement& node)
    {
        int values[4] = {0, 0, 0, 0};
        int count = 0;
        std::wstring current;
        for (wchar_t c : text)
        {
            if (c == L',' || c == L' ')
            {
                if (!current.empty() && count < 4)
                {
                    values[count++] = _wtoi(current.c_str());
                    current.clear();
                }
            }
            else
            {
                current.push_back(c);
            }
        }
        if (!current.empty() && count < 4)
            values[count++] = _wtoi(current.c_str());

        if (count == 1)
            node.marginL = node.marginT = node.marginR = node.marginB = values[0];
        else if (count == 2)
        {
            // CSS 两值顺序是「上下 左右」（`margin:8px 16px` = 上下 8、左右 16）。
            // 早先按「左右 上下」算，属性式写法与 style 写法排出来的位置正好对调，
            // 属于同一语义两种结果 —— 这里按 CSS 口径收敛。
            node.marginT = node.marginB = values[0];
            node.marginL = node.marginR = values[1];
        }
        else if (count == 3)
        {
            // CSS 三值：上 左右 下
            node.marginT = values[0];
            node.marginL = node.marginR = values[1];
            node.marginB = values[2];
        }
        else if (count >= 4)
        {
            node.marginL = values[0];
            node.marginT = values[1];
            node.marginR = values[2];
            node.marginB = values[3];
        }
    }

    // 把 "8" / "8 4" / "8,4" 解析成最多 4 个整数（flex 的 Gap 用）
    std::vector<int> ThicknessValues(const std::wstring& text)
    {
        std::vector<int> out;
        std::wstring cur;
        for (wchar_t c : text)
        {
            if (c == L',' || c == L' ' || c == L'\t')
            {
                if (!cur.empty())
                {
                    out.push_back(_wtoi(cur.c_str()));
                    cur.clear();
                }
            }
            else
                cur.push_back(c);
            if (out.size() >= 4)
                break;
        }
        if (!cur.empty() && out.size() < 4)
            out.push_back(_wtoi(cur.c_str()));
        return out;
    }

    // 解析 Padding（四值/两值/单值），写入 padL/T/R/B
    void ParsePadding(const std::wstring& text, XmlElement& node)
    {
        if (text.empty()) return;
        int values[4] = {0, 0, 0, 0};
        int count = 0;
        std::wstring current;
        for (wchar_t c : text)
        {
            if (c == L',' || c == L' ')
            {
                if (!current.empty() && count < 4)
                {
                    values[count++] = _wtoi(current.c_str());
                    current.clear();
                }
            }
            else
            {
                current.push_back(c);
            }
        }
        if (!current.empty() && count < 4)
            values[count++] = _wtoi(current.c_str());

        if (count == 1)
            node.padL = node.padT = node.padR = node.padB = values[0];
        else if (count == 2)
        {
            // CSS 两值顺序「上下 左右」：`Padding="8 16"` 与 `padding:8px 16px` 必须排成同一个位置。
            node.padT = node.padB = values[0];
            node.padL = node.padR = values[1];
        }
        else if (count == 3)
        {
            // CSS 三值：上 左右 下
            node.padT = values[0];
            node.padL = node.padR = values[1];
            node.padB = values[2];
        }
        else if (count >= 4)
        {
            node.padL = values[0];
            node.padT = values[1];
            node.padR = values[2];
            node.padB = values[3];
        }
    }

    std::wstring TagOf(IXMLDOMElement* e)
    {
        if (e == nullptr)
            return {};
        BSTR t = nullptr;
        e->get_tagName(&t);
        std::wstring s = t != nullptr ? t : L"";
        ::SysFreeString(t);
        return s;
    }

    std::wstring NameTail(const std::wstring& s)
    {
        const size_t colon = s.rfind(L':');
        const size_t brace = s.rfind(L'}');
        const size_t cut = (std::max)(colon == std::wstring::npos ? 0 : colon + 1,
                                      brace == std::wstring::npos ? 0 : brace + 1);
        return cut == 0 ? s : s.substr(cut);
    }

    // 按名字取属性（兼容 x:Key 这种带命名空间前缀的写法）
    std::wstring AttrByName(IXMLDOMElement* element, const wchar_t* name)
    {
        if (element == nullptr)
            return {};
        IXMLDOMNamedNodeMap* attrs = nullptr;
        element->get_attributes(&attrs);
        if (attrs == nullptr)
            return Attr(element, name);
        std::wstring result;
        long n = 0;
        attrs->get_length(&n);
        for (long i = 0; i < n; ++i)
        {
            IXMLDOMNode* an = nullptr;
            attrs->get_item(i, &an);
            if (an == nullptr)
                continue;
            BSTR nm = nullptr;
            an->get_nodeName(&nm);
            const std::wstring key = nm != nullptr ? nm : L"";
            ::SysFreeString(nm);
            VARIANT vv;
            VariantInit(&vv);
            an->get_nodeValue(&vv);
            const std::wstring val = (vv.vt == VT_BSTR && vv.bstrVal != nullptr) ? vv.bstrVal : L"";
            ::VariantClear(&vv);
            an->Release();
            if (_wcsicmp(NameTail(key).c_str(), NameTail(name).c_str()) == 0)
            {
                result = val;
                break;
            }
        }
        attrs->Release();
        return result;
    }

    void ParseDefs(IXMLDOMElement* element, bool rows, XmlElement& node)
    {
        IXMLDOMNodeList* children = nullptr;
        element->get_childNodes(&children);
        if (children == nullptr)
            return;

        const std::wstring defsTag = rows ? L"Grid.RowDefinitions" : L"Grid.ColumnDefinitions";
        const std::wstring itemTag = rows ? L"RowDefinition" : L"ColumnDefinition";
        const std::wstring sizeAttr = rows ? L"Height" : L"Width";

        long count = 0;
        children->get_length(&count);
        for (long i = 0; i < count; ++i)
        {
            IXMLDOMNode* n = nullptr;
            children->get_item(i, &n);
            if (n == nullptr)
                continue;
            DOMNodeType type = NODE_INVALID;
            n->get_nodeType(&type);
            IXMLDOMElement* child = nullptr;
            n->QueryInterface(IID_IXMLDOMElement, reinterpret_cast<void**>(&child));
            n->Release();
            if (type != NODE_ELEMENT || child == nullptr)
                continue;

            BSTR tagB = nullptr;
            child->get_tagName(&tagB);
            std::wstring childTag = tagB != nullptr ? tagB : L"";
            ::SysFreeString(tagB);
            if (_wcsicmp(xmlmfc::BareTag(childTag).c_str(), defsTag.c_str()) != 0)
            {
                child->Release();
                continue;
            }

            IXMLDOMNodeList* defs = nullptr;
            child->get_childNodes(&defs);
            child->Release();
            if (defs == nullptr)
                continue;

            long defCount = 0;
            defs->get_length(&defCount);
            auto& target = rows ? node.gridRows : node.gridCols;
            for (long d = 0; d < defCount; ++d)
            {
                IXMLDOMNode* dn = nullptr;
                defs->get_item(d, &dn);
                if (dn == nullptr)
                    continue;
                IXMLDOMElement* def = nullptr;
                dn->QueryInterface(IID_IXMLDOMElement, reinterpret_cast<void**>(&def));
                dn->Release();
                if (def == nullptr)
                    continue;
                BSTR dt = nullptr;
                def->get_tagName(&dt);
                std::wstring defTag = dt != nullptr ? dt : L"";
                ::SysFreeString(dt);
                if (_wcsicmp(xmlmfc::BareTag(defTag).c_str(), itemTag.c_str()) == 0)
                    target.emplace_back(sizeAttr, Attr(def, sizeAttr.c_str()));
                def->Release();
            }
            defs->Release();
        }
        children->Release();
    }

    using TemplateTable = std::vector<std::pair<std::wstring, std::vector<XmlElement>>>;

    // ---------- 资源字典跨文件合并（ResourceDictionary Source） 路径工具 ----------
    std::wstring PathDirOf(const std::wstring& path)
    {
        const size_t p = path.find_last_of(L"\\/");
        return p == std::wstring::npos ? std::wstring() : path.substr(0, p);
    }

    // 规范化绝对路径：统一分隔符、去掉末尾反斜杠
    std::wstring NormPath(const std::wstring& path)
    {
        wchar_t buf[MAX_PATH * 2] = {0};
        if (::GetFullPathNameW(path.c_str(), MAX_PATH * 2, buf, nullptr) == 0)
            return path;
        std::wstring s = buf;
        for (wchar_t& c : s)
            if (c == L'/')
                c = L'\\';
        while (s.size() > 3 && s.back() == L'\\')
            s.pop_back();
        return s;
    }

    // Source 解析：相对路径基于当前文档目录；绝对路径原样；pack:// 之类的 URI 不支持
    bool ResolveRefPath(const std::wstring& src, const std::wstring& baseDir, std::wstring& out)
    {
        if (src.empty())
            return false;
        if (src.rfind(L"pack:", 0) == 0 || src.rfind(L"ms-appx:", 0) == 0 ||
            src.find(L"://") != std::wstring::npos)
            return false;
        std::wstring s = src;
        for (wchar_t& c : s)
            if (c == L'/')
                c = L'\\';
        if (s.size() >= 2 && s[1] == L':' && (s[0] >= L'A' && s[0] <= L'Z' || s[0] >= L'a' && s[0] <= L'z'))
        {
            out = s;   // C:\...
            return true;
        }
        if (!s.empty() && s[0] == L'\\')
        {
            out = s;   // \server\share
            return true;
        }
        if (baseDir.empty())
            return false;
        out = baseDir + L"\\" + s;
        return true;
    }

    bool IsResourceContainerTag(const std::wstring& tag)
    {
        return tag == L"ResourceDictionary" || tag == L"MergedDictionaries" ||
               tag.find(L"Resources") != std::wstring::npos;
    }

    void CollectStyle(IXMLDOMElement* styleEl, std::vector<XmlStyle>& out)
    {
        XmlStyle st;
        st.key = AttrByName(styleEl, L"x:Key");
        if (st.key.empty())
            st.key = AttrByName(styleEl, L"Key");
        st.targetType = AttrByName(styleEl, L"TargetType");
        const std::wstring basedOn = AttrByName(styleEl, L"BasedOn");
        std::wstring bk;
        st.basedOn = xmlIsResourceRef(basedOn, bk) ? bk : basedOn;

        // <Trigger Property=".." Value=".."> 里的 Setter 收集到对应触发器
        auto collectTrigger = [&](IXMLDOMElement* trigEl) {
            XmlTrigger trig;
            trig.prop = AttrByName(trigEl, L"Property");
            trig.value = AttrByName(trigEl, L"Value");
            if (trig.prop.empty())
                return;
            IXMLDOMNodeList* tn = nullptr;
            trigEl->get_childNodes(&tn);
            if (tn != nullptr)
            {
                long tc = 0;
                tn->get_length(&tc);
                for (long t = 0; t < tc; ++t)
                {
                    IXMLDOMNode* tnode = nullptr;
                    tn->get_item(t, &tnode);
                    if (tnode == nullptr)
                        continue;
                    IXMLDOMElement* tele = nullptr;
                    tnode->QueryInterface(IID_IXMLDOMElement, reinterpret_cast<void**>(&tele));
                    tnode->Release();
                    if (tele != nullptr)
                    {
                        if (TagOf(tele) == L"Setter")
                        {
                            XmlSetter s;
                            s.prop = AttrByName(tele, L"Property");
                            s.value = AttrByName(tele, L"Value");
                            if (!s.prop.empty())
                                trig.setters.push_back(s);
                        }
                        tele->Release();
                    }
                }
                tn->Release();
            }
            st.triggers.push_back(trig);
        };

        IXMLDOMNodeList* kids = nullptr;
        styleEl->get_childNodes(&kids);
        if (kids != nullptr)
        {
            long n = 0;
            kids->get_length(&n);
            for (long i = 0; i < n; ++i)
            {
                IXMLDOMNode* kn = nullptr;
                kids->get_item(i, &kn);
                if (kn == nullptr)
                    continue;
                IXMLDOMElement* ke = nullptr;
                kn->QueryInterface(IID_IXMLDOMElement, reinterpret_cast<void**>(&ke));
                kn->Release();
                if (ke == nullptr)
                    continue;
                const std::wstring ktag = TagOf(ke);
                if (ktag == L"Setter")
                {
                    XmlSetter s;
                    s.prop = Attr(ke, L"Property");
                    if (s.prop.empty())
                        s.prop = Attr(ke, L"Name");
                    s.value = Attr(ke, L"Value");
                    if (!s.prop.empty())
                        st.setters.push_back(s);
                }
                else if (ktag == L"Trigger")
                {
                    collectTrigger(ke);
                }
                else if (ktag.find(L"Triggers") != std::wstring::npos)
                {
                    // <Style.Triggers> 容器：往里找 Trigger
                    IXMLDOMNodeList* gk = nullptr;
                    ke->get_childNodes(&gk);
                    if (gk != nullptr)
                    {
                        long gc = 0;
                        gk->get_length(&gc);
                        for (long g = 0; g < gc; ++g)
                        {
                            IXMLDOMNode* gnode = nullptr;
                            gk->get_item(g, &gnode);
                            if (gnode == nullptr)
                                continue;
                            IXMLDOMElement* gele = nullptr;
                            gnode->QueryInterface(IID_IXMLDOMElement, reinterpret_cast<void**>(&gele));
                            gnode->Release();
                            if (gele != nullptr)
                            {
                                if (TagOf(gele) == L"Trigger")
                                    collectTrigger(gele);
                                gele->Release();
                            }
                        }
                        gk->Release();
                    }
                }
                ke->Release();
            }
            kids->Release();
        }
        out.push_back(st);
    }


    // ---------- Repeater 模板克隆时的占位符替换 ----------
    std::wstring SubstAll(std::wstring s, const std::wstring& key, const std::wstring& val)
    {
        size_t p = 0;
        while ((p = s.find(key, p)) != std::wstring::npos)
        {
            s.replace(p, key.size(), val);
            p += val.size();
        }
        return s;
    }

    void SubstDom(IXMLDOMElement* el, int index, int count, const std::wstring& item, int top)
    {
        if (el == nullptr)
            return;
        const std::wstring idx = std::to_wstring(index);
        const std::wstring idx1 = std::to_wstring(index + 1);
        const std::wstring cnt = std::to_wstring(count);
        const std::wstring topS = std::to_wstring(top);

        IXMLDOMNamedNodeMap* attrs = nullptr;
        el->get_attributes(&attrs);
        if (attrs != nullptr)
        {
            long n = 0;
            attrs->get_length(&n);
            for (long i = 0; i < n; ++i)
            {
                IXMLDOMNode* att = nullptr;
                attrs->get_item(i, &att);
                if (att == nullptr)
                    continue;
                VARIANT vv;
                VariantInit(&vv);
                att->get_nodeValue(&vv);
                std::wstring val = (vv.vt == VT_BSTR && vv.bstrVal != nullptr) ? vv.bstrVal : L"";
                ::VariantClear(&vv);
                std::wstring out = val;
                out = SubstAll(out, L"{item}", item);
                out = SubstAll(out, L"{count}", cnt);
                out = SubstAll(out, L"{index1}", idx1);
                out = SubstAll(out, L"{index}", idx);
                out = SubstAll(out, L"{top}", topS);
                if (out != val)
                {
                    VARIANT nv;
                    VariantInit(&nv);
                    nv.vt = VT_BSTR;
                    nv.bstrVal = ::SysAllocString(out.c_str());
                    att->put_nodeValue(nv);
                    ::VariantClear(&nv);
                }
                att->Release();
            }
            attrs->Release();
        }

        IXMLDOMNodeList* kids = nullptr;
        el->get_childNodes(&kids);
        if (kids == nullptr)
            return;
        long kc = 0;
        kids->get_length(&kc);
        for (long i = 0; i < kc; ++i)
        {
            IXMLDOMNode* kid = nullptr;
            kids->get_item(i, &kid);
            if (kid == nullptr)
                continue;
            IXMLDOMElement* ke = nullptr;
            kid->QueryInterface(IID_IXMLDOMElement, reinterpret_cast<void**>(&ke));
            kid->Release();
            if (ke != nullptr)
            {
                SubstDom(ke, index, count, item, top);
                ke->Release();
            }
        }
        kids->Release();
    }

    // 行盒随字号长高：<h1>(32px) / <label style="font-size:34px"> 不写 Height 时，
    // 早先一律按控件目录的默认 24 定高 —— 字形上下被裁、背板比字还矮，
    // 而 HTML 的行盒天然跟着 font-size 走（约 1.5 倍行距）。
    // 放在 ParseElement 收尾统一做，绕开各默认值块互相覆盖（它们会把 24 写回去）。
    // 换行后的行盒高：按 GDI 的 DT_CALCRECT|DT_WORDBREAK 量（浏览器也是按盒宽换行后再定高）。
    // 只做高度用，不参与绘制；量不出来（无 DC）返回 0 由调用方退回单行口径。
    static int MeasureWrappedHeight(const std::wstring& s, int fs, int width, bool bold,
                                    const std::wstring& family)
    {
        if (s.empty() || fs <= 0 || width <= 0)
            return 0;
        HDC dc = ::GetDC(nullptr);
        if (dc == nullptr)
            return 0;
        LOGFONTW lf;
        ZeroMemory(&lf, sizeof(lf));
        lf.lfHeight = -fs;
        lf.lfWeight = bold ? FW_BOLD : FW_NORMAL;
        lf.lfCharSet = DEFAULT_CHARSET;
        lf.lfQuality = CLEARTYPE_QUALITY;
        wcsncpy_s(lf.lfFaceName, family.empty() ? L"Microsoft YaHei" : family.c_str(), _TRUNCATE);
        HFONT fnt = ::CreateFontIndirectW(&lf);
        HGDIOBJ oldF = ::SelectObject(dc, fnt);
        RECT rc;
        rc.left = 0;
        rc.top = 0;
        rc.right = width;
        rc.bottom = 0;
        ::DrawTextW(dc, s.c_str(), (int)s.size(), &rc,
                    DT_CALCRECT | DT_WORDBREAK | DT_LEFT | DT_NOPREFIX);
        const int h = rc.bottom - rc.top;
        ::SelectObject(dc, oldF);
        ::DeleteObject(fnt);
        ::ReleaseDC(nullptr, dc);
        return h;
    }

    // 收集 `<select>` 下的 `<option>`（含 `<optgroup>` 里包着的那批），拼成 Items="a|b"。
    static void CollectSelectOptions(IXMLDOMElement* el, std::wstring& items)
    {
        if (el == nullptr)
            return;
        IXMLDOMNodeList* lk = nullptr;
        el->get_childNodes(&lk);
        if (lk == nullptr)
            return;
        long ln = 0;
        lk->get_length(&ln);
        for (long i = 0; i < ln; ++i)
        {
            IXMLDOMNode* kn = nullptr;
            lk->get_item(i, &kn);
            if (kn == nullptr)
                continue;
            DOMNodeType kt = NODE_INVALID;
            kn->get_nodeType(&kt);
            IXMLDOMElement* ke = nullptr;
            kn->QueryInterface(IID_IXMLDOMElement, reinterpret_cast<void**>(&ke));
            kn->Release();
            if (kt != NODE_ELEMENT || ke == nullptr)
                continue;
            BSTR tn = nullptr;
            ke->get_tagName(&tn);
            const std::wstring tg = tn != nullptr ? tn : L"";
            ::SysFreeString(tn);
            if (_wcsicmp(tg.c_str(), L"optgroup") == 0)
            {
                // 分组标签本身不是选项，往下挖一层
                CollectSelectOptions(ke, items);
            }
            else if (_wcsicmp(tg.c_str(), L"option") == 0)
            {
                std::wstring v = AttrByName(ke, L"value");
                if (v.empty())
                    v = AttrByName(ke, L"Text");
                if (v.empty())
                {
                    BSTR body = nullptr;
                    ke->get_text(&body);
                    v = body != nullptr ? body : L"";
                    ::SysFreeString(body);
                    const size_t a = v.find_first_not_of(L" \t\r\n");
                    const size_t b = v.find_last_not_of(L" \t\r\n");
                    v = (a == std::wstring::npos) ? std::wstring() : v.substr(a, b - a + 1);
                }
                if (!v.empty())
                {
                    if (!items.empty())
                        items += L'|';
                    items += v;
                }
            }
            ke->Release();
        }
        lk->Release();
    }

    void FitTextHeight(XmlElement& node)
    {
        // 不考虑"谁写了 Height"：只要当前高度装不下字号行盒就抬到 HTML 行距
        int fs = 0;
        const std::wstring fsz = node.get(L"FontSize");
        if (!fsz.empty())
            fs = (int)wcstol(fsz.c_str(), nullptr, 10);
        // 行内 style 里的 font-size 由糖化层落属性，时机可能晚于本函数；
        // 先退一步自己从 Style 原文里取，避免标签只剩默认 24 高。
        if (fs <= 0)
        {
            const std::wstring sty = node.get(L"Style");
            const size_t pos = sty.find(L"font-size");
            if (pos != std::wstring::npos)
            {
                size_t i = pos + 9;
                while (i < sty.size() && (sty[i] < L'0' || sty[i] > L'9') && sty[i] != L'.')
                    ++i;
                fs = (int)wcstol(sty.c_str() + i, nullptr, 10);
            }
        }
        std::wstring bare = node.tag;
        const size_t dot = bare.rfind(L'.');
        if (dot != std::wstring::npos)
            bare = bare.substr(dot + 1);
        std::wstring low = bare;
        for (wchar_t& c : low) c = (wchar_t)towlower(c);
        const bool heading = low.size() == 2 && low[0] == L'h' && low[1] >= L'1' && low[1] <= L'6';
        if (fs <= 0 && heading)
        {
            static const int kH[6] = {32, 24, 19, 16, 13, 11};
            fs = kH[low[1] - L'1'];
        }
        if (fs <= 0)
            fs = 13;   // HTML 的默认正文尺寸，作者不写 font-size 时浏览器就是 16px 级的正文行盒
        const bool texty = low == L"label" || low == L"textblock" || low == L"span" ||
                           low == L"p" || low == L"xlabel" || heading;
        if (!texty || fs <= 0)
            return;
        int linePx = (int)(fs * 1.5);
        double lhVal = 0.0;
        bool lhMult = false;
        std::wstring lhRaw = node.get(L"LineHeight");
        if (lhRaw.empty())
        {
            // 行内 style 的 line-height 与 font-size 一样，落属性的时机晚于本函数
            const std::wstring sty = node.get(L"Style");
            const size_t pos = sty.find(L"line-height");
            if (pos != std::wstring::npos)
            {
                size_t i = pos + 11;
                while (i < sty.size() && (sty[i] < L'0' || sty[i] > L'9') && sty[i] != L'.')
                    ++i;
                size_t j = i;
                while (j < sty.size() && ((sty[j] >= L'0' && sty[j] <= L'9') || sty[j] == L'.' ||
                                          sty[j] == L'%' || sty[j] == L'p' || sty[j] == L'x'))
                    ++j;
                lhRaw = sty.substr(i, j - i);
            }
        }
        if (xmlapp::ResolveLineHeight(lhRaw, lhVal, lhMult))
            linePx = lhMult ? (int)(fs * lhVal + 0.5) : (int)(lhVal + 0.5);
        int need = linePx + node.padT + node.padB;
        // 多行：盒宽定死时文字会折行，盒子必须按折行结果长高（HTML 的块容器行为）
        const int contentW = node.w - node.padL - node.padR;
        const bool bold = _wcsicmp(node.get(L"FontWeight").c_str(), L"Bold") == 0;
        // 只有真的开了折行才按折行结果定高：宿主单行绘制时盒子长高只会把字顶到中间
        const std::wstring tw = node.get(L"TextWrapping");
        const bool wrap = !tw.empty() && _wcsicmp(tw.c_str(), L"NoWrap") != 0;
        const int mh = wrap ? MeasureWrappedHeight(node.text, fs, contentW, bold, node.get(L"FontFamily")) : 0;
        if (mh > 0 && mh + node.padT + node.padB > need)
            need = mh + node.padT + node.padB;
        if (need > node.h)
            node.h = need;
        node.hasH = true;
        node.set(L"Height", std::to_wstring(node.h));
    }

    void ParseElement(IXMLDOMElement* element, XmlElement& node,
                      std::vector<XmlStyle>* styles, TemplateTable* templates,
                      const std::wstring& baseDir,
                      std::vector<std::wstring>* resStack)
    {
        BSTR tagName = nullptr;
        element->get_tagName(&tagName);
        node.tag = tagName != nullptr ? tagName : L"";
        ::SysFreeString(tagName);

        // Loader：整个节点换成 Control 指定的标签；写了子元素时以子模板为准。
        // 必须放在属性解析之前，否则 Control/Canvas.Left 的语义换算会落在旧标签上。
        if (_wcsicmp(node.tag.c_str(), L"Loader") == 0)
        {
            std::wstring ctl = AttrByName(element, L"Control");
            if (ctl.empty())
                ctl = AttrByName(element, L"TargetType");
            IXMLDOMNodeList* lk = nullptr;
            element->get_childNodes(&lk);
            bool usedKid = false;
            if (lk != nullptr)
            {
                long ln = 0;
                lk->get_length(&ln);
                for (long li = 0; li < ln && !usedKid; ++li)
                {
                    IXMLDOMNode* kn = nullptr;
                    lk->get_item(li, &kn);
                    if (kn == nullptr)
                        continue;
                    DOMNodeType kt = NODE_INVALID;
                    kn->get_nodeType(&kt);
                    IXMLDOMElement* ke = nullptr;
                    kn->QueryInterface(IID_IXMLDOMElement, reinterpret_cast<void**>(&ke));
                    kn->Release();
                    if (kt != NODE_ELEMENT || ke == nullptr)
                        continue;
                    usedKid = true;
                    ParseElement(ke, node, styles, templates, baseDir, resStack);
                    ke->Release();
                }
                lk->Release();
            }
            if (usedKid)
                return;
            if (ctl.empty())
                ctl = L"Button";
            node.tag = ctl;
        }

        // 资源扫描：递归收集 <Style> / <ControlTemplate>，可嵌在任意 *Resources 容器里。
        // 资源容器上写 Source="xxx.xml" 时，把目标文档的**资源**合并进来（控件节点不合并）。
        std::function<void(const std::wstring&)> mergeResFile;
        std::function<void(IXMLDOMElement*)> scanRes = [&](IXMLDOMElement* resEl) {
            if (resEl == nullptr)
                return;
            const std::wstring rtag = xmlmfc::BareTag(TagOf(resEl));
            if (IsResourceContainerTag(rtag))
            {
                const std::wstring src = AttrByName(resEl, L"Source");
                if (!src.empty())
                    mergeResFile(src);
            }
            if (rtag == L"Style")
            {
                if (styles != nullptr)
                    CollectStyle(resEl, *styles);
                return;
            }
            if (rtag == L"ControlTemplate")
            {
                if (templates == nullptr)
                    return;
                std::wstring key = AttrByName(resEl, L"x:Key");
                if (key.empty())
                    key = AttrByName(resEl, L"Key");
                if (key.empty())
                    return;
                std::vector<XmlElement> nodes;
                IXMLDOMNodeList* tn = nullptr;
                resEl->get_childNodes(&tn);
                if (tn != nullptr)
                {
                    long tc = 0;
                    tn->get_length(&tc);
                    for (long ti = 0; ti < tc; ++ti)
                    {
                        IXMLDOMNode* tnode = nullptr;
                        tn->get_item(ti, &tnode);
                        if (tnode == nullptr)
                            continue;
                        DOMNodeType ttype = NODE_INVALID;
                        tnode->get_nodeType(&ttype);
                        IXMLDOMElement* tele = nullptr;
                        tnode->QueryInterface(IID_IXMLDOMElement, reinterpret_cast<void**>(&tele));
                        tnode->Release();
                        if (ttype == NODE_ELEMENT && tele != nullptr)
                        {
                            nodes.emplace_back();
                            ParseElement(tele, nodes.back(), styles, templates, baseDir, resStack);
                            tele->Release();
                        }
                    }
                    tn->Release();
                }
                (*templates).emplace_back(key, nodes);
                return;
            }

            // 其余（Window.Resources / Resources / 嵌套容器）继续往下扫
            IXMLDOMNodeList* kids = nullptr;
            resEl->get_childNodes(&kids);
            if (kids == nullptr)
                return;
            long n = 0;
            kids->get_length(&n);
            for (long i = 0; i < n; ++i)
            {
                IXMLDOMNode* kn = nullptr;
                kids->get_item(i, &kn);
                if (kn == nullptr)
                    continue;
                DOMNodeType ktype = NODE_INVALID;
                kn->get_nodeType(&ktype);
                IXMLDOMElement* ke = nullptr;
                kn->QueryInterface(IID_IXMLDOMElement, reinterpret_cast<void**>(&ke));
                kn->Release();
                if (ktype == NODE_ELEMENT && ke != nullptr)
                {
                    scanRes(ke);
                    ke->Release();
                }
            }
            kids->Release();
        };

        // 合并另一个文件的资源表：只取 Style / ControlTemplate，绝不引入控件节点。
        // resStack 保存正在加载的文件链，用来挡住循环引用与自引用。
        mergeResFile = [&](const std::wstring& src) {
            std::wstring abs;
            if (!ResolveRefPath(src, baseDir, abs))
                return;
            const std::wstring norm = NormPath(abs);
            if (resStack != nullptr)
            {
                for (const auto& s : *resStack)
                    if (_wcsicmp(s.c_str(), norm.c_str()) == 0)
                        return;   // 已在本链上：循环引用，直接跳过
            }
            IXMLDOMDocument2* doc = nullptr;
            if (FAILED(::CoCreateInstance(__uuidof(DOMDocument60), nullptr, CLSCTX_INPROC_SERVER,
                                          IID_IXMLDOMDocument2, reinterpret_cast<void**>(&doc)) ||
                       doc == nullptr))
                return;
            doc->put_async(VARIANT_FALSE);
            doc->put_validateOnParse(VARIANT_FALSE);
            VARIANT_BOOL ok = VARIANT_FALSE;
            doc->load(_variant_t(norm.c_str()), &ok);
            if (ok != VARIANT_TRUE)
            {
                doc->Release();
                return;
            }
            IXMLDOMElement* resRoot = nullptr;
            doc->get_documentElement(&resRoot);
            if (resRoot != nullptr)
            {
                if (resStack != nullptr)
                    resStack->push_back(norm);

                if (IsResourceContainerTag(TagOf(resRoot)))
                {
                    scanRes(resRoot);   // 纯资源字典文件：整棵扫
                }
                else
                {
                    // 普通文档（Window / Page / UserControl）：只取其中的 *Resources 容器
                    IXMLDOMNodeList* rk = nullptr;
                    resRoot->get_childNodes(&rk);
                    if (rk != nullptr)
                    {
                        long rn = 0;
                        rk->get_length(&rn);
                        for (long ri = 0; ri < rn; ++ri)
                        {
                            IXMLDOMNode* rnode = nullptr;
                            rk->get_item(ri, &rnode);
                            if (rnode == nullptr)
                                continue;
                            DOMNodeType rtype = NODE_INVALID;
                            rnode->get_nodeType(&rtype);
                            IXMLDOMElement* rel = nullptr;
                            rnode->QueryInterface(IID_IXMLDOMElement, reinterpret_cast<void**>(&rel));
                            rnode->Release();
                            if (rtype == NODE_ELEMENT && rel != nullptr)
                            {
                                if (IsResourceContainerTag(TagOf(rel)))
                                    scanRes(rel);
                                rel->Release();
                            }
                        }
                        rk->Release();
                    }
                }

                if (resStack != nullptr && !resStack->empty())
                    resStack->pop_back();
                resRoot->Release();
            }
            doc->Release();
        };

        // 采集全部属性到 attrs（保留顺序与原始名）
        IXMLDOMNamedNodeMap* attrs = nullptr;
        element->get_attributes(&attrs);
        if (attrs != nullptr)
        {
            long ac = 0;
            attrs->get_length(&ac);
            for (long ai = 0; ai < ac; ++ai)
            {
                IXMLDOMNode* an = nullptr;
                attrs->get_item(ai, &an);
                if (an == nullptr)
                    continue;
                BSTR aname = nullptr;
                an->get_nodeName(&aname);
                std::wstring ak = aname != nullptr ? aname : L"";
                ::SysFreeString(aname);
                VARIANT vv;
                VariantInit(&vv);
                an->get_nodeValue(&vv);
                std::wstring v = (vv.vt == VT_BSTR && vv.bstrVal != nullptr) ? vv.bstrVal : L"";
                ::VariantClear(&vv);
                node.attrs.emplace_back(WpfAlias(ak), v);
                an->Release();
            }
            attrs->Release();
        }

        // 语义字段
        node.name = node.get(L"x:Name");
        if (node.name.empty())
            node.name = node.get(L"Name");
        node.text = node.get(L"Text");
        if (node.text.empty())
            node.text = node.get(L"Title");     // Title = 显示标题（Name 为控件标识）
        if (node.text.empty())
            node.text = node.get(L"Content");   // Popup 的正文
        if (node.text.empty())
            node.text = node.get(L"Header");

        // 文本即内容（HTML 式写法）：<XLabel>标题</XLabel> 等价于 Text="标题"。
        // 只看"第一个子节点且是纯文本"，避免把容器的后代文本全吸进来。
        if (node.text.empty())
        {
            IXMLDOMNode* firstChild = nullptr;
            element->get_firstChild(&firstChild);
            if (firstChild != nullptr)
            {
                DOMNodeType nt = NODE_ELEMENT;
                firstChild->get_nodeType(&nt);
                if (nt == NODE_TEXT || nt == NODE_CDATA_SECTION)
                {
                    VARIANT cv;
                    VariantInit(&cv);
                    firstChild->get_nodeValue(&cv);
                    if (cv.vt == VT_BSTR && cv.bstrVal != nullptr)
                    {
                        // 折掉换行/制表/连续空格，剩下就是控件文本。
                        // 例外：textarea / pre / code 里的换行是内容的一部分，必须留着。
                        std::wstring rawTag = node.tag;
                        for (wchar_t& c : rawTag)
                            c = (wchar_t)::towlower(c);
                        const bool keepBreaks = rawTag == L"textarea" || rawTag == L"pre" ||
                                                rawTag == L"code";
                        std::wstring out;
                        if (keepBreaks)
                        {
                            const std::wstring rawText(cv.bstrVal);
                            const size_t b = rawText.find_first_not_of(L" \t\r\n");
                            if (b != std::wstring::npos)
                            {
                                const size_t e2 = rawText.find_last_not_of(L" \t\r\n");
                                for (size_t i = b; i <= e2; ++i)
                                {
                                    if (rawText[i] == L'\r')
                                        continue;         // 统一成 \n，别留 \r\n
                                    out += rawText[i];
                                }
                            }
                        }
                        else
                        {
                            bool pendingSpace = false;
                            for (const wchar_t c : std::wstring(cv.bstrVal))
                            {
                                if (c == L'\r' || c == L'\n' || c == L'\t' || c == L' ')
                                {
                                    pendingSpace = true;
                                    continue;
                                }
                                if (pendingSpace && !out.empty())
                                    out += L' ';
                                pendingSpace = false;
                                out += c;
                            }
                        }
                        if (!out.empty())
                        {
                            node.text = out;
                            // 落成一条本地 Text 声明，下游属性下发走同一条路（不再单独补丁）
                            node.attrs.emplace_back(L"Text", out);
                        }
                    }
                    ::VariantClear(&cv);
                }
                firstChild->Release();
            }
        }
        node.icon = node.get(L"Icon");   // 图标路径（res/xxx.png，相对 exe 目录）

        node.x = IntAttr(node.get(L"Left"), 0);
        node.y = IntAttr(node.get(L"Top"), 0);
        node.hasX = node.has(L"Left");
        node.hasY = node.has(L"Top");
        {
            // 缺省尺寸来自 control_catalog.xml（未注册的标签才用最终兜底）
            int dw = 100, dh = 28;
            defaultSizeFor(node.tag, dw, dh);
            // Width/Height="Auto" 按 WPF 语义 = "不声明尺寸，交给布局算"。
            // 不能让 _wtoi("Auto") 得到 0，否则控件会塌成 0 像素。
            const std::wstring wAttr = node.get(L"Width");
            const std::wstring hAttr = node.get(L"Height");
            const bool wAuto = AttrIsAuto(wAttr);
            const bool hAuto = AttrIsAuto(hAttr);
            // 属性级百分比：Width="100%" 与 style="width:100%" 等价（CSS 写法）。
            // 旧实现是 IntAttr(wAttr) → wcstol，会把 "100%" 读成 100px。
            // 50% 这类真百分比要按父尺寸换算，这里只记 pct，布局阶段再用。
            const bool wPct = !wAttr.empty() && wAttr.find(L'%') != std::wstring::npos;
            const bool hPct = !hAttr.empty() && hAttr.find(L'%') != std::wstring::npos;
            if (wPct)
            {
                node.pctW = static_cast<float>(_wtof(wAttr.c_str()));
                node.fillW = node.pctW >= 100.0f;
            }
            if (hPct)
            {
                node.pctH = static_cast<float>(_wtof(hAttr.c_str()));
                node.fillH = node.pctH >= 100.0f;
            }
            node.hasW = !wAttr.empty() && !wAuto && !wPct;
            // autoH = 高度由内容算出来的（HTML 块容器）：重算几何时不能被当成"没写高度"
            node.hasH = (!hAttr.empty() && !hAuto && !hPct) || (node.autoH && node.h > 0);
            node.w = (wAttr.empty() || wAuto || wPct) ? dw : IntAttr(wAttr, dw);
            node.h = node.autoH
                ? node.h
                : ((hAttr.empty() || hAuto || hPct) ? dh : IntAttr(hAttr, dh));
            // CSS box-sizing:border-box —— 浏览器里 `*{box-sizing:border-box}` 是最常见的写法，
            // 它让 width:200px 指的"含 padding 的外框"是 200。XML 缺省是 content-box（和 CSS 默认一致），
            // 写了 border-box 才把 padding 从声明尺寸里扣掉，外框才等于声明值。
            {
                const std::wstring bs = node.get(L"BoxSizing");
                if (!bs.empty() && (_wcsicmp(bs.c_str(), L"border-box") == 0 ||
                                    _wcsicmp(bs.c_str(), L"borderbox") == 0))
                {
                    if (!node.boxSizingBorder)
                    {
                        node.boxSizingBorder = true;
                        if (node.hasW)
                            node.w = (std::max)(0, node.w - node.padL - node.padR);
                        if (node.hasH)
                            node.h = (std::max)(0, node.h - node.padT - node.padB);
                    }
                }
            }
            // HTML 默认值表：<button>/<input> 不写尺寸时按控件自身尺寸，不能被父容器拉满
            // （HTML 里它们是 inline-block，宽度由内容决定；XML 以前不写 Width 就会铺满一行）
            {
                const std::wstring kind = KindOf(node.tag);
                const bool htmlBtn = (kind == L"button");
                // 只有 HTML 标签写法（原文小写 <button>）才按内容定宽；
                // <XButton> 是老界面写法，保持既有 88/定宽行为不变
                const bool htmlTagBtn = (node.tag == L"button" || node.has(L"HtmlButton"));
                const bool htmlInput = (kind == L"edit" || kind == L"textfield" || kind == L"input");
                // HTML 的 <label> 是 inline 元素：宽度由文字决定，不加默认 padding。
                // 只对原文小写的 <label> 生效 —— <XLabel> 有 40 处没写 Width，
                // 直接改会打散一半老界面排版，属于"别名只映射、不加控件实现"的边界。
                const bool htmlLabel = (node.tag == L"label" || node.has(L"HtmlLabel"));
                // textarea/select 的 UA 默认与 input 同批补，必须进这一块
                const bool htmlField = node.has(L"HtmlTextarea") || node.has(L"HtmlSelect") ||
                                       _wcsicmp(node.tag.c_str(), L"textarea") == 0 ||
                                       _wcsicmp(node.tag.c_str(), L"select") == 0;
                if (htmlBtn || htmlInput || htmlLabel || htmlField)
                {
                    // HTML 原生 <button> 的默认外观：浏览器给的是圆角（现代浏览器 6px 左右）
                    // + 浅色底 + 边框 + 文字居中。XML 早先不写 Radius 就是直角，
                    // 同一段界面 HTML/XML 并排一眼能看出差别。这里补成默认值，
                    // 显式写了 border-radius / Radius 的元素（含 0）一律不覆盖。
                    if (htmlTagBtn && !node.has(L"Radius") && !node.has(L"radius") &&
                        !node.has(L"border-radius") && !node.has(L"BorderRadius"))
                        node.set(L"Radius", L"6");
                    // HTML 原生 <button> 的默认"背板"：浏览器 UA 给的是浅灰底（#efefef）
                    // + 2px 灰框 + 黑字。XML 早先只补了圆角，底色/边框/字色全空 —— 于是
                    // 同一段界面里 HTML 的按钮是"一块浅灰按钮"，XML 的按钮只剩浮在背景上的
                    // 几个字，必须手写 background/border/color 才追平，这是最典型的"更啰嗦"。
                    // 只对原文小写的 <button> 生效，显式写了任一项都不覆盖。
                    if (htmlTagBtn && !node.has(L"Background") && !node.has(L"background") &&
                        !node.has(L"BackColor") && !node.has(L"BgColor"))
                        node.set(L"Background", L"#efefef");
                    if (htmlTagBtn && !node.has(L"Foreground") && !node.has(L"Color") &&
                        !node.has(L"color"))
                        node.set(L"Foreground", L"#111111");
                    if (htmlTagBtn && !node.has(L"BorderThickness") && !node.has(L"BorderWidth") &&
                        !node.has(L"border") && !node.has(L"BorderSize"))
                    {
                        node.set(L"BorderThickness", L"2");
                        if (!node.has(L"BorderColor") && !node.has(L"border-color"))
                            node.set(L"BorderColor", L"#767676");
                    }
                    // HTML 原生 <input> 的默认外观：单行、1px 边框、4px 圆角。
                    // 早先不写就是直角无边框，和浏览器观感差一截；显式声明一律不覆盖。
                    if (htmlInput && !node.has(L"Radius") && !node.has(L"radius") &&
                        !node.has(L"border-radius") && !node.has(L"BorderRadius"))
                        node.set(L"Radius", L"4");
                    if (htmlInput && !node.has(L"BorderThickness") && !node.has(L"BorderWidth") &&
                        !node.has(L"border") && !node.has(L"BorderSize"))
                        node.set(L"BorderThickness", L"1");
                    // 边框颜色同批补上：只给宽度不给颜色，浏览器里那圈灰框就画不出来
                    if ((node.tag == L"input" || node.has(L"HtmlInput")) &&
                        !node.has(L"BorderColor") && !node.has(L"border-color"))
                        node.set(L"BorderColor", L"#767676");
                    // 输入框的 UA 白底 + 文字内缩：浏览器不写样式也是"白底浅框 + 字不贴边"，
                    // 早先只有边框和圆角，深色面板上写 <input> 会跟背景糊成一块。
                    if ((node.tag == L"input" || node.has(L"HtmlInput")) &&
                        !node.has(L"Background") && !node.has(L"background") &&
                        !node.has(L"BackColor") && !node.has(L"BgColor"))
                        node.set(L"Background", L"#ffffff");
                    if ((node.tag == L"input" || node.has(L"HtmlInput")) &&
                        !node.has(L"Padding") && !node.has(L"padding"))
                        node.set(L"Padding", L"2 8");
                    // input 同理：白底浅字等于看不见，UA 字色补成 #111111
                    if ((node.tag == L"input" || node.has(L"HtmlInput")) &&
                        !node.has(L"Foreground") && !node.has(L"Color") &&
                        !node.has(L"color") && !node.has(L"ForeColor"))
                        node.set(L"Foreground", L"#111111");
                    // <textarea>/<select> 与 <input> 是同一类"白底浅框"的表单件：
                    // 浏览器不写样式时三者观感一致，XML 早先只有 input 有默认，
                    // 同一张表单里 select 是主题灰块、textarea 是一块无框空白，明显更丑。
                    const bool htmlArea = node.has(L"HtmlTextarea") ||
                                          _wcsicmp(node.tag.c_str(), L"textarea") == 0;
                    const bool htmlSel = node.has(L"HtmlSelect") ||
                                         _wcsicmp(node.tag.c_str(), L"select") == 0;
                    if (htmlArea || htmlSel)
                    {
                        if (!node.has(L"Radius") && !node.has(L"radius") &&
                            !node.has(L"border-radius") && !node.has(L"BorderRadius"))
                            node.set(L"Radius", L"4");
                        if (!node.has(L"BorderThickness") && !node.has(L"BorderWidth") &&
                            !node.has(L"border") && !node.has(L"BorderSize"))
                        {
                            node.set(L"BorderThickness", L"1");
                            if (!node.has(L"BorderColor") && !node.has(L"border-color"))
                                node.set(L"BorderColor", L"#767676");
                        }
                        if (!node.has(L"Background") && !node.has(L"background") &&
                            !node.has(L"BackColor") && !node.has(L"BgColor"))
                            node.set(L"Background", L"#ffffff");
                        if (!node.has(L"Padding") && !node.has(L"padding"))
                            node.set(L"Padding", htmlArea ? L"2 4" : L"2 6");
                        // 白底上必须有深字：不写 color 时主题的浅字落在白底上几乎看不见
                        if (!node.has(L"Foreground") && !node.has(L"Color") &&
                            !node.has(L"color") && !node.has(L"ForeColor"))
                            node.set(L"Foreground", L"#111111");
                        // HTML 的 <select> 不写 selected 时默认选中第一项，
                        // XML 侧不写 SelectedIndex 就是"什么都没选"，框里一片空白。
                        // <option> 子元素也要在这里汇成 Items：糖化层的 CollectItems 不在本运行时路径上，
                        // 结果 `<select><option>方案 A</option></select>` 是个空下拉框。
                        if (htmlSel && !node.has(L"Items") && element != nullptr)
                        {
                            std::wstring items;
                            CollectSelectOptions(element, items);
                            if (!items.empty())
                                node.set(L"Items", items);
                        }
                        if (htmlSel && !node.has(L"SelectedIndex"))
                            node.set(L"SelectedIndex", L"0");
                    }
                    if (wAttr.empty() || wAuto)
                    {
                        int cw = 0;
                        if (htmlTagBtn)
                        {
                            // HTML 的 <button> 宽 = 文字宽 + 左右默认 padding（CSS 约 10px/侧）+ 边框。
                            // 早先这里硬编码 88，短文案（"确定"/"取消"）被撑成大块，和浏览器观感差明显。
                            // 显式 width（属性 / style）不进这个分支，仍是定宽按钮。
                            const std::wstring body = !node.text.empty() ? node.text : node.get(L"Text");
                            int pt = 14;
                            const std::wstring fs = node.get(L"FontSize");
                            if (!fs.empty())
                            {
                                const int v = (int)wcstol(fs.c_str(), nullptr, 10);
                                if (v > 0)
                                    pt = v;
                            }
                            double px = 0.0;
                            for (wchar_t c : body)
                                px += (c < 0x2E80) ? pt * 0.55 : (double)pt;
                            if (!body.empty())
                                cw = (std::max)(48, (int)(px + 0.5) + node.padL + node.padR + 4 + 20);
                        }
                        node.w = htmlBtn ? (cw > 0 ? cw : 88) : 160;
                        node.autoW = !htmlLabel;   // 内容/默认自然宽，非作者声明：网格单元格可拉伸
                        // HTML 原生 <input type="text"> 不写宽度时，浏览器按 size=20 个字符宽。
                        // 早先定死 160：14 号字下比浏览器窄、20 号字下偏宽，和 HTML 观感对不上。
                        // 老写法 <XTextField> 保持 160 不变（不改动既有界面排版）。
                        if (!htmlBtn && (node.tag == L"input" || node.has(L"HtmlInput")))
                        {
                            int pt = 14;
                            const std::wstring fs = node.get(L"FontSize");
                            if (!fs.empty())
                            {
                                const int v = (int)wcstol(fs.c_str(), nullptr, 10);
                                if (v > 0)
                                    pt = v;
                            }
                            const int tw = (int)(20 * pt * 0.55 + 0.5);
                            node.w = (std::max)(72, tw + 8);
                        }
                        // <label> 宽 = 文字宽 + 4（HTML label 没有默认 padding）
                        if (htmlLabel)
                        {
                            const std::wstring body = !node.text.empty() ? node.text : node.get(L"Text");
                            int pt = 14;
                            const std::wstring fs = node.get(L"FontSize");
                            if (!fs.empty())
                            {
                                const int v = (int)wcstol(fs.c_str(), nullptr, 10);
                                if (v > 0)
                                    pt = v;
                            }
                            double px = 0.0;
                            for (wchar_t c : body)
                                px += (c < 0x2E80) ? pt * 0.55 : (double)pt;
                            node.w = body.empty() ? 100
                                                 : (int)(px + 0.5) + node.padL + node.padR + 4;
                        }
                        node.hasW = true;
                        node.sugarW = true;   // 伪宽：网格单元格仍可拉满
                    }
                    if ((hAttr.empty() || hAuto) && !node.autoH)
                    {
                        node.h = htmlBtn ? 30 : 26;
                        if (htmlLabel)
                            node.h = 24;   // HTML label 一行高，没有 button/input 的额外内边距
                        // 单行输入的高度 = 一行文字 + 上下边距（HTML 里约 1.3 倍行高）。
                        // 同样只对原生 <input> 生效，<XTextField> 维持 26。
                        if (!htmlBtn && (node.tag == L"input" || node.has(L"HtmlInput")))
                        {
                            int pt = 14;
                            const std::wstring fs = node.get(L"FontSize");
                            if (!fs.empty())
                            {
                                const int v = (int)wcstol(fs.c_str(), nullptr, 10);
                                if (v > 0)
                                    pt = v;
                            }
                            node.h = (std::max)(20, pt + 6);
                        }
                        node.hasH = true;
                        node.sugarH = true;   // 伪高：网格单元格仍可拉满
                    }
                }
            }
        }
        if (!node.hasH)
        {
            const std::wstring bare = xmlmfc::BareTag(node.tag);
            if (_wcsicmp(bare.c_str(), L"ToolBar") == 0) node.h = 32;
            else if (_wcsicmp(bare.c_str(), L"MenuBar") == 0) node.h = 24;
            else if (_wcsicmp(bare.c_str(), L"StatusBar") == 0) node.h = 24;
        }
        node.row = IntAttr(node.get(L"Grid.Row"), 0);
        node.col = IntAttr(node.get(L"Grid.Column"), 0);
        node.rowSpan = (std::max)(1, IntAttr(node.get(L"Grid.RowSpan"), 1));
        node.colSpan = (std::max)(1, IntAttr(node.get(L"Grid.ColumnSpan"), 1));
        // grid-column / grid-row 由 Html 糖化落成 Grid.Column/Grid.Row 与 *Span，
        // 这里只标"作者显式指定过"，供 auto-placement 区分。
        node.hasGridCol = node.has(L"Grid.Column");
        node.hasGridRow = node.has(L"Grid.Row");
        ParseThickness(node.get(L"Margin"), node);
        ParsePadding(node.get(L"Padding"), node);
        // 没有声明就保留既有值：片段根/程序里给的默认间距不能被"重算几何"抹成 0
        if (node.has(L"Spacing"))
            node.spacing = IntAttr(node.get(L"Spacing"), node.spacing);
        if (node.has(L"Orientation"))
        {
            node.horizontal = _wcsicmp(node.get(L"Orientation").c_str(), L"Horizontal") == 0;
            node.hasOrientation = true;
        }

        // QML 布局写法：Layout.row / Layout.column / Layout.preferredWidth / anchors.* / x / y
        // 只做"写法归一"，落到的仍是 Grid 附加属性与 x/y/w/h，布局算法不变。
        {
            const std::wstring lrow = node.get(L"Layout.row");
            if (!lrow.empty()) node.row = IntAttr(lrow, node.row);
            const std::wstring lcol = node.get(L"Layout.column");
            if (!lcol.empty()) node.col = IntAttr(lcol, node.col);
            const std::wstring lrs = node.get(L"Layout.rowSpan");
            if (!lrs.empty()) node.rowSpan = (std::max)(1, IntAttr(lrs, node.rowSpan));
            const std::wstring lcs = node.get(L"Layout.columnSpan");
            if (!lcs.empty()) node.colSpan = (std::max)(1, IntAttr(lcs, node.colSpan));
            if (!node.hasW)
            {
                const std::wstring pw = node.get(L"Layout.preferredWidth");
                const std::wstring qw = node.get(L"width");
                if (!pw.empty() && pw.find(L'%') == std::wstring::npos) { node.w = IntAttr(pw, node.w); node.hasW = true; }
                else if (!qw.empty() && qw.find(L'%') == std::wstring::npos) { node.w = IntAttr(qw, node.w); node.hasW = true; }
            }
            if (!node.hasH)
            {
                const std::wstring ph = node.get(L"Layout.preferredHeight");
                const std::wstring qh = node.get(L"height");
                if (!ph.empty() && ph.find(L'%') == std::wstring::npos) { node.h = IntAttr(ph, node.h); node.hasH = true; }
                else if (!qh.empty() && qh.find(L'%') == std::wstring::npos) { node.h = IntAttr(qh, node.h); node.hasH = true; }
            }
            if (!node.hasX)
            {
                const std::wstring qx = node.get(L"x");
                if (!qx.empty()) { node.x = IntAttr(qx, node.x); node.hasX = true; }
            }
            if (!node.hasY)
            {
                const std::wstring qy = node.get(L"y");
                if (!qy.empty()) { node.y = IntAttr(qy, node.y); node.hasY = true; }
            }
        }

        // CSS flex：HTML 里排版靠它，所以在 XML 里也给同一套短属性。
        // Display="flex" 开启（div 带 class="row|flex" 时由 htmlsugar 补上）；
        // Flex/FlexDirection 定方向，Gap 定间距，Justify/Align 定主/交叉轴对齐，
        // Grow/Self 写在子项上。
        {
            const std::wstring disp = node.get(L"Display");
            const std::wstring fx = node.get(L"Flex");
            const std::wstring fdir = node.get(L"FlexDirection");
            std::wstring dirWord = !fdir.empty() ? fdir : fx;
            if (!disp.empty() && _wcsicmp(disp.c_str(), L"flex") == 0)
                node.flex = true;
            if (!dirWord.empty() && _wcsicmp(dirWord.c_str(), L"flex") != 0)
            {
                node.flex = true;                 // 只写 Flex="row" 也算开 flex
                node.flexDir = _wcsicmp(dirWord.c_str(), L"column") == 0 ? 2
                            : _wcsicmp(dirWord.c_str(), L"column-reverse") == 0 ? 3
                            : _wcsicmp(dirWord.c_str(), L"row-reverse") == 0 ? 1
                            : 0;
                node.horizontal = node.flexDir == 0 || node.flexDir == 1;
                node.hasOrientation = true;
            }
            // 开了 flex 却没写方向：跟随 Orientation（XStackPanel 竖排就是 column 流）
            if (node.flex && dirWord.empty())
                node.flexDir = node.horizontal ? 0 : 2;
            const std::wstring gap = node.get(L"Gap");
            if (!gap.empty())
            {
                const std::vector<int> gv = ThicknessValues(gap);
                node.flexGapX = gv.empty() ? 0 : gv[0];
                node.flexGapY = gv.size() >= 2 ? gv[1] : node.flexGapX;
            }
            const std::wstring jc = node.get(L"Justify");
            const std::wstring jc2 = node.get(L"JustifyContent");
            const std::wstring jw = !jc2.empty() ? jc2 : jc;
            if (!jw.empty())
                node.flexJustify = _wcsicmp(jw.c_str(), L"center") == 0 ? 1
                                 : _wcsicmp(jw.c_str(), L"end") == 0 ||
                                           _wcsicmp(jw.c_str(), L"flex-end") == 0 ? 2
                                 : _wcsicmp(jw.c_str(), L"between") == 0 ||
                                           _wcsicmp(jw.c_str(), L"space-between") == 0 ? 3
                                 : _wcsicmp(jw.c_str(), L"around") == 0 ||
                                           _wcsicmp(jw.c_str(), L"space-around") == 0 ? 4
                                 : _wcsicmp(jw.c_str(), L"evenly") == 0 ||
                                           _wcsicmp(jw.c_str(), L"space-evenly") == 0 ? 5
                                                                                    : 0;
            const std::wstring ai = node.get(L"Align");
            const std::wstring ai2 = node.get(L"AlignItems");
            const std::wstring aw = !ai2.empty() ? ai2 : ai;
            if (!aw.empty())
            {
                node.flexAlign = _wcsicmp(aw.c_str(), L"start") == 0 ||
                                         _wcsicmp(aw.c_str(), L"flex-start") == 0 ? 0
                               : _wcsicmp(aw.c_str(), L"center") == 0 ? 1
                               : _wcsicmp(aw.c_str(), L"end") == 0 ||
                                         _wcsicmp(aw.c_str(), L"flex-end") == 0 ? 2
                                                                                : 3;
                if (_wcsicmp(aw.c_str(), L"stretch") == 0)
                    node.flexAlign = 3;
            }
            const std::wstring wr = node.get(L"Wrap");
            const std::wstring wr2 = node.get(L"FlexWrap");
            const std::wstring ww = !wr2.empty() ? wr2 : wr;
            node.flexWrap = _wcsicmp(ww.c_str(), L"1") == 0 ||
                            _wcsicmp(ww.c_str(), L"true") == 0 ||
                            _wcsicmp(ww.c_str(), L"wrap") == 0;
            const std::wstring gr = node.get(L"Grow");
            const std::wstring gr2 = node.get(L"FlexGrow");
            const std::wstring gw = !gr2.empty() ? gr2 : gr;
            if (!gw.empty())
                node.flexGrow = static_cast<float>(_wtof(gw.c_str()));
            // flex-shrink / flex-basis：HTML 里这俩几乎总是和 grow 一起出现（或干脆写成 flex:1），
            // XML 早先把 shrink 硬编码成 1、basis 拿自然尺寸顶替，等于"能设但设不进去"。
            const std::wstring sk = node.get(L"shrink");
            const std::wstring sk2 = node.get(L"flex-shrink");
            const std::wstring sk3 = node.get(L"FlexShrink");
            const std::wstring skw = !sk3.empty() ? sk3 : (!sk2.empty() ? sk2 : sk);
            if (!skw.empty())
                node.flexShrink = static_cast<float>(_wtof(skw.c_str()));
            const std::wstring bs = node.get(L"basis");
            const std::wstring bs2 = node.get(L"flex-basis");
            const std::wstring bs3 = node.get(L"FlexBasis");
            const std::wstring bsw = !bs3.empty() ? bs3 : (!bs2.empty() ? bs2 : bs);
            if (!bsw.empty())
            {
                const int bpx = (int)wcstol(bsw.c_str(), nullptr, 10);
                // CSS 的 flex-basis 只有 auto / 百分比 才等于"按内容算"；px 值（**包括 0**）都是实打实的基准。
                // flex:1 就是 flex:1 1 0%，basis=0 必须保留成 0，否则三栏 flex:1 会退化成"内容宽 + grow"，
                // 表现为三个卡片宽 197/256/173 而不是等宽 —— HTML 里绝不会这样。
                node.flexBasis = (_wcsicmp(bsw.c_str(), L"auto") == 0 ||
                                  bsw.find(L'%') != std::wstring::npos || bpx < 0)
                                     ? -1
                                     : bpx;
            }
            const std::wstring sf = node.get(L"Self");
            const std::wstring sf2 = node.get(L"AlignSelf");
            // HTML/CSS 写法：align-self（含中划线形式，属性名大小写与分隔符都不敏感）
            const std::wstring sf3 = node.get(L"align-self");
            const std::wstring sw = !sf2.empty() ? sf2 : (!sf3.empty() ? sf3 : sf);
            if (!sw.empty())
            {
                // CSS 值域：auto / flex-start / flex-end / center / baseline / stretch。
                // 早先只认 start|center|end，写 `align-self:flex-start` 会落进 else 当成 stretch，
                // 子项在交叉轴被拉满 —— 与浏览器差一整个语义。
                node.flexSelf = (_wcsicmp(sw.c_str(), L"auto") == 0) ? -1
                             : (_wcsicmp(sw.c_str(), L"start") == 0 ||
                                _wcsicmp(sw.c_str(), L"flex-start") == 0) ? 0
                             : (_wcsicmp(sw.c_str(), L"center") == 0 ||
                                _wcsicmp(sw.c_str(), L"middle") == 0) ? 1
                             : (_wcsicmp(sw.c_str(), L"end") == 0 ||
                                _wcsicmp(sw.c_str(), L"flex-end") == 0) ? 2
                             : (_wcsicmp(sw.c_str(), L"baseline") == 0) ? 4
                                                                       : 3;
            }
        }

        // 历史遗留写法 Layout.alignment / Layout.fillWidth（新代码用 CSS flex:1 + Align/Self）
        {
            {
                const std::wstring al = node.get(L"Layout.alignment");
                if (!al.empty())
                {
                    auto pick = [&](const wchar_t* key) {
                        return al.find(key) != std::wstring::npos;
                    };
                    if (pick(L"AlignHCenter"))
                        node.alignX = 2;
                    else if (pick(L"AlignLeft"))
                        node.alignX = 0;
                    else if (pick(L"AlignRight"))
                        node.alignX = 1;
                    if (pick(L"AlignFill"))
                        node.alignX = 3;
                    if (pick(L"AlignVCenter"))
                        node.alignY = 2;
                    else if (pick(L"AlignTop"))
                        node.alignY = 0;
                    else if (pick(L"AlignBottom"))
                        node.alignY = 1;
                    if (pick(L"AlignFill"))
                        node.alignY = 3;
                    if (pick(L"AlignBaseline"))
                        node.alignY = 4;   // 基线对齐：按行内最大 ascent 落位
                }
                const std::wstring fw = node.get(L"Layout.fillWidth");
                const std::wstring fh = node.get(L"Layout.fillHeight");
                node.fillW = _wcsicmp(fw.c_str(), L"true") == 0 || fw == L"1";
                node.fillH = _wcsicmp(fh.c_str(), L"true") == 0 || fh == L"1";
            }
        }

        if (_wcsicmp(xmlmfc::BareTag(node.tag).c_str(), L"Grid") == 0)
        {
            ParseDefs(element, true, node);
            ParseDefs(element, false, node);
        }

        // 子元素
        IXMLDOMNodeList* children = nullptr;
        element->get_childNodes(&children);
        if (children != nullptr)
        {
            long count = 0;
            children->get_length(&count);
            for (long i = 0; i < count; ++i)
            {
                IXMLDOMNode* childNode = nullptr;
                children->get_item(i, &childNode);
                if (childNode == nullptr)
                    continue;
                DOMNodeType type = NODE_INVALID;
                childNode->get_nodeType(&type);
                IXMLDOMElement* childElement = nullptr;
                childNode->QueryInterface(IID_IXMLDOMElement, reinterpret_cast<void**>(&childElement));
                childNode->Release();
                if (type != NODE_ELEMENT || childElement == nullptr)
                    continue;

                BSTR ct = nullptr;
                childElement->get_tagName(&ct);
                std::wstring childTag = ct != nullptr ? ct : L"";
                ::SysFreeString(ct);
                const std::wstring childBare = xmlmfc::BareTag(childTag);

                // 资源类节点：不参与布局，交给 scanRes 采集样式/模板
                if (childBare == L"Grid.RowDefinitions" ||
                    childBare == L"Grid.ColumnDefinitions")
                {
                    childElement->Release();
                    continue;
                }
                if (IsResourceContainerTag(childBare) ||
                    childBare == L"Style" ||
                    childBare == L"ControlTemplate" ||
                    childBare == L"DataTemplate")
                {
                    scanRes(childElement);
                    childElement->Release();
                    continue;
                }


                // ---- Repeater：解析期按数据把模板克隆 N 份（本身不建控件） ----
                if (_wcsicmp(childTag.c_str(), L"Repeater") == 0)
                {
                    std::vector<std::wstring> rows;
                    const std::wstring items = AttrByName(childElement, L"Items");
                    if (!items.empty())
                    {
                        std::wstring cur;
                        for (wchar_t c : items)
                        {
                            if (c == L'\r')
                                continue;
                            if (c == L'|' || c == L';' || c == L'\n')
                            {
                                rows.push_back(cur);
                                cur.clear();
                                continue;
                            }
                            cur.push_back(c);
                        }
                        rows.push_back(cur);
                    }
                    int count = _wtoi(AttrByName(childElement, L"Count").c_str());
                    if (count <= 0)
                        count = rows.empty() ? 1 : static_cast<int>(rows.size());
                    if (count > 400)
                        count = 400;
                    const int itemH = _wtoi(AttrByName(childElement, L"ItemHeight").c_str());
                    const int spacing = _wtoi(AttrByName(childElement, L"Spacing").c_str());
                    const int stride = (itemH > 0 ? itemH : 0) + spacing;

                    IXMLDOMNodeList* repKids = nullptr;
                    childElement->get_childNodes(&repKids);
                    if (repKids != nullptr)
                    {
                        long rc = 0;
                        repKids->get_length(&rc);
                        for (long ri = 0; ri < rc; ++ri)
                        {
                            IXMLDOMNode* rnode = nullptr;
                            repKids->get_item(ri, &rnode);
                            if (rnode == nullptr)
                                continue;
                            DOMNodeType rtype = NODE_INVALID;
                            rnode->get_nodeType(&rtype);
                            IXMLDOMElement* rel = nullptr;
                            rnode->QueryInterface(IID_IXMLDOMElement, reinterpret_cast<void**>(&rel));
                            if (rtype != NODE_ELEMENT || rel == nullptr)
                            {
                                rnode->Release();
                                continue;
                            }
                            for (int i = 0; i < count; ++i)
                            {
                                const std::wstring row = i < static_cast<int>(rows.size()) ? rows[i] : std::wstring();
                                IXMLDOMNode* clone = nullptr;
                                rel->cloneNode(VARIANT_TRUE, &clone);
                                if (clone == nullptr)
                                    continue;
                                IXMLDOMElement* ce = nullptr;
                                clone->QueryInterface(IID_IXMLDOMElement, reinterpret_cast<void**>(&ce));
                                clone->Release();
                                if (ce == nullptr)
                                    continue;
                                SubstDom(ce, i, count, row, stride * i);
                                node.children.emplace_back();
                                ParseElement(ce, node.children.back(), styles, templates, baseDir, resStack);
                                // 克隆出来的控件必须有名（宿主建控件依赖 x:Name 找 id）
                                {
                                    XmlElement& made = node.children.back();
                                    std::wstring base = made.name;
                                    if (base.empty())
                                        base = L"item";
                                    made.name = base + L"_" + std::to_wstring(i);
                                }
                                ce->Release();
                            }
                            rel->Release();
                            rnode->Release();
                        }
                        repKids->Release();
                    }
                    childElement->Release();
                    continue;
                }

                node.children.emplace_back();
                ParseElement(childElement, node.children.back(), styles, templates, baseDir, resStack);
                childElement->Release();
            }
            children->Release();
        }
        FitTextHeight(node);
    }

    // ---------- 布局引擎（复刻 guiload 语义） ----------
    struct Track
    {
        int kind = 2;   // 0 fixed, 1 auto, 2 star
        int value = 1;
    };

    int Sum(const std::vector<int>& sizes, int start, int count)
    {
        int total = 0;
        for (int i = start; i < start + count && i < (int)sizes.size(); ++i)
            total += sizes[i];
        return total;
    }

    // 标签 -> 内部短名（与 guiload TagKind 一致：小写、去命名空间、控件映射）
    std::wstring KindOf(const std::wstring& raw)
    {
        std::wstring tag = raw;
        const size_t brace = tag.rfind(L'}');
        if (brace != std::wstring::npos)
            tag = tag.substr(brace + 1);
        // 统一命名：<XButton> / <XStackPanel> 去掉前缀后按本意名分派
        if (tag.size() > 1 && (tag[0] == L'X' || tag[0] == L'x') && iswupper(tag[1]))
            tag = tag.substr(1);
        for (wchar_t& c : tag)
            c = (wchar_t)towlower(c);
        const size_t dot = tag.rfind(L'.');
        if (dot != std::wstring::npos)
            tag = tag.substr(dot + 1);

        if (tag == L"textblock") return L"label";
        if (tag == L"textbox") return L"edit";
        if (tag == L"radiobutton") return L"radio";
        if (tag == L"groupbox") return L"group";
        if (tag == L"stackpanel") return L"stackpanel";
        if (tag == L"dockpanel") return L"dockpanel";
        if (tag == L"wrappanel") return L"wrappanel";
        if (tag == L"layout" || tag == L"layoutpanel" || tag == L"absolutepanel" ||
            tag == L"abspanel" || tag == L"canvaspanel" || tag == L"canvaslayout")
            return L"canvas";          // 布局侧统一按 canvas 走
        if (tag == L"paletteset") return L"palette";
        if (tag == L"tabitem") return L"page";
        if (tag == L"rowdefinition") return L"rowdefinition";
        if (tag == L"columndefinition") return L"columndefinition";
        return tag; // button/listbox/checkbox/combo/progress/slider/grid/window/toolbar 等原样
    }

    bool IsKnownControl(const std::wstring& tag)
    {
        return tag == L"label" || tag == L"button" || tag == L"edit" || tag == L"listbox" ||
               tag == L"checkbox" || tag == L"radio" || tag == L"group" || tag == L"combo" ||
               tag == L"progress" || tag == L"slider";
    }

    void ReplaceToken(std::wstring& s, const std::wstring& from, const std::wstring& to)
    {
        if (from.empty() || to.empty())
            return;
        size_t p = 0;
        while ((p = s.find(from, p)) != std::wstring::npos)
        {
            s.replace(p, from.size(), to);
            p += to.size();
        }
    }

    // 一行字段：{modelData} = 整行首字段，{index} = 行号，{0}/{1}/… = 各字段
    void ApplyRowFields(XmlElement& el, const std::vector<std::wstring>& fields, int row)
    {
        const std::wstring idx = std::to_wstring(row);
        const std::wstring first = fields.empty() ? std::wstring() : fields[0];
        for (auto& kv : el.attrs)
        {
            std::wstring v = kv.second;
            ReplaceToken(v, L"{index}", idx);
            ReplaceToken(v, L"{modelData}", first);
            for (size_t i = 0; i < fields.size(); ++i)
                ReplaceToken(v, L"{" + std::to_wstring(i) + L"}", fields[i]);
            kv.second = v;
        }
        if (!el.name.empty())
        {
            std::wstring nm = el.name;
            ReplaceToken(nm, L"{index}", idx);
            ReplaceToken(nm, L"{modelData}", first);
            for (size_t i = 0; i < fields.size(); ++i)
                ReplaceToken(nm, L"{" + std::to_wstring(i) + L"}", fields[i]);
            el.name = nm;
        }
        for (auto& c : el.children)
            ApplyRowFields(c, fields, row);
    }

    void MarkDelegateRow(XmlElement& el)
    {
        el.delegateRow = true;
        for (auto& c : el.children)
            MarkDelegateRow(c);
    }

    bool IsContainer(const std::wstring& tag)
    {
        if (_wcsicmp(tag.c_str(), L"itemview") == 0)
            return true;

        // AbsolutePanel 是"绝对定位布局"的正名；Canvas 是它的历史别名（画布另有 DrawCanvas）。
        return tag == L"stackpanel" || tag == L"grid" || tag == L"dockpanel" ||
               tag == L"wrappanel" || tag == L"canvas" || tag == L"absolutepanel" ||
               tag == L"abspanel" || tag == L"canvaspanel" || tag == L"canvaslayout" ||
               tag == L"layout" || tag == L"layoutpanel" ||
               // 侧栏：<PaletteSet> 是多页宿主，<page> 是一页（内部照样放容器和控件）
               tag == L"palette" || tag == L"page" ||
               // QML 的容器类（Pane / Frame / Dialog / Drawer / GroupBox / TabBar / TabPage）
               tag == L"pane" || tag == L"frame" || tag == L"dialog" || tag == L"drawer" ||
               tag == L"groupbox" || tag == L"tabpage" || tag == L"tabbar";
    }

    int DefaultHeight(const std::wstring& tag)
    {
        // 优先取 catalog 注入的缺省尺寸；未注册才用内置兜底
        int dw = 120, dh = 0;
        if (xmlGetDefaultSize(tag.c_str(), dw, dh))
            return dh;
        if (tag == L"label") return 24;
        if (tag == L"button") return 32;
        if (tag == L"edit") return 28;
        if (tag == L"checkbox" || tag == L"radio") return 24;
        if (tag == L"group") return 80;
        if (tag == L"listbox") return 120;
        if (tag == L"combo") return 120;
        if (tag == L"progress") return 20;
        if (tag == L"slider") return 32;
        return 24;
    }

    // 控件的"自然宽"（横排里没写 Width 时用）。优先 catalog，未加载才用内置兜底，
    // 兜底值与 control_catalog.xml 保持一致，免得设计器和运行时宽窄对不上。
    int NaturalWidth(const std::wstring& tag)
    {
        int dw = 120, dh = 28;
        if (xmlGetDefaultSize(tag.c_str(), dw, dh))
            return (std::max)(16, dw);
        if (tag == L"label") return 100;
        if (tag == L"button") return 100;
        if (tag == L"edit" || tag == L"textfield" || tag == L"textbox" ||
            tag == L"textarea" || tag == L"combobox" || tag == L"combo" || tag == L"choice")
            return 120;
        if (tag == L"checkbox" || tag == L"radiobutton" || tag == L"radio") return 100;
        if (tag == L"slider") return 140;
        if (tag == L"progressbar" || tag == L"progress") return 120;
        if (tag == L"switch") return 56;
        if (tag == L"divider") return 60;
        return 120;
    }

    std::vector<int> ResolveTracks(const std::vector<std::pair<std::wstring, std::wstring>>& defs,
                                   bool rows, const std::vector<XmlElement>& children, int available)
    {
        std::vector<Track> tracks;
        for (const auto& kv : defs)
        {
            Track t;
            std::wstring text = kv.second;
            std::transform(text.begin(), text.end(), text.begin(), towlower);
            if (text == L"auto")
            {
                t.kind = 1;
                t.value = 0;
            }
            else if (!text.empty() && text.back() == L'*')
            {
                t.kind = 2;
                std::wstring weight = text.substr(0, text.size() - 1);
                t.value = weight.empty() ? 1 : (std::max)(1, _wtoi(weight.c_str()));
            }
            else
            {
                t.kind = 0;
                t.value = (std::max)(0, _wtoi(text.c_str()));
            }
            tracks.push_back(t);
        }
        if (tracks.empty())
            tracks.push_back(Track{2, 1});

        int fixedSum = 0, autoSum = 0, starWeight = 0;
        for (size_t i = 0; i < tracks.size(); ++i)
        {
            Track& t = tracks[i];
            if (t.kind == 0) fixedSum += t.value;
            else if (t.kind == 1)
            {
                int need = rows ? 24 : 100;
                for (const auto& child : children)
                {
                    if ((rows ? child.row : child.col) != (int)i)
                        continue;
                    const std::wstring kind = KindOf(child.tag);
                    if (!IsKnownControl(kind) && !IsContainer(kind))
                        continue;
                    need = (std::max)(need, rows ? child.h : child.w);
                }
                t.value = need;
                autoSum += need;
            }
            else starWeight += (std::max)(1, t.value);
        }

        const int remaining = (std::max)(0, available - fixedSum - autoSum);
        int unit = starWeight > 0 ? remaining / starWeight : 0;
        if (unit <= 0) unit = layoutDefaults().starMin;

        std::vector<int> result;
        int lastStar = -1;
        for (size_t i = 0; i < tracks.size(); ++i)
        {
            const Track& t = tracks[i];
            result.push_back(t.kind == 2 ? (std::max)(1, unit * (std::max)(1, t.value)) : t.value);
            if (t.kind == 2)
                lastStar = static_cast<int>(i);
        }
        // 取整会丢掉余数：640 宽 12 + 3*star + 12 会算成 639。
        // 余数补给最后一个 star 轨道，保证各轨道之和正好等于可用宽度。
        if (lastStar >= 0)
        {
            int sum = 0;
            for (int v : result)
                sum += v;
            if (sum > 0 && sum < available)
                result[lastStar] += (available - sum);
        }
        return result;
    }

    // 流式布局上下文（对应 guiload 的 GuiFlow）
    // 布局诊断开关：只在本进程第一次进入布局时读一次环境变量
    extern const bool g_layoutDbg = xmlprop2::EnvFlagOn(L"PYARX_XML_LAYOUT_DBG");


    // ===================================================================
}
