#pragma once
#include "XamlMfcControlBase.h"
#include "XamlContentParse.h"

// <Panel> 通用容器
class XAML_CORE_API XamlPanel : public XamlMfcControlBase
{

public:
    XamlPanel() : XamlMfcControlBase(L"panel") {}

protected:
        void createNative(IXamlMfcHost* host, int id, int , int , int x, int y, int w, int h) override;

        std::vector<XamlProperty> ownProperties() const override;
};
