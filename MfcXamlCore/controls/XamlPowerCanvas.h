#pragma once
#include "XamlMfcControlBase.h"
#include "XamlContentParse.h"

// <PowerCanvas> 配电规划图画布：自绘窗口，内容由 Data 属性驱动
//   Data="<svg ...>"  → 走 SVG 场景（几何 + 文字）
//   Data="{...json}"  → 走布局结果（powerplan.toJson 的 JSON）
// 运行时也可从 Python 侧 loadPowerPlan / loadPowerSvg 换内容（更适合大 JSON）。
class XAML_CORE_API XamlPowerCanvas : public XamlMfcControlBase
{

public:
    XamlPowerCanvas() : XamlMfcControlBase(L"powercanvas") {}

protected:
        void createNative(IXamlMfcHost* host, int id, int , int , int x, int y, int w, int h) override;

        std::vector<XamlProperty> ownProperties() const override;

        bool setOwnProperty(const wchar_t* name, const std::wstring& value) override;
};
