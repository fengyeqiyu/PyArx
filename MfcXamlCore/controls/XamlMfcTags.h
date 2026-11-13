#pragma once
// XamlMfcTags —— 控件标签的"规范名 / 别名 / 容器"唯一来源。
//
// 背景：同一个标签要在三处判断，过去三张表各写一遍，必然漂移 ——
// 预览器校验说"未知标签 Layout"，而运行时的 Layout 一直好好的。
// 这里把判定收成一份，运行时（PyMfcCore）、预览器（XamlViewer）、
// 设计器（PyMfcDesigner）都查这张表。
//
// 编码：本文件 UTF-8 with BOM。

#ifndef XAML_MFC_TAGS_H
#define XAML_MFC_TAGS_H

#include <windows.h>

#include <set>
#include <string>

namespace xamlmfc
{
    // 命名约定（与 docs\PyMfcXaml-对比WPF与QML.md 一致）：
    //   布局是布局，画布是画布
    //   Layout（旧别名 LayoutPanel / AbsolutePanel / AbsPanel / CanvasPanel / CanvasLayout）
    //   QCanvas 才是绘图画布（旧别名 DrawCanvas / PaintCanvas / CanvasSurface / Painter）
    //   <Canvas> 历史上一直当布局用，改名属破坏性切换，暂时仍按容器处理。
    inline const std::set<std::wstring>& containerTags()
    {
        static const std::set<std::wstring> kSet = {
            // 根与布局
            L"Window", L"UserControl", L"Page", L"page",
            L"Layout", L"LayoutPanel", L"AbsolutePanel", L"AbsPanel",
            L"CanvasPanel", L"CanvasLayout", L"Canvas",
            L"Grid", L"StackPanel", L"DockPanel", L"WrapPanel",
            // 容器控件
            L"Panel", L"Border", L"Frame", L"GroupBox", L"TabControl", L"TabPage", L"TabItem",
            L"ScrollViewer", L"ScrolledWindow", L"ContentControl",
            // 外壳构件
            L"MenuBar", L"MenuItem", L"ToolBar", L"StatusBar", L"PropertyGrid", L"ListView",
            L"TreeView", L"PaletteSet", L"PalettePage", L"PaletteSet.Resources",
            L"PanelHeader", L"TabStrip", L"Tab", L"ChipBar", L"Chip", L"Stage",
            // 声明层资源（不参与布局但合法）
            L"Style", L"Setter", L"ControlTemplate", L"DataTemplate", L"Binding",
            L"Trigger", L"Style.Triggers", L"DataTrigger", L"Resources",
            L"Window.Resources", L"UserControl.Resources", L"Grid.Resources",
            L"Grid.RowDefinitions", L"Grid.ColumnDefinitions",
            L"RowDefinition", L"ColumnDefinition",
            L"ResourceDictionary", L"MergedDictionaries",
            L"ResourceDictionary.MergedDictionaries"
        };
        return kSet;
    }

    inline bool isContainerTag(const std::wstring& tag)
    {
        return containerTags().count(tag) > 0;
    }

    // 控件标签的规范名：别名归一后与运行时实体化的种类一致。
    // 返回空串表示"不是已知控件标签"。
    inline std::wstring canonicalTag(const std::wstring& tag)
    {
        static const wchar_t* kAliases[][2] = {
            // 别名            规范名
            {L"Edit", L"TextBox"},
            {L"MaskedTextBox", L"TextBox"},
            {L"RichEdit", L"RichText"},
            {L"Choice", L"ComboBox"},
            {L"Tree", L"TreeView"},
            {L"DataGrid", L"ListCtrl"},
            {L"Image", L"StaticBitmap"},
            {L"BitmapButton", L"Button"},
            {L"ToolButton", L"Button"},
            {L"LinkLabel", L"Hyperlink"},
            {L"Label", L"Label"},
            {L"StatusBar", L"StatusBar"},
            {L"Separator", L"StaticLine"},
            {L"PlanCanvas", L"PowerCanvas"},
        };
        for (const auto& a : kAliases)
            if (_wcsicmp(tag.c_str(), a[0]) == 0)
                return a[1];
        return tag;
    }

    // 设计器/预览器共用：这个标签最终会建出什么控件（空 = 不建控件）
    inline std::wstring leafCanonical(const std::wstring& tag)
    {
        if (isContainerTag(tag))
            return std::wstring();
        const std::wstring c = canonicalTag(tag);
        static const std::set<std::wstring> kLeaf{
            L"Button", L"ToggleButton", L"CheckBox", L"RadioButton", L"Label", L"TextBlock",
            L"Hyperlink", L"TextBox", L"RichText", L"ComboBox", L"ListBox", L"ProgressBar",
            L"Slider", L"TrackBar", L"TabControl", L"TreeView", L"ListCtrl", L"DatePicker",
            L"Calendar", L"CalendarCtrl", L"StaticBitmap", L"StaticLine", L"Splitter", L"Panel",
            L"PowerCanvas"};
        return kLeaf.count(c) > 0 ? c : std::wstring();
    }
} // namespace xamlmfc

#endif // XAML_MFC_TAGS_H
