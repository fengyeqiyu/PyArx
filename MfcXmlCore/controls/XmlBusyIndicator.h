#pragma once
// XmlBusyIndicator —— XML <busyindicator> 忙碌指示器。无自有属性，Level/TabIndex/Theme 与外观均由基类统一处理。
// 编码：本文件 UTF-8 with BOM。

#include "XmlMfcControlBase.h"

class XML_CORE_API XmlBusyIndicator : public XmlMfcControlBase
{
public:
    XmlBusyIndicator() : XmlMfcControlBase(L"busyindicator") {}

protected:
    void createNative(IXmlMfcHost* host, int id, int min, int max, int x, int y, int w, int h) override;
};