#pragma once
#include "XmlMfcControlBase.h"
#include "XmlContentParse.h"

// <SpinBox> 数字输入（点上下三角 / 滚轮改值）
// 属性走 XmlTypes.h 的属性表（S1）：Minimum|Min、Maximum|Max、Value|Position。
class XML_CORE_API XmlSpinBox : public XmlMfcControlBase
{

public:
    XmlSpinBox() : XmlMfcControlBase(L"spinbox") {}

protected:
    void createNative(IXmlMfcHost* host, int id, int minValue, int maxValue, int x, int y, int w, int h) override;

    const XmlPropEntry* ownPropTable(size_t& count) const override
    {
        const std::vector<XmlPropEntry>& t = PropTableData();
        count = t.size();
        return t.data();
    }

private:
    static const std::vector<XmlPropEntry>& PropTableData();

    int m_value = 0;
    bool m_valueSet = false;
};
