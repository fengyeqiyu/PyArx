#pragma once
#include "XamlMfcControlBase.h"
#include "XamlContentParse.h"

// XamlMenu
class XAML_CORE_API XamlMenu : public XamlMfcControlBase
{

public:
    XamlMenu() : XamlMfcControlBase(L"menu") {}

protected:
        void createNative(IXamlMfcHost* host, int id, int minValue, int maxValue, int x, int y, int w, int h) override;

        std::vector<XamlProperty> ownProperties() const override;

        bool setOwnProperty(const wchar_t* name, const std::wstring& value) override;

        std::wstring getOwnProperty(const wchar_t* name) const override;
};
