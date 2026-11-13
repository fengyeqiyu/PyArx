#pragma once
// XmlMfcTags —— 控件标签的唯一命名来源。
//
// 命名规则（2026-11 起统一，不再保留别名）：
//     所有自有元素一律 X + 本意名：<XButton> <XTextField> <XStackPanel> <XWindow>
//     前缀只为消除与 WPF / QML / Win32 同名类的歧义，不改变本意名。
//     大小写不敏感；资源类结构标签（Style / Setter / RowDefinition ...）不加前缀。
//
// 判定规则：比较前统一用 BareTag 去掉命名空间与 X 前缀，只在一处收口，
// 避免"预览器说未知标签而运行时正常"这类漂移。
//
// 编码：本文件 UTF-8 with BOM。

#ifndef XML_MFC_TAGS_H
#define XML_MFC_TAGS_H

#include <windows.h>

#include <set>
#include <string>

#include <cwctype>

namespace xmlmfc
{
    // 去掉 XML 命名空间（{...}Tag、a.b.Tag）与统一前缀 X，得到本意名。
    inline std::wstring BareTag(const std::wstring& tag)
    {
        std::wstring t = tag;
        const size_t brace = t.rfind(L'}');
        if (brace != std::wstring::npos)
            t = t.substr(brace + 1);
        if (t.size() > 1 && (t[0] == L'X' || t[0] == L'x') && iswupper(t[1]))
            t = t.substr(1);
        return t;
    }

    // 容器（布局层）：可以承载子元素，自身不一定实体化为控件
    inline const std::set<std::wstring>& containerTags()
    {
        static const std::set<std::wstring> kSet = {
            // 根与布局
            L"Window", L"UserControl", L"Layout", L"LayoutPanel", L"AbsolutePanel",
            L"AbsPanel", L"CanvasPanel", L"CanvasLayout", L"Canvas",
            L"Grid", L"StackPanel", L"DockPanel", L"WrapPanel",
            // 容器控件
            L"Pane", L"Border", L"Frame", L"Dialog", L"Drawer", L"GroupBox", L"TabBar", L"TabPage",
            L"TabItem", L"ScrollViewer", L"ScrollView", L"ContentControl",
            // 外壳构件
            L"MenuBar", L"MenuItem", L"ToolBar", L"StatusBar", L"PropertyGrid", L"ListView",
            L"TreeView", L"PaletteSet", L"PalettePage", L"PaletteSet.Resources",
            L"PanelHeader", L"TabStrip", L"Tab", L"ChipBar", L"Chip", L"Stage",
            L"ShellTitle", L"DrawBar",
            L"TreeRow", L"PropRow", L"ListRow",   // 列表/树/属性表的预设行
            // 声明层资源（不参与布局但合法）
            L"Style", L"Setter", L"ControlTemplate", L"DataTemplate", L"Binding",
            L"Trigger", L"Style.Triggers", L"DataTrigger", L"Resources",
            L"Window.Resources", L"UserControl.Resources", L"Grid.Resources",
            L"Grid.RowDefinitions", L"Grid.ColumnDefinitions",
            L"RowDefinition", L"ColumnDefinition",
            L"ResourceDictionary", L"MergedDictionaries"
        };
        return kSet;
    }

    inline bool isContainerTag(const std::wstring& tag)
    {
        return containerTags().count(BareTag(tag)) > 0;
    }

    // 会实体化为控件的标签（本意名，不含 X 前缀）
    inline const std::set<std::wstring>& controlTags()
    {
        static const std::set<std::wstring> kSet = {
            L"Button", L"Label", L"TextField", L"ListBox", L"CodeEditor", L"CheckBox",
            L"RadioButton", L"GroupBox", L"ComboBox", L"ProgressBar", L"Slider",
            L"TabBar", L"TreeView", L"ListCtrl", L"Choice", L"DatePicker", L"Pane",
            L"SplitView", L"Divider", L"PowerCanvas", L"Hyperlink", L"TextArea",
            L"ToggleButton", L"IconButton", L"Image", L"GridLayout", L"ScrollView",
            L"MonthGrid", L"Switch", L"SpinBox", L"RangeSlider", L"BusyIndicator",
            L"PageIndicator", L"Dial", L"Tumbler", L"ScrollBar", L"TableView", L"TreeTable",
            L"ToolBar", L"ToolTip", L"Menu", L"MenuBar", L"Popup", L"VirtualList",
            L"StackView", L"SwipeView", L"GridView", L"PathView", L"QCanvas", L"Page",
            L"Flickable",
            // QML Controls 2 对齐补齐
            L"Frame", L"Dialog", L"Drawer", L"RoundButton", L"DelayButton", L"ToolButton", L"ToolSeparator", L"TabButton", L"ScrollIndicator", L"MenuItem", L"MenuSeparator",
            L"ItemView", L"Delegate",
            // 绘制图元
            L"Line", L"Rectangle", L"Ellipse", L"Arc"
        };
        return kSet;
    }

    // 规范名：X + 本意名。返回空串表示"不是已知控件标签"。
    inline std::wstring canonicalTag(const std::wstring& tag)
    {
        const std::wstring bare = BareTag(tag);
        if (controlTags().count(bare) == 0)
            return std::wstring();
        return L"X" + bare;
    }

    // 设计器/预览器共用：这个标签最终会建出什么控件（空 = 不建控件）
    inline std::wstring leafCanonical(const std::wstring& tag)
    {
        return canonicalTag(tag);
    }
} // namespace xmlmfc

#endif // XML_MFC_TAGS_H
