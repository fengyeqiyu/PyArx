// XmlCore.Element.cpp —— 拆自 XmlCore.cpp 第 22-114 行。
#include "XmlCoreCommon.h"
#include "XmlCoreInternal.h"

// ===========================================================================
// XmlElement
// ===========================================================================
namespace xmlcore_impl
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

    bool HasLocal(const XmlElement& el, const std::wstring& key)
    {
        for (const auto& kv : el.attrs)
            if (_wcsicmp(kv.first.c_str(), key.c_str()) == 0)
                return true;
        return false;
    }

    void SetEffective(XmlElement& el, const std::wstring& key, const std::wstring& v)
    {
        for (auto& kv : el.styledAttrs)
            if (_wcsicmp(kv.first.c_str(), key.c_str()) == 0)
            {
                kv.second = v;
                return;
            }
        el.styledAttrs.emplace_back(key, v);
    }

    void ClearEffective(XmlElement& el)
    {
        el.styledAttrs.clear();
        el.styled = false;
    }
}

bool XmlElement::has(const wchar_t* key) const
{
    for (const auto& kv : attrs)
        if (_wcsicmp(kv.first.c_str(), key) == 0)
            return true;
    for (const auto& kv : styledAttrs)
        if (_wcsicmp(kv.first.c_str(), key) == 0)
            return true;
    return false;
}

std::wstring XmlElement::get(const wchar_t* key) const
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

void XmlElement::set(const wchar_t* key, const std::wstring& v)
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
