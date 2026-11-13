#pragma once
// XamlRichText —— XAML <RichText> 富文本（原生 RichEdit）。
// 属性表驱动：Text、FontSize、Bold、Italic、Foreground。
// 编码：本文件 UTF-8 with BOM。

#include "XamlMfcControlBase.h"

class XAML_CORE_API XamlRichText : public XamlMfcControlBase
{
public:
    XamlRichText() : XamlMfcControlBase(L"richtext") {}

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

    void applyFormat();

    std::wstring m_text;
    int m_fontPt = 9;
    bool m_bold = false;
    bool m_italic = false;
    unsigned long m_color = 0xFFFFFF;
};