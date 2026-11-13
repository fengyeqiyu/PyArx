#pragma once
// XamlQCanvas —— XAML <QCanvas> 画命令画布（kind 20）。
// 属性表驱动：Commands、Script（同一份数据：数据化画命令列表）。
// 编码：本文件 UTF-8 with BOM。

#include "XamlMfcControlBase.h"

class XAML_CORE_API XamlQCanvas : public XamlMfcControlBase
{
public:
    XamlQCanvas() : XamlMfcControlBase(L"qcanvas") {}

protected:
    void createNative(IXamlMfcHost* host, int id, int min, int max, int x, int y, int w, int h) override;

    const XamlPropEntry* ownPropTable(size_t& count) const override
    {
        const std::vector<XamlPropEntry>& t = PropTableData();
        count = t.size();
        return t.data();
    }

private:
    static const std::vector<XamlPropEntry>& PropTableData();

    void applyCommands();

    std::wstring m_commands;
};