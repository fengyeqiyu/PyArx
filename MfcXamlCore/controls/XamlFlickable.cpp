// XamlFlickable.cpp —— 无自有属性（UTF-8 with BOM）
#include "XamlFlickable.h"

void XamlFlickable::createNative(IXamlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addSkinBox(id, 21, x, y, w, h);
}