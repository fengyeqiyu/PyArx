// XamlPanel.cpp —— 实现（拆分自 XamlExtraControls.h，UTF-8 with BOM）
#include "XamlPanel.h"

void XamlPanel::createNative(IXamlMfcHost* host, int id, int /*min*/, int /*max*/, int x, int y, int w, int h)
{
        host->addPanel(id, x, y, w, h);
    }

std::vector<XamlProperty> XamlPanel::ownProperties() const
{
        return {{L"Name", XamlPropType::String, true, true}};
    }
