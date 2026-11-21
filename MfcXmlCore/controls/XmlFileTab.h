// XmlFileTab.h —— XML <XFileTab> 多文档文件页签条。
// 模型/布局/绘制/命中全部复用控件库 xmlctl::FileTab；本类只负责建自绘宿主窗口。
// 子项 XFileTabItem 由宿主 filetabParse 解析，控件类不再逐节点建原生子控件。
// 编码：本文件 UTF-8 with BOM。
#pragma once

#include "XmlMfcControlBase.h"

class XML_CORE_API XmlFileTab : public XmlMfcControlBase
{
public:
    XmlFileTab() : XmlMfcControlBase(L"filetab") {}

protected:
    void createNative(IXmlMfcHost* host, int id, int min, int max,
                      int x, int y, int w, int h) override;

    const XmlPropEntry* ownPropTable(size_t& count) const override
    {
        count = 0;
        return nullptr;
    }
};
