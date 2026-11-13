// XamlStaticLine.cpp —— 实现（拆分自 XamlExtraControls.h，UTF-8 with BOM）
#include "XamlStaticLine.h"

void XamlStaticLine::createNative(IXamlMfcHost* host, int id, int /*min*/, int /*max*/, int x, int y, int w, int h)
{
        host->addStaticLine(id, x, y, w, h);
    }

std::vector<XamlProperty> XamlStaticLine::ownProperties() const
{
        return {{L"Name", XamlPropType::String, true, true}};
    }
