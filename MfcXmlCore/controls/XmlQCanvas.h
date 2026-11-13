#pragma once
// XmlQCanvas —— XML <QCanvas> 画命令画布（kind 20）。
// 属性表驱动：Commands、Script（同一份数据：数据化画命令列表）。
// 编码：本文件 UTF-8 with BOM。

#include "XmlMfcControlBase.h"

class XML_CORE_API XmlQCanvas : public XmlMfcControlBase
{
public:
    XmlQCanvas() : XmlMfcControlBase(L"qcanvas") {}

protected:
    void createNative(IXmlMfcHost* host, int id, int min, int max, int x, int y, int w, int h) override;

    const XmlPropEntry* ownPropTable(size_t& count) const override
    {
        const std::vector<XmlPropEntry>& t = PropTableData();
        count = t.size();
        return t.data();
    }

private:
    static const std::vector<XmlPropEntry>& PropTableData();

    void applyCommands();

    std::wstring m_commands;
};