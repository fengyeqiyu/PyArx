#pragma once
// XmlRichText —— XML <RichText> 富文本（原生 RichEdit）。
// 自有属性：Text、Bold、Italic；字号与前景色沿用基类统一外观的 FontSize / Foreground，
// 由 onAppearanceChanged 转成 CHARFORMAT（不重复声明同名属性）。
// 编码：本文件 UTF-8 with BOM。

#include "XmlMfcControlBase.h"

class XML_CORE_API XmlRichText : public XmlMfcControlBase
{
public:
    XmlRichText() : XmlMfcControlBase(L"richtext") {}

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

    void applyFormat();
    void onAppearanceChanged() override;

    std::wstring m_text;
    int m_fontPt = 9;
    bool m_bold = false;
    bool m_italic = false;
    unsigned long m_color = 0xFFFFFF;
};