#pragma once
// XamlFlickable —— XAML <flickable> 可滑动视图（kind 21）。无自有属性，Level/TabIndex/Theme 与外观均由基类统一处理。
// 编码：本文件 UTF-8 with BOM。

#include "XamlMfcControlBase.h"

class XAML_CORE_API XamlFlickable : public XamlMfcControlBase
{
public:
    XamlFlickable() : XamlMfcControlBase(L"flickable") {}

protected:
    void createNative(IXamlMfcHost* host, int id, int min, int max, int x, int y, int w, int h) override;
};