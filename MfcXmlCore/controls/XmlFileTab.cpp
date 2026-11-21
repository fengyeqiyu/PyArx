// XmlFileTab.cpp —— FileTab 控件类实现（UTF-8 with BOM）
#include "XmlFileTab.h"

void XmlFileTab::createNative(IXmlMfcHost* host, int id, int /*min*/, int /*max*/,
                              int x, int y, int w, int h)
{
    host->addFileTab(id, x, y, w, h);
}
