#pragma once
// XamlStaticLine —— XAML <staticline> 直线分隔控件。无自有属性，Level/TabIndex/Theme 与外观均由基类统一处理。
// 编码：本文件 UTF-8 with BOM。

#include "XamlMfcControlBase.h"

class XAML_CORE_API XamlStaticLine : public XamlMfcControlBase
{
public:
    XamlStaticLine() : XamlMfcControlBase(L"staticline") {}

protected:
    void createNative(IXamlMfcHost* host, int id, int min, int max, int x, int y, int w, int h) override;
};