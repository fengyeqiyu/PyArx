#pragma once
// XmlCodeEditor —— XML <CodeEditor> 注册级实现。
// 属性表驱动：Text、Syntax（XML/PYTHON）、IsReadOnly。
// 编码：本文件 UTF-8 with BOM。

#include "XmlMfcControlBase.h"

class XML_CORE_API XmlCodeEditor : public XmlMfcControlBase
{
public:
    XmlCodeEditor() : XmlMfcControlBase(L"codeeditor") {}

protected:
    void createNative(IXmlMfcHost* host, int id, int min, int max, int x, int y, int w, int h) override;

    const XmlPropEntry* ownPropTable(size_t& count) const override
    {
        const std::vector<XmlPropEntry>& t = PropTableData();
        count = t.size();
        return t.data();
    }

private:
    static const std::vector<XmlPropEntry>& PropTableData();

    std::wstring syntaxName() const { return m_python ? L"python" : L"xml"; }
    void rebuildForSyntax();

    std::wstring m_text;
    bool m_readOnly = false;
    bool m_python = false;
};