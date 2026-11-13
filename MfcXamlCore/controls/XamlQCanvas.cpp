// XamlQCanvas.cpp —— 属性表实现（UTF-8 with BOM）
#include "XamlQCanvas.h"

void XamlQCanvas::createNative(IXamlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addSkinBox(id, 20, x, y, w, h);
    applyCommands();
}

void XamlQCanvas::applyCommands()
{
    if (m_host != nullptr && !m_commands.empty())
        m_host->setCanvasCommands(m_id, m_commands);
}

const std::vector<XamlPropEntry>& XamlQCanvas::PropTableData()
{
    static const std::vector<XamlPropEntry> kTable = {
        {L"Commands", nullptr, XamlPropType::String, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlQCanvas&>(b);
             c.m_commands = v;
             c.applyCommands();
             return true;
         },
         [](const XamlMfcControlBase& b) {
             const auto& c = static_cast<const XamlQCanvas&>(b);
             return c.m_host != nullptr ? c.m_host->getCanvasCommands(c.m_id) : c.m_commands;
         }},
        {L"Script", nullptr, XamlPropType::String, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlQCanvas&>(b);
             c.m_commands = v;
             c.applyCommands();
             return true;
         },
         [](const XamlMfcControlBase& b) { return static_cast<const XamlQCanvas&>(b).m_commands; }},
    };
    return kTable;
}