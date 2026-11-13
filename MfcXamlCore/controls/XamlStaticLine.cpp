// XamlStaticLine.cpp —— 无自有属性（UTF-8 with BOM）
#include "XamlStaticLine.h"

void XamlStaticLine::createNative(IXamlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addStaticLine(id, x, y, w, h);
}