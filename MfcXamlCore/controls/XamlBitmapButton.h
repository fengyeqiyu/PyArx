#pragma once
#include "XamlMfcControlBase.h"
#include "XamlContentParse.h"

// <BitmapButton> 位图按钮
class XAML_CORE_API XamlBitmapButton : public XamlMfcControlBase
{

public:
    XamlBitmapButton() : XamlMfcControlBase(L"bitmapbutton") {}

protected:
        void createNative(IXamlMfcHost* host, int id, int , int , int x, int y, int w, int h) override;

        std::vector<XamlProperty> ownProperties() const override;

        bool setOwnProperty(const wchar_t* name, const std::wstring& value) override;
};
