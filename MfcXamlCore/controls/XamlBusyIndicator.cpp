// XamlBusyIndicator.cpp —— 实现（拆分自 XamlExtraControls.h，UTF-8 with BOM）
#include "XamlBusyIndicator.h"

void XamlBusyIndicator::createNative(IXamlMfcHost* host, int id, int /*min*/, int /*max*/, int x, int y, int w, int h)
{
        host->addSkinBox(id, XamlSkinId(XamlSkinKind::Busy), x, y, w, h);
    }

// 属性表口径：只声明真正能写的属性。
// 以前这里还列了 Running，但宿主没有"停止/恢复忙碌动画"的接口，写了也不生效，属于元数据说谎。
std::vector<XamlProperty> XamlBusyIndicator::ownProperties() const
{
        return {{L"Name", XamlPropType::String, true, false}};
    }
