#pragma once
// XmlQmlNames —— 属性名的 QML 写法映射（唯一来源）。
//
// 目的：XML 里的属性名可以按 QML 的写法写，渲染仍是 MFC 自绘。
//      <XLabel text="运行" color="#1F2937" font.pixelSize="13"/>
//      <XButton onClicked="on_run" radius="6"/>
// 归一后交给现有属性表，旧写法（Text / Foreground / FontSize）继续可用。
//
// 编码：本文件 UTF-8 with BOM。

#ifndef XML_QML_NAMES_H
#define XML_QML_NAMES_H

#include <string>
#include <cwctype>

namespace xmlmfc
{
    // QML 写法 -> 内部属性名。没有映射就原样返回。
    inline std::wstring QmlPropName(const std::wstring& name)
    {
        std::wstring t = name;
        for (auto& c : t) c = (wchar_t)towlower(c);

        struct Map { const wchar_t* qml; const wchar_t* internal; };
        static const Map kMap[] = {
            // 通用外观
            { L"text",                 L"Text" },
            { L"color",                L"Foreground" },
            { L"background",           L"BackColor" },
            { L"foreground",           L"FrontColor" },
            { L"radius",               L"Radius" },
            { L"opacity",              L"Opacity" },
            { L"z",                    L"Level" },
            { L"border.color",         L"BorderColor" },
            { L"border.width",         L"BorderThickness" },
            { L"border.style",         L"BorderStyle" },
            { L"font.pixelSize",       L"FontSize" },
            { L"font.family",          L"FontFamily" },
            { L"font.weight",          L"FontWeight" },
            { L"font.style",           L"FontStyle" },
            { L"horizontalAlignment",  L"TextAlign" },
            { L"verticalAlignment",    L"VerticalAlignment" },
            { L"wrapMode",             L"TextWrapping" },
            { L"padding",              L"Padding" },
            // 控件属性
            { L"checked",              L"Checked" },
            { L"checkable",            L"Checkable" },
            { L"value",                L"Value" },
            { L"from",                 L"Minimum" },
            { L"to",                   L"Maximum" },
            { L"currentIndex",         L"SelectedIndex" },
            { L"model",                L"Items" },
            { L"readOnly",             L"IsReadOnly" },
            { L"placeholderText",      L"Placeholder" },
            { L"maximumLength",        L"MaxLength" },
            { L"icon.source",          L"IconSource" },
            { L"icon.name",            L"IconSource" },
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
            { L"contentX",             L"OffsetX" },
            { L"contentY",             L"OffsetY" },
            { L"firstValue",           L"LowValue" },
            { L"secondValue",          L"HighValue" },
            { L"itemHeight",           L"ItemHeight" },
            { L"cellWidth",            L"CellWidth" },
            { L"cellHeight",           L"CellHeight" },
        };
        for (const Map& m : kMap)
            if (t == m.qml)
                return m.internal;
        return name;
    }
} // namespace xmlmfc

#endif
