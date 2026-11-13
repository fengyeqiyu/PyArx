#pragma once
// XmlStaticLine —— XML <staticline> 直线分隔控件。无自有属性，Level/TabIndex/Theme 与外观均由基类统一处理。
// 编码：本文件 UTF-8 with BOM。

#include "XmlMfcControlBase.h"

class XML_CORE_API XmlStaticLine : public XmlMfcControlBase
{
public:
    XmlStaticLine() : XmlMfcControlBase(L"staticline") {}

protected:
    void createNative(IXmlMfcHost* host, int id, int min, int max, int x, int y, int w, int h) override;
};