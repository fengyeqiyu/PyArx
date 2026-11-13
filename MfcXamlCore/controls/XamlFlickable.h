#pragma once
#include "XamlMfcControlBase.h"
#include "XamlContentParse.h"

// ================= Flickable（第十批）：可拖拽内容视口 =================
// 视口自己是一个画布窗（kind 21）；页内容由 Python 交给宿主：
//   w.setFlickContent(fid, [kid...], contentW, contentH)
// 宿主把子控件 SetParent 到视口窗，于是超出视口的部分被父窗裁剪，
// 拖拽 1:1 跟随、松手惯性、触边弹性回弹。
class XAML_CORE_API XamlFlickable : public XamlMfcControlBase
{

public:
    XamlFlickable() : XamlMfcControlBase(L"flickable") {}

protected:
        void createNative(IXamlMfcHost* host, int id, int minValue, int maxValue, int x, int y, int w, int h) override;

        std::vector<XamlProperty> ownProperties() const override;
};
