// XmlRibbon.cpp —— Ribbon 控件类实现（UTF-8 with BOM）
#include "XmlRibbon.h"

void XmlRibbon::createNative(IXmlMfcHost* host, int id, int /*min*/, int /*max*/,
                             int x, int y, int w, int h)
{
    host->addRibbon(id, x, y, w, h);
}
