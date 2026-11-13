#pragma once
#include "XamlMfcControlBase.h"
#include "XamlContentParse.h"

// <StaticBitmap> 静态图片
class XAML_CORE_API XamlStaticBitmap : public XamlMfcControlBase
{

public:
    XamlStaticBitmap() : XamlMfcControlBase(L"staticbitmap") {}

protected:
        void createNative(IXamlMfcHost* host, int id, int , int , int x, int y, int w, int h) override;

        std::vector<XamlProperty> ownProperties() const override;

        bool setOwnProperty(const wchar_t* name, const std::wstring& value) override;
};
