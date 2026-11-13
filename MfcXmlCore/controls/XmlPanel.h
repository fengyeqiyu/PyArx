#pragma once
// XmlPanel —— XML <panel> 通用容器。无自有属性，Level/TabIndex/Theme 与外观均由基类统一处理。
// 编码：本文件 UTF-8 with BOM。

#include "XmlMfcControlBase.h"

class XML_CORE_API XmlPanel : public XmlMfcControlBase
{
public:
    XmlPanel() : XmlMfcControlBase(L"panel") {}

protected:
    void createNative(IXmlMfcHost* host, int id, int min, int max, int x, int y, int w, int h) override;
};