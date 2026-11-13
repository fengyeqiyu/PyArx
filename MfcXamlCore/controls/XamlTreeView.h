#pragma once
#include "XamlMfcControlBase.h"
#include "XamlContentParse.h"

// <TreeView> 层级表格：Nodes="文字@层级|文字@层级>"，末尾 '>' 表示有子节点
class XAML_CORE_API XamlTreeView : public XamlMfcControlBase
{

public:
    XamlTreeView() : XamlMfcControlBase(L"treeview") {}

protected:
        void createNative(IXamlMfcHost* host, int id, int minValue, int maxValue, int x, int y, int w, int h) override;

        std::vector<XamlProperty> ownProperties() const override;

        bool setOwnProperty(const wchar_t* name, const std::wstring& value) override;

        std::wstring getOwnProperty(const wchar_t* name) const override;
};
