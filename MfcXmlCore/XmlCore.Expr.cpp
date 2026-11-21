// XmlCore.Expr.cpp —— 拆自 XmlCore.cpp 第 6751-7435 行。
#include "XmlCoreCommon.h"
#include "XmlCoreInternal.h"

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
namespace xmlcore_impl
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

    bool EvalDataExpression(const std::wstring& raw, XmlElement* self, XmlElement* root,
                            int depth, std::wstring* out);

    // 标识符取值：Name.Prop 走树查找；裸 Prop 取当前元素；值本身是表达式时继续递归
    bool ResolveIdent(const std::wstring& ident, XmlElement* self, XmlElement* root,
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

        XmlElement* el = elemName.empty() ? self : FindByName(*root, elemName);
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
        XmlElement* self = nullptr;
        XmlElement* root = nullptr;
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

    bool EvalDataExpression(const std::wstring& raw, XmlElement* self, XmlElement* root,
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
    void RecomputeGeometry(XmlElement& node)
    {
        int dw = 100, dh = 28;
        defaultSizeFor(node.tag, dw, dh);
        const std::wstring wAttr = node.get(L"Width");
        const std::wstring hAttr = node.get(L"Height");
        const bool wAuto = AttrIsAuto(wAttr);
        const bool hAuto = AttrIsAuto(hAttr);
        // 几何重算也必须认百分比（与主解析路径保持一致），
        // 否则 Width="100%" 在下一轮重算时又被当没写宽度
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
        if (node.hasW) node.autoW = false;
        // autoH = 高度由内容算出来的（HTML 块容器）：重算几何时不能被当成"没写高度"
        node.hasH = (!hAttr.empty() && !hAuto && !hPct) || (node.autoH && node.h > 0);
        node.w = (wAttr.empty() || wAuto || wPct) ? dw : IntAttr(wAttr, dw);
        node.h = node.autoH
            ? node.h
            : ((hAttr.empty() || hAuto || hPct) ? dh : IntAttr(hAttr, dh));
        // CSS box-sizing:border-box（与主解析路径同步；boxSizingBorder 保证只扣一次）
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
        // HTML 默认值表（与主解析路径保持一致）：<button>/<input> 不写尺寸时按控件自身尺寸
        {
            const std::wstring kind = KindOf(node.tag);
            const bool htmlBtn = (kind == L"button");
            const bool htmlInput = (kind == L"edit" || kind == L"textfield" || kind == L"input");
            const bool htmlLabel = (node.tag == L"label" || node.has(L"HtmlLabel"));
            if (htmlBtn || htmlInput || htmlLabel)
            {
                if (wAttr.empty() || wAuto)
                {
                    node.w = htmlBtn ? 88 : 160;
                    // 与主解析路径同一条 HTML 默认值：原生 <input> 按 20 字符宽
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
                        node.w = (std::max)(72, (int)(20 * pt * 0.55 + 0.5) + 8);
                    }
                    // 与主解析路径同一条 HTML 默认值：<label> 按文字宽
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
                        node.w = body.empty() ? 100 : (int)(px + 0.5) + node.padL + node.padR + 4;
                    }
                    node.hasW = true;
                    node.autoW = !htmlLabel;
                }
                if ((hAttr.empty() || hAuto) && !node.autoH)
                {
                    node.h = htmlBtn ? 30 : 26;
                    if (htmlLabel)
                        node.h = 24;
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
        node.x = IntAttr(node.get(L"Left"), 0);
        node.y = IntAttr(node.get(L"Top"), 0);
        node.hasX = node.has(L"Left");
        node.hasY = node.has(L"Top");
        ParseThickness(node.get(L"Margin"), node);
        ParsePadding(node.get(L"Padding"), node);
        // 没有声明就保留既有值：片段根/程序里的默认间距不能被"重算几何"抹成 0
        if (node.has(L"Spacing"))
            node.spacing = IntAttr(node.get(L"Spacing"), node.spacing);
        if (node.has(L"Orientation"))
        {
            node.horizontal = _wcsicmp(node.get(L"Orientation").c_str(), L"Horizontal") == 0;
            node.hasOrientation = true;
        }
        for (auto& c : node.children)
            RecomputeGeometry(c);
    }
}

bool xmlIsExpression(const std::wstring& value)
{
    return IsDataExpression(value);
}

void XmlRuntime::applyExpressions()
{
    if (!m_root)
        return;
    std::function<void(XmlElement&)> walk = [&](XmlElement& el) {
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

void XmlRuntime::applyBindings()
{
    if (!m_root)
        return;

    // 表达式先落地：绑定与后续读取都站在"求值后的有效值"上
    applyExpressions();

    // 每次都重新采集：模板实例化后视觉层里也可能带 {Binding}，表要跟着更新。
    // 求值用"元素指针"直接定位目标——模板里的元素通常没有 Name，按名字找会落到根上。
    m_bindings.clear();
    std::vector<std::pair<XmlElement*, XmlBinding>> local;
    {
        std::function<void(XmlElement&)> walk = [&](XmlElement& el) {
            for (const auto& kv : el.attrs)
            {
                std::wstring src, prop;
                if (xmlIsBindingExpr(kv.second, src, prop))
                {
                    XmlBinding b;
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
        XmlElement* target = lb.first;
        if (target == nullptr)
            continue;
        XmlElement* source = lb.second.sourceName.empty()
                                  ? target
                                  : FindByName(*m_root, lb.second.sourceName);
        if (source == nullptr)
            continue;
        SetEffective(*target, lb.second.targetProp, source->get(lb.second.sourceProp.c_str()));
        SyncText(*target);
    }
}

bool XmlRuntime::setProperty(const std::wstring& name, const std::wstring& prop,
                              const std::wstring& value)
{
    if (!m_root)
        return false;
    XmlElement* el = name.empty() ? m_root.get() : FindByName(*m_root, name);
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
        XmlElement* source = b.sourceName.empty() ? el : FindByName(*m_root, b.sourceName);
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

XmlElement* XmlRuntime::elementByName(const std::wstring& name)
{
    if (!m_root)
        return nullptr;
    if (name.empty())
        return m_root.get();
    return FindByName(*m_root, name);
}

bool XmlRuntime::attachFlexContainer(XmlElement& el)
{
    const bool ok = AttachPanel(el);
    if (GetEnvironmentVariableW(L"PYARX_FLEX_DBG", nullptr, 0) != 0)
    {
        wchar_t tmp[MAX_PATH] = {0};
        GetTempPathW(MAX_PATH, tmp);
        FILE* f = _wfopen((std::wstring(tmp) + L"flexpanel.log").c_str(), L"a");
        if (f != nullptr)
        {
            fwprintf(f, L"attach tag=%s ctrl=%d kids=%d ok=%d\n", el.tag.c_str(),
                     el.control != nullptr ? 1 : 0, (int)el.children.size(), ok ? 1 : 0);
            fclose(f);
        }
    }
    return ok;
}

std::wstring XmlRuntime::kindOfElement(const std::wstring& name)
{
    XmlElement* el = elementByName(name);
    return el == nullptr ? std::wstring() : KindOf(el->tag);
}

bool XmlRuntime::refreshItemView(const std::wstring& name)
{
    XmlElement* el = elementByName(name);
    if (el == nullptr)
        return false;
    if (_wcsicmp(KindOf(el->tag).c_str(), L"itemview") != 0)
        return false;
    ExpandDelegate(*el, &m_templates);
    return true;
}

void XmlRuntime::applyDeclarations()
{
    applyStyles();
    applyTemplates();
    applyBindings();
}
