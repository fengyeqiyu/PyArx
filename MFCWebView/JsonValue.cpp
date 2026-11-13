#include "JsonValue.h"

#include <cmath>
#include <cwchar>
#include <cwctype>

namespace webmfc
{
    namespace
    {
        class Parser
        {
        public:
            Parser(const std::wstring& text, std::wstring* error)
                : m_text(text), m_error(error) {}

            bool run(JsonValue& out)
            {
                if (!parseValue(out))
                    return false;
                skipSpace();
                if (m_pos < m_text.size())
                    return fail(L"根值之后还有多余字符");
                return true;
            }

        private:
            const std::wstring& m_text;
            std::wstring* m_error;
            size_t m_pos = 0;

            bool fail(const wchar_t* what)
            {
                if (m_error != nullptr)
                {
                    *m_error = std::wstring(what) + L"（位置 " + std::to_wstring(m_pos) + L"）";
                }
                return false;
            }

            void skipSpace()
            {
                while (m_pos < m_text.size())
                {
                    const wchar_t c = m_text[m_pos];
                    if (c == L' ' || c == L'\t' || c == L'\r' || c == L'\n')
                    {
                        ++m_pos;
                        continue;
                    }
                    break;
                }
            }

            bool parseValue(JsonValue& out)
            {
                skipSpace();
                if (m_pos >= m_text.size())
                    return fail(L"值缺失");

                const wchar_t c = m_text[m_pos];
                if (c == L'{')
                    return parseObject(out);
                if (c == L'[')
                    return parseArray(out);
                if (c == L'"')
                {
                    out.kind = JsonValue::String;
                    return parseString(out.stringValue);
                }
                if (c == L't' || c == L'f')
                    return parseBool(out);
                if (c == L'n')
                    return parseNull(out);
                return parseNumber(out);
            }

            bool parseObject(JsonValue& out)
            {
                out.kind = JsonValue::Object;
                ++m_pos;   // '{'
                skipSpace();
                if (m_pos < m_text.size() && m_text[m_pos] == L'}')
                {
                    ++m_pos;
                    return true;
                }

                for (;;)
                {
                    skipSpace();
                    std::wstring key;
                    if (!parseString(key))
                        return false;
                    skipSpace();
                    if (m_pos >= m_text.size() || m_text[m_pos] != L':')
                        return fail(L"对象里缺冒号");
                    ++m_pos;
                    JsonValue item;
                    if (!parseValue(item))
                        return false;
                    out.object[key] = std::move(item);
                    skipSpace();
                    if (m_pos < m_text.size() && m_text[m_pos] == L',')
                    {
                        ++m_pos;
                        continue;
                    }
                    if (m_pos < m_text.size() && m_text[m_pos] == L'}')
                    {
                        ++m_pos;
                        return true;
                    }
                    return fail(L"对象里缺逗号或右花括号");
                }
            }

            bool parseArray(JsonValue& out)
            {
                out.kind = JsonValue::Array;
                ++m_pos;   // '['
                skipSpace();
                if (m_pos < m_text.size() && m_text[m_pos] == L']')
                {
                    ++m_pos;
                    return true;
                }

                for (;;)
                {
                    JsonValue item;
                    if (!parseValue(item))
                        return false;
                    out.array.push_back(std::move(item));
                    skipSpace();
                    if (m_pos < m_text.size() && m_text[m_pos] == L',')
                    {
                        ++m_pos;
                        continue;
                    }
                    if (m_pos < m_text.size() && m_text[m_pos] == L']')
                    {
                        ++m_pos;
                        return true;
                    }
                    return fail(L"数组里缺逗号或右方括号");
                }
            }

