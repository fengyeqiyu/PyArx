// XmlDwgCanvas.cpp —— 属性表实现（UTF-8 with BOM）
#include "XmlDwgCanvas.h"

void XmlDwgCanvas::createNative(IXmlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addDwgCanvas(id, x, y, w, h);
    applyData();
}

void XmlDwgCanvas::applyData()
{
    if (m_host == nullptr || m_data.empty()) return;
    const size_t first = m_data.find_first_not_of(L" \t\r\n");
    if (first == std::wstring::npos) return;
    if (m_data[first] == L'<')
    {
        // 以 '<' 开头有两种前端语言：<svg…> 走 nanosvg；<XScene>（或直接 <XLine…>）是
        // XML 声明式显示列表，宿主直接建场景，不做 SVG 文本往返。
        const wchar_t* p = m_data.c_str() + first;
        bool isSvg = false;
        bool isXmlScene = false;
        for (int k = 1; p[k] != L'\0' && k < 24; ++k)
        {
            if (p[k] == L'>') break;
            if (p[k] == L's' || p[k] == L'S')
            {
                // "<svg" / "<svg:svg"
                if (p[k + 1] != L'\0' && (p[k + 1] == L'v' || p[k + 1] == L'V'))
                {
                    if (k == 1) isSvg = true;
                }
            }
            if ((p[k] == L'X' || p[k] == L'x') && k == 1) isXmlScene = true;
        }
        if (isXmlScene && !isSvg)
            m_host->setDwgXml(m_id, m_data);
        else
            m_host->setDwgSvg(m_id, m_data);
    }
    else if (m_data.find(L"\"layout\"") != std::wstring::npos)
        m_host->setDwgScene(m_id, m_data);   // 坐标已算好，直接按坐标画
    else
        m_host->setDwgLayout(m_id, m_data);    // 只有 nodes，交给 C++ 排版
}

const std::vector<XmlPropEntry>& XmlDwgCanvas::PropTableData()
{
    static const std::vector<XmlPropEntry> kTable = {
        {L"Data", nullptr, XmlPropType::String, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlDwgCanvas&>(b);
             c.m_data = v;
             c.applyData();
             return true;
         },
         [](const XmlMfcControlBase& b) { return static_cast<const XmlDwgCanvas&>(b).m_data; }},
    };
    return kTable;
}