// XamlFlickable.cpp —— 实现（拆分自 XamlExtraControls.h，UTF-8 with BOM）
#include "XamlFlickable.h"

void XamlFlickable::createNative(IXamlMfcHost* host, int id, int minValue, int maxValue, int x, int y, int w, int h)
{
        host->addSkinBox(id, 21, x, y, w, h);
        (void)minValue;
        (void)maxValue;
    }

std::vector<XamlProperty> XamlFlickable::ownProperties() const
{
        return {{L"Name", XamlPropType::String, true, true}};
    }
