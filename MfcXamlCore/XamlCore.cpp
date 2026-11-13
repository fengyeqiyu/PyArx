#include "XamlCore.h"
#include "controls/XamlMfcTags.h"

#include <msxml6.h>
#include <algorithm>
#include <cmath>
#include <comdef.h>
#include <cwchar>
#include <cwctype>
#include <fstream>
#include <locale>
#include <map>
#include <set>
#include <string>
#include <typeindex>

#pragma comment(lib, "msxml6.lib")

// ===========================================================================
// XamlElement
// ===========================================================================
namespace
{
    // 本地声明是"要算的值"时，读取要落到求值后的有效值。
    // 包括 {Binding ...} 与内置表达式 {...}；{StaticResource}/{TemplateBinding}
    // 由样式/模板阶段直接替换进 attrs，不在此列。
    bool IsBindingExpression(const std::wstring& v)
    {
        const size_t b = v.find_first_not_of(L" \t\r\n");
        if (b == std::wstring::npos)
            return false;
        if (_wcsnicmp(v.c_str() + b, L"{Binding", 8) == 0 ||
            _wcsnicmp(v.c_str() + b, L"{Bind", 5) == 0)
            return true;
        if (v.c_str()[b] != L'{' || v.back() != L'}')
            return false;
        const wchar_t* p = v.c_str() + b + 1;
        while (*p == L' ' || *p == L'\t')
            ++p;
        return !(_wcsnicmp(p, L"StaticResource", 14) == 0 ||
                 _wcsnicmp(p, L"TemplateBinding", 15) == 0);
    }

    bool HasLocal(const XamlElement& el, const std::wstring& key)
    {
        for (const auto& kv : el.attrs)
            if (_wcsicmp(kv.first.c_str(), key.c_str()) == 0)
                return true;
        return false;
    }

    void SetEffective(XamlElement& el, const std::wstring& key, const std::wstring& v)
    {
        for (auto& kv : el.styledAttrs)
            if (_wcsicmp(kv.first.c_str(), key.c_str()) == 0)
            {
                kv.second = v;
                return;
            }
        el.styledAttrs.emplace_back(key, v);
    }

    void ClearEffective(XamlElement& el)
    {
        el.styledAttrs.clear();
        el.styled = false;
    }
}

bool XamlElement::has(const wchar_t* key) const
{
    for (const auto& kv : attrs)
        if (_wcsicmp(kv.first.c_str(), key) == 0)
            return true;
    for (const auto& kv : styledAttrs)
        if (_wcsicmp(kv.first.c_str(), key) == 0)
            return true;
    return false;
}

std::wstring XamlElement::get(const wchar_t* key) const
{
    const std::wstring* local = nullptr;
    for (const auto& kv : attrs)
        if (_wcsicmp(kv.first.c_str(), key) == 0)
        {
            local = &kv.second;
            break;
        }
    // 本地值就是绑定表达式时，用求值后的有效值
    if (local != nullptr && !IsBindingExpression(*local))
        return *local;
    for (const auto& kv : styledAttrs)
        if (_wcsicmp(kv.first.c_str(), key) == 0)
            return kv.second;
    return local != nullptr ? *local : std::wstring();
}

void XamlElement::set(const wchar_t* key, const std::wstring& v)
{
    for (auto& kv : attrs)
    {
        if (_wcsicmp(kv.first.c_str(), key) == 0)
        {
            kv.second = v;
            return;
        }
    }
    attrs.emplace_back(key, v);
}

