#pragma once
#include "XamlMfcControlBase.h"
#include "XamlContentParse.h"

// <ToolBar> 工具栏：Items="打开|保存|运行"
class XAML_CORE_API XamlToolBar : public XamlMfcControlBase
{

public:
    XamlToolBar() : XamlMfcControlBase(L"toolbar") {}

protected:
        void createNative(IXamlMfcHost* host, int id, int minValue, int maxValue, int x, int y, int w, int h) override;

        std::vector<XamlProperty> ownProperties() const override;

        bool setOwnProperty(const wchar_t* name, const std::wstring& value) override;

        std::wstring getOwnProperty(const wchar_t* name) const override;
};
