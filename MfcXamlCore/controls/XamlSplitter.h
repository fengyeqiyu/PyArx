#pragma once
// XamlSplitter —— XAML <splitter> 分隔条。无自有属性，Level/TabIndex/Theme 与外观均由基类统一处理。
// 编码：本文件 UTF-8 with BOM。

#include "XamlMfcControlBase.h"

class XAML_CORE_API XamlSplitter : public XamlMfcControlBase
{
public:
    XamlSplitter() : XamlMfcControlBase(L"splitter") {}

protected:
    void createNative(IXamlMfcHost* host, int id, int min, int max, int x, int y, int w, int h) override;
};