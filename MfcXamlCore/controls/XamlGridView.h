#pragma once
#include "XamlMfcControlBase.h"
#include "XamlContentParse.h"

// ================= 网格委托（第九批）：GridView =================
// 与 VirtualList 共用一套数据槽位（Items / SelectedIndex），只是布局换成格子。
class XAML_CORE_API XamlGridView : public XamlMfcControlBase
{

public:
    XamlGridView() : XamlMfcControlBase(L"gridview") {}

protected:
        void createNative(IXamlMfcHost* host, int id, int minValue, int maxValue, int x, int y, int w, int h) override;

        std::vector<XamlProperty> ownProperties() const override;

        bool setOwnProperty(const wchar_t* name, const std::wstring& value) override;

        std::wstring getOwnProperty(const wchar_t* name) const override;

    int m_cellW = 0;
    int m_cellH = 0;
};
