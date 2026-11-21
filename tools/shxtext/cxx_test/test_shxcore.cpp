// test_shxcore.cpp —— ShxFontCore 无头验证：解析真实字体、打印度量、输出 SVG/XScene 对照
// 编译：
//   cl /EHsc /std:c++17 /utf-8 /O2 test_shxcore.cpp ..\..\..\MfcXmlCore\controls\ShxFontCore.cpp
#include "../../../MfcXmlCore/controls/ShxFontCore.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static std::wstring JoinDir(const std::wstring& dir, const wchar_t* name)
{
    return dir + L"\\" + name;
}

static bool WriteText(const std::wstring& path, const std::wstring& text)
{
    FILE* f = nullptr;
    if (_wfopen_s(&f, path.c_str(), L"wb") != 0 || f == nullptr) return false;
    const size_t n = text.size();
    fwrite(text.c_str(), sizeof(wchar_t), n, f);
    fclose(f);
    return true;
}

static bool WriteUtf8(const std::wstring& path, const std::string& text)
{
    FILE* f = nullptr;
    if (_wfopen_s(&f, path.c_str(), L"wb") != 0 || f == nullptr) return false;
    fwrite(text.data(), 1, text.size(), f);
    fclose(f);
    return true;
}

static std::string ToUtf8(const std::wstring& s)
{
    std::string out;
    if (s.empty()) return out;
    int need = WideCharToMultiByte(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0, nullptr, nullptr);
    out.resize(need);
    WideCharToMultiByte(CP_UTF8, 0, s.c_str(), (int)s.size(), out.data(), need, nullptr, nullptr);
    return out;
}

// 折线 → SVG（y 翻转，等比例缩放到 w×h）
static std::string ToSvg(const std::vector<std::vector<xmlshx::Pt>>& polys, int w, int h,
                         const char* stroke)
{
    if (polys.empty()) return "<svg/>";
    const xmlshx::Box b = xmlshx::bounds(polys);
    const double bw = b.x1 - b.x0, bh = b.y1 - b.y0;
    const double pad = 8.0;
    const double s = (bh > 1e-9) ? (h - 2 * pad) / bh : 1.0;
    char buf[256];
    std::string svg;
    snprintf(buf, sizeof(buf), "<svg xmlns='http://www.w3.org/2000/svg' width='%d' height='%d' "
                               "viewBox='0 0 %d %d'><rect width='100%%' height='100%%' fill='#FFFFFF'/>",
             w, h, w, h);
    svg += buf;
    for (const auto& poly : polys)
    {
        if (poly.size() < 2) continue;
        svg += "<polyline fill='none' stroke='";
        svg += stroke;
        svg += "' stroke-width='1.4' points='";
        for (size_t i = 0; i < poly.size(); ++i)
        {
            const double x = (poly[i].x - b.x0) * s + pad;
            const double y = (b.y1 - poly[i].y) * s + pad;
            snprintf(buf, sizeof(buf), "%.1f,%.1f ", x, y);
            svg += buf;
        }
        svg += "'/>";
    }
    svg += "</svg>";
    return svg;
}

