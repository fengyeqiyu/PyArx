// XmlView3D.h —— XML <View3D> 三维视图画布（自绘：透视投影 + 视口旋转 + 缩放）
// 属性表：Data —— 几何文本，记录用 ';' 或换行分隔：
//   box  x0 y0 z0 x1 y1 z1 rrggbb
//   line x0 y0 z0 x1 y1 z1 rrggbb [px]
//   tri  x0 y0 z0 x1 y1 z1 x2 y2 z2 rrggbb
// 编码：本文件 UTF-8 with BOM。
#pragma once

#include "XmlMfcControlBase.h"

class XML_CORE_API XmlView3D : public XmlMfcControlBase
{
public:
    XmlView3D() : XmlMfcControlBase(L"view3d") {}

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

    void applyData();

    std::wstring m_data;
};
