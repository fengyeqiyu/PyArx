#pragma once
#include "XamlMfcControlBase.h"
#include "XamlContentParse.h"

// <Switch> 开关（QML Switch / WPF ToggleSwitch）
class XAML_CORE_API XamlSwitch : public XamlMfcControlBase
{

public:
    XamlSwitch() : XamlMfcControlBase(L"switch") {}

protected:
        void createNative(IXamlMfcHost* host, int id, int , int , int x, int y, int w, int h) override;

        std::vector<XamlProperty> ownProperties() const override;

        bool setOwnProperty(const wchar_t* name, const std::wstring& value) override;

        std::wstring getOwnProperty(const wchar_t* name) const override;
};
