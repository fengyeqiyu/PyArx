#pragma once
// XamlGroupBox —— XAML <GroupBox>/<Group> 的标准 MFC 实现（分组框）。
// 属性：Content（标题）。编码：本文件 UTF-8 with BOM。

#include "XamlMfcControlBase.h"

class XAML_CORE_API XamlGroupBox : public XamlMfcControlBase
{

public:
    XamlGroupBox() : XamlMfcControlBase(L"GroupBox") {}

protected:
        void createNative(IXamlMfcHost* host, int id, int , int , int x, int y, int w, int h) override;


        std::vector<XamlProperty> ownProperties() const override;


        bool setOwnProperty(const wchar_t* name, const std::wstring& value) override;


        std::wstring getOwnProperty(const wchar_t* name) const override;
};
