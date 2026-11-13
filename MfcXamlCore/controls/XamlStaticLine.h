#pragma once
#include "XamlMfcControlBase.h"
#include "XamlContentParse.h"

// <StaticLine> 分割线
class XAML_CORE_API XamlStaticLine : public XamlMfcControlBase
{

public:
    XamlStaticLine() : XamlMfcControlBase(L"staticline") {}

protected:
        void createNative(IXamlMfcHost* host, int id, int , int , int x, int y, int w, int h) override;

        std::vector<XamlProperty> ownProperties() const override;
};
