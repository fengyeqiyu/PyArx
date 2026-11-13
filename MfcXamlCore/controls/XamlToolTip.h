#pragma once
#include "XamlMfcControlBase.h"
#include "XamlContentParse.h"

// XamlToolTip
class XAML_CORE_API XamlToolTip : public XamlMfcControlBase
{

public:
    XamlToolTip() : XamlMfcControlBase(L"tooltip") {}

protected:
        void createNative(IXamlMfcHost* host, int id, int minValue, int maxValue, int x, int y, int w, int h) override;

        std::vector<XamlProperty> ownProperties() const override;

        bool setOwnProperty(const wchar_t* name, const std::wstring& value) override;
};
