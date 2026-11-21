#pragma once
// XmlStaticBitmap —— XML <StaticBitmap> 静态位图。属性表驱动：Image。
// 编码：本文件 UTF-8 with BOM。

#include "XmlMfcControlBase.h"

class XML_CORE_API XmlStaticBitmap : public XmlMfcControlBase
{
public:
    XmlStaticBitmap() : XmlMfcControlBase(L"staticbitmap") {}

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

    std::wstring m_image;
    std::wstring m_objectFit;   // CSS object-fit：fill（默认）/ contain / cover / none / scale-down
    std::wstring m_objectPos;   // CSS object-position：图片在盒子里的对齐位置
};