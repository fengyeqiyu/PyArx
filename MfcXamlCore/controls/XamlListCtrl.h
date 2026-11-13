#pragma once
#include "XamlMfcControlBase.h"
#include "XamlContentParse.h"

// <ListCtrl> 列表控件
class XAML_CORE_API XamlListCtrl : public XamlMfcControlBase
{

public:
    XamlListCtrl() : XamlMfcControlBase(L"listctrl") {}

protected:
        void createNative(IXamlMfcHost* host, int id, int , int , int x, int y, int w, int h) override;

        std::vector<XamlProperty> ownProperties() const override;

        bool setOwnProperty(const wchar_t* name, const std::wstring& value) override;
};
