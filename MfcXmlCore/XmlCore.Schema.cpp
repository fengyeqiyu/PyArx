// XmlCore.Schema.cpp —— 拆自 XmlCore.cpp 第 7436-7540 行。
#include "XmlCoreCommon.h"
#include "XmlCoreInternal.h"

// ===========================================================================
// 属性 schema 导出（S2）：设计器属性面板 / Python 内省共用
// ===========================================================================
namespace xmlcore_impl
{
    const char* PropTypeName(XmlPropType t)
    {
        switch (t)
        {
        case XmlPropType::Int: return "Int";
        case XmlPropType::String: return "String";
        case XmlPropType::Bool: return "Bool";
        case XmlPropType::Color: return "Color";
        case XmlPropType::Enum: return "Enum";
        case XmlPropType::Items: return "Items";
        case XmlPropType::Double: return "Double";
        case XmlPropType::Point: return "Point";
        case XmlPropType::Rect: return "Rect";
        case XmlPropType::Thickness: return "Thickness";
        case XmlPropType::Image: return "Image";
        case XmlPropType::Columns: return "Columns";
        case XmlPropType::Rows: return "Rows";
        case XmlPropType::Flags: return "Flags";
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

std::string XmlPropertySchemaJson()
{
    XmlControlRegistry& reg = XmlControlRegistry::instance();
    std::set<std::type_index> seen;   // 同一实现类只导出一条（主标签）
    std::string out = "{\"version\":1,\"controls\":[";
    bool firstControl = true;

    for (const std::wstring& tag : reg.tags())
    {
        IXmlControl* control = reg.create(tag.c_str());
        if (control == nullptr)
            continue;
        const std::type_index key(typeid(*control));
        if (!seen.insert(key).second)
        {
            delete control;
            continue;
        }
        const std::vector<XmlProperty> props = control->properties();
        delete control;

        if (!firstControl) out += ",";
        firstControl = false;
        out += "{\"tag\":\"";
        out += JsonAscii(tag.c_str());
        out += "\",\"properties\":[";
        bool firstProp = true;
        for (const XmlProperty& p : props)
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