// ===========================================================================
// 工具
// ===========================================================================
namespace
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

    // WPF 惯用写法 → 内部属性名的别名归一化。
    // 目的：让按 WPF 常识写的 XAML 直接生效，而不是静默失效（我们踩过的坑）。
    // 只做"名字层"的映射，不改任何语义；值域必须与内部名一致。
    std::wstring WpfAlias(const std::wstring& a)
    {
        static const struct { const wchar_t* from; const wchar_t* to; } k[] = {
            { L"Canvas.Left",    L"Left" },         // 我们用 Left/Top（画布定位）
            { L"Canvas.Top",     L"Top" },
            { L"DockPanel.Dock", L"Dock" },          // 我们用 Dock
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

    inline bool AttrIsAuto(const std::wstring& s)
    {
        return _wcsicmp(s.c_str(), L"Auto") == 0 || _wcsicmp(s.c_str(), L"NaN") == 0;
    }

    // ---------------- 布局缺省值（由 catalog 注入，避免 C++ 硬编码） ----------------
    std::wstring KindOf(const std::wstring& raw);   // 前置声明（定义在布局段）

    struct LayoutDefaults
    {
        std::vector<std::pair<std::wstring, std::pair<int, int>>> sizes; // tag -> (w,h)
        int starMin = 30;      // 星号轨道最小宽度
        int groupLeft = 12;    // GroupBox 边框物理内区左偏移
        int groupTop = 24;     // GroupBox 边框物理内区上偏移
    };

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

    void ParseThickness(const std::wstring& text, XamlElement& node)
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
            node.marginL = node.marginR = values[0];
            node.marginT = node.marginB = values[1];
        }
        else if (count >= 4)
        {
            node.marginL = values[0];
            node.marginT = values[1];
            node.marginR = values[2];
            node.marginB = values[3];
        }
    }

    // 解析 Padding（四值/两值/单值），写入 padL/T/R/B
    void ParsePadding(const std::wstring& text, XamlElement& node)
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
            node.padL = node.padR = values[0];
            node.padT = node.padB = values[1];
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

    void ParseDefs(IXMLDOMElement* element, bool rows, XamlElement& node)
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
            if (_wcsicmp(xamlmfc::BareTag(childTag).c_str(), defsTag.c_str()) != 0)
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
                if (_wcsicmp(xamlmfc::BareTag(defTag).c_str(), itemTag.c_str()) == 0)
                    target.emplace_back(sizeAttr, Attr(def, sizeAttr.c_str()));
                def->Release();
            }
            defs->Release();
        }
        children->Release();
    }

    using TemplateTable = std::vector<std::pair<std::wstring, std::vector<XamlElement>>>;

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

    void CollectStyle(IXMLDOMElement* styleEl, std::vector<XamlStyle>& out)
    {
        XamlStyle st;
        st.key = AttrByName(styleEl, L"x:Key");
        if (st.key.empty())
            st.key = AttrByName(styleEl, L"Key");
        st.targetType = AttrByName(styleEl, L"TargetType");
        const std::wstring basedOn = AttrByName(styleEl, L"BasedOn");
        std::wstring bk;
        st.basedOn = xamlIsResourceRef(basedOn, bk) ? bk : basedOn;

        // <Trigger Property=".." Value=".."> 里的 Setter 收集到对应触发器
        auto collectTrigger = [&](IXMLDOMElement* trigEl) {
            XamlTrigger trig;
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
                            XamlSetter s;
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
                    XamlSetter s;
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
    void ParseElement(IXMLDOMElement* element, XamlElement& node,
                      std::vector<XamlStyle>* styles, TemplateTable* templates,
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
        // 资源容器上写 Source="xxx.xaml" 时，把目标文档的**资源**合并进来（控件节点不合并）。
        std::function<void(const std::wstring&)> mergeResFile;
        std::function<void(IXMLDOMElement*)> scanRes = [&](IXMLDOMElement* resEl) {
            if (resEl == nullptr)
                return;
            const std::wstring rtag = xamlmfc::BareTag(TagOf(resEl));
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
                std::vector<XamlElement> nodes;
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
            node.text = node.get(L"Content");   // Popup 的正文
        if (node.text.empty())
            node.text = node.get(L"Header");
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
            node.hasW = !wAttr.empty() && !wAuto;
            node.hasH = !hAttr.empty() && !hAuto;
            node.w = (wAttr.empty() || wAuto) ? dw : IntAttr(wAttr, dw);
            node.h = (hAttr.empty() || hAuto) ? dh : IntAttr(hAttr, dh);
        }
        if (!node.hasH)
        {
            const std::wstring bare = xamlmfc::BareTag(node.tag);
            if (_wcsicmp(bare.c_str(), L"ToolBar") == 0) node.h = 32;
            else if (_wcsicmp(bare.c_str(), L"MenuBar") == 0) node.h = 24;
            else if (_wcsicmp(bare.c_str(), L"StatusBar") == 0) node.h = 24;
        }
        node.row = IntAttr(node.get(L"Grid.Row"), 0);
        node.col = IntAttr(node.get(L"Grid.Column"), 0);
        node.rowSpan = (std::max)(1, IntAttr(node.get(L"Grid.RowSpan"), 1));
        node.colSpan = (std::max)(1, IntAttr(node.get(L"Grid.ColumnSpan"), 1));
        ParseThickness(node.get(L"Margin"), node);
        ParsePadding(node.get(L"Padding"), node);
        node.spacing = IntAttr(node.get(L"Spacing"), 0);
        node.horizontal = _wcsicmp(node.get(L"Orientation").c_str(), L"Horizontal") == 0;
        node.hasOrientation = !node.get(L"Orientation").empty();

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
                if (!pw.empty()) { node.w = IntAttr(pw, node.w); node.hasW = true; }
                else if (!qw.empty()) { node.w = IntAttr(qw, node.w); node.hasW = true; }
            }
            if (!node.hasH)
            {
                const std::wstring ph = node.get(L"Layout.preferredHeight");
                const std::wstring qh = node.get(L"height");
                if (!ph.empty()) { node.h = IntAttr(ph, node.h); node.hasH = true; }
                else if (!qh.empty()) { node.h = IntAttr(qh, node.h); node.hasH = true; }
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

        // QML anchors：锚到父容器内框（值写 parent 或任意 id 都按父处理）
        {
            node.anchorFill = !node.get(L"anchors.fill").empty();
            node.anchorCenterIn = !node.get(L"anchors.centerIn").empty();
            node.anchorLeft = !node.get(L"anchors.left").empty();
            node.anchorRight = !node.get(L"anchors.right").empty();
            node.anchorTop = !node.get(L"anchors.top").empty();
            node.anchorBottom = !node.get(L"anchors.bottom").empty();
            node.anchorHCenter = !node.get(L"anchors.horizontalCenter").empty() ||
                                 !node.get(L"anchors.centering").empty();
            node.anchorVCenter = !node.get(L"anchors.verticalCenter").empty() ||
                                 !node.get(L"anchors.centering").empty();
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
            node.anchorFillRaw = node.get(L"anchors.fill");
            node.anchorLeftRaw = node.get(L"anchors.left");
            node.anchorRightRaw = node.get(L"anchors.right");
            node.anchorTopRaw = node.get(L"anchors.top");
            node.anchorBottomRaw = node.get(L"anchors.bottom");
            node.anchorHCenterRaw = node.get(L"anchors.horizontalCenter");
            node.anchorVCenterRaw = node.get(L"anchors.verticalCenter");
            node.anchorBaselineRaw = node.get(L"anchors.baseline");
            const std::wstring mg = node.get(L"anchors.margins");
            node.anchorMargin = mg.empty() ? 0 : (std::max)(0, IntAttr(mg, 0));
            node.hasAnchors = node.anchorFill || node.anchorCenterIn || node.anchorLeft ||
                              node.anchorRight || node.anchorTop || node.anchorBottom ||
                              node.anchorHCenter || node.anchorVCenter ||
                              !node.anchorBaselineRaw.empty();
        }

        if (_wcsicmp(xamlmfc::BareTag(node.tag).c_str(), L"Grid") == 0)
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
                const std::wstring childBare = xamlmfc::BareTag(childTag);

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
                                    XamlElement& made = node.children.back();
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

    // anchors 的值写着 parent（或空）就是锚父容器，否则是锚兄弟对象
    bool AnchorIsParent(const std::wstring& raw)
    {
        return raw.empty() || _wcsicmp(raw.c_str(), L"parent") == 0;
    }

    bool HasSiblingAnchor(const XamlElement& el)
    {
        return !AnchorIsParent(el.anchorFillRaw) || !AnchorIsParent(el.anchorLeftRaw) ||
               !AnchorIsParent(el.anchorRightRaw) || !AnchorIsParent(el.anchorTopRaw) ||
               !AnchorIsParent(el.anchorBottomRaw) || !AnchorIsParent(el.anchorHCenterRaw) ||
               !AnchorIsParent(el.anchorVCenterRaw) || !AnchorIsParent(el.anchorBaselineRaw);
    }

    // "A.right" -> id=A, edge=right
    void SplitAnchor(const std::wstring& raw, std::wstring& id, std::wstring& edge)
    {
        const size_t dot = raw.find(L'.');
        if (dot == std::wstring::npos)
        {
            id = raw;
            edge.clear();
        }
        else
        {
            id = raw.substr(0, dot);
            edge = raw.substr(dot + 1);
        }
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
    void ApplyRowFields(XamlElement& el, const std::vector<std::wstring>& fields, int row)
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

    void MarkDelegateRow(XamlElement& el)
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
        if (xamlGetDefaultSize(tag.c_str(), dw, dh))
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

    std::vector<int> ResolveTracks(const std::vector<std::pair<std::wstring, std::wstring>>& defs,
                                   bool rows, const std::vector<XamlElement>& children, int available)
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
    struct XamlFlow
    {
        int margin = 0;      // 不再有暗默认边距：位置一律来自 XAML 的 Left/Top/Margin
        int spacing = 0;     // 子元素间距，来自容器 XAML 的 Spacing（缺省 0）
        int cursorY = 0;     // 相对 offsetY
        int cursorX = 0;     // 横向排列游标
        int width = 240;     // 容器内容宽度
        int offsetX = 0;
        int offsetY = 0;
        bool horizontal = false;
        int height = 0;      // 容器可用高度
        int padding = 0;     // 左内边距（XAML Padding）
        int paddingY = 0;    // 上内边距（XAML Padding）
        std::vector<int> gridRows;
        std::vector<int> gridCols;
    };

    // ===================================================================
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

    TextMetrics TextMetricsOf(const XamlElement& el)
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
    int BaselineFromTop(const XamlElement& el, int boxH = 0)
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

    void LayoutNode(XamlElement& node, XamlFlow& flow)
    {
        // ItemView 行定位：delegate 行按 ItemHeight 逐行堆叠，行内再走普通布局
        if (_wcsicmp(KindOf(node.tag).c_str(), L"itemview") == 0)
        {
            int rowH = _wtoi(node.get(L"ItemHeight").c_str());
            if (rowH <= 0)
                rowH = 44;
            const int availW = (std::max)(1, flow.width - flow.padding * 2);
            int i = 0;
            for (auto& child : node.children)
            {
                child.previewX = flow.offsetX + flow.padding;
                child.previewY = flow.offsetY + flow.paddingY + i * rowH;
                child.previewW = availW;
                child.previewH = rowH;

                XamlFlow inner;
                inner.margin = 0;
                inner.spacing = child.spacing;
                inner.padding = 0;
                inner.paddingY = 0;
                inner.width = availW;
                inner.height = rowH;
                inner.offsetX = child.previewX;
                inner.offsetY = child.previewY;
                inner.cursorX = 0;
                inner.cursorY = 0;
                inner.horizontal = child.hasOrientation ? child.horizontal : true;   // 行内默认横排
                LayoutNode(child, inner);
                ++i;
            }
            return;
        }
        // Dock 布局：容器内任一子元素声明 Dock（非 None）即启用，
        // 该子元素占一块剩余区，其余按声明顺序依次切分（WinForms 语义）
        bool dockMode = false;
        for (const auto& c : node.children)
        {
            const std::wstring d = c.get(L"Dock");
            if (!d.empty() && _wcsicmp(d.c_str(), L"None") != 0)
            {
                dockMode = true;
                break;
            }
        }
        // 基线对齐（Qt.AlignBaseline）用：本层子项里最大的"盒顶到基线"距离
        int rowMaxAsc = 0;
        for (const auto& c : node.children)
        {
            const std::wstring ct = KindOf(c.tag);
            if (ct == L"page" || ct == L"rowdefinition" || ct == L"columndefinition")
                continue;
            if (HasSiblingAnchor(c))
                continue;                                  // 绝对锚定的项不参与行内基线
            const std::wstring dk = c.get(L"Dock");
            if (!dk.empty() && _wcsicmp(dk.c_str(), L"None") != 0)
                continue;
            rowMaxAsc = (std::max)(rowMaxAsc, BaselineFromTop(c));
        }

        int dockL = flow.padding, dockT = flow.paddingY;
        int dockR = (std::max)(dockL + 1, flow.width);
        int dockB = flow.height > 0 ? flow.height : (dockT + 1);
        const bool paletteRoot = KindOf(node.tag) == L"palette";
        bool firstPage = true;

        for (auto& child : node.children)
        {
            const std::wstring tag = KindOf(child.tag);
            // 侧栏的多页：预览与设计器只画第一页（其余页数据保留，运行时靠 tab 切换）
            if (tag == L"page")
            {
                if (paletteRoot && !firstPage)
                    continue;
                firstPage = false;
            }
            else if (tag == L"rowdefinition" || tag == L"columndefinition")
                continue;

            const int defH = IsContainer(tag)
                ? (flow.height > 0 ? flow.height : 160)
                : DefaultHeight(tag);
            int h = child.hasH ? child.h : defH;
            int w = child.hasW ? child.w
                               : (std::max)(1, flow.width - flow.margin * 2 - flow.padding * 2);

            // 坐标：默认流式，写全 Left/Top 用绝对
            const bool useGridX = !flow.gridCols.empty();
            const bool useGridY = !flow.gridRows.empty();
            int localX = 0, localY = 0;
            bool docked = false;

            // QML anchors：直接占父容器内框的一块，不参与流式推进
            bool anchored = false;
            if (child.hasAnchors && !HasSiblingAnchor(child))
            {
                const int availL = flow.padding;
                const int availT = flow.paddingY;
                const int availR = (std::max)(availL + 1, flow.width - flow.padding);
                const int availB = (std::max)(availT + 1, flow.height - flow.paddingY);
                const int m = child.anchorMargin;
                const int boxW = (std::max)(1, availR - availL - m * 2);
                const int boxH = (std::max)(1, availB - availT - m * 2);
                if (child.anchorFill)
                {
                    localX = availL + m;
                    localY = availT + m;
                    w = boxW;
                    h = boxH;
                    anchored = true;
                }
                else
                {
                    if (!child.hasW) w = boxW;
                    if (!child.hasH) h = boxH;
                    if (child.anchorCenterIn || child.anchorHCenter)
                        localX = availL + m + (boxW - w) / 2;
                    else if (child.anchorLeft)
                        localX = availL + m;
                    else if (child.anchorRight)
                        localX = (std::max)(availL, availR - m - w);
                    else
                        localX = child.hasX ? child.x : availL + m;
                    if (!child.anchorBaselineRaw.empty() && AnchorIsParent(child.anchorBaselineRaw))
                        localY = availT + m + BaselineFromTop(node) - BaselineFromTop(child, h);
                    else if (child.anchorCenterIn || child.anchorVCenter)
                        localY = availT + m + (boxH - h) / 2;
                    else if (child.anchorTop)
                        localY = availT + m;
                    else if (child.anchorBottom)
                        localY = (std::max)(availT, availB - m - h);
                    else
                        localY = child.hasY ? child.y : availT + m;
                    anchored = true;
                }
            }
            if (dockMode)
            {
                const std::wstring d = child.get(L"Dock");
                if (!d.empty() && _wcsicmp(d.c_str(), L"None") != 0)
                {
                    docked = true;
                    const int gap = flow.spacing;
                    if (_wcsicmp(d.c_str(), L"Fill") == 0)
                    {
                        localX = dockL;
                        localY = dockT;
                        w = (std::max)(1, dockR - dockL);
                        h = (std::max)(1, dockB - dockT);
                    }
                    else if (_wcsicmp(d.c_str(), L"Top") == 0)
                    {
                        localX = dockL;
                        localY = dockT;
                        w = (std::max)(1, dockR - dockL);
                        dockT += h + gap;
                    }
                    else if (_wcsicmp(d.c_str(), L"Bottom") == 0)
                    {
                        localX = dockL;
                        localY = (std::max)(dockT, dockB - h);
                        w = (std::max)(1, dockR - dockL);
                        dockB -= h + gap;
                    }
                    else if (_wcsicmp(d.c_str(), L"Left") == 0)
                    {
                        localX = dockL;
                        localY = dockT;
                        h = (std::max)(1, dockB - dockT);
                        dockL += w + gap;
                    }
                    else if (_wcsicmp(d.c_str(), L"Right") == 0)
                    {
                        localX = (std::max)(dockL, dockR - w);
                        localY = dockT;
                        h = (std::max)(1, dockB - dockT);
                        dockR -= w + gap;
                    }
                }
            }

            if (!docked && !anchored)
            {
            if (flow.horizontal)
            {
                localX = child.hasX ? child.x : flow.cursorX;
                localY = child.hasY ? child.y : flow.cursorY;
                if (!useGridX && !useGridY)
                    flow.cursorX = (std::max)(flow.cursorX, localX + w + flow.spacing);
            }
            else
            {
                localX = child.hasX ? child.x : flow.padding;
                localY = child.hasY ? child.y : flow.cursorY;
                if (!useGridX && !useGridY)
                {
                    if (child.hasY)
                        flow.cursorY = (std::max)(flow.cursorY, localY + h + flow.spacing);
                    else
                        flow.cursorY += h + flow.spacing;
                }
            }

            // Grid 单元格：行、列两个维度各自独立生效，未声明的维度退回 Left/Top
            if (useGridX || useGridY)
            {
                if (useGridX && !useGridY && !child.hasY)
                    localY = 0;   // 只有列定义时，子元素都在同一行
                if (useGridY && !useGridX && !child.hasX)
                    localX = 0;
                if (useGridX)
                {
                    const int col = (std::max)(0, (std::min)(child.col, (int)flow.gridCols.size() - 1));
                    const int colSpan =
                        (std::max)(1, (std::min)(child.colSpan, (int)flow.gridCols.size() - col));
                    localX = flow.padding + Sum(flow.gridCols, 0, col);
                    const int cellW = Sum(flow.gridCols, col, colSpan);
                    if (child.hasW)
                        w = (std::min)(w, cellW);
                    else
                        w = cellW;
                }

                if (useGridY)
                {
                    const int row = (std::max)(0, (std::min)(child.row, (int)flow.gridRows.size() - 1));
                    const int rowSpan =
                        (std::max)(1, (std::min)(child.rowSpan, (int)flow.gridRows.size() - row));
                    localY = flow.paddingY + Sum(flow.gridRows, 0, row);
                    const int cellH = Sum(flow.gridRows, row, rowSpan);
                    if (child.hasH)
                        h = (std::min)(h, cellH);
                    else
                        h = cellH;
                }
            }
            }   // if (!docked)

            // Margin（锚定项由 anchors.margins 负责，不再叠加）
            if (!anchored)
            {
                localX += child.marginL;
                localY += child.marginT;
                w = (std::max)(1, w - child.marginL - child.marginR);
                h = (std::max)(1, h - child.marginT - child.marginB);
            }

            // Layout.alignment 落位：垂直流看水平分量，水平流看垂直分量
            if (!anchored)
            {
                const int availW = (std::max)(1, flow.width - flow.padding * 2);
                const int availH = (std::max)(1, flow.height - flow.paddingY * 2);
                if (!flow.horizontal && child.alignX != 0)
                {
                    if (child.alignX == 3)
                    {
                        w = (std::max)(1, availW - child.marginL - child.marginR);
                        localX = flow.padding + child.marginL;
                    }
                    else if (child.alignX == 1)
                        localX = (std::max)(flow.padding, flow.width - flow.padding - w - child.marginR);
                    else
                        localX = flow.padding + (std::max)(0, availW - w) / 2;
                }
                else if (flow.horizontal && child.alignY != 0)
                {
                    if (child.alignY == 4)
                    {
                        // 基线对齐：让本项文字基线与行内基线（最大 ascent 那条）齐平
                        localY = flow.paddingY +
                                 (std::max)(0, rowMaxAsc - BaselineFromTop(child, h));
                    }
                    else if (child.alignY == 3)
                    {
                        h = (std::max)(1, availH - child.marginT - child.marginB);
                        localY = flow.paddingY + child.marginT;
                    }
                    else if (child.alignY == 1)
                        localY = (std::max)(flow.paddingY, flow.height - flow.paddingY - h - child.marginB);
                    else
                        localY = flow.paddingY + (std::max)(0, availH - h) / 2;
                }
            }

            child.previewX = flow.offsetX + localX;
            child.previewY = flow.offsetY + localY;
            child.previewW = w;
            child.previewH = h;

            // 容器递归
            if (tag == L"stackpanel" || tag == L"page" || tag == L"palette")
            {
                XamlFlow inner;
                inner.margin = 0;
                inner.spacing = child.spacing;
                inner.padding = child.padL;
                inner.paddingY = child.padT;
                inner.width = w;
                inner.offsetX = child.previewX;
                inner.offsetY = child.previewY;
                inner.cursorX = child.padL;
                inner.cursorY = child.padT;
                inner.horizontal = child.horizontal;
                inner.height = h;
                LayoutNode(child, inner);
            }
            else if (tag == L"grid")
            {
                XamlFlow inner;
                inner.margin = 0;
                inner.spacing = child.spacing;
                inner.padding = child.padL;
                inner.paddingY = child.padT;
                inner.width = w;
                inner.height = h;
                inner.offsetX = child.previewX;
                inner.offsetY = child.previewY;
                // 只有 XAML 显式声明了对应维度的轨道时才生成轨道，未声明则保留 Left/Top 绝对定位
                if (!child.gridCols.empty())
                    inner.gridCols = ResolveTracks(child.gridCols, false, child.children,
                                                   (std::max)(1, w));
                if (!child.gridRows.empty())
                    inner.gridRows = ResolveTracks(child.gridRows, true, child.children,
                                                   (std::max)(1, h));
                LayoutNode(child, inner);
            }
            else if (tag == L"dockpanel" || tag == L"wrappanel" || tag == L"canvas" ||
                     tag == L"absolutepanel" || tag == L"abspanel" || tag == L"canvaspanel" ||
                     tag == L"canvaslayout")
            {
                XamlFlow inner;
                inner.margin = 0;
                inner.spacing = child.spacing;
                inner.padding = child.padL;
                inner.paddingY = child.padT;
                inner.width = w;
                inner.height = h;
                inner.offsetX = child.previewX;
                inner.offsetY = child.previewY;
                inner.cursorX = child.padL;
                inner.cursorY = child.padT;
                inner.horizontal = (tag == L"wrappanel");
                LayoutNode(child, inner);
            }
            else if (tag == L"toolbar" || tag == L"menubar" || tag == L"statusbar")
            {
                XamlFlow inner;
                inner.margin = 0;
                inner.spacing = child.spacing;
                inner.padding = child.padL;
                inner.paddingY = child.padT;
                inner.width = w;
                inner.height = h;
                inner.offsetX = child.previewX;
                inner.offsetY = child.previewY;
                inner.cursorX = child.padL;
                inner.cursorY = child.padT;
                inner.horizontal = child.hasOrientation ? child.horizontal : true;
                LayoutNode(child, inner);
            }
            else if (tag == L"group")
            {
                // group 是控件，也是容器：内部坐标相对组内区，独立流式游标
                // 内区偏移是 GroupBox 边框的物理占位（可经 xamlSetGroupInset 覆盖）
                XamlFlow inner;
                inner.margin = layoutDefaults().groupLeft;
                inner.spacing = child.spacing;
                inner.cursorY = layoutDefaults().groupTop;
                inner.width = (std::max)(w - layoutDefaults().groupLeft * 2, 40);
                inner.offsetX = child.previewX + layoutDefaults().groupLeft;
                inner.offsetY = child.previewY + layoutDefaults().groupTop;
                LayoutNode(child, inner);
            }
            else if (!child.children.empty())
            {
                // 控件模板实体化出来的视觉层：当作该控件内部的容器排布
                XamlFlow inner;
                inner.margin = 0;
                inner.spacing = child.spacing;
                inner.padding = child.padL;
                inner.paddingY = child.padT;
                inner.width = w;
                inner.height = h;
                inner.offsetX = child.previewX;
                inner.offsetY = child.previewY;
                inner.cursorX = child.padL;
                inner.cursorY = child.padT;
                inner.horizontal = child.hasOrientation ? child.horizontal : false;
                LayoutNode(child, inner);
            }
        }

        // Layout.fillWidth / Layout.fillHeight 重排：占满可用宽度，或均分剩余高度/宽度
        {
            bool hasFillItem = false;
            for (const auto& c : node.children)
                if (c.fillW || c.fillH)
                {
                    hasFillItem = true;
                    break;
                }

            if (hasFillItem)
            {
                std::vector<XamlElement*> flowItems;
                for (auto& c : node.children)
                    if (!(c.hasAnchors && !HasSiblingAnchor(c)))
                        flowItems.push_back(&c);

                const int availW = (std::max)(1, flow.width - flow.padding * 2);
                const int availH = (std::max)(1, flow.height - flow.paddingY * 2);
                const int gaps = (int)flowItems.size() - 1;
                const int gapTotal = gaps > 0 ? gaps * flow.spacing : 0;

                int fixedMain = 0, nFillMain = 0;
                for (auto* p : flowItems)
                {
                    const bool fillMain = flow.horizontal ? p->fillW : p->fillH;
                    if (fillMain)
                        nFillMain++;
                    else
                        fixedMain += flow.horizontal ? p->previewW : p->previewH;
                }
                const int rest = availH - fixedMain - gapTotal;
                const int eachMain = nFillMain > 0 ? (std::max)(24, rest / nFillMain) : 0;

                int cursor = flow.horizontal ? flow.padding : flow.paddingY;
                for (auto* p : flowItems)
                {
                    const int ww = flow.horizontal ? (p->fillW ? eachMain : p->previewW) : (p->fillW ? availW : p->previewW);
                    const int hh = flow.horizontal ? (p->fillH ? availH : p->previewH) : (p->fillH ? eachMain : p->previewH);
                    if (flow.horizontal)
                    {
                        p->previewX = flow.offsetX + cursor;
                        p->previewY = flow.offsetY + flow.paddingY;
                    }
                    else
                    {
                        p->previewX = flow.offsetX + flow.padding;
                        p->previewY = flow.offsetY + cursor;
                    }
                    p->previewW = ww;
                    p->previewH = hh;

                    if (!p->children.empty())
                    {
                        XamlFlow inner;
                        inner.margin = 0;
                        inner.spacing = p->spacing;
                        inner.padding = p->padL;
                        inner.paddingY = p->padT;
                        inner.width = ww;
                        inner.height = hh;
                        inner.offsetX = p->previewX;
                        inner.offsetY = p->previewY;
                        inner.cursorX = p->padL;
                        inner.cursorY = p->padT;
                        inner.horizontal = p->hasOrientation ? p->horizontal : false;
                        LayoutNode(*p, inner);
                    }

                    cursor += (flow.horizontal ? ww : hh) + flow.spacing;
                }
            }
        }

        // 兄弟锚定：等所有兄弟的 preview 都算出来之后，再做一次修正
        for (auto& child : node.children)
        {
            if (!HasSiblingAnchor(child))
                continue;

            auto findSibling = [&](const std::wstring& id) -> const XamlElement* {
                if (id.empty())
                    return nullptr;
                for (const auto& sib : node.children)
                    if (!sib.name.empty() && _wcsicmp(sib.name.c_str(), id.c_str()) == 0)
                        return &sib;
                return nullptr;
            };

            const int availL = flow.padding;
            const int availT = flow.paddingY;
            const int availR = (std::max)(availL + 1, flow.width - flow.padding);
            const int availB = (std::max)(availT + 1, flow.height - flow.paddingY);

            int w = child.previewW, h = child.previewH;
            int localX = child.previewX - flow.offsetX;
            int localY = child.previewY - flow.offsetY;
            bool hasX = false, hasY = false, hasR = false, hasB = false;
            int xPos = 0, yPos = 0, rPos = 0, bPos = 0;

            // 一条锚边：写 parent 用父内框，写兄弟用兄弟的边（A.left / A.right / A.bottom / A.top）
            auto pickEdge = [&](const std::wstring& raw, int parentPos, bool horiz, bool isFar) {
                if (raw.empty())
                    return;
                int v = parentPos;
                if (!AnchorIsParent(raw))
                {
                    std::wstring id, edge;
                    SplitAnchor(raw, id, edge);
                    const XamlElement* sib = findSibling(id);
                    if (sib == nullptr)
                        return;
                    const int sp = horiz ? (sib->previewX - flow.offsetX)
                                         : (sib->previewY - flow.offsetY);
                    const int se = sp + (horiz ? sib->previewW : sib->previewH);
                    const bool useEnd = _wcsicmp(edge.c_str(), horiz ? L"right" : L"bottom") == 0;
                    if (_wcsicmp(edge.c_str(), L"baseline") == 0)
                        v = sp + BaselineFromTop(*sib, sib->previewH);   // A.baseline：兄弟文字基线
                    else
                        v = useEnd ? se : sp;
                }
                if (horiz)
                {
                    if (isFar) { rPos = v; hasR = true; }
                    else { xPos = v; hasX = true; }
                }
                else
                {
                    if (isFar) { bPos = v; hasB = true; }
                    else { yPos = v; hasY = true; }
                }
            };

            // fill="A"：位置与尺寸都跟兄弟一致
            {
                std::wstring id, edge;
                SplitAnchor(child.anchorFillRaw, id, edge);
                if (!AnchorIsParent(child.anchorFillRaw))
                {
                    if (const XamlElement* sib = findSibling(id))
                    {
                        xPos = sib->previewX - flow.offsetX;
                        yPos = sib->previewY - flow.offsetY;
                        rPos = xPos + sib->previewW;
                        bPos = yPos + sib->previewH;
                        hasX = hasR = hasY = hasB = true;
                        if (!child.hasW) w = sib->previewW;
                        if (!child.hasH) h = sib->previewH;
                    }
                }
            }

            pickEdge(child.anchorLeftRaw, availL, true, false);
            pickEdge(child.anchorRightRaw, availR, true, true);
            pickEdge(child.anchorTopRaw, availT, false, false);
            pickEdge(child.anchorBottomRaw, availB, false, true);

            // 单轴居中：锚兄弟的中心线
            auto pickCenter = [&](const std::wstring& raw, bool horiz) {
                if (raw.empty() || AnchorIsParent(raw))
                    return;
                std::wstring id, edge;
                SplitAnchor(raw, id, edge);
                const XamlElement* sib = findSibling(id);
                if (sib == nullptr)
                    return;
                if (horiz)
                {
                    xPos = sib->previewX - flow.offsetX + sib->previewW / 2 - w / 2;
                    hasX = true;
                }
                else
                {
                    yPos = sib->previewY - flow.offsetY + sib->previewH / 2 - h / 2;
                    hasY = true;
                }
            };
            pickCenter(child.anchorHCenterRaw, true);
            pickCenter(child.anchorVCenterRaw, false);

            // anchors.baseline="A"：本控件文字基线压到兄弟 A 的基线上
            if (!child.anchorBaselineRaw.empty() && !AnchorIsParent(child.anchorBaselineRaw))
            {
                std::wstring id, edge;
                SplitAnchor(child.anchorBaselineRaw, id, edge);
                if (const XamlElement* sib = findSibling(id))
                {
                    yPos = (sib->previewY - flow.offsetY) + BaselineFromTop(*sib, sib->previewH) -
                           BaselineFromTop(child, child.previewH);
                    hasY = true;
                }
            }

            // 两边都锚定 → 两点之间拉宽/拉高；单边锚定只挪位置，不改显式尺寸
            if (hasX && hasR)
                w = (std::max)(1, rPos - xPos);
            if (hasY && hasB)
                h = (std::max)(1, bPos - yPos);

            const int m = child.anchorMargin;
            if (hasX)
                localX = xPos + m;
            else if (hasR)
                localX = (std::max)(0, rPos - m - w);
            if (hasY)
                localY = yPos + m;
            else if (hasB)
                localY = (std::max)(0, bPos - m - h);
            if (hasX && hasR)
                w = (std::max)(1, w - m * 2);
            if (hasY && hasB)
                h = (std::max)(1, h - m * 2);

            child.previewX = flow.offsetX + localX;
            child.previewY = flow.offsetY + localY;
            child.previewW = w;
            child.previewH = h;

            if (!child.children.empty())
            {
                XamlFlow inner;
                inner.margin = 0;
                inner.spacing = child.spacing;
                inner.padding = child.padL;
                inner.paddingY = child.padT;
                inner.width = w;
                inner.height = h;
                inner.offsetX = child.previewX;
                inner.offsetY = child.previewY;
                inner.cursorX = child.padL;
                inner.cursorY = child.padT;
                inner.horizontal = child.hasOrientation ? child.horizontal : false;
                LayoutNode(child, inner);
            }
        }
    }
}

// ===========================================================================
// 工具导出
// ===========================================================================
namespace
{
    // 供 xamlLayoutSubtree 使用的入口
    void LayoutNodeAt(XamlElement& root, int rootX, int rootY, int rootW, int rootH)
    {
        root.previewX = rootX;
        root.previewY = rootY;
        root.previewW = (std::max)(1, rootW > 0 ? rootW : root.w);
        root.previewH = (std::max)(1, rootH > 0 ? rootH : root.h);
        // 根节点按普通容器处理：根自身无外边距，子元素贴顶贴左
        XamlFlow flow;
        flow.width = root.previewW;
        flow.height = root.previewH;
        flow.offsetX = root.previewX;
        flow.offsetY = root.previewY;
        flow.margin = 0;
        flow.spacing = root.spacing;
        flow.padding = root.padL;
        flow.paddingY = root.padT;
        flow.cursorX = root.padL;
        flow.cursorY = root.padT;
        LayoutNode(root, flow);
    }
}

void xamlLayoutSubtree(XamlElement& root, int rootX, int rootY, int rootW, int rootH)
{
    LayoutNodeAt(root, rootX, rootY, rootW, rootH);
}

void xamlSetDefaultSize(const wchar_t* tag, int w, int h)
{
    if (tag == nullptr || tag[0] == 0)
        return;
    if (w <= 0) w = 120;
    if (h <= 0) h = 28;
    const std::wstring key = KindOf(std::wstring(tag));
    for (auto& it : layoutDefaults().sizes)
    {
        if (it.first == key)
        {
            it.second.first = w;
            it.second.second = h;
            return;
        }
    }
    layoutDefaults().sizes.emplace_back(key, std::make_pair(w, h));
}

bool xamlGetDefaultSize(const wchar_t* tag, int& w, int& h)
{
    const std::wstring key = KindOf(tag != nullptr ? std::wstring(tag) : std::wstring());
    for (const auto& it : layoutDefaults().sizes)
    {
        if (it.first == key)
        {
            w = it.second.first;
            h = it.second.second;
            return true;
        }
    }
    return false;
}

void xamlSetStarMinPx(int px)
{
    layoutDefaults().starMin = px > 0 ? px : 30;
}

void xamlSetGroupInset(int left, int top)
{
    layoutDefaults().groupLeft = left >= 0 ? left : 12;
    layoutDefaults().groupTop = top >= 0 ? top : 24;
}

std::wstring xamlColorToString(COLORREF c)
{
    wchar_t buf[16];
    swprintf_s(buf, L"#%02X%02X%02X", GetRValue(c), GetGValue(c), GetBValue(c));
    return buf;
}

bool xamlParseColor(const std::wstring& s, COLORREF& out)
{
    std::wstring v = s;
    // trim
    size_t b = v.find_first_not_of(L" \t\r\n");
    if (b == std::wstring::npos) return false;
    v = v.substr(b);
    size_t e = v.find_last_not_of(L" \t\r\n");
    if (e != std::wstring::npos) v = v.substr(0, e + 1);

    if (v.empty()) return false;
    if (v[0] == L'#')
    {
        // #AARRGGBB（8 位）的数值可能是 0xFFxxxxxx，超过 LONG_MAX。
        // 用有符号 long 接会溢出成 LONG_MAX，颜色整体变白。
        const unsigned long n = wcstoul(v.c_str() + 1, nullptr, 16);
        out = RGB((n >> 16) & 0xFF, (n >> 8) & 0xFF, n & 0xFF);
        return true;
    }
    if (v.find(L"RGB") == 0 || v.find(L"rgb") == 0)
    {
        int a, b, c;
        if (swscanf_s(v.c_str(), L"RGB(%d,%d,%d)", &a, &b, &c) == 3 ||
            swscanf_s(v.c_str(), L"rgb(%d,%d,%d)", &a, &b, &c) == 3)
        {
            out = RGB(a, b, c);
            return true;
        }
    }
    return false;
}

std::vector<std::wstring> xamlSplit(const std::wstring& s, wchar_t sep)
{
    std::vector<std::wstring> out;
    std::wstring cur;
    for (wchar_t c : s)
    {
        if (c == sep)
        {
            if (!cur.empty()) out.push_back(cur);
            cur.clear();
        }
        else cur.push_back(c);
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}

// ===========================================================================
// XamlControlRegistry
// ===========================================================================
XamlControlRegistry& XamlControlRegistry::instance()
{
    static XamlControlRegistry s_inst;
    return s_inst;
}

bool XamlControlRegistry::registerFactory(const wchar_t* tag, Factory factory)
{
    if (tag == nullptr || !factory)
        return false;
    for (auto& kv : m_factories)
        if (_wcsicmp(kv.first.c_str(), tag) == 0)
        {
            kv.second = std::move(factory);
            return true;
        }
    m_factories.emplace_back(tag, std::move(factory));
    return true;
}

bool XamlControlRegistry::isKnown(const wchar_t* tag) const
{
    if (tag == nullptr) return false;
    for (auto& kv : m_factories)
        if (_wcsicmp(kv.first.c_str(), tag) == 0)
            return true;
    return false;
}

IXamlControl* XamlControlRegistry::create(const wchar_t* tag) const
{
    if (tag == nullptr) return nullptr;
    for (auto& kv : m_factories)
        if (_wcsicmp(kv.first.c_str(), tag) == 0)
            return kv.second ? kv.second() : nullptr;
    return nullptr;
}

std::vector<std::wstring> XamlControlRegistry::tags() const
{
    std::vector<std::wstring> out;
    for (auto& kv : m_factories)
        out.push_back(kv.first);
    return out;
}

// ===========================================================================
// XamlRuntime
// ===========================================================================
XamlRuntime::~XamlRuntime()
{
    m_controls.clear();
    m_root.reset();
}

bool XamlRuntime::loadFile(const wchar_t* path, std::wstring& error)
{
    ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    IXMLDOMDocument2* document = nullptr;
    HRESULT hr = ::CoCreateInstance(__uuidof(DOMDocument60), nullptr, CLSCTX_INPROC_SERVER,
                                    IID_IXMLDOMDocument2, reinterpret_cast<void**>(&document));
    if (FAILED(hr) || document == nullptr)
    {
        error = L"无法创建 MSXML 文档对象";
        return false;
    }
    document->put_async(VARIANT_FALSE);
    document->put_validateOnParse(VARIANT_FALSE);
    VARIANT_BOOL ok = VARIANT_FALSE;
    document->load(_variant_t(path), &ok);
    if (ok != VARIANT_TRUE)
    {
        error = L"XML 解析失败（文件不存在或格式错误）";
        document->Release();
        return false;
    }
    IXMLDOMElement* rootElement = nullptr;
    document->get_documentElement(&rootElement);
    if (rootElement == nullptr)
    {
        error = L"XML 无根元素";
        document->Release();
        return false;
    }
    m_root = std::make_unique<XamlElement>();
    m_styles.clear();
    m_templates.clear();
    m_bindings.clear();
    // 资源字典 Source 的相对路径以本文件所在目录为基准；resStack 挡住循环引用
    const std::wstring baseDir = PathDirOf(NormPath(path));
    std::vector<std::wstring> resStack;
    resStack.push_back(NormPath(path));
    ParseElement(rootElement, *m_root, &m_styles, &m_templates, baseDir, &resStack);
    rootElement->Release();
    document->Release();
    applyDeclarations();
    return true;
}

bool XamlRuntime::loadString(const wchar_t* xml, std::wstring& error)
{
    if (xml == nullptr)
    {
        error = L"XML 字符串为空";
        return false;
    }
    ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    IXMLDOMDocument2* document = nullptr;
    HRESULT hr = ::CoCreateInstance(__uuidof(DOMDocument60), nullptr, CLSCTX_INPROC_SERVER,
                                    IID_IXMLDOMDocument2, reinterpret_cast<void**>(&document));
    if (FAILED(hr) || document == nullptr)
    {
        error = L"无法创建 MSXML 文档对象";
        return false;
    }
    document->put_async(VARIANT_FALSE);
    document->put_validateOnParse(VARIANT_FALSE);
    BSTR bstrXml = ::SysAllocString(xml);
    VARIANT_BOOL ok = VARIANT_FALSE;
    document->loadXML(bstrXml, &ok);
    ::SysFreeString(bstrXml);
    if (ok != VARIANT_TRUE)
    {
        error = L"XML 解析失败";
        document->Release();
        return false;
    }
    IXMLDOMElement* rootElement = nullptr;
    document->get_documentElement(&rootElement);
    if (rootElement == nullptr)
    {
        error = L"XML 无根元素";
        document->Release();
        return false;
    }
    m_root = std::make_unique<XamlElement>();
    m_styles.clear();
    m_templates.clear();
    m_bindings.clear();
    ParseElement(rootElement, *m_root, &m_styles, &m_templates, std::wstring(), nullptr);
    rootElement->Release();
    document->Release();
    applyDeclarations();
    return true;
}

void XamlRuntime::layout(int rootX, int rootY, int rootW, int rootH)
{
    if (!m_root)
        return;
    LayoutNodeAt(*m_root, rootX, rootY, rootW, rootH);
}

void XamlRuntime::setControlCallback(ControlCallback cb)
{
    m_onControl = std::move(cb);
}

IXamlControl* XamlRuntime::instantiate(void* parent)
{
    if (!m_root)
        return nullptr;
    m_controls.clear();

    // 递归实例化
    std::function<IXamlControl*(XamlElement&)> build = [&](XamlElement& el) -> IXamlControl* {
        if (XamlControlRegistry::instance().isKnown(el.tag.c_str()) == false)
        {
            // 未注册标签：视为容器或布局元素，不创建控件，但继续处理子树
            for (auto& child : el.children)
            {
                if (child.templateChild)
                    continue;
                build(child);
            }
            return nullptr;
        }
        IXamlControl* ctrl = XamlControlRegistry::instance().create(el.tag.c_str());
        if (ctrl == nullptr)
            return nullptr;
        m_controls.emplace_back(ctrl);
        m_ctrlByElement[&el] = ctrl;
        if (ctrl->create(parent))
        {
            ctrl->setBounds(el.previewX, el.previewY, el.previewW, el.previewH);
            // 属性：按声明顺序写入（Name/事件名由宿主在回调里处理）
            for (const auto& kv : el.attrs)
            {
                const std::wstring key = LocalName(kv.first);
                ctrl->setProperty(key.c_str(), kv.second);
            }
        }
        if (m_onControl)
            m_onControl(el, ctrl);
        // 子控件（模板实体化出来的视觉层由控件自绘，不进运行时控件树）
        for (auto& child : el.children)
        {
            if (child.templateChild)
                continue;
            build(child);
        }
        return ctrl;
    };

    return build(*m_root);
}

XamlElement* XamlRuntime::root() { return m_root.get(); }
const XamlElement* XamlRuntime::root() const { return m_root.get(); }

std::wstring XamlRuntime::toXml() const
{
    std::wstring out;
    if (!m_root) return out;
    out += L"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";

    std::function<void(const XamlElement&, int)> write = [&](const XamlElement& n, int depth) {
        std::wstring indent((size_t)depth * 2, L' ');
        out += indent + L"<" + n.tag;
        if (n.attrs.size() <= 3)
        {
            for (const auto& kv : n.attrs)
                out += L" " + kv.first + L"=\"" + XmlEscape(kv.second) + L"\"";
        }
        else
        {
            // 属性多时一个属性一行，避免出现超长的单行
            const std::wstring ai((size_t)(depth + 1) * 2, L' ');
            for (const auto& kv : n.attrs)
                out += L"\n" + ai + kv.first + L"=\"" + XmlEscape(kv.second) + L"\"";
        }
        if (n.children.empty())
        {
            out += L"/>\n";
            return;
        }
        out += L">\n";
        if (!n.gridRows.empty())
        {
            out += indent + L"  <Grid.RowDefinitions>\n";
            for (const auto& kv : n.gridRows)
                out += indent + L"    <RowDefinition Height=\"" + XmlEscape(kv.second) + L"\"/>\n";
            out += indent + L"  </Grid.RowDefinitions>\n";
        }
        if (!n.gridCols.empty())
        {
            out += indent + L"  <Grid.ColumnDefinitions>\n";
            for (const auto& kv : n.gridCols)
                out += indent + L"    <ColumnDefinition Width=\"" + XmlEscape(kv.second) + L"\"/>\n";
            out += indent + L"  </Grid.ColumnDefinitions>\n";
        }
        for (const auto& c : n.children)
        {
            if (c.templateChild)
                continue;   // 模板实例化出来的视觉层不写回文档
            write(c, depth + 1);
        }
        out += indent + L"</" + n.tag + L">\n";
    };
    write(*m_root, 0);
    return out;
}

bool XamlRuntime::save(const wchar_t* path, std::wstring& error) const
{
    if (!m_root || path == nullptr)
    {
        error = L"文档为空或路径无效";
        return false;
    }
    std::wofstream out(path, std::ios::binary);
    if (!out)
    {
        error = L"无法写入文件";
        return false;
    }
    out.imbue(std::locale(".UTF-8"));
    // 与 toXml 共用同一套序列化：磁盘文件和代码视图格式一致
    std::wstring xml = toXml();
    std::wstring crlf;
    crlf.reserve(xml.size() + 64);
    for (wchar_t c : xml)
    {
        if (c == L'\n')
        {
            crlf += L'\r';
            crlf += L'\n';
        }
        else
            crlf += c;
    }
    out << crlf;
    return true;
}

void XamlRuntime::reset()
{
    m_root.reset();
    m_controls.clear();
}

// ===========================================================================
// 样式 / 控件模板 / 属性绑定
// ===========================================================================
namespace
{
    std::wstring TrimWs(const std::wstring& s)
    {
        size_t b = s.find_first_not_of(L" \t\r\n");
        if (b == std::wstring::npos)
            return {};
        size_t e = s.find_last_not_of(L" \t\r\n");
        return s.substr(b, e - b + 1);
    }

    bool StartsNoCase(const std::wstring& s, const wchar_t* prefix)
    {
        const size_t n = wcslen(prefix);
        return s.size() >= n && _wcsnicmp(s.c_str(), prefix, n) == 0;
    }

    XamlElement* FindByName(XamlElement& node, const std::wstring& name)
    {
        if (!name.empty() && node.name == name)
            return &node;
        for (auto& c : node.children)
            if (auto* hit = FindByName(c, name))
                return hit;
        return nullptr;
    }

    // 属性改了文字类属性后，语义字段（el.text）要跟着更新，否则渲染端读到旧值
    void SyncText(XamlElement& el)
    {
        el.text = el.get(L"Text");
        if (el.text.empty())
            el.text = el.get(L"Content");   // Popup 的正文
        if (el.text.empty())
            el.text = el.get(L"Header");
    }

    // 模板实体化：深拷贝 + 打标记 + 替换 {TemplateBinding Prop}
    void StampTemplate(std::vector<XamlElement>& nodes, const XamlElement& owner)
    {
        for (auto& n : nodes)
        {
            n.templateChild = true;
            for (auto& kv : n.attrs)
            {
                const std::wstring v = TrimWs(kv.second);
                if (v.size() > 2 && v.front() == L'{' && StartsNoCase(v, L"{TemplateBinding"))
                {
                    std::wstring prop = TrimWs(v.substr(wcslen(L"{TemplateBinding")));
                    if (!prop.empty() && prop.back() == L'}')
                        prop.pop_back();
                    prop = TrimWs(prop);
                    kv.second = owner.get(prop.c_str());
                }
            }
            SyncText(n);
            StampTemplate(n.children, owner);
        }
    }
}

bool xamlIsResourceRef(const std::wstring& value, std::wstring& key)
{
    const std::wstring v = TrimWs(value);
    if (v.size() < 2 || v.front() != L'{' || v.back() != L'}')
        return false;
    const std::wstring inner = TrimWs(v.substr(1, v.size() - 2));
    if (!StartsNoCase(inner, L"StaticResource"))
        return false;
    key = TrimWs(inner.substr(wcslen(L"StaticResource")));
    return !key.empty();
}

bool xamlIsBindingExpr(const std::wstring& value, std::wstring& source, std::wstring& prop)
{
    const std::wstring v = TrimWs(value);
    if (v.size() < 2 || v.front() != L'{' || v.back() != L'}')
        return false;
    std::wstring inner = TrimWs(v.substr(1, v.size() - 2));
    if (!StartsNoCase(inner, L"Binding") && !StartsNoCase(inner, L"Bind"))
        return false;
    const size_t kw = StartsNoCase(inner, L"Binding") ? wcslen(L"Binding") : wcslen(L"Bind");
    std::wstring rest = TrimWs(inner.substr(kw));

    // 支持三种写法：{Binding Src.Prop}、{Binding Path=Src.Prop}、
    // {Binding Src.Prop Mode=TwoWay}（其余 key=value 参数忽略）
    std::vector<std::wstring> tokens;
    {
        std::wstring cur;
        for (wchar_t c : rest)
        {
            if (c == L' ' || c == L'\t')
            {
                if (!cur.empty())
                {
                    tokens.push_back(cur);
                    cur.clear();
                }
            }
            else
                cur += c;
        }
        if (!cur.empty())
            tokens.push_back(cur);
    }

    std::wstring path;
    for (const auto& tk : tokens)
    {
        const size_t eq = tk.find(L'=');
        if (eq != std::wstring::npos)
        {
            if (_wcsicmp(tk.substr(0, eq).c_str(), L"Path") == 0)
                path = tk.substr(eq + 1);
            continue;
        }
        if (path.empty())
            path = tk;
    }
    if (path.empty())
        return false;

    const size_t dot = path.find_last_of(L'.');
    if (dot == std::wstring::npos)
    {
        source.clear();
        prop = path;
    }
    else
    {
        source = TrimWs(path.substr(0, dot));
        prop = TrimWs(path.substr(dot + 1));
    }
    return !prop.empty();
}

void XamlRuntime::clearStyles()
{
    m_styles.clear();
    m_templates.clear();
    m_bindings.clear();
}

void XamlRuntime::addStyle(const XamlStyle& style)
{
    m_styles.push_back(style);
}

bool XamlRuntime::getStyle(const std::wstring& key, XamlStyle& out) const
{
    if (key.empty())
        return false;
    // 后定义的覆盖先定义的（与 ResourceDictionary.MergedDictionaries 的合并顺序一致）
    for (auto it = m_styles.rbegin(); it != m_styles.rend(); ++it)
        if (it->key == key)
        {
            out = *it;
            return true;
        }
    return false;
}

std::vector<XamlStyle> XamlRuntime::styles() const
{
    return m_styles;
}

void XamlRuntime::addTemplate(const std::wstring& key, const std::vector<XamlElement>& nodes)
{
    if (key.empty())
        return;
    for (auto& kv : m_templates)
        if (kv.first == key)
        {
            kv.second = nodes;
            return;
        }
    m_templates.emplace_back(key, nodes);
}

bool XamlRuntime::getTemplate(const std::wstring& key, std::vector<XamlElement>& out) const
{
    // 后定义的覆盖先定义的（同 key 多次合并时，最后一个胜出）
    for (auto it = m_templates.rbegin(); it != m_templates.rend(); ++it)
        if (it->first == key)
        {
            out = it->second;
            return true;
        }
    return false;
}

std::vector<std::wstring> XamlRuntime::templateKeys() const
{
    std::vector<std::wstring> keys;
    keys.reserve(m_templates.size());
    for (const auto& kv : m_templates)
        keys.push_back(kv.first);
    return keys;
}

const std::vector<XamlBinding>& XamlRuntime::bindings() const
{
    return m_bindings;
}

void XamlRuntime::applyStyles()
{
    if (!m_root)
        return;

    std::function<void(const XamlStyle&, std::vector<XamlSetter>&, std::vector<XamlTrigger>&, int)>
        gather = [&](const XamlStyle& s, std::vector<XamlSetter>& out,
                     std::vector<XamlTrigger>& trigs, int depth) {
            if (depth > 8)
                return;
            if (!s.basedOn.empty())
            {
                XamlStyle base;
                if (getStyle(s.basedOn, base))
                    gather(base, out, trigs, depth + 1);
            }
            for (const auto& st : s.setters)
                out.push_back(st);
            for (const auto& tr : s.triggers)
                trigs.push_back(tr);
        };

    std::function<void(XamlElement&)> walk = [&](XamlElement& el) {
        std::wstring ref = el.get(L"Style");
        std::wstring key;
        if (!xamlIsResourceRef(ref, key))
            key = TrimWs(ref);

        XamlStyle st;
        bool found = false;
        if (!key.empty() && getStyle(key, st))
            found = true;
        else if (key.empty())
        {
            // 隐式样式：无 x:Key 且 TargetType 匹配
            for (const auto& s : m_styles)
                if (s.key.empty() && _wcsicmp(s.targetType.c_str(), el.tag.c_str()) == 0)
                {
                    st = s;
                    found = true;
                    break;
                }
        }

        if (found)
        {
            std::vector<XamlSetter> setters;
            std::vector<XamlTrigger> triggers;
            gather(st, setters, triggers, 0);
            // 回归可重复调用：先清掉上一轮注入的有效属性，只保留本地声明
            ClearEffective(el);
            for (const auto& s : setters)
            {
                if (HasLocal(el, s.prop))
                    continue;   // 本地声明优先
                SetEffective(el, s.prop, s.value);
                el.styled = true;
            }
            // 触发器：按元素当前（含样式合并后）属性值判定，命中则覆盖同属性
            for (const auto& tr : triggers)
            {
                const std::wstring cur = el.get(tr.prop.c_str());
                bool hit = _wcsicmp(cur.c_str(), tr.value.c_str()) == 0;
                if (!hit && !el.has(tr.prop.c_str()) &&
                    (tr.value.empty() || _wcsicmp(tr.value.c_str(), L"False") == 0))
                    hit = true;   // 属性未声明 → 取默认 False/空，与 False 触发器匹配
                if (!hit)
                    continue;
                el.styled = true;
                for (const auto& s : tr.setters)
                {
                    if (HasLocal(el, s.prop))
                        continue;
                    SetEffective(el, s.prop, s.value);
                }
            }
            SyncText(el);
        }
        for (auto& c : el.children)
            walk(c);
    };
    walk(*m_root);
}

// QML delegate 行展开：XItemView 的唯一子节点是行模板，按 Model 行数实例化。
// Model 两种写法：
//   XAML     —— `AT1-1|回路一;AT1-2|回路二`（; 或换行分行，竖线分字段）
//   Python   —— 绑定层把 list 拼成 `AT1-1<TAB>回路一|AT1-2<TAB>回路二`（竖线分行，制表符分字段）
// 所以先看串里有没有制表符来决定分行/分字段符号，两种写法都落到同一个 rows。
static void ExpandDelegate(XamlElement& el,
                           const std::vector<std::pair<std::wstring, std::vector<XamlElement>>>* tpls)
{
    // QML 里 delegate 可以指向外部 Component：这里写 Delegate="模板键"，
    // 从模板表（<ControlTemplate x:Key="...">）取子树当行模板。
    // 每次展开都重新拷一份，行内的 {index}/{0} 替换只动副本，模板本体不被污染。
    const std::wstring dlgKey = TrimWs(el.get(L"Delegate"));
    if (tpls != nullptr && !dlgKey.empty())
    {
        for (const auto& kv : *tpls)
        {
            if (_wcsicmp(kv.first.c_str(), dlgKey.c_str()) != 0 || kv.second.empty())
                continue;
            el.delegateTemplate = kv.second;
            el.children.clear();   // 引用了外部模板，就不用内联子节点当模板了
            break;
        }
    }
    if (el.delegateTemplate.empty() && !el.children.empty())
    {
        el.delegateTemplate.push_back(el.children.front());
        el.children.clear();
    }
    el.children.erase(
        std::remove_if(el.children.begin(), el.children.end(),
                       [](const XamlElement& c) { return c.delegateRow; }),
        el.children.end());
    if (el.delegateTemplate.empty())
        return;

    const std::wstring model = el.get(L"Model");
    const bool tabFormat = model.find(L'\t') != std::wstring::npos;
    const wchar_t fieldSep = tabFormat ? L'\t' : L'|';
    auto isRowBreak = [&](wchar_t ch) {
        if (ch == L'\n' || ch == L'\r')
            return true;
        return tabFormat ? ch == L'|' : ch == L';';
    };

    std::vector<std::vector<std::wstring>> rows;
    std::wstring cur;
    for (size_t i = 0; i <= model.size(); ++i)
    {
        const wchar_t ch = i < model.size() ? model[i] : L'\n';
        if (!isRowBreak(ch))
        {
            cur.push_back(ch);
            continue;
        }
        const std::wstring line = TrimWs(cur);
        if (!line.empty())
        {
            std::vector<std::wstring> fields;
            std::wstring f;
            for (size_t k = 0; k <= line.size(); ++k)
            {
                const wchar_t fc = k < line.size() ? line[k] : fieldSep;
                if (fc == fieldSep)
                {
                    fields.push_back(TrimWs(f));
                    f.clear();
                }
                else
                    f.push_back(fc);
            }
            rows.push_back(fields);
        }
        cur.clear();
    }

    for (size_t r = 0; r < rows.size(); ++r)
    {
        std::vector<XamlElement> copy = el.delegateTemplate;
        ApplyRowFields(copy[0], rows[r], (int)r);
        MarkDelegateRow(copy[0]);
        el.children.push_back(std::move(copy[0]));
    }
}

void XamlRuntime::applyTemplates()
{
    if (!m_root)
        return;

    std::function<void(XamlElement&)> walk = [&](XamlElement& el) {
        // QML delegate：XItemView 的唯一子节点是行模板，按 Model 的行数实例化
        if (_wcsicmp(KindOf(el.tag).c_str(), L"itemview") == 0)
            ExpandDelegate(el, &m_templates);
        // 先清掉上一次实体化的模板子树，再递归真实子节点
        el.children.erase(
            std::remove_if(el.children.begin(), el.children.end(),
                           [](const XamlElement& c) { return c.templateChild; }),
            el.children.end());
        for (auto& c : el.children)
            walk(c);

        std::wstring ref = el.get(L"Template");
        std::wstring key;
        if (!xamlIsResourceRef(ref, key))
            key = TrimWs(ref);
        if (key.empty())
            return;
        std::vector<XamlElement> tpl;
        if (!getTemplate(key, tpl) || tpl.empty())
            return;
        StampTemplate(tpl, el);
        el.children.insert(el.children.end(), tpl.begin(), tpl.end());
    };
    walk(*m_root);
}

// ===========================================================================
// 内置表达式
//   Width="{a.Width / 2 + 8}"     元素属性引用 + 算术
//   Left="{a.Left + a.Width + 8}" 相对布局
//   Text="{a.Text + ' px'}"       字符串拼接（任一侧是字符串就走拼接）
// 语法与 {Binding}/{StaticResource}/{TemplateBinding} 共用花括号，
// 前缀不是那三个关键字时按表达式求值，结果写进"有效属性"（读取路径与绑定一致）。
// 求值带深度上限：a.Width="{b.Width}" 与 b.Width="{a.Width}" 这种环会直接失败，
// 不会把栈打穿。
// ===========================================================================
namespace
{
    bool IsDataExpression(const std::wstring& v)
    {
        const size_t b = v.find_first_not_of(L" \t\r\n");
        if (b == std::wstring::npos || v[b] != L'{' || v.back() != L'}')
            return false;
        const wchar_t* p = v.c_str() + b + 1;
        while (*p == L' ' || *p == L'\t')
            ++p;
        if (*p == L'=')                       // {= expr} 写法也认
            return true;
        return !(_wcsnicmp(p, L"Binding", 7) == 0 || _wcsnicmp(p, L"Bind", 4) == 0 ||
                 _wcsnicmp(p, L"StaticResource", 14) == 0 ||
                 _wcsnicmp(p, L"TemplateBinding", 15) == 0);
    }

    std::wstring DataExpressionInner(const std::wstring& v)
    {
        size_t b = v.find_first_not_of(L" \t\r\n");
        size_t e = v.find_last_not_of(L" \t\r\n");
        if (b == std::wstring::npos || e <= b + 1)
            return std::wstring();
        std::wstring inner = v.substr(b + 1, e - b - 1);
        size_t s = inner.find_first_not_of(L" \t\r\n");
        if (s != std::wstring::npos && inner[s] == L'=')
            inner.erase(0, s + 1);
        return inner;
    }

    struct ExprValue
    {
        bool isStr = false;
        double num = 0.0;
        std::wstring str;
    };

    bool IsIdentStart(wchar_t c) { return iswalpha(c) || c == L'_' || c > 127; }
    bool IsIdentChar(wchar_t c) { return iswalnum(c) || c == L'_' || c == L'.' || c > 127; }

    bool EvalDataExpression(const std::wstring& raw, XamlElement* self, XamlElement* root,
                            int depth, std::wstring* out);

    // 标识符取值：Name.Prop 走树查找；裸 Prop 取当前元素；值本身是表达式时继续递归
    bool ResolveIdent(const std::wstring& ident, XamlElement* self, XamlElement* root,
                      int depth, ExprValue& out)
    {
        std::wstring elemName = ident;
        std::wstring prop = ident;
        const size_t dot = ident.rfind(L'.');
        if (dot != std::wstring::npos && dot > 0)
        {
            elemName = ident.substr(0, dot);
            prop = ident.substr(dot + 1);
        }
        else
        {
            elemName.clear();
        }

        XamlElement* el = elemName.empty() ? self : FindByName(*root, elemName);
        if (el == nullptr)
            return false;

        const std::wstring v = el->get(prop.c_str());
        if (v.empty())
        {
            out.isStr = false;
            out.num = 0.0;
            return true;
        }
        if (IsDataExpression(v))
        {
            std::wstring ev;
            if (!EvalDataExpression(v, el, root, depth + 1, &ev))
                return false;
            out.str = ev;
        }
        else
            out.str = v;
        out.isStr = false;

        // 全数字才算数值，其余当字符串（"1.5" 可以，"12px" 不行）
        wchar_t* endp = nullptr;
        const double d = wcstod(out.str.c_str(), &endp);
        if (endp != nullptr && *endp == L'\0' && !out.str.empty())
        {
            out.num = d;
            out.isStr = false;
        }
        else
            out.isStr = true;
        return true;
    }

    class ExprParser
    {
    public:
        const wchar_t* p = nullptr;
        XamlElement* self = nullptr;
        XamlElement* root = nullptr;
        int depth = 0;
        bool ok = true;

        explicit ExprParser(int d) : depth(d) {}

        void skipWs()
        {
            while (*p == L' ' || *p == L'\t' || *p == L'\r' || *p == L'\n')
                ++p;
        }

        ExprValue parse()
        {
            ExprValue v = parseAdd();
            skipWs();
            if (*p != L'\0')                 // 有多余字符 = 语法错，宁可不算
                ok = false;
            return v;
        }

        ExprValue parseAdd()
        {
            ExprValue a = parseMul();
            for (;;)
            {
                skipWs();
                if (!ok || *p != L'+' && *p != L'-')
                    return a;
                const wchar_t op = *p++;
                ExprValue b = parseMul();
                if (!ok)
                    return a;
                if (op == L'+' && (a.isStr || b.isStr))
                {
                    ExprValue r;
                    r.isStr = true;
                    r.str = (a.isStr ? a.str : format(a)) + (b.isStr ? b.str : format(b));
                    a = r;
                }
                else
                {
                    a.num = (op == L'+') ? a.num + b.num : a.num - b.num;
                    a.isStr = false;
                }
            }
        }

        ExprValue parseMul()
        {
            ExprValue a = parseUnary();
            for (;;)
            {
                skipWs();
                if (!ok || (*p != L'*' && *p != L'/' && *p != L'%'))
                    return a;
                const wchar_t op = *p++;
                ExprValue b = parseUnary();
                if (!ok)
                    return a;
                if (op == L'*')
                    a.num *= b.num;
                else if (op == L'%')
                    a.num = (b.num != 0.0) ? fmod(a.num, b.num) : 0.0;
                else
                    a.num = (b.num != 0.0) ? a.num / b.num : 0.0;
                a.isStr = false;
            }
        }

        ExprValue parseUnary()
        {
            skipWs();
            if (*p == L'-')
            {
                ++p;
                ExprValue v = parseUnary();
                v.num = -v.num;
                v.isStr = false;
                return v;
            }
            if (*p == L'+')
            {
                ++p;
                return parseUnary();
            }
            return parsePrimary();
        }

        ExprValue parsePrimary()
        {
            skipWs();
            ExprValue v;
            if (*p == L'(')
            {
                ++p;
                v = parseAdd();
                skipWs();
                if (*p != L')')
                    ok = false;
                else
                    ++p;
                return v;
            }
            if (*p == L'\'' || *p == L'"')
            {
                const wchar_t q = *p++;
                std::wstring s;
                while (*p != L'\0' && *p != q)
                    s += *p++;
                if (*p != q)
                    ok = false;
                else
                    ++p;
                v.isStr = true;
                v.str = s;
                return v;
            }
            if (iswdigit(*p) || *p == L'.')
            {
                wchar_t* endp = nullptr;
                v.num = wcstod(p, &endp);
                p = endp;
                v.isStr = false;
                return v;
            }
            if (IsIdentStart(*p))
            {
                std::wstring ident;
                while (IsIdentChar(*p))
                    ident += *p++;
                // 函数调用
                skipWs();
                if (*p == L'(' && !ident.empty())
                {
                    ++p;
                    std::vector<double> args;
                    ExprValue sv;
                    bool hasStr = false;
                    for (;;)
                    {
                        skipWs();
                        ExprValue a = parseAdd();
                        if (a.isStr)
                        {
                            hasStr = true;
                            sv = a;
                        }
                        args.push_back(a.num);
                        skipWs();
                        if (*p == L',')
                        {
                            ++p;
                            continue;
                        }
                        break;
                    }
                    if (*p != L')')
                        ok = false;
                    else
                        ++p;
                    v = callFunction(ident, args, hasStr, sv);
                    return v;
                }
                if (!ResolveIdent(ident, self, root, depth, v))
                    ok = false;
                return v;
            }
            ok = false;
            return v;
        }

        ExprValue callFunction(const std::wstring& fn, const std::vector<double>& a, bool hasStr,
                               const ExprValue& strArg)
        {
            ExprValue v;
            if (hasStr)
            {
                v.isStr = true;
                v.str = (fn == L"len" || fn == L"Len") ? std::to_wstring(strArg.str.size()) : strArg.str;
                if (fn == L"len" || fn == L"Len")
                    v.isStr = false, v.num = static_cast<double>(strArg.str.size());
                return v;
            }
            if (fn == L"min" || fn == L"Min")
                v.num = a.empty() ? 0 : a[0];
            else if (fn == L"max" || fn == L"Max")
                v.num = a.empty() ? 0 : a[0];
            else if (fn == L"abs" || fn == L"Abs")
                v.num = a.empty() ? 0 : fabs(a[0]);
            else if (fn == L"round" || fn == L"Round")
                v.num = a.empty() ? 0 : floor(a[0] + 0.5);
            else if (fn == L"floor" || fn == L"Floor")
                v.num = a.empty() ? 0 : floor(a[0]);
            else if (fn == L"ceil" || fn == L"Ceil")
                v.num = a.empty() ? 0 : ceil(a[0]);
            else if (fn == L"int" || fn == L"Int")
                v.num = a.empty() ? 0 : static_cast<double>(static_cast<long long>(a[0]));
            else if (fn == L"clamp" || fn == L"Clamp")
                v.num = a.empty() ? 0 : a[0];
            else
            {
                v.num = 0;
                ok = false;
                return v;
            }
            if ((fn == L"min" || fn == L"Min" || fn == L"max" || fn == L"Max") && a.size() > 1)
            {
                const bool wantMin = (fn == L"min" || fn == L"Min");
                double acc = a[0];
                for (size_t i = 1; i < a.size(); ++i)
                    acc = wantMin ? (std::min)(acc, a[i]) : (std::max)(acc, a[i]);
                v.num = acc;
            }
            if (fn == L"clamp" || fn == L"Clamp")
            {
                if (a.size() >= 3)
                    v.num = (std::max)(a[1], (std::min)(a[2], a[0]));
                else
                    v.num = a.empty() ? 0 : a[0];
            }
            v.isStr = false;
            return v;
        }

        static std::wstring format(const ExprValue& v)        {
            if (v.isStr)
                return v.str;
            if (fabs(v.num - floor(v.num)) < 1e-9 && fabs(v.num) < 1e12)
            {
                wchar_t buf[64] = {};
                swprintf_s(buf, L"%.0f", v.num);
                return buf;
            }
            wchar_t buf[64] = {};
            swprintf_s(buf, L"%.3f", v.num);
            std::wstring s = buf;
            while (!s.empty() && s.back() == L'0')
                s.pop_back();
            if (!s.empty() && s.back() == L'.')
                s.pop_back();
            return s;
        }
    };

    bool EvalDataExpression(const std::wstring& raw, XamlElement* self, XamlElement* root,
                            int depth, std::wstring* out)
    {
        if (depth > 12)                    // 引用环 / 太深的链，直接判失败
            return false;
        const std::wstring inner = DataExpressionInner(raw);
        if (inner.empty())
            return false;
        ExprParser ps(depth);
        ps.p = inner.c_str();
        ps.self = self;
        ps.root = root;
        const ExprValue v = ps.parse();
        if (!ps.ok)
            return false;
        if (out != nullptr)
            *out = ExprParser::format(v);
        return true;
    }

    // 表达式改的是有效属性，几何必须在求值后重新提取一遍，
    // 否则 Width="{...}" 会被 wtoi 当成 0 塌掉。
    void RecomputeGeometry(XamlElement& node)
    {
        int dw = 100, dh = 28;
        defaultSizeFor(node.tag, dw, dh);
        const std::wstring wAttr = node.get(L"Width");
        const std::wstring hAttr = node.get(L"Height");
        const bool wAuto = AttrIsAuto(wAttr);
        const bool hAuto = AttrIsAuto(hAttr);
        node.hasW = !wAttr.empty() && !wAuto;
        node.hasH = !hAttr.empty() && !hAuto;
        node.w = (wAttr.empty() || wAuto) ? dw : IntAttr(wAttr, dw);
        node.h = (hAttr.empty() || hAuto) ? dh : IntAttr(hAttr, dh);
        if (!node.hasH)
        {
            const std::wstring bare = xamlmfc::BareTag(node.tag);
            if (_wcsicmp(bare.c_str(), L"ToolBar") == 0) node.h = 32;
            else if (_wcsicmp(bare.c_str(), L"MenuBar") == 0) node.h = 24;
            else if (_wcsicmp(bare.c_str(), L"StatusBar") == 0) node.h = 24;
        }
        node.x = IntAttr(node.get(L"Left"), 0);
        node.y = IntAttr(node.get(L"Top"), 0);
        node.hasX = node.has(L"Left");
        node.hasY = node.has(L"Top");
        ParseThickness(node.get(L"Margin"), node);
        ParsePadding(node.get(L"Padding"), node);
        node.spacing = IntAttr(node.get(L"Spacing"), 0);
        node.horizontal = _wcsicmp(node.get(L"Orientation").c_str(), L"Horizontal") == 0;
        node.hasOrientation = !node.get(L"Orientation").empty();
        for (auto& c : node.children)
            RecomputeGeometry(c);
    }
}

bool xamlIsExpression(const std::wstring& value)
{
    return IsDataExpression(value);
}

void XamlRuntime::applyExpressions()
{
    if (!m_root)
        return;
    std::function<void(XamlElement&)> walk = [&](XamlElement& el) {
        for (const auto& kv : el.attrs)
        {
            if (!IsDataExpression(kv.second))
                continue;
            std::wstring v;
            if (EvalDataExpression(kv.second, &el, m_root.get(), 0, &v))
                SetEffective(el, kv.first, v);
        }
        for (auto& c : el.children)
            walk(c);
    };
    walk(*m_root);
    RecomputeGeometry(*m_root);
}

void XamlRuntime::applyBindings()
{
    if (!m_root)
        return;

    // 表达式先落地：绑定与后续读取都站在"求值后的有效值"上
    applyExpressions();

    // 每次都重新采集：模板实例化后视觉层里也可能带 {Binding}，表要跟着更新。
    // 求值用"元素指针"直接定位目标——模板里的元素通常没有 Name，按名字找会落到根上。
    m_bindings.clear();
    std::vector<std::pair<XamlElement*, XamlBinding>> local;
    {
        std::function<void(XamlElement&)> walk = [&](XamlElement& el) {
            for (const auto& kv : el.attrs)
            {
                std::wstring src, prop;
                if (xamlIsBindingExpr(kv.second, src, prop))
                {
                    XamlBinding b;
                    b.targetName = el.name;
                    b.targetProp = kv.first;
                    b.sourceName = src;
                    b.sourceProp = prop;
                    const std::wstring& raw = kv.second;
                    if (raw.find(L"TwoWay") != std::wstring::npos ||
                        raw.find(L"twoWay") != std::wstring::npos)
                        b.twoWay = true;
                    m_bindings.push_back(b);
                    local.emplace_back(&el, b);
                }
            }
            for (auto& c : el.children)
                walk(c);
        };
        walk(*m_root);
    }

    for (auto& lb : local)
    {
        XamlElement* target = lb.first;
        if (target == nullptr)
            continue;
        XamlElement* source = lb.second.sourceName.empty()
                                  ? target
                                  : FindByName(*m_root, lb.second.sourceName);
        if (source == nullptr)
            continue;
        SetEffective(*target, lb.second.targetProp, source->get(lb.second.sourceProp.c_str()));
        SyncText(*target);
    }
}

bool XamlRuntime::setProperty(const std::wstring& name, const std::wstring& prop,
                              const std::wstring& value)
{
    if (!m_root)
        return false;
    XamlElement* el = name.empty() ? m_root.get() : FindByName(*m_root, name);
    if (el == nullptr)
        return false;

    // 双向绑定：目标被修改时回写源，再由 applyBindings 广播到所有绑定目标
    if (m_bindings.empty())
        applyBindings();
    bool twoWayHit = false;
    for (const auto& b : m_bindings)
    {
        if (!b.twoWay)
            continue;
        if (_wcsicmp(b.targetProp.c_str(), prop.c_str()) != 0)
            continue;
        if (_wcsicmp(b.targetName.c_str(), name.c_str()) != 0)
            continue;
        XamlElement* source = b.sourceName.empty() ? el : FindByName(*m_root, b.sourceName);
        if (source == nullptr)
            continue;
        source->set(b.sourceProp.c_str(), value);
        SyncText(*source);
        twoWayHit = true;
    }

    el->set(prop.c_str(), value);
    SyncText(*el);
    // 同步到控件实例：QML 里改 model / items / text 要立刻反映到界面上
    {
        auto it = m_ctrlByElement.find(el);
        if (it != m_ctrlByElement.end() && it->second != nullptr)
            it->second->setProperty(prop.c_str(), value);
    }
    applyBindings();   // 双向或单向都重算一次，保证依赖该值的元素同步
    (void)twoWayHit;
    return true;
}

XamlElement* XamlRuntime::elementByName(const std::wstring& name)
{
    if (!m_root)
        return nullptr;
    if (name.empty())
        return m_root.get();
    return FindByName(*m_root, name);
}

std::wstring XamlRuntime::kindOfElement(const std::wstring& name)
{
    XamlElement* el = elementByName(name);
    return el == nullptr ? std::wstring() : KindOf(el->tag);
}

bool XamlRuntime::refreshItemView(const std::wstring& name)
{
    XamlElement* el = elementByName(name);
    if (el == nullptr)
        return false;
    if (_wcsicmp(KindOf(el->tag).c_str(), L"itemview") != 0)
        return false;
    ExpandDelegate(*el, &m_templates);
    return true;
}

void XamlRuntime::applyDeclarations()
{
    applyStyles();
    applyTemplates();
    applyBindings();
}

// ===========================================================================
// 属性 schema 导出（S2）：设计器属性面板 / Python 内省共用
// ===========================================================================
namespace
{
    const char* PropTypeName(XamlPropType t)
    {
        switch (t)
        {
        case XamlPropType::Int: return "Int";
        case XamlPropType::String: return "String";
        case XamlPropType::Bool: return "Bool";
        case XamlPropType::Color: return "Color";
        case XamlPropType::Enum: return "Enum";
        case XamlPropType::Items: return "Items";
        case XamlPropType::Double: return "Double";
        case XamlPropType::Point: return "Point";
        case XamlPropType::Rect: return "Rect";
        case XamlPropType::Thickness: return "Thickness";
        case XamlPropType::Image: return "Image";
        case XamlPropType::Columns: return "Columns";
        case XamlPropType::Rows: return "Rows";
        case XamlPropType::Flags: return "Flags";
        default: return "String";
        }
    }

    // 属性名/枚举值都是 ASCII 标识符，这里只做最小转义，避免引入额外依赖
    std::string JsonAscii(const wchar_t* s)
    {
        if (s == nullptr) return std::string();
        std::string out;
        for (const wchar_t* p = s; *p != L'\0'; ++p)
        {
            const wchar_t ch = *p;
            if (ch == L'"' || ch == L'\\')
            {
                out.push_back('\\');
                out.push_back(static_cast<char>(ch));
            }
            else if (ch < 32 || ch > 126)
            {
                out += "\\u";
                char buf[8];
                _snprintf_s(buf, _countof(buf), _TRUNCATE, "%04X", static_cast<unsigned>(ch & 0xFFFF));
                out += buf;
            }
            else
            {
                out.push_back(static_cast<char>(ch));
            }
        }
        return out;
    }
}

std::string XamlPropertySchemaJson()
{
    XamlControlRegistry& reg = XamlControlRegistry::instance();
    std::set<std::type_index> seen;   // 同一实现类只导出一条（主标签）
    std::string out = "{\"version\":1,\"controls\":[";
    bool firstControl = true;

    for (const std::wstring& tag : reg.tags())
    {
        IXamlControl* control = reg.create(tag.c_str());
        if (control == nullptr)
            continue;
        const std::type_index key(typeid(*control));
        if (!seen.insert(key).second)
        {
            delete control;
            continue;
        }
        const std::vector<XamlProperty> props = control->properties();
        delete control;

        if (!firstControl) out += ",";
        firstControl = false;
        out += "{\"tag\":\"";
        out += JsonAscii(tag.c_str());
        out += "\",\"properties\":[";
        bool firstProp = true;
        for (const XamlProperty& p : props)
        {
            if (p.name == nullptr)
                continue;
            if (!firstProp) out += ",";
            firstProp = false;
            out += "{\"name\":\"";
            out += JsonAscii(p.name);
            out += "\",\"type\":\"";
            out += PropTypeName(p.type);
            out += "\",\"readOnly\":";
            out += (p.writable ? "false" : "true");
            out += ",\"enum\":\"";
            out += JsonAscii(p.enumValues);
            out += "\"}";
        }
        out += "]}";
    }
    out += "]}";
    return out;
}