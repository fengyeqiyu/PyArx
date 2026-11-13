// XmlPowerCanvas.cpp —— 属性表实现（UTF-8 with BOM）
#include "XmlPowerCanvas.h"

void XmlPowerCanvas::createNative(IXmlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addPowerCanvas(id, x, y, w, h);
    applyData();
}

void XmlPowerCanvas::applyData()
{
    if (m_host == nullptr || m_data.empty()) return;
    const size_t first = m_data.find_first_not_of(L" \t\r\n");
    if (first == std::wstring::npos) return;
    if (m_data[first] == L'<')
        m_host->setPowerCanvasSvg(m_id, m_data);
    else if (m_data.find(L"\"layout\"") != std::wstring::npos)
        m_host->setPowerCanvasScene(m_id, m_data);   // 坐标已算好，直接按坐标画
    else
        m_host->setPowerCanvasPlan(m_id, m_data);    // 只有 nodes，交给 C++ 排版
}

const std::vector<XmlPropEntry>& XmlPowerCanvas::PropTableData()
{
    static const std::vector<XmlPropEntry> kTable = {
        {L"Data", nullptr, XmlPropType::String, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlPowerCanvas&>(b);
             c.m_data = v;
             c.applyData();
             return true;
         },
         [](const XmlMfcControlBase& b) { return static_cast<const XmlPowerCanvas&>(b).m_data; }},
    };
    return kTable;
}