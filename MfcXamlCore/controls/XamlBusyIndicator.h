#pragma once
#include "XamlMfcControlBase.h"
#include "XamlContentParse.h"

// <BusyIndicator> 忙碌指示（不定进度）
class XAML_CORE_API XamlBusyIndicator : public XamlMfcControlBase
{

public:
    XamlBusyIndicator() : XamlMfcControlBase(L"busyindicator") {}

protected:
        void createNative(IXamlMfcHost* host, int id, int , int , int x, int y, int w, int h) override;

        std::vector<XamlProperty> ownProperties() const override;
};
