// XmlView3D.cpp —— 属性表实现（UTF-8 with BOM）
#include "XmlView3D.h"

void XmlView3D::createNative(IXmlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addView3D(id, x, y, w, h);
    applyData();
}

void XmlView3D::applyData()
{
    if (m_host == nullptr || m_data.empty()) return;
    m_host->setView3DModel(m_id, m_data);
}

const std::vector<XmlPropEntry>& XmlView3D::PropTableData()
{
    static const std::vector<XmlPropEntry> kTable = {
        {L"Data", nullptr, XmlPropType::String, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlView3D&>(b);
             c.m_data = v;
             c.applyData();
             return true;
         },
         [](const XmlMfcControlBase& b) { return static_cast<const XmlView3D&>(b).m_data; }},
    };
    return kTable;
}
