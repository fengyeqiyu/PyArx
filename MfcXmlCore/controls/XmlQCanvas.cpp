// XmlQCanvas.cpp —— 属性表实现（UTF-8 with BOM）
#include "XmlQCanvas.h"

void XmlQCanvas::createNative(IXmlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addSkinBox(id, 20, x, y, w, h);
    applyCommands();
}

void XmlQCanvas::applyCommands()
{
    if (m_host != nullptr && !m_commands.empty())
        m_host->setCanvasCommands(m_id, m_commands);
}

const std::vector<XmlPropEntry>& XmlQCanvas::PropTableData()
{
    static const std::vector<XmlPropEntry> kTable = {
        {L"Commands", nullptr, XmlPropType::String, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlQCanvas&>(b);
             c.m_commands = v;
             c.applyCommands();
             return true;
         },
         [](const XmlMfcControlBase& b) {
             const auto& c = static_cast<const XmlQCanvas&>(b);
             return c.m_host != nullptr ? c.m_host->getCanvasCommands(c.m_id) : c.m_commands;
         }},
        {L"Script", nullptr, XmlPropType::String, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlQCanvas&>(b);
             c.m_commands = v;
             c.applyCommands();
             return true;
         },
         [](const XmlMfcControlBase& b) { return static_cast<const XmlQCanvas&>(b).m_commands; }},
    };
    return kTable;
}