            bool parseString(std::wstring& out)
            {
                skipSpace();
                if (m_pos >= m_text.size() || m_text[m_pos] != L'"')
                    return fail(L"期望字符串");

                ++m_pos;
                out.clear();
                while (m_pos < m_text.size())
                {
                    wchar_t c = m_text[m_pos++];
                    if (c == L'"')
                        return true;

                    if (c != L'\\')
                    {
                        out.push_back(c);
                        continue;
                    }

                    if (m_pos >= m_text.size())
                        return fail(L"转义符后面没有字符");

                    const wchar_t e = m_text[m_pos++];
                    switch (e)
                    {
                    case L'"': out.push_back(L'"'); break;
                    case L'\\': out.push_back(L'\\'); break;
                    case L'/': out.push_back(L'/'); break;
                    case L'b': out.push_back(L'\b'); break;
                    case L'f': out.push_back(L'\f'); break;
                    case L'n': out.push_back(L'\n'); break;
                    case L'r': out.push_back(L'\r'); break;
                    case L't': out.push_back(L'\t'); break;
                    case L'u':
                    {
                        // \uXXXX：直接按 UTF-16 存；成对代理项原样连着放，正好是 Windows 原生编码
                        if (m_pos + 4 > m_text.size())
                            return fail(L"\\u 后面不足 4 位十六进制");
                        wchar_t hex[5] = { 0 };
                        for (int k = 0; k < 4; ++k)
                            hex[k] = m_text[m_pos + k];
                        for (int k = 0; k < 4; ++k)
                        {
                            const wchar_t h = hex[k];
                            const bool okHex = (h >= L'0' && h <= L'9') || (h >= L'a' && h <= L'f') ||
                                               (h >= L'A' && h <= L'F');
                            if (!okHex)
                                return fail(L"\\u 里有非十六进制字符");
                        }
                        m_pos += 4;
                        const unsigned long v = wcstoul(hex, nullptr, 16);
                        out.push_back(static_cast<wchar_t>(v));
                        break;
                    }
                    default:
                        return fail(L"不认识的转义符");
                    }
                }
                return fail(L"字符串没有闭合");
            }

            bool parseNumber(JsonValue& out)
            {
                skipSpace();
                const size_t begin = m_pos;
                if (m_pos < m_text.size() && (m_text[m_pos] == L'-' || m_text[m_pos] == L'+'))
                    ++m_pos;
                while (m_pos < m_text.size())
                {
                    const wchar_t c = m_text[m_pos];
                    if ((c >= L'0' && c <= L'9') || c == L'.' || c == L'e' || c == L'E' ||
                        c == L'+' || c == L'-')
                    {
                        ++m_pos;
                        continue;
                    }
                    break;
                }
                if (m_pos == begin)
                    return fail(L"不是合法的 JSON 值");

                const std::wstring token = m_text.substr(begin, m_pos - begin);
                out.kind = JsonValue::Number;
                out.numberValue = wcstod(token.c_str(), nullptr);
                return true;
            }

            bool parseBool(JsonValue& out)
            {
                if (m_text.compare(m_pos, 4, L"true") == 0)
                {
                    m_pos += 4;
                    out.kind = JsonValue::Bool;
                    out.boolValue = true;
                    return true;
                }
                if (m_text.compare(m_pos, 5, L"false") == 0)
                {
                    m_pos += 5;
                    out.kind = JsonValue::Bool;
                    out.boolValue = false;
                    return true;
                }
                return fail(L"不是合法的 true/false");
            }

            bool parseNull(JsonValue& out)
            {
                if (m_text.compare(m_pos, 4, L"null") == 0)
                {
                    m_pos += 4;
                    out.kind = JsonValue::Null;
                    return true;
                }
                return fail(L"不是合法的 null");
            }
        };
    }

    const JsonValue* JsonValue::find(const std::wstring& key) const
    {
        if (kind == Kind::Object)
        {
            auto it = object.find(key);
            return it == object.end() ? nullptr : &it->second;
        }
        if (kind == Kind::Array)
        {
            const int index = _wtoi(key.c_str());
            if (index >= 0 && static_cast<size_t>(index) < array.size())
                return &array[static_cast<size_t>(index)];
        }
        return nullptr;
    }

