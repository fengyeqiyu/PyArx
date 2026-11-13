#pragma once
// XamlCodeEditor —— XAML <CodeEditor> 注册级实现。
// 属性表驱动：Text、Syntax（XAML/PYTHON）、IsReadOnly。
// 编码：本文件 UTF-8 with BOM。

#include "XamlMfcControlBase.h"

class XAML_CORE_API XamlCodeEditor : public XamlMfcControlBase
{
public:
    XamlCodeEditor() : XamlMfcControlBase(L"codeeditor") {}

protected:
    void createNative(IXamlMfcHost* host, int id, int min, int max, int x, int y, int w, int h) override;

    const XamlPropEntry* ownPropTable(size_t& count) const override
    {
        const std::vector<XamlPropEntry>& t = PropTableData();
        count = t.size();
        return t.data();
    }

private:
    static const std::vector<XamlPropEntry>& PropTableData();

    std::wstring syntaxName() const { return m_python ? L"python" : L"xaml"; }
    void rebuildForSyntax();

    std::wstring m_text;
    bool m_readOnly = false;
    bool m_python = false;
};