// XamlBusyIndicator.cpp —— 无自有属性（UTF-8 with BOM）
#include "XamlBusyIndicator.h"

void XamlBusyIndicator::createNative(IXamlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addSkinBox(id, XamlSkinId(XamlSkinKind::Busy), x, y, w, h);
}