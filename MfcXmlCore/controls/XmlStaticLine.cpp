// XmlStaticLine.cpp —— 无自有属性（UTF-8 with BOM）
#include "XmlStaticLine.h"

void XmlStaticLine::createNative(IXmlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addStaticLine(id, x, y, w, h);
}