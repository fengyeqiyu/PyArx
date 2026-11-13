// XamlPowerCanvas.cpp —— 实现（拆分自 XamlExtraControls.h，UTF-8 with BOM）
#include "XamlPowerCanvas.h"

void XamlPowerCanvas::createNative(IXamlMfcHost* host, int id, int /*min*/, int /*max*/, int x, int y, int w, int h)
{
        host->addPowerCanvas(id, x, y, w, h);
    }

// 属性表口径：只声明真正能写的属性。
// Source 之前被声明成可写，但没有"从文件加载"的语义，写了只会落到未知属性，现已移除。
std::vector<XamlProperty> XamlPowerCanvas::ownProperties() const
{
        return {{L"Name", XamlPropType::String, true, false},
                {L"Data", XamlPropType::String, false, true}};
    }

bool XamlPowerCanvas::setOwnProperty(const wchar_t* name, const std::wstring& value)
{
        if (m_host == nullptr) return false;
        const std::string key = xamlprop::Lower(xamlprop::WideToUtf8(name ? name : L""));
        if (!xamlprop::StrEq(key, "data")) return false;
        // 首字符判定：'<' 是 SVG 文档，'{' 是 JSON（布局结果）
        const size_t first = value.find_first_not_of(L" \t\r\n");
        if (first == std::wstring::npos) return true;
        if (value[first] == L'<')
            m_host->setPowerCanvasSvg(m_id, value);
        else if (value.find(L"\"layout\"") != std::wstring::npos)
            // 带 layout 的 JSON（powerplan.toJson / engine.scene）坐标已经算好，直接按坐标画；
            // 再走一遍 setPowerCanvasPlan 会二次排版，坐标就和外部算的对不上。
            m_host->setPowerCanvasScene(m_id, value);
        else
            // 只给 nodes 的老文档没有坐标，退化到"让 C++ 排版"这条老路
            m_host->setPowerCanvasPlan(m_id, value);
        return true;
    }
