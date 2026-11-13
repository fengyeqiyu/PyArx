#pragma once
#include "XamlMfcControlBase.h"
#include "XamlContentParse.h"

// <TreeCtrl> 树控件
class XAML_CORE_API XamlTreeCtrl : public XamlMfcControlBase
{

public:
    XamlTreeCtrl() : XamlMfcControlBase(L"TreeCtrl") {}

protected:
        void createNative(IXamlMfcHost* host, int id, int , int , int x, int y, int w, int h) override;

        std::vector<XamlProperty> ownProperties() const override;

        bool setOwnProperty(const wchar_t* name, const std::wstring& value) override;
};
