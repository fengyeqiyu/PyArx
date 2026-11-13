#pragma once
#include "XamlMfcControlBase.h"
#include "XamlContentParse.h"

// <DatePicker> 日期选择
class XAML_CORE_API XamlDatePicker : public XamlMfcControlBase
{

public:
    XamlDatePicker() : XamlMfcControlBase(L"datepicker") {}

protected:
        void createNative(IXamlMfcHost* host, int id, int , int , int x, int y, int w, int h) override;

        std::vector<XamlProperty> ownProperties() const override;

        bool setOwnProperty(const wchar_t* name, const std::wstring& value) override;

        std::wstring getOwnProperty(const wchar_t* name) const override;
};
