#pragma once
#include "XamlMfcControlBase.h"
#include "XamlContentParse.h"

// <PageIndicator> 页码点（QML PageIndicator）
class XAML_CORE_API XamlPageIndicator : public XamlMfcControlBase
{

public:
    XamlPageIndicator() : XamlMfcControlBase(L"pageindicator") {}

protected:
        void createNative(IXamlMfcHost* host, int id, int , int , int x, int y, int w, int h) override;

        std::vector<XamlProperty> ownProperties() const override;

        bool setOwnProperty(const wchar_t* name, const std::wstring& value) override;

        std::wstring getOwnProperty(const wchar_t* name) const override;
};
