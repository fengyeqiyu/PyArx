#pragma once
#include "XamlMfcControlBase.h"
#include "XamlContentParse.h"

// <Hyperlink> 超链接
class XAML_CORE_API XamlHyperlink : public XamlMfcControlBase
{

public:
    XamlHyperlink() : XamlMfcControlBase(L"hyperlink") {}

protected:
        void createNative(IXamlMfcHost* host, int id, int , int , int x, int y, int w, int h) override;

        std::vector<XamlProperty> ownProperties() const override;

        bool setOwnProperty(const wchar_t* name, const std::wstring& value) override;

        std::wstring getOwnProperty(const wchar_t* name) const override;
};
