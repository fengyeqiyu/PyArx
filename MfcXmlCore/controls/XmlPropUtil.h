#pragma once
// XmlPropUtil —— 控件属性读写小工具（大小写不敏感匹配 + 宽窄转换）。
// 编码：本文件 UTF-8 with BOM。

#include <string>

namespace xmlprop
{
    inline bool StrEq(const std::string& a, const std::string& b)
    {
        if (a.size() != b.size())
            return false;
        for (size_t i = 0; i < a.size(); ++i)
        {
            if (tolower((unsigned char)a[i]) != tolower((unsigned char)b[i]))
                return false;
        }
        return true;
    }

    inline std::string WideToUtf8(const std::wstring& w)
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

    // 属性名统一小写（用于 StrEq 匹配）
    inline std::string Lower(const std::string& s)
    {
        std::string out = s;
        for (char& c : out)
            c = (char)tolower((unsigned char)c);
        return out;
    }

    // bool 值："1"/"true"/"yes" -> true
    inline bool ToBool(const std::wstring& v)
    {
        return !v.empty() && v != L"0" && v != L"false" && v != L"no";
    }
}
