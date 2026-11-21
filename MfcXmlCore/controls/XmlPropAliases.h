#pragma once
// XmlPropAliases —— XML 属性名的小写写法归一（唯一来源）。
//
// 背景：XML 属性书写大小写混杂（Text / text、Radius / radius）。这里把全部属性名
// 统一小写后按表映射到内部属性名，内部属性表本身仍用首字母大写。
//      <XLabel text="运行" color="#1F2937" radius="6"/>
// 归一后交给属性表，两种写法都可用。
//
// 说明：曾用名 XmlQmlNames.h / QmlPropName，方案已改为以 HTML/CSS 为唯一基准，
// 因此 font.pixelSize / border.width / icon.source 这类 QML 专有点号写法已删除，
// 字号/边框/图标请用 CSS 侧写法（font-size / border-* / src）。
//
// 编码：本文件 UTF-8 with BOM。

#ifndef XML_PROP_ALIASES_H
#define XML_PROP_ALIASES_H

#include <string>
#include <cwchar>
#include <cwctype>

namespace xmlmfc
{
    // 属性名别名 -> 内部属性名。没有映射就原样返回。
    inline std::wstring PropAlias(const std::wstring& name)
    {
        std::wstring t = name;
        for (auto& c : t) c = (wchar_t)towlower(c);

        struct Map { const wchar_t* alias; const wchar_t* internal; };
        static const Map kMap[] = {
            // 通用外观
            { L"text",                 L"Text" },
            { L"color",                L"Foreground" },
            { L"background",           L"BackColor" },
            // foreground / foregroundColor 都是"前景（文字/图标）色"，不是前置背景层。
            { L"foreground",           L"Foreground" },
            { L"foregroundcolor",      L"Foreground" },
            { L"radius",               L"Radius" },
            { L"opacity",              L"Opacity" },
            { L"z",                    L"Level" },
            { L"padding",              L"Padding" },
            // 控件属性
            { L"checked",              L"Checked" },
            { L"checkable",            L"Checkable" },
            { L"value",                L"Value" },
            { L"from",                 L"Minimum" },
            { L"to",                   L"Maximum" },
            { L"model",                L"Items" },
            { L"readOnly",             L"IsReadOnly" },
            { L"placeholderText",      L"Placeholder" },
            { L"maximumLength",        L"MaxLength" },
            { L"display",              L"Display" },
            { L"modal",                L"Modal" },
            { L"orientation",          L"Orientation" },
            { L"delay",                L"Delay" },
            { L"position",             L"Position" },
            { L"size",                 L"Size" },
            { L"columns",              L"Columns" },
            { L"rows",                 L"Rows" },
            { L"count",                L"Count" },
            { L"title",                L"Title" },
            { L"source",               L"Image" },
            { L"link",                 L"Url" },
            { L"selectionMode",        L"SelectionMode" },
            { L"header",               L"Header" },
            { L"rowCount",             L"RowCount" },
            { L"pageCount",            L"PageCount" },
            { L"itemHeight",           L"ItemHeight" },
            { L"cellWidth",            L"CellWidth" },
            { L"cellHeight",           L"CellHeight" },
        };
        // 比较必须大小写不敏感：`t` 已经小写，而表里保留了 `readOnly` / `placeholderText`
        // 这类驼峰写法 —— 早先用 == 逐字比对，凡是带大写字母的别名全都永不命中
        // （表现是"写了 readonly 等于没写"，必须改成内部名 IsReadOnly 才生效）。
        for (const Map& m : kMap)
            if (_wcsicmp(t.c_str(), m.alias) == 0)
                return m.internal;
        return name;
    }
} // namespace xmlmfc

#endif
