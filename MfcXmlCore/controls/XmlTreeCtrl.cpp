// XmlTreeCtrl.cpp —— 属性表实现（UTF-8 with BOM）
#include "XmlTreeCtrl.h"

void XmlTreeCtrl::createNative(IXmlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addTree(id, x, y, w, h);
    applyNodes();
}

void XmlTreeCtrl::applyNodes()
{
    if (m_host != nullptr && !m_nodes.empty())
        xmlextra::addNodes(m_host, m_id, m_nodes);
}

const std::vector<XmlPropEntry>& XmlTreeCtrl::PropTableData()
{
    static const std::vector<XmlPropEntry> kTable = {
        {L"Nodes", nullptr, XmlPropType::String, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlTreeCtrl&>(b);
             c.m_nodes = v;
             c.applyNodes();
             return true;
         },
         [](const XmlMfcControlBase& b) { return static_cast<const XmlTreeCtrl&>(b).m_nodes; }},
    };
    return kTable;
}