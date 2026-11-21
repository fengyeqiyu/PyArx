// XmlShxText.cpp —— SHX 文字控件实现（UTF-8 with BOM）
#include "XmlShxText.h"

#include "ShxFontCore.h"

#include <cstdlib>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace
{
    bool FileExists(const std::wstring& path)
    {
        const DWORD attr = GetFileAttributesW(path.c_str());
        return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY) == 0;
    }

    std::wstring ExeDir()
    {
        wchar_t buf[MAX_PATH] = {0};
        GetModuleFileNameW(nullptr, buf, MAX_PATH);
        std::wstring p = buf;
        const size_t pos = p.find_last_of(L"\\/");
        return pos == std::wstring::npos ? std::wstring() : p.substr(0, pos);
    }

    // 候选字体目录：exe 同级 Fonts、工程 Fonts、AutoCAD 默认字体目录
    std::vector<std::wstring> FontDirs()
    {
        std::vector<std::wstring> dirs;
        const std::wstring exe = ExeDir();
        if (!exe.empty())
        {
            dirs.push_back(exe + L"\\Fonts");
            dirs.push_back(exe + L"\\..\\Fonts");
        }
        dirs.push_back(L"G:\\PyArx\\Fonts");
        dirs.push_back(L"G:\\PyArx\\ACAD_min\\Fonts");
        dirs.push_back(L"C:\\Program Files\\Autodesk\\AutoCAD 2024\\Fonts");
        dirs.push_back(L"C:\\Program Files\\Autodesk\\AutoCAD 2023\\Fonts");
        return dirs;
    }
}

void XmlShxText::createNative(IXmlMfcHost* host, int id, int /*minValue*/, int /*maxValue*/,
                              int x, int y, int w, int h)
{
    m_w = w;
    m_h = h;
    host->addDwgCanvas(id, x, y, w, h);
    rebuild();
}

void XmlShxText::setBounds(int x, int y, int w, int h)
{
    XmlMfcControlBase::setBounds(x, y, w, h);
    if (w != m_w || h != m_h)
    {
        m_w = w;
        m_h = h;
        rebuild();
    }
}

std::vector<XmlProperty> XmlShxText::ownProperties() const
{
    return {
        {L"Text", XmlPropType::String, true, true, nullptr},
        {L"FontFile", XmlPropType::String, true, true, nullptr},
        {L"BigFontFile", XmlPropType::String, true, true, nullptr},
        {L"FontHeight", XmlPropType::Double, true, true, nullptr},
        {L"WidthFactor", XmlPropType::Double, true, true, nullptr},
        {L"Oblique", XmlPropType::Double, true, true, nullptr},
        {L"Stroke", XmlPropType::Color, true, true, nullptr},
    };
}

bool XmlShxText::setOwnProperty(const wchar_t* name, const std::wstring& value)
{
    if (name == nullptr) return false;
    if (wcscmp(name, L"Text") == 0) { m_text = value; rebuild(); return true; }
    if (wcscmp(name, L"FontFile") == 0) { m_fontFile = value; rebuild(); return true; }
    if (wcscmp(name, L"BigFontFile") == 0) { m_bigFontFile = value; rebuild(); return true; }
    if (wcscmp(name, L"Stroke") == 0) { m_stroke = value; rebuild(); return true; }
    if (wcscmp(name, L"FontHeight") == 0)
    {
        m_fontHeight = value.empty() ? 0.0 : wcstod(value.c_str(), nullptr);
        rebuild();
        return true;
    }
    if (wcscmp(name, L"WidthFactor") == 0)
    {
        m_widthFactor = value.empty() ? 1.0 : wcstod(value.c_str(), nullptr);
        if (m_widthFactor <= 0.0) m_widthFactor = 1.0;
        rebuild();
        return true;
    }
    if (wcscmp(name, L"Oblique") == 0)
    {
        m_oblique = value.empty() ? 0.0 : wcstod(value.c_str(), nullptr);
        rebuild();
        return true;
    }
    return false;
}

std::wstring XmlShxText::getOwnProperty(const wchar_t* name) const
{
    if (name == nullptr) return {};
    wchar_t buf[64];
    if (wcscmp(name, L"Text") == 0) return m_text;
    if (wcscmp(name, L"FontFile") == 0) return m_fontFile;
    if (wcscmp(name, L"BigFontFile") == 0) return m_bigFontFile;
    if (wcscmp(name, L"Stroke") == 0) return m_stroke;
    if (wcscmp(name, L"FontHeight") == 0)
    {
        swprintf_s(buf, L"%g", m_fontHeight);
        return buf;
    }
    if (wcscmp(name, L"WidthFactor") == 0)
    {
        swprintf_s(buf, L"%g", m_widthFactor);
        return buf;
    }
    if (wcscmp(name, L"Oblique") == 0)
    {
        swprintf_s(buf, L"%g", m_oblique);
        return buf;
    }
    return {};
}

void XmlShxText::onAppearanceChanged()
{
    rebuild();
    pushAppearance();
}

std::wstring XmlShxText::resolveFont(const std::wstring& name) const
{
    if (name.empty()) return {};
    if (FileExists(name)) return name;   // 绝对路径 / 相对当前目录

    for (const std::wstring& dir : FontDirs())
    {
        const std::wstring p = dir + L"\\" + name;
        if (FileExists(p)) return p;
    }
    // 试几种常见拼写（大小写、.shx 后缀）
    std::wstring alt = name;
    if (alt.size() > 4 && alt.substr(alt.size() - 4) != L".shx" && alt.substr(alt.size() - 4) != L".SHX")
        alt += L".shx";
    for (const std::wstring& dir : FontDirs())
    {
        const std::wstring p = dir + L"\\" + alt;
        if (FileExists(p)) return p;
    }
    return {};
}

std::wstring XmlShxText::buildScene() const
{
    const std::wstring shx = resolveFont(m_fontFile);
    if (shx.empty()) return {};

    xmlshx::Style style;
    if (!style.setFonts(shx, resolveFont(m_bigFontFile)))
        return {};

    const double h = (m_fontHeight > 0.0)
                         ? m_fontHeight
                         : (m_h > 0 ? m_h * 0.55 : 21.0);
    auto polys = style.layoutMultiline(m_text, h, m_widthFactor, m_oblique, 1.7);
    if (polys.empty()) return {};
    return xmlshx::toXScene(polys, m_w, m_h, m_stroke, 1.2);
}

void XmlShxText::rebuild()
{
    if (m_host == nullptr || m_id == 0) return;

    int x = 0, y = 0, w = m_w, h = m_h;
    m_host->getControlRect(m_id, x, y, w, h);
    if (w > 0) m_w = w;
    if (h > 0) m_h = h;
    if (m_w <= 0) m_w = 800;   // 布局尚未落定时的兜底尺寸
    if (m_h <= 0) m_h = 64;

    const std::wstring scene = buildScene();
    if (!scene.empty())
        m_host->setDwgXml(m_id, scene);
}
