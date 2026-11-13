#pragma once
// XmlTextBox —— XML <TextBox>/<Edit> 的标准 MFC 实现。
//
// 基础功能：输入文字、多行、只读、输入长度上限、空内容占位提示（Placeholder）；
// 事件（契约见 XmlEvents.h）：TextChanged / KeyDown / KeyUp / GotFocus / LostFocus /
// Click / DoubleClick。
//
// 属性走 XmlTypes.h 的属性表（S1）：名字/别名/类型/读写只写一次，
// 不再 ownProperties + setOwnProperty + getOwnProperty 三份平行定义。
//
// 多行与只读属于"创建期样式"，XML 属性在控件创建之后才下发，
// 所以这里缓存这些值，值变化时按同一矩形重建原生控件。
//
// 编码：本文件 UTF-8 with BOM。

#include "XmlMfcControlBase.h"

class XML_CORE_API XmlTextBox : public XmlMfcControlBase
{

public:
    XmlTextBox() : XmlMfcControlBase(L"textbox") {}

protected:
    void createNative(IXmlMfcHost* host, int id, int, int, int x, int y, int w, int h) override;

    const XmlPropEntry* ownPropTable(size_t& count) const override
    {
        const std::vector<XmlPropEntry>& t = PropTableData();
        count = t.size();
        return t.data();
    }

private:
    static const std::vector<XmlPropEntry>& PropTableData();

    // 表里每一项的读写实现（无宿主时只缓存，attach 后由 createNative 一次性下发）
    bool SetTextProp(const std::wstring& v);
    std::wstring GetTextProp() const;
    bool SetReadOnlyProp(const std::wstring& v);
    bool SetMultilineProp(const std::wstring& v);
    bool SetMaxLengthProp(const std::wstring& v);
    bool SetPlaceholderProp(const std::wstring& v);

    // 多行/只读样式变了：按当前矩形重建原生控件，再推外观
    void rebuild()
    {
        if (m_host == nullptr) return;
        int x = 0, y = 0, w = 0, h = 0;
        getBounds(x, y, w, h);
        if (w <= 0 || h <= 0) return;
        m_host->addEditAdvanced(m_id, m_text, m_multiline, m_readOnly, m_maxLength,
                                m_placeholder, x, y, w, h);
        pushAppearance();
    }

    bool m_multiline = false;
    bool m_readOnly = false;
    int m_maxLength = 0;
    std::wstring m_text;
    std::wstring m_placeholder;
};
