// XmlBusyIndicator.cpp —— 无自有属性（UTF-8 with BOM）
#include "XmlBusyIndicator.h"

void XmlBusyIndicator::createNative(IXmlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addSkinBox(id, XmlSkinId(XmlSkinKind::Busy), x, y, w, h);
}