int wmain(int argc, wchar_t** argv)
{
    std::wstring fontDir = L"C:\\Program Files\\Autodesk\\AutoCAD 2024\\Fonts";
    if (argc > 1) fontDir = argv[1];
    const std::wstring outDir = L"G:\\PyArx\\tools\\shxtext\\out";

    auto rom = xmlshx::Font::Load(JoinDir(fontDir, L"romans.shx"));
    auto txt = xmlshx::Font::Load(JoinDir(fontDir, L"txt.shx"));
    auto hz = xmlshx::Font::Load(JoinDir(fontDir, L"hztxt.shx"));
    auto gb = xmlshx::Font::Load(JoinDir(fontDir, L"gbcbig.shx"));

    wprintf(L"romans: valid=%d kind=%d glyphs=%d base=%.2f cap=%.2f\n", rom->valid() ? 1 : 0,
            (int)rom->kind(), rom->glyphCount(), rom->baseY(), rom->capH());
    wprintf(L"txt   : valid=%d kind=%d glyphs=%d base=%.2f cap=%.2f\n", txt->valid() ? 1 : 0,
            (int)txt->kind(), txt->glyphCount(), txt->baseY(), txt->capH());
    wprintf(L"hztxt : valid=%d kind=%d glyphs=%d base=%.2f cap=%.2f\n", hz->valid() ? 1 : 0,
            (int)hz->kind(), hz->glyphCount(), hz->baseY(), hz->capH());
    wprintf(L"gbcbig: valid=%d kind=%d glyphs=%d base=%.2f cap=%.2f\n", gb->valid() ? 1 : 0,
            (int)gb->kind(), gb->glyphCount(), gb->baseY(), gb->capH());

    // 单字形抽查：A / H / g / 中 / 图
    struct Probe { const wchar_t* label; int code; };
    const Probe probes[] = {{L"A", 'A'}, {L"H", 'H'}, {L"g", 'g'}, {L"5", '5'},
                            {L"中", 0xD6D0}, {L"图", 0xCDBC}, {L"建", 0xBDA8}, {L"设", 0xC9E8}};
    for (const Probe& p : probes)
    {
        const xmlshx::Font* f = (p.code > 0x80) ? hz.get() : rom.get();
        const xmlshx::Glyph& g = const_cast<xmlshx::Font*>(f)->glyph(p.code);
        wprintf(L"glyph %ls code=0x%04X polys=%zu pts=%zu bbox=(%.1f,%.1f)-(%.1f,%.1f) adv=%.2f\n",
                p.label, p.code, g.polylines.size(),
                [&] { size_t n = 0; for (auto& pl : g.polylines) n += pl.size(); return n; }(),
                g.xmin, g.ymin, g.xmax, g.ymax, g.advance);
    }

    // 排版
    xmlshx::Style style;
    style.setFonts(JoinDir(fontDir, L"romans.shx"), JoinDir(fontDir, L"hztxt.shx"));

    const std::wstring latin = L"SHX single-line font 0123456789";
    const std::wstring chinese = L"图纸设计 建筑说明 结构";
    const std::wstring mix = L"SHX 汉字 ABC 123";
    auto pLatin = style.layoutMultiline(latin, 24.0, 1.0, 0.0, 1.7);
    auto pCn = style.layoutMultiline(chinese, 24.0, 1.0, 0.0, 1.7);
    auto pMix = style.layoutMultiline(mix, 24.0, 1.0, 0.0, 1.7);
    auto pOblique = style.layoutMultiline(L"Tilted 15 deg text", 24.0, 1.0, 15.0, 1.7);
    auto pNarrow = style.layoutMultiline(L"Width factor 0.75", 24.0, 0.75, 0.0, 1.7);

    auto stat = [](const std::vector<std::vector<xmlshx::Pt>>& p) {
        size_t pts = 0;
        for (auto& pl : p) pts += pl.size();
        const xmlshx::Box b = xmlshx::bounds(p);
        return std::make_pair(pts, b);
    };

    auto s1 = stat(pLatin);
    auto s2 = stat(pCn);
    auto s3 = stat(pMix);
    wprintf(L"latin  : polys=%zu pts=%zu bbox=(%.1f,%.1f)-(%.1f,%.1f)\n", pLatin.size(), s1.first, s1.second.x0, s1.second.y0, s1.second.x1, s1.second.y1);
    wprintf(L"chinese: polys=%zu pts=%zu bbox=(%.1f,%.1f)-(%.1f,%.1f)\n", pCn.size(), s2.first, s2.second.x0, s2.second.y0, s2.second.x1, s2.second.y1);
    wprintf(L"mixed  : polys=%zu pts=%zu bbox=(%.1f,%.1f)-(%.1f,%.1f)\n", pMix.size(), s3.first, s3.second.x0, s3.second.y0, s3.second.x1, s3.second.y1);

    WriteUtf8(outDir + L"\\cxx_latin.svg", ToSvg(pLatin, 900, 70, "#1A1A1A"));
    WriteUtf8(outDir + L"\\cxx_cn.svg", ToSvg(pCn, 760, 70, "#1A1A1A"));
    WriteUtf8(outDir + L"\\cxx_mix.svg", ToSvg(pMix, 760, 70, "#1A1A1A"));
    WriteUtf8(outDir + L"\\cxx_oblique.svg", ToSvg(pOblique, 760, 70, "#1A1A1A"));
    WriteUtf8(outDir + L"\\cxx_narrow.svg", ToSvg(pNarrow, 760, 70, "#1A1A1A"));

    // XScene（宿主通路）：一条西文 + 一条中文
    std::wstring scene = xmlshx::toXScene(pLatin, 900, 70, L"#1A1A1A", 1.2);
    WriteUtf8(outDir + L"\\cxx_scene.xml", ToUtf8(scene));

    // 负例：不存在的字体应明确失败
    xmlshx::Style bad;
    const bool badOk = bad.setFonts(JoinDir(fontDir, L"__missing__.shx"), L"");
    wprintf(L"missing-font setFonts=%d valid=%d\n", badOk ? 1 : 0, bad.valid() ? 1 : 0);

    wprintf(L"done\n");
    return 0;
}
