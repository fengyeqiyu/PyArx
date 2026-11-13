// XamlSplitter.cpp —— 实现（拆分自 XamlExtraControls.h，UTF-8 with BOM）
#include "XamlSplitter.h"

void XamlSplitter::createNative(IXamlMfcHost* host, int id, int /*min*/, int /*max*/, int x, int y, int w, int h)
{
        host->addSplitter(id, x, y, w, h);
    }

std::vector<XamlProperty> XamlSplitter::ownProperties() const
{
        return {{L"Name", XamlPropType::String, true, true}};
    }
