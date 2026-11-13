#pragma once
#include "XamlMfcControlBase.h"
#include "XamlContentParse.h"

// Popup / Dialog / Drawer 共用：标题条 + 多行正文 + 点正文关闭
class XAML_CORE_API XamlPopup : public XamlMfcControlBase
{

public:
    XamlPopup() : XamlMfcControlBase(L"popup") {}

protected:
        void createNative(IXamlMfcHost* host, int id, int minValue, int maxValue, int x, int y, int w, int h) override;

        std::vector<XamlProperty> ownProperties() const override;

        bool setOwnProperty(const wchar_t* name, const std::wstring& value) override;

private:
    std::wstring m_popTitle;
    std::vector<std::wstring> m_popLines;
};
