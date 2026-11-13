#pragma once
#include "XamlMfcControlBase.h"
#include "XamlContentParse.h"

// <SpinBox> 数字输入（点上下三角 / 滚轮改值）
// 属性走 XamlTypes.h 的属性表（S1）：Minimum|Min、Maximum|Max、Value|Position。
class XAML_CORE_API XamlSpinBox : public XamlMfcControlBase
{

public:
    XamlSpinBox() : XamlMfcControlBase(L"spinbox") {}

protected:
    void createNative(IXamlMfcHost* host, int id, int minValue, int maxValue, int x, int y, int w, int h) override;

    const XamlPropEntry* ownPropTable(size_t& count) const override
    {
        const std::vector<XamlPropEntry>& t = PropTableData();
        count = t.size();
        return t.data();
    }

private:
    static const std::vector<XamlPropEntry>& PropTableData();

    int m_value = 0;
    bool m_valueSet = false;
};