    const JsonValue* JsonValue::findPath(const std::wstring& path) const
    {
        if (path.empty())
            return this;

        const JsonValue* cur = this;
        size_t begin = 0;
        while (begin <= path.size() && cur != nullptr)
        {
            size_t dot = path.find(L'.', begin);
            const std::wstring seg = path.substr(begin, dot == std::wstring::npos ? std::wstring::npos : dot - begin);
            cur = cur->find(seg);
            if (dot == std::wstring::npos)
                break;
            begin = dot + 1;
        }
        return cur;
    }

    std::wstring JsonValue::asString(const std::wstring& dflt) const
    {
        if (kind == Kind::String)
            return stringValue;
        if (kind == Kind::Number)
        {
            wchar_t buf[64] = { 0 };
            if (std::fabs(numberValue - static_cast<double>(static_cast<long long>(numberValue))) < 1e-9)
                swprintf_s(buf, L"%.0f", numberValue);
            else
                swprintf_s(buf, L"%g", numberValue);
            return buf;
        }
        if (kind == Kind::Bool)
            return boolValue ? L"true" : L"false";
        return dflt;
    }

    double JsonValue::asNumber(double dflt) const
    {
        if (kind == Kind::Number)
            return numberValue;
        if (kind == Kind::String)
            return wcstod(stringValue.c_str(), nullptr);
        if (kind == Kind::Bool)
            return boolValue ? 1.0 : 0.0;
        return dflt;
    }

    int JsonValue::asInt(int dflt) const
    {
        if (kind == Kind::Number)
            return static_cast<int>(numberValue);
        if (kind == Kind::String)
            return _wtoi(stringValue.c_str());
        if (kind == Kind::Bool)
            return boolValue ? 1 : 0;
        return dflt;
    }

    bool JsonValue::asBool(bool dflt) const
    {
        if (kind == Kind::Bool)
            return boolValue;
        if (kind == Kind::Number)
            return numberValue != 0.0;
        if (kind == Kind::String)
            return stringValue == L"true" || stringValue == L"1";
        return dflt;
    }

    bool parseJson(const std::wstring& text, JsonValue& out, std::wstring* error)
    {
        out = JsonValue();
        Parser parser(text, error);
        return parser.run(out);
    }

    std::wstring escapeJson(const std::wstring& text)
    {
        std::wstring r;
        r.reserve(text.size());
        for (wchar_t c : text)
        {
            switch (c)
            {
            case L'"': r += L"\\\""; break;
            case L'\\': r += L"\\\\"; break;
            case L'\n': r += L"\\n"; break;
            case L'\r': r += L"\\r"; break;
            case L'\t': r += L"\\t"; break;
            case L'\b': r += L"\\b"; break;
            case L'\f': r += L"\\f"; break;
            default:
                if (c < 0x20)
                {
                    wchar_t buf[8] = { 0 };
                    swprintf_s(buf, L"\\u%04x", static_cast<unsigned>(c));
                    r += buf;
                }
                else
                {
                    r += c;
                }
                break;
            }
        }
        return r;
    }

    std::wstring jsonString(const JsonValue& root, const std::wstring& path, const std::wstring& dflt)
    {
        const JsonValue* v = root.findPath(path);
        return v == nullptr ? dflt : v->asString(dflt);
    }

    double jsonNumber(const JsonValue& root, const std::wstring& path, double dflt)
    {
        const JsonValue* v = root.findPath(path);
        return v == nullptr ? dflt : v->asNumber(dflt);
    }

    int jsonInt(const JsonValue& root, const std::wstring& path, int dflt)
    {
        const JsonValue* v = root.findPath(path);
        return v == nullptr ? dflt : v->asInt(dflt);
    }

    bool jsonBool(const JsonValue& root, const std::wstring& path, bool dflt)
    {
        const JsonValue* v = root.findPath(path);
        return v == nullptr ? dflt : v->asBool(dflt);
    }
}
