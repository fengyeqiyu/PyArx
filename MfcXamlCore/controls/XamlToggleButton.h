#pragma once
#include "XamlMfcControlBase.h"
#include "XamlContentParse.h"

// <ToggleButton> 切换按钮
class XAML_CORE_API XamlToggleButton : public XamlMfcControlBase
{

public:
    XamlToggleButton() : XamlMfcControlBase(L"ToggleButton") {}

protected:
        void createNative(IXamlMfcHost* host, int id, int , int , int x, int y, int w, int h) override;

        std::vector<XamlProperty> ownProperties() const override;

        bool setOwnProperty(const wchar_t* name, const std::wstring& value) override;

        std::wstring getOwnProperty(const wchar_t* name) const override;
};
