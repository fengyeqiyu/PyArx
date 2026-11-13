// XamlTreeCtrl.cpp —— 属性表实现（UTF-8 with BOM）
#include "XamlTreeCtrl.h"

void XamlTreeCtrl::createNative(IXamlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addTree(id, x, y, w, h);
    applyNodes();
}

void XamlTreeCtrl::applyNodes()
{
    if (m_host != nullptr && !m_nodes.empty())
        xamlextra::addNodes(m_host, m_id, m_nodes);
}

const std::vector<XamlPropEntry>& XamlTreeCtrl::PropTableData()
{
    static const std::vector<XamlPropEntry> kTable = {
        {L"Nodes", nullptr, XamlPropType::String, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlTreeCtrl&>(b);
             c.m_nodes = v;
             c.applyNodes();
             return true;
         },
         [](const XamlMfcControlBase& b) { return static_cast<const XamlTreeCtrl&>(b).m_nodes; }},
    };
    return kTable;
}