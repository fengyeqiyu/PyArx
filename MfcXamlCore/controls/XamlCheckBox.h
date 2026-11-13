#pragma once
// XamlCheckBox —— XAML <CheckBox> 的标准 MFC 实现。
// 属性：Content、IsChecked；事件：Click（宿主绑定）。
// 编码：本文件 UTF-8 with BOM。

#include "XamlMfcControlBase.h"

class XAML_CORE_API XamlCheckBox : public XamlMfcControlBase
{

public:
    XamlCheckBox() : XamlMfcControlBase(L"checkbox") {}

protected:
        void createNative(IXamlMfcHost* host, int id, int , int , int x, int y, int w, int h) override;


        std::vector<XamlProperty> ownProperties() const override;


        bool setOwnProperty(const wchar_t* name, const std::wstring& value) override;


        std::wstring getOwnProperty(const wchar_t* name) const override;
};
