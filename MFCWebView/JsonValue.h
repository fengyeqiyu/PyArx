#pragma once
// JsonValue：真正的 JSON 解析（对象 / 数组 / 字符串转义 / 数字 / 布尔 / null）。
//
// 之前 WebShell 与 PyPaletteWeb 各自用"字符串查找"读字段，代价是：
//   - 嵌套值读不到（只吞掉）；
//   - 页面传来的 \n 变成字面反斜杠+n，多行代码直接语法错误；
//   - 路径里的反斜杠必须手写两个，否则被当转义吃掉。
// 这里换成递归下降解析，字节级正确，任何一层都能按路径取。
//
// 用法：webmfc::JsonValue v; webmfc::parseJson(text, v); v.findPath(L"args.path")->asString();
#include <map>
#include <string>
#include <vector>

namespace webmfc
{
    class JsonValue
    {
    public:
        enum Kind { Null, Bool, Number, String, Array, Object };

        Kind kind = Kind::Null;
        bool boolValue = false;
        double numberValue = 0.0;
        std::wstring stringValue;
        std::vector<JsonValue> array;
        std::map<std::wstring, JsonValue> object;

        bool isObject() const { return kind == Kind::Object; }
        bool isArray() const { return kind == Kind::Array; }
        bool isString() const { return kind == Kind::String; }
        bool isNumber() const { return kind == Kind::Number; }
        bool isBool() const { return kind == Kind::Bool; }

        // 只在本层找键（对象）；数组按下标找
        const JsonValue* find(const std::wstring& key) const;
        // 按 "a.b.c" / "list.0.name" 逐层找，任一层缺失返回 nullptr
        const JsonValue* findPath(const std::wstring& path) const;

        std::wstring asString(const std::wstring& dflt = L"") const;
        double asNumber(double dflt = 0.0) const;
        int asInt(int dflt = 0) const;
        bool asBool(bool dflt = false) const;
    };

    // 解析整段文本；失败时 error 里给"第几个字符附近"的定位（便于页面调试）
    bool parseJson(const std::wstring& text, JsonValue& out, std::wstring* error = nullptr);

    // 转成 JSON 字符串字面量内容（不含两侧引号）
    std::wstring escapeJson(const std::wstring& text);

    // 便捷读：路径不存在或类型不符时返回默认值
    std::wstring jsonString(const JsonValue& root, const std::wstring& path,
                            const std::wstring& dflt = L"");
    double jsonNumber(const JsonValue& root, const std::wstring& path, double dflt = 0.0);
    int jsonInt(const JsonValue& root, const std::wstring& path, int dflt = 0);
    bool jsonBool(const JsonValue& root, const std::wstring& path, bool dflt = false);
}
