#pragma once
#include "XamlMfcControlBase.h"
#include "XamlContentParse.h"

// ================= 页容器（第八批）：Page / StackView / SwipeView =================
// 容器自己只画底部页码条（kind 17），页里的子控件是真 HWND，由宿主整组平移切页。
// 子控件在 XAML 里与容器同级（放在同一个 Canvas 里），再用 Python addPage 分组。
class XAML_CORE_API XamlPageView : public XamlMfcControlBase
{

public:
    explicit XamlPageView(const wchar_t* tag) : XamlMfcControlBase(tag) {}

protected:
        void createNative(IXamlMfcHost* host, int id, int minValue, int maxValue, int x, int y, int w, int h) override;

        std::vector<XamlProperty> ownProperties() const override;

        bool setOwnProperty(const wchar_t* name, const std::wstring& value) override;

        std::wstring getOwnProperty(const wchar_t* name) const override;
};
