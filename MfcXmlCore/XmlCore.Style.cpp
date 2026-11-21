// XmlCore.Style.cpp —— 拆自 XmlCore.cpp 第 6340-6750 行。
#include "XmlCoreCommon.h"
#include "XmlCoreInternal.h"

// ===========================================================================
// 样式 / 控件模板 / 属性绑定
// ===========================================================================
namespace xmlcore_impl
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

    XmlElement* FindByName(XmlElement& node, const std::wstring& name)
    {
        if (!name.empty() && node.name == name)
            return &node;
        for (auto& c : node.children)
            if (auto* hit = FindByName(c, name))
                return hit;
        return nullptr;
    }

    // 属性改了文字类属性后，语义字段（el.text）要跟着更新，否则渲染端读到旧值
    void SyncText(XmlElement& el)
    {
        el.text = el.get(L"Text");
        if (el.text.empty())
            el.text = el.get(L"Title");     // Title 与 Text 同义（Popup/Dialog/容器标题）
        if (el.text.empty())
            el.text = el.get(L"Content");   // Popup 的正文
        if (el.text.empty())
            el.text = el.get(L"Header");
    }

    // 模板实体化：深拷贝 + 打标记 + 替换 {TemplateBinding Prop}
    void StampTemplate(std::vector<XmlElement>& nodes, const XmlElement& owner)
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

bool xmlIsResourceRef(const std::wstring& value, std::wstring& key)
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

bool xmlIsBindingExpr(const std::wstring& value, std::wstring& source, std::wstring& prop)
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

void XmlRuntime::clearStyles()
{
    m_styles.clear();
    m_templates.clear();
    m_bindings.clear();
}

void XmlRuntime::addStyle(const XmlStyle& style)
{
    m_styles.push_back(style);
}

bool XmlRuntime::getStyle(const std::wstring& key, XmlStyle& out) const
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

std::vector<XmlStyle> XmlRuntime::styles() const
{
    return m_styles;
}

void XmlRuntime::addTemplate(const std::wstring& key, const std::vector<XmlElement>& nodes)
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

bool XmlRuntime::getTemplate(const std::wstring& key, std::vector<XmlElement>& out) const
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

std::vector<std::wstring> XmlRuntime::templateKeys() const
{
    std::vector<std::wstring> keys;
    keys.reserve(m_templates.size());
    for (const auto& kv : m_templates)
        keys.push_back(kv.first);
    return keys;
}

const std::vector<XmlBinding>& XmlRuntime::bindings() const
{
    return m_bindings;
}

void XmlRuntime::applyStyles()
{
    if (!m_root)
        return;

    std::function<void(const XmlStyle&, std::vector<XmlSetter>&, std::vector<XmlTrigger>&, int)>
        gather = [&](const XmlStyle& s, std::vector<XmlSetter>& out,
                     std::vector<XmlTrigger>& trigs, int depth) {
            if (depth > 8)
                return;
            if (!s.basedOn.empty())
            {
                XmlStyle base;
                if (getStyle(s.basedOn, base))
                    gather(base, out, trigs, depth + 1);
            }
            for (const auto& st : s.setters)
                out.push_back(st);
            for (const auto& tr : s.triggers)
                trigs.push_back(tr);
        };

    std::function<void(XmlElement&)> walk = [&](XmlElement& el) {
        std::wstring ref = el.get(L"Style");
        std::wstring key;
        if (!xmlIsResourceRef(ref, key))
            key = TrimWs(ref);

        XmlStyle st;
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
            std::vector<XmlSetter> setters;
            std::vector<XmlTrigger> triggers;
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
//   XML     —— `AT1-1|回路一;AT1-2|回路二`（; 或换行分行，竖线分字段）
//   Python   —— 绑定层把 list 拼成 `AT1-1<TAB>回路一|AT1-2<TAB>回路二`（竖线分行，制表符分字段）
// 所以先看串里有没有制表符来决定分行/分字段符号，两种写法都落到同一个 rows。
 void ExpandDelegate(XmlElement& el,
                           const std::vector<std::pair<std::wstring, std::vector<XmlElement>>>* tpls)
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
                       [](const XmlElement& c) { return c.delegateRow; }),
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
        std::vector<XmlElement> copy = el.delegateTemplate;
        ApplyRowFields(copy[0], rows[r], (int)r);
        MarkDelegateRow(copy[0]);
        el.children.push_back(std::move(copy[0]));
    }
}

void XmlRuntime::applyTemplates()
{
    if (!m_root)
        return;

    std::function<void(XmlElement&)> walk = [&](XmlElement& el) {
        // QML delegate：XItemView 的唯一子节点是行模板，按 Model 的行数实例化
        if (_wcsicmp(KindOf(el.tag).c_str(), L"itemview") == 0)
            ExpandDelegate(el, &m_templates);
        // 先清掉上一次实体化的模板子树，再递归真实子节点
        el.children.erase(
            std::remove_if(el.children.begin(), el.children.end(),
                           [](const XmlElement& c) { return c.templateChild; }),
            el.children.end());
        for (auto& c : el.children)
            walk(c);

        std::wstring ref = el.get(L"Template");
        std::wstring key;
        if (!xmlIsResourceRef(ref, key))
            key = TrimWs(ref);
        if (key.empty())
            return;
        std::vector<XmlElement> tpl;
        if (!getTemplate(key, tpl) || tpl.empty())
            return;
        StampTemplate(tpl, el);
        el.children.insert(el.children.end(), tpl.begin(), tpl.end());
    };
    walk(*m_root);
}
