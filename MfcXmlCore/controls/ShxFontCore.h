#pragma once
// ShxFontCore —— SHX 字体解析与排版核心（纯 STL，不依赖 MFC，也不依赖 AutoCAD）。
//
// 和 tools/shxtext/shxfont.py 是同一套解析规则：unifont / bigfont 两种容器、
// 16 方向向量、八分弧 / 分数弧 / 凸度弧、子字形引用、缩放指令；排版按字体自身
// 度量（大写字母 ymin 中位数 = 基线，ymax 中位数 = cap 高）。
//
// 输出折线是世界坐标：y 向上、基线 y = 0、原点在文本起点；
// 要交给 <XScene>（左上原点、y 向下）时用 toXScene 做一次映射。
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace xmlshx
{
    struct Pt
    {
        double x = 0.0;
        double y = 0.0;
    };

    struct Glyph
    {
        std::vector<std::vector<Pt>> polylines;
        double advance = 0.0;
        double ymin = 0.0;
        double ymax = 0.0;
        double xmin = 0.0;
        double xmax = 0.0;
        bool empty() const { return polylines.empty(); }
    };

    enum class Kind
    {
        Unknown,
        Unifont,   // 西文 / AutoCAD-86 unifont
        Bigfont,   // 中文大字体 / AutoCAD-86 bigfont
        Shapes     // 形文件（不含字形表，仅识别类型）
    };

    class Font
    {
    public:
        // 按路径加载并缓存（同一路径只解析一次；线程内共享）
        static std::shared_ptr<Font> Load(const std::wstring& path);

        bool valid() const { return m_kind != Kind::Unknown; }
        Kind kind() const { return m_kind; }
        int glyphCount() const { return static_cast<int>(m_raw.size()); }
        bool has(int code) const { return m_raw.find(code) != m_raw.end(); }

        // 取字形（带缓存）；未知码返回空字形
        const Glyph& glyph(int code);

        double baseY() const { return m_baseY; }
        double capH() const { return m_capH; }
        const std::wstring& path() const { return m_path; }

    private:
        Font() = default;
        void parse();
        void probeMetrics();
        Glyph decodeRecord(const std::vector<unsigned char>& rec, int depth);
        Glyph run(const std::vector<unsigned char>& code, int depth);

        std::wstring m_path;
        std::vector<unsigned char> m_data;
        Kind m_kind = Kind::Unknown;
        int m_count = 0;
        std::map<int, std::vector<unsigned char>> m_raw;
        std::map<int, Glyph> m_cache;
        double m_baseY = 0.0;
        double m_capH = 21.0;
    };

    // 西文 + 中文大字体组合，模拟 CAD 的 shx 文字样式
    class Style
    {
    public:
        // 返回 false 表示西文字体加载失败；大字体可选
        bool setFonts(const std::wstring& shxPath, const std::wstring& bigFontPath);

        bool valid() const { return m_font && m_font->valid(); }
        std::shared_ptr<Font> font() const { return m_font; }
        std::shared_ptr<Font> bigFont() const { return m_big; }

        // 排版：height = 大写字高（cap height）
        // widthFactor > 0；obliqueDeg = 倾斜角（度，正值右倾）；spacing 额外字距（世界单位）
        std::vector<std::vector<Pt>> layout(const std::wstring& text, double height,
                                           double widthFactor = 1.0, double obliqueDeg = 0.0,
                                           double spacing = 0.0) const;

        // 排版成一条一条（按 \n 换行，行距 = 1.8 * height），返回全部折线
        std::vector<std::vector<Pt>> layoutMultiline(const std::wstring& text, double height,
                                                    double widthFactor = 1.0,
                                                    double obliqueDeg = 0.0,
                                                    double lineGap = 1.8) const;

    private:
        bool pick(wchar_t ch, const Glyph*& g, double& base, double& cap, bool& cjk) const;

        std::shared_ptr<Font> m_font;
        std::shared_ptr<Font> m_big;
        double m_baseW = 0.0;
        double m_capW = 21.0;
        double m_baseC = 0.0;
        double m_capC = 21.0;
    };

    struct Box
    {
        double x0 = 0.0;
        double y0 = 0.0;
        double x1 = 0.0;
        double y1 = 0.0;
        bool empty() const { return x1 <= x0 && y1 <= y0; }
    };

    Box bounds(const std::vector<std::vector<Pt>>& polys);

    // 折线 → <XScene> XML（左上原点、y 向下）：等比映射进 w×h 画布并居中。
    // stroke = "#RRGGBB"；strokeWidth 单位是场景像素。
    std::wstring toXScene(const std::vector<std::vector<Pt>>& polys, int w, int h,
                          const std::wstring& stroke = L"#1A1A1A", double strokeWidth = 1.2);
}
