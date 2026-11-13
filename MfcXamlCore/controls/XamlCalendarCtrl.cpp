// XamlCalendarCtrl.cpp —— 无自有属性（UTF-8 with BOM）
#include "XamlCalendarCtrl.h"

void XamlCalendarCtrl::createNative(IXamlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addCalendar(id, x, y, w, h);
}