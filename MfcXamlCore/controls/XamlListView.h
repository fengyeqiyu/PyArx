#pragma once
#include "XamlMfcControlBase.h"
#include "XamlContentParse.h"

// ================= 虚拟化列表（第七批）：ListModel + delegate =================
// 数据在宿主里，绘制只画可见窗口内的行（kind 16）。一行字符串用 "标题|副标题|值"，
// 多行用 ";" 或换行分隔。
class XAML_CORE_API XamlListView : public XamlMfcControlBase
{

public:
    XamlListView() : XamlMfcControlBase(L"virtuallist") {}

protected:
        void createNative(IXamlMfcHost* host, int id, int minValue, int maxValue, int x, int y, int w, int h) override;

        std::vector<XamlProperty> ownProperties() const override;

        bool setOwnProperty(const wchar_t* name, const std::wstring& value) override;

        std::wstring getOwnProperty(const wchar_t* name) const override;


private:
    int m_itemH = 44;
};
