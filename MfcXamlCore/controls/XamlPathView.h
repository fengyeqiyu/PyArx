#pragma once
#include "XamlMfcControlBase.h"
#include "XamlContentParse.h"

// ================= 路径委托（第九批）：PathView =================
// 选中项居中放大，两侧沿水平弧线收敛；滚轮/点左右半边切换。
class XAML_CORE_API XamlPathView : public XamlMfcControlBase
{

public:
    XamlPathView() : XamlMfcControlBase(L"pathview") {}

protected:
        void createNative(IXamlMfcHost* host, int id, int minValue, int maxValue, int x, int y, int w, int h) override;

        std::vector<XamlProperty> ownProperties() const override;

        bool setOwnProperty(const wchar_t* name, const std::wstring& value) override;

        std::wstring getOwnProperty(const wchar_t* name) const override;
};
