#pragma once
// XamlCalendarCtrl —— XAML <CalendarCtrl> 日历控件。无自有属性，Level/TabIndex/Theme 与外观均由基类统一处理。
// 编码：本文件 UTF-8 with BOM。

#include "XamlMfcControlBase.h"

class XAML_CORE_API XamlCalendarCtrl : public XamlMfcControlBase
{
public:
    XamlCalendarCtrl() : XamlMfcControlBase(L"CalendarCtrl") {}

protected:
    void createNative(IXamlMfcHost* host, int id, int min, int max, int x, int y, int w, int h) override;
};