#pragma once
#include "XamlMfcControlBase.h"
#include "XamlContentParse.h"

// ================= 画命令画布（第九批）：QCanvas =================
// QML 的 Canvas 用 onPaint 回调；这里换成"数据化画命令列表"，属性直接落库给宿主绘制层。
// 标签不能用 Canvas（Canvas 是本引擎的布局容器名，会被容器先吃掉），故主名 QCanvas。
class XAML_CORE_API XamlQCanvas : public XamlMfcControlBase
{

public:
    XamlQCanvas() : XamlMfcControlBase(L"qcanvas") {}

protected:
        void createNative(IXamlMfcHost* host, int id, int minValue, int maxValue, int x, int y, int w, int h) override;

        std::vector<XamlProperty> ownProperties() const override;

        bool setOwnProperty(const wchar_t* name, const std::wstring& value) override;

        std::wstring getOwnProperty(const wchar_t* name) const override;
};
