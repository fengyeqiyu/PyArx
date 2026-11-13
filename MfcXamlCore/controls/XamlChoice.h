#pragma once
#include "XamlMfcControlBase.h"
#include "XamlContentParse.h"

// <Choice> 下拉选择（同 ComboBox 下拉，独立类型）
class XAML_CORE_API XamlChoice : public XamlMfcControlBase
{

public:
    XamlChoice() : XamlMfcControlBase(L"choice") {}

protected:
        void createNative(IXamlMfcHost* host, int id, int , int , int x, int y, int w, int h) override;

        std::vector<XamlProperty> ownProperties() const override;

        bool setOwnProperty(const wchar_t* name, const std::wstring& value) override;

        std::wstring getOwnProperty(const wchar_t* name) const override;
};
