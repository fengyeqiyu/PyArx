// XmlRibbon.h —— XML <XRibbon> 命令功能区。
// 模型/布局/绘制/命中全部复用控件库 xmlctl::XRibbon；本类只负责建自绘宿主窗口。
// Tab/Group/Button 子树由宿主 ribbonParse 解析，控件类不再逐节点建原生子控件。
// 编码：本文件 UTF-8 with BOM。
#pragma once

#include "XmlMfcControlBase.h"

class XML_CORE_API XmlRibbon : public XmlMfcControlBase
{
public:
    XmlRibbon() : XmlMfcControlBase(L"xribbon") {}

protected:
    void createNative(IXmlMfcHost* host, int id, int min, int max,
                      int x, int y, int w, int h) override;

    const XmlPropEntry* ownPropTable(size_t& count) const override
    {
        count = 0;
        return nullptr;
    }
};
