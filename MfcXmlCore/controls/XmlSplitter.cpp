// XmlSplitter.cpp —— 无自有属性（UTF-8 with BOM）
#include "XmlSplitter.h"

void XmlSplitter::createNative(IXmlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addSplitter(id, x, y, w, h);
}