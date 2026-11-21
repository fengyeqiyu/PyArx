// XmlCoreInternal.h —— 跨 TU 内部声明（拆 TU 后原先匿名 namespace 的共享件）。
// 类型定义在前（多 TU 共用同一份布局），函数只声明、定义留在各自 .cpp。
#pragma once
#include "XmlCoreCommon.h"

namespace xmlcore_impl
{
    // 流式布局上下文（原 02_util 末尾）
    struct XmlFlow
    {
        int margin = 0;      // 不再有暗默认边距：位置一律来自 XML 的 Left/Top/Margin
        int spacing = 0;     // 子元素间距，来自容器 XML 的 Spacing（缺省 0）
        int cursorY = 0;     // 相对 offsetY
        int cursorX = 0;     // 横向排列游标
        int width = 240;     // 容器内容宽度
        int offsetX = 0;
        int offsetY = 0;
        bool horizontal = false;
        int height = 0;      // 容器可用高度
        int padding = 0;     // 左内边距（XML Padding）
        int paddingY = 0;    // 上内边距（XML Padding）
        std::vector<int> gridRows;
        std::vector<int> gridCols;
    };

    using TemplateTable = std::vector<std::pair<std::wstring, std::vector<XmlElement>>>;

    struct LayoutDefaults
    {
        std::vector<std::pair<std::wstring, std::pair<int, int>>> sizes; // tag -> (w,h)
        int starMin = 30;      // 星号轨道最小宽度
        int groupLeft = 12;    // GroupBox 边框物理内区左偏移
        int groupTop = 24;     // GroupBox 边框物理内区上偏移
    };


    void ApplyRowFields(XmlElement& el, const std::vector<std::wstring>& fields, int row);
    inline bool AttrIsAuto(const std::wstring& s)
    {
        return _wcsicmp(s.c_str(), L"Auto") == 0 || _wcsicmp(s.c_str(), L"NaN") == 0;
    }
    int BaselineFromTop(const XmlElement& el, int boxH = 0);
    void ClearEffective(XmlElement& el);
    int DefaultHeight(const std::wstring& tag);
    void defaultSizeFor(const std::wstring& tag, int& w, int& h);
    bool ElHidden(const XmlElement& e);
    XmlElement* FindByName(XmlElement& node, const std::wstring& name);
    extern const bool g_layoutDbg;
    bool HasLocal(const XmlElement& el, const std::wstring& key);
    int IntAttr(const std::wstring& s, int fallback);
    std::wstring KindOf(const std::wstring& raw);
    LayoutDefaults& layoutDefaults();
    void LayoutNode(XmlElement& node, XmlFlow& flow);
    void LayoutNodeAt(XmlElement& root, int rootX, int rootY, int rootW, int rootH);
    std::wstring LocalName(const std::wstring& k);
    void MarkDelegateRow(XmlElement& el);
    std::wstring NormPath(const std::wstring& path);
    void ParseElement(IXMLDOMElement* element, XmlElement& node,
                      std::vector<XmlStyle>* styles, TemplateTable* templates,
                      const std::wstring& baseDir,
                      std::vector<std::wstring>* resStack);
    void ParsePadding(const std::wstring& text, XmlElement& node);
    void ParseThickness(const std::wstring& text, XmlElement& node);
    std::wstring PathDirOf(const std::wstring& path);
    void SetEffective(XmlElement& el, const std::wstring& key, const std::wstring& v);
    void SyncText(XmlElement& el);
    std::wstring XmlEscape(const std::wstring& s);
}

using namespace xmlcore_impl;   // 各 TU 保持原代码的裸名写法

// 定义在全局作用域的共享辅助（Runtime / Style 等 TU）
bool AttachPanel(XmlElement& el);
void ExpandDelegate(XmlElement& el,
                           const std::vector<std::pair<std::wstring, std::vector<XmlElement>>>* tpls);

namespace htmlsugar
{
    void ApplyStyleFlexProps(XmlElement& e);
}

namespace xmlcore_impl
{
    int CssEdgePx(const std::wstring& s, int base, int fallback);
    bool IsContainer(const std::wstring& tag);
    int NaturalWidth(const std::wstring& tag);
    std::vector<int> ResolveTracks(const std::vector<std::pair<std::wstring, std::wstring>>& defs,
                                       bool rows, const std::vector<XmlElement>& children, int available);
    void SplitAnchor(const std::wstring& raw, std::wstring& id, std::wstring& edge);
    int Sum(const std::vector<int>& sizes, int start, int count);
}
