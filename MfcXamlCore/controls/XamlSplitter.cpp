// XamlSplitter.cpp —— 无自有属性（UTF-8 with BOM）
#include "XamlSplitter.h"

void XamlSplitter::createNative(IXamlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addSplitter(id, x, y, w, h);
}