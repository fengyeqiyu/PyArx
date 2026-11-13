#pragma once
#include "XamlMfcControlBase.h"
#include "XamlContentParse.h"

// <CalendarCtrl> 日历
class XAML_CORE_API XamlCalendarCtrl : public XamlMfcControlBase
{

public:
    XamlCalendarCtrl() : XamlMfcControlBase(L"calendar") {}

protected:
        void createNative(IXamlMfcHost* host, int id, int , int , int x, int y, int w, int h) override;

        std::vector<XamlProperty> ownProperties() const override;
};
