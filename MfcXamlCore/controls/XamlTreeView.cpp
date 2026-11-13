// XamlTreeView.cpp —— 属性表实现（UTF-8 with BOM）
#include "XamlTreeView.h"

void XamlTreeView::createNative(IXamlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addSkinBox(id, 10, x, y, w, h);
    applyNodes();
    if (m_selectedIndex >= 0)
        host->setTreeSelectedRow(id, m_selectedIndex);
}

void XamlTreeView::applyNodes()
{
    if (m_host == nullptr || m_nodes.empty()) return;
    std::wstring cur;
    auto flush = [&]() {
        if (cur.empty()) return;
        bool kids = false;
        if (cur.back() == L'>')
        {
            kids = true;
            cur.pop_back();
        }
        int level = 0;
        std::wstring text = cur;
        const size_t at = cur.find_last_of(L'@');
        if (at != std::wstring::npos)
        {
            text = cur.substr(0, at);
            level = xamlprop2::ToInt(cur.substr(at + 1), 0);
        }
        m_host->addTreeItem(m_id, text, level, kids);
        cur.clear();
    };
    for (wchar_t ch : m_nodes)
    {
        if (ch == L'|' || ch == L'\n' || ch == L'\r')
            flush();
        else
            cur.push_back(ch);
    }
    flush();
}

const std::vector<XamlPropEntry>& XamlTreeView::PropTableData()
{
    static const std::vector<XamlPropEntry> kTable = {
        {L"Nodes", nullptr, XamlPropType::String, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlTreeView&>(b);
             c.m_nodes = v;
             c.applyNodes();
             return true;
         },
         [](const XamlMfcControlBase& b) { return static_cast<const XamlTreeView&>(b).m_nodes; }},
        {L"SelectedIndex", nullptr, XamlPropType::Int, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlTreeView&>(b);
             c.m_selectedIndex = xamlprop2::ToInt(v, -1);
             if (c.m_host != nullptr) c.m_host->setTreeSelectedRow(c.m_id, c.m_selectedIndex);
             return true;
         },
         [](const XamlMfcControlBase& b) {
             const auto& c = static_cast<const XamlTreeView&>(b);
             const int i = c.m_host != nullptr ? c.m_host->getTreeSelectedRow(c.m_id) : c.m_selectedIndex;
             return std::to_wstring(i);
         }},
    };
    return kTable;
}