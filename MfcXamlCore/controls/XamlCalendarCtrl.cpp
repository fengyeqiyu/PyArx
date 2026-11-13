// XamlCalendarCtrl.cpp —— 实现（拆分自 XamlExtraControls.h，UTF-8 with BOM）
#include "XamlCalendarCtrl.h"

void XamlCalendarCtrl::createNative(IXamlMfcHost* host, int id, int /*min*/, int /*max*/, int x, int y, int w, int h)
{
        host->addCalendar(id, x, y, w, h);
    }

std::vector<XamlProperty> XamlCalendarCtrl::ownProperties() const
{
        return {{L"Name", XamlPropType::String, true, true}};
    }
