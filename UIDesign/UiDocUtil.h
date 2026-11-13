#pragma once
// PyMfcDesigner 辅助：XamlElement 树的编辑/遍历/路径定位
// 设计器以统一运行时 XamlElement 作为唯一文档模型。
// XamlElement.children 为值语义（vector<XamlElement>），push_back 可能触发 realloc，
// 因此设计器不长期持有裸指针，改用「路径」（根到节点的 child 索引链）寻址。
#include "stdafx.h"
#include <vector>
#include <string>
#include <deque>
#include "XamlCore.h"

namespace uidesign
{
    // 根到节点的 child 索引路径（空 = 根）
    using Path = std::vector<int>;

    // 沿路径定位节点；p 为空或越界返回 nullptr
    inline XamlElement* Resolve(XamlElement* root, const Path& p)
    {
        XamlElement* cur = root;
        for (int idx : p)
        {
            if (cur == nullptr || idx < 0 || idx >= (int)cur->children.size())
                return nullptr;
            cur = &cur->children[idx];
        }
        return cur;
    }

    // 深度遍历：对每个节点回调(path, el)；路径即当前节点位置
    template <typename Fn>
    void Walk(XamlElement* node, Path& path, Fn&& fn)
    {
        if (node == nullptr) return;
        fn(path, *node);
        for (int i = 0; i < (int)node->children.size(); ++i)
        {
            path.push_back(i);
            Walk(&node->children[i], path, fn);
            path.pop_back();
        }
    }
    template <typename Fn>
    void Walk(XamlElement* root, Fn&& fn)
    {
        Path p;
        Walk(root, p, std::forward<Fn>(fn));
    }

    // 统计节点数
    inline int CountNodes(XamlElement* node)
    {
        if (node == nullptr) return 0;
        int c = 1;
        for (auto& ch : node->children) c += CountNodes(&ch);
        return c;
    }

    // 是否容器标签（画布/树需要按容器处理）
    // 布局容器：AbsolutePanel 是"绝对定位布局"的正名（Canvas 只是它的兼容别名，
    // 画布另有 DrawCanvas/PaintCanvas/QCanvas）。
    inline bool IsContainer(const std::wstring& tag)
    {
        static const wchar_t* k[] = {
            L"Window", L"Grid", L"StackPanel", L"Canvas", L"Layout", L"LayoutPanel", L"AbsolutePanel", L"AbsPanel", L"CanvasPanel",
            L"GroupBox",
            L"TabControl", L"TabPage", L"Border", L"Panel", L"WrapPanel", L"DockPanel", L"GridPanel",
            L"ToolBar", L"MenuBar", L"StatusBar", L"ScrollViewer", L"ScrolledWindow",
            // 侧栏：<PaletteSet> 是多页宿主，<page>/<TabItem> 是一页
            L"PaletteSet", L"Page", L"page", L"TabItem"
        };
        for (auto kk : k) if (tag == kk) return true;
        return false;
    }

    // 是否为纯容器（不建 HWND）：Window/Grid/MenuBar/ToolBar/StatusBar/MenuItem/Splitter
    inline bool IsPureContainer(const std::wstring& tag)
    {
        static const wchar_t* k[] = {
            L"Window", L"Grid", L"MenuBar", L"ToolBar", L"StatusBar", L"MenuItem", L"Splitter"
        };
        for (auto kk : k) if (tag == kk) return true;
        return false;
    }

    // 绘图图元（没有文本内容，只靠尺寸/方向/颜色描述）
    inline bool IsShapeTag(const std::wstring& tag)
    {
        return tag == L"Line" || tag == L"Rectangle" || tag == L"Ellipse" || tag == L"Arc";
    }

    // 默认尺寸（AddControl 时用）
    inline void DefaultSize(const std::wstring& tag, int& w, int& h)
    {
        w = 100; h = 28;
        if (tag == L"ListBox" || tag == L"ListView") { h = 80; }
        else if (tag == L"DataGrid" || tag == L"DataGridView") { w = 240; h = 120; }
        else if (tag == L"ComboBox") { h = 24; }
        else if (tag == L"GroupBox") { w = 160; h = 80; }
        else if (tag == L"Image") { w = 80; h = 80; }
        else if (tag == L"ProgressBar") { w = 120; h = 16; }
        else if (tag == L"TabControl") { w = 200; h = 120; }
        else if (tag == L"TextBlock" || tag == L"Label") { w = 160; h = 24; }
        // 绘图图元：直线默认水平、矩形/圆给一块像样的初始尺寸
        else if (tag == L"Line") { w = 120; h = 2; }
        else if (tag == L"Rectangle") { w = 160; h = 90; }
        else if (tag == L"Ellipse") { w = 120; h = 120; }
        else if (tag == L"Arc") { w = 140; h = 100; }
    }

    // 追加子节点并设置布局属性；返回新节点（仅本次调用有效，勿长期持有）
    inline XamlElement* AddChild(XamlElement* parent, const std::wstring& tag,
                                 const std::wstring& name, const std::wstring& text,
                                 int x, int y, int w, int h)
    {
        XamlElement el;
        el.tag = tag;
        el.name = name;
        el.text = text;
        el.hasX = true; el.x = x;
        el.hasY = true; el.y = y;
        el.hasW = true; el.w = w;
        el.hasH = true; el.h = h;
        if (!name.empty()) el.set(L"Name", name);
        if (!text.empty()) el.set(L"Content", text);
        parent->children.push_back(std::move(el));
        return &parent->children.back();
    }

    // 从父节点删除子节点（按 Path 最后一跳）
    inline bool RemoveChild(XamlElement* parent, int idx)
    {
        if (parent == nullptr || idx < 0 || idx >= (int)parent->children.size())
            return false;
        parent->children.erase(parent->children.begin() + idx);
        return true;
    }

    // 定位父节点与子索引（给定完整路径，返回父节点；idx 为该子节点在父中的位置）
    inline XamlElement* ParentOf(XamlElement* root, const Path& p, int& outIdx)
    {
        if (p.empty()) { outIdx = -1; return nullptr; }
        Path pp(p.begin(), p.end() - 1);
        outIdx = p.back();
        return Resolve(root, pp);
    }
}
