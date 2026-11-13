#pragma once
#include "XamlMfcControlBase.h"
#include "XamlContentParse.h"

// <TabControl> 选项卡容器
class XAML_CORE_API XamlTabControl : public XamlMfcControlBase
{

public:
    XamlTabControl() : XamlMfcControlBase(L"tab") {}

protected:
        void createNative(IXamlMfcHost* host, int id, int , int , int x, int y, int w, int h) override;

        std::vector<XamlProperty> ownProperties() const override;

        bool setOwnProperty(const wchar_t* name, const std::wstring& value) override;
};
