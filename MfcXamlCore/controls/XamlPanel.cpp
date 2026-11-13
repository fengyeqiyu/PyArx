// XamlPanel.cpp —— 无自有属性（UTF-8 with BOM）
#include "XamlPanel.h"

void XamlPanel::createNative(IXamlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addPanel(id, x, y, w, h);
}