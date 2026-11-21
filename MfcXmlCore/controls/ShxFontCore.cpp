// ShxFontCore.cpp —— SHX 解析与排版实现（UTF-8 with BOM）
#include "ShxFontCore.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <mutex>

#ifndef NOMINMAX
#define NOMINMAX          // windows.h 的 min/max 宏会和 std::min/std::max 冲突
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace xmlshx
{

namespace
{
    bool ReadAll(const std::wstring& path, std::vector<unsigned char>& out)
    {
        FILE* f = nullptr;
        if (_wfopen_s(&f, path.c_str(), L"rb") != 0 || f == nullptr)
            return false;
        fseek(f, 0, SEEK_END);
        const long n = ftell(f);
        fseek(f, 0, SEEK_SET);
        if (n <= 0)
        {
            fclose(f);
            return false;
        }
        out.resize(static_cast<size_t>(n));
        const size_t got = fread(out.data(), 1, out.size(), f);
        fclose(f);
        out.resize(got);
        return got > 0;
    }

    inline int U16(const std::vector<unsigned char>& d, size_t o)
    {
        return static_cast<int>(d[o]) | (static_cast<int>(d[o + 1]) << 8);
    }

    inline unsigned int U32(const std::vector<unsigned char>& d, size_t o)
    {
        return static_cast<unsigned int>(d[o]) | (static_cast<unsigned int>(d[o + 1]) << 8) |
               (static_cast<unsigned int>(d[o + 2]) << 16) |
               (static_cast<unsigned int>(d[o + 3]) << 24);
    }

    inline int S8(unsigned char b) { return b < 128 ? static_cast<int>(b) : static_cast<int>(b) - 256; }

    // 16 方向单位向量：沿主轴长度为 1，对角 dx = dy = 1（斜线由此得到 45° 走向）
    const double kDir[16][2] = {
        {1.0, 0.0},  {1.0, 0.414},  {1.0, 1.0},  {0.414, 1.0},
        {0.0, 1.0},  {-0.414, 1.0}, {-1.0, 1.0}, {-1.0, 0.414},
        {-1.0, 0.0}, {-1.0, -0.414}, {-1.0, -1.0}, {-0.414, -1.0},
        {0.0, -1.0}, {0.414, -1.0}, {1.0, -1.0}, {1.0, -0.414}};

    double Median(std::vector<double> v)
    {
        if (v.empty()) return 0.0;
        std::sort(v.begin(), v.end());
        const size_t n = v.size();
        return (n % 2) ? v[n / 2] : (v[n / 2 - 1] + v[n / 2]) * 0.5;
    }

    // UTF-16 字符 → 单字节码（ASCII）或 GB2312 双字节原码
    bool ToShxCode(wchar_t ch, int& code, bool& cjk)
    {
        const unsigned u = static_cast<unsigned>(ch);
        if (u < 0x80)
        {
            code = static_cast<int>(u);
            cjk = false;
            return true;
        }
        char buf[4] = {0, 0, 0, 0};
        const int n = WideCharToMultiByte(936, 0, &ch, 1, buf, 2, nullptr, nullptr);
        if (n == 2)
        {
            code = (static_cast<unsigned char>(buf[0]) << 8) | static_cast<unsigned char>(buf[1]);
            cjk = true;
            return true;
        }
        if (n == 1)
        {
            code = static_cast<unsigned char>(buf[0]);
            cjk = false;
            return true;
        }
        return false;
    }

    void AppendPt(std::vector<Pt>& v, double x, double y) { v.push_back(Pt{x, y}); }

    // 三点定圆：p0/p1 为端点，mid 为弧上一点（用来定方向与半径）
    bool CircleFromArc(const Pt& p0, const Pt& p1, const Pt& mid, Pt& c, double& r)
    {
        const double ax = p0.x, ay = p0.y, bx = p1.x, by = p1.y, cx = mid.x, cy = mid.y;
        const double d = 2.0 * (ax * (by - cy) + bx * (cy - ay) + cx * (ay - by));
        if (std::fabs(d) < 1e-12) return false;
        const double a2 = ax * ax + ay * ay, b2 = bx * bx + by * by, c2 = cx * cx + cy * cy;
        c.x = (a2 * (by - cy) + b2 * (cy - ay) + c2 * (ay - by)) / d;
        c.y = (a2 * (cx - bx) + b2 * (ax - cx) + c2 * (bx - ax)) / d;
        r = std::hypot(ax - c.x, ay - c.y);
        return r > 1e-9;
    }
}

// ---------------------------------------------------------------- Font
std::shared_ptr<Font> Font::Load(const std::wstring& path)
{
    static std::mutex s_mu;
    static std::map<std::wstring, std::weak_ptr<Font>> s_cache;
    std::lock_guard<std::mutex> lk(s_mu);

    auto it = s_cache.find(path);
    if (it != s_cache.end())
    {
        if (auto sp = it->second.lock())
            return sp;
    }
    auto sp = std::shared_ptr<Font>(new Font());
    sp->m_path = path;
    sp->parse();
    s_cache[path] = sp;
    return sp;
}

void Font::parse()
{
    if (!ReadAll(m_path, m_data) || m_data.size() < 32)
        return;

    // 头 40 字节内找签名
    std::string head;
    for (size_t i = 0; i < m_data.size() && i < 64; ++i)
    {
        const unsigned char c = m_data[i];
        if (c == 0) break;
        head.push_back(static_cast<char>(c));
    }
    if (head.find("unifont") != std::string::npos)
        m_kind = Kind::Unifont;
    else if (head.find("bigfont") != std::string::npos)
        m_kind = Kind::Bigfont;
    else if (head.find("shapes") != std::string::npos)
        m_kind = Kind::Shapes;
    else
        return;

    size_t stop = 0;
    bool found = false;
    for (size_t i = 0; i < m_data.size() && i < 40; ++i)
    {
        if (m_data[i] == 0x1A)
        {
            stop = i;
            found = true;
            break;
        }
    }
    if (!found) return;

    const size_t ofs = stop + 1;
    if (ofs + 2 > m_data.size()) return;
    m_count = U16(m_data, ofs);

    if (m_kind == Kind::Unifont)
    {
        size_t p = ofs + 2;
        for (int n = 0; n < m_count && p + 4 <= m_data.size(); ++n)
        {
            const int code = U16(m_data, p);
            const int len = U16(m_data, p + 2);
            p += 4;
            if (p + static_cast<size_t>(len) > m_data.size() || len < 0) break;
            m_raw[code] = std::vector<unsigned char>(m_data.begin() + p, m_data.begin() + p + len);
            p += static_cast<size_t>(len);
        }
    }
    else if (m_kind == Kind::Bigfont)
    {
        // bigfont 头比 unifont 多一个 u16（shape 数）：
        //   ofs: u16 shapeCount，ofs+2: u16 条目数，ofs+4: u16 nchange，
        //   ofs+6+4*nchange: 索引起点，每项 u16 code + u16 len + u32 offset
        if (ofs + 6 > m_data.size()) return;
        m_count = U16(m_data, ofs + 2);
        const int nChange = U16(m_data, ofs + 4);
        size_t p = ofs + 6 + static_cast<size_t>(nChange) * 4;
        for (int n = 0; n < m_count && p + 8 <= m_data.size(); ++n)
        {
            const int code = U16(m_data, p);
            const int len = U16(m_data, p + 2);
            const unsigned int off = U32(m_data, p + 4);
            p += 8;
            if (off + static_cast<unsigned int>(len) > m_data.size() || len <= 0) continue;
            m_raw[code] = std::vector<unsigned char>(m_data.begin() + off, m_data.begin() + off + len);
        }
    }

    probeMetrics();
}

void Font::probeMetrics()
{
    std::vector<double> lo, hi;
    if (m_kind == Kind::Bigfont)
    {
        // 常用汉字度量：中 国 图 建 设 计 说 一
        static const int kProbe[] = {0xD6D0, 0xB9FA, 0xCDBC, 0xBDA8, 0xC9E8, 0xBCC6, 0xCBB5, 0xD2BB};
        for (int c : kProbe)
        {
            const Glyph& g = glyph(c);
            if (!g.empty())
            {
                lo.push_back(g.ymin);
                hi.push_back(g.ymax);
            }
        }
    }
    if (lo.empty())
    {
        for (int c = 'A'; c <= 'Z'; ++c)
        {
            const Glyph& g = glyph(c);
            if (!g.empty())
            {
                lo.push_back(g.ymin);
                hi.push_back(g.ymax);
            }
        }
    }
    if (lo.empty())
    {
        m_baseY = 0.0;
        m_capH = 21.0;
        return;
    }
    m_baseY = Median(lo);
    m_capH = Median(hi) - m_baseY;
    if (m_capH < 1e-6) m_capH = 21.0;
}

const Glyph& Font::glyph(int code)
{
    static const Glyph kEmpty;
    auto c = m_cache.find(code);
    if (c != m_cache.end()) return c->second;

    auto it = m_raw.find(code);
    if (it == m_raw.end())
    {
        m_cache[code] = Glyph();
        return m_cache[code];
    }
    Glyph g = decodeRecord(it->second, 0);
    // advance：按字形宽度 + 一点侧边距（字体自带的 advance 常不可靠）
    g.advance = (g.empty() ? 0.0 : (g.xmax - g.xmin));
    m_cache[code] = g;
    return m_cache[code];
}

Glyph Font::decodeRecord(const std::vector<unsigned char>& rec, int depth)
{
    // 记录前缀可能是名字（"LATIN CAPITAL LETTER C" + 0x00），正文从 0 之后开始
    size_t i = 0;
    while (i < rec.size() && rec[i] != 0) ++i;
    if (i >= rec.size()) return Glyph();
    std::vector<unsigned char> body(rec.begin() + i + 1, rec.end());
    return run(body, depth);
}

Glyph Font::run(const std::vector<unsigned char>& code, int depth)
{
    Glyph g;
    if (depth > 4) return g;

    double x = 0.0, y = 0.0;
    double scale = 1.0;
    bool pen = false;
    std::vector<Pt> cur;
    std::vector<std::vector<Pt>> out;

    struct Saved { double x, y, scale; bool pen; };
    std::vector<Saved> stack;

    auto flush = [&]() {
        if (cur.size() >= 2) out.push_back(cur);
        cur.clear();
    };

    auto moveTo = [&](double nx, double ny, bool draw) {
        if (draw)
        {
            if (cur.empty()) AppendPt(cur, x, y);
            AppendPt(cur, nx, ny);
        }
        else
        {
            flush();
        }
        x = nx;
        y = ny;
    };

    // 弧：从 (cx,cy,r,a0) 起按 dir 走 |span|，步长 2°
    auto arc = [&](double cx, double cy, double r, double a0, double span, bool draw) {
        const int steps = std::max(2, static_cast<int>(std::ceil(std::fabs(span) / 2.0)));
        if (draw)
        {
            if (cur.empty()) AppendPt(cur, x, y);
        }
        else
        {
            flush();
        }
        for (int k = 0; k <= steps; ++k)
        {
            const double a = a0 + span * (static_cast<double>(k) / steps);
            const double px = cx + r * std::cos(a);
            const double py = cy + r * std::sin(a);
            AppendPt(cur, px, py);
        }
        x = cx + r * std::cos(a0 + span);
        y = cy + r * std::sin(a0 + span);
    };

    // 凸度弧：起点 (x,y)，位移 (dx,dy)，bulge = tan(theta/4)
    auto bulgeArc = [&](double dx, double dy, double bulge, bool draw) {
        const double len = std::hypot(dx, dy);
        if (len < 1e-9) { moveTo(x + dx, y + dy, draw); return; }
        const double theta = 4.0 * std::atan(bulge);
        Pt p0{x, y}, p1{x + dx, y + dy};
        // 弧中点：p0p1 中垂线上偏移 h = R(1-cos(theta/2))，方向由 bulge 符号决定
        const double r = std::fabs(theta) < 1e-6 ? 0.0 : len / (2.0 * std::sin(std::fabs(theta) * 0.5));
        const double nx = -dy / len, ny = dx / len;
        const double h = (r > 0.0) ? r * (1.0 - std::cos(std::fabs(theta) * 0.5)) : 0.0;
        const double sgn = bulge >= 0.0 ? 1.0 : -1.0;
        Pt mid{(p0.x + p1.x) * 0.5 + nx * h * sgn, (p0.y + p1.y) * 0.5 + ny * h * sgn};
        Pt c{};
        double rr = 0.0;
        if (!CircleFromArc(p0, p1, mid, c, rr))
        {
            moveTo(p1.x, p1.y, draw);
            return;
        }
        const double a0 = std::atan2(p0.y - c.y, p0.x - c.x);
        double a1 = std::atan2(p1.y - c.y, p1.x - c.x);
        double span = std::fabs(theta);
        if (sgn > 0.0)
        {
            while (a1 < a0) a1 += 6.283185307179586;
        }
        else
        {
            while (a1 > a0) a1 -= 6.283185307179586;
        }
        // 用 theta 决定实际跨度方向，避免走优弧
        span = sgn > 0.0 ? std::fabs(theta) : -std::fabs(theta);
        arc(c.x, c.y, rr, a0, span, draw);
    };

    size_t i = 0;
    while (i < code.size())
    {
        const unsigned char b = code[i];

        if (b < 0x10)
        {
            switch (b)
            {
            case 0x00:  // 结束
                flush();
                i = code.size();
                break;
            case 0x01:  // 落笔
                pen = true;
                ++i;
                break;
            case 0x02:  // 抬笔
                pen = false;
                flush();
                ++i;
                break;
            case 0x03:  // scale /= n
                if (i + 1 < code.size()) { scale /= static_cast<double>(code[i + 1]); i += 2; }
                else i = code.size();
                break;
            case 0x04:  // scale *= n
                if (i + 1 < code.size()) { scale *= static_cast<double>(code[i + 1]); i += 2; }
                else i = code.size();
                break;
            case 0x05:  // push
                stack.push_back(Saved{x, y, scale, pen});
                ++i;
                break;
            case 0x06:  // pop
                if (!stack.empty())
                {
                    const Saved s = stack.back();
                    stack.pop_back();
                    x = s.x; y = s.y; scale = s.scale; pen = s.pen;
                }
                ++i;
                break;
            case 0x07:  // 引用子字形（形索引），不推进笔
            {
                if (i + 1 >= code.size()) { i = code.size(); break; }
                const int shape = code[i + 1];
                i += 2;
                auto it = m_raw.find(shape);
                if (it != m_raw.end())
                {
                    const Glyph sub = decodeRecord(it->second, depth + 1);
                    for (const auto& poly : sub.polylines)
                    {
                        std::vector<Pt> moved;
                        moved.reserve(poly.size());
                        for (const Pt& p : poly) moved.push_back(Pt{x + p.x * scale, y + p.y * scale});
                        if (moved.size() >= 2) out.push_back(std::move(moved));
                    }
                }
                break;
            }
            case 0x08:  // 有符号位移 × scale
                if (i + 2 < code.size())
                {
                    const double dx = S8(code[i + 1]) * scale;
                    const double dy = S8(code[i + 2]) * scale;
                    i += 3;
                    moveTo(x + dx, y + dy, pen);
                }
                else i = code.size();
                break;
            case 0x09:  // 连续有符号位移，直到 (0,0)
            {
                i += 1;
                while (i + 2 <= code.size())
                {
                    const int ddx = S8(code[i]);
                    const int ddy = S8(code[i + 1]);
                    i += 2;
                    if (ddx == 0 && ddy == 0) break;
                    moveTo(x + ddx * scale, y + ddy * scale, pen);
                }
                break;
            }
            case 0x0A:  // 八分弧：r, s|c
                if (i + 2 < code.size())
                {
                    const double r = code[i + 1] * scale;
                    const unsigned char sc = code[i + 2];
                    const int start = (sc >> 4) & 0x07;
                    const int cnt = (sc & 0x07) ? (sc & 0x07) : 8;
                    const bool cw = (sc & 0x80) != 0;
                    i += 3;
                    const double a0 = start * 0.7853981633974483;
                    const double span = (cw ? -1.0 : 1.0) * cnt * 0.7853981633974483;
                    arc(x - r * std::cos(a0), y - r * std::sin(a0), r, a0, span, pen);
                }
                else i = code.size();
                break;
            case 0x0B:  // 分数弧：s_off, e_off, r_hi, r_lo, sc
                if (i + 4 < code.size())
                {
                    const int sOff = code[i + 1];
                    const int eOff = code[i + 2];
                    const double r = (code[i + 3] * 256 + code[i + 4]) * scale;
                    const unsigned char sc = code[i + 5];
                    const double a0 = sOff * 0.017453292519943295;
                    double a1 = eOff * 0.017453292519943295;
                    if (eOff < sOff) a1 += 6.283185307179586;
                    const bool cw = (sc & 0x80) != 0;
                    const double span = (cw ? -1.0 : 1.0) * std::fabs(a1 - a0);
                    i += 6;
                    arc(x - r * std::cos(a0), y - r * std::sin(a0), r, a0, span, pen);
                }
                else i = code.size();
                break;
            case 0x0C:  // 凸度弧：dx, dy, bulge
                if (i + 2 < code.size())
                {
                    const double dx = S8(code[i + 1]) * scale;
                    const double dy = S8(code[i + 2]) * scale;
                    const double bulge = S8(code[i + 3]) / 127.0;
                    i += 4;
                    bulgeArc(dx, dy, bulge, pen);
                }
                else i = code.size();
                break;
            case 0x0D:  // 多凸度弧：三字节一组，直到 (0,0)
                i += 1;
                while (i + 3 <= code.size())
                {
                    const int dx = S8(code[i]);
                    const int dy = S8(code[i + 1]);
                    const double bulge = S8(code[i + 2]) / 127.0;
                    i += 3;
                    if (dx == 0 && dy == 0) break;
                    bulgeArc(dx * scale, dy * scale, bulge, pen);
                }
                break;
            case 0x0E:  // 无参数 no-op（必须只跳 1 字节，否则整条指令流错位）
                ++i;
                break;
            default:
                ++i;
                break;
            }
            continue;
        }

        // 向量：高半字节长度，低半字节方向
        const int len = (b >> 4);
        const int dir = (b & 0x0F);
        const double vx = kDir[dir][0] * len * scale;
        const double vy = kDir[dir][1] * len * scale;
        ++i;
        moveTo(x + vx, y + vy, pen);
    }

    flush();
    g.polylines = std::move(out);

    // bbox
    bool first = true;
    for (const auto& poly : g.polylines)
    {
        for (const Pt& p : poly)
        {
            if (first)
            {
                g.xmin = g.xmax = p.x;
                g.ymin = g.ymax = p.y;
                first = false;
            }
            else
            {
                g.xmin = std::min(g.xmin, p.x);
                g.xmax = std::max(g.xmax, p.x);
                g.ymin = std::min(g.ymin, p.y);
                g.ymax = std::max(g.ymax, p.y);
            }
        }
    }
    return g;
}

// ---------------------------------------------------------------- Style
bool Style::setFonts(const std::wstring& shxPath, const std::wstring& bigFontPath)
{
    m_font = Font::Load(shxPath);
    if (!m_font || !m_font->valid()) return false;
    m_baseW = m_font->baseY();
    m_capW = m_font->capH();

    if (!bigFontPath.empty())
    {
        m_big = Font::Load(bigFontPath);
        if (m_big && m_big->valid())
        {
            m_baseC = m_big->baseY();
            m_capC = m_big->capH();
        }
        else
        {
            m_big.reset();
        }
    }
    return true;
}

bool Style::pick(wchar_t ch, const Glyph*& g, double& base, double& cap, bool& cjk) const
{
    int code = 0;
    bool isCjk = false;
    if (!ToShxCode(ch, code, isCjk)) return false;

    if (isCjk && m_big && m_big->valid() && m_big->has(code))
    {
        g = &m_big->glyph(code);
        base = m_baseC;
        cap = m_capC;
        cjk = true;
        return true;
    }
    if (m_font && m_font->has(code))
    {
        g = &m_font->glyph(code);
        base = m_baseW;
        cap = m_capW;
        cjk = false;
        return true;
    }
    // 中文没有大字体时退回西文字体的 '?'（0x3F）
    if (m_font && m_font->has(0x3F))
    {
        g = &m_font->glyph(0x3F);
        base = m_baseW;
        cap = m_capW;
        cjk = false;
        return true;
    }
    return false;
}

std::vector<std::vector<Pt>> Style::layout(const std::wstring& text, double height,
                                           double widthFactor, double obliqueDeg,
                                           double spacing) const
{
    std::vector<std::vector<Pt>> polys;
    if (height <= 0.0) height = 21.0;
    if (widthFactor <= 0.0) widthFactor = 1.0;

    const double tanObl = std::tan(obliqueDeg * 0.017453292519943295);
    double pen = 0.0;

    for (wchar_t ch : text)
    {
        if (ch == L' ')
        {
            pen += height * 0.5 + spacing;
            continue;
        }
        if (ch == L'\t')
        {
            pen += height * 1.5;
            continue;
        }

        const Glyph* g = nullptr;
        double base = 0.0, cap = 21.0;
        bool cjk = false;
        if (!pick(ch, g, base, cap, cjk) || g == nullptr) continue;
        if (g->empty()) continue;

        const double s = height / cap;
        const double xNorm = cjk ? 0.0 : (g->xmin * s);

        for (const auto& poly : g->polylines)
        {
            std::vector<Pt> outPoly;
            outPoly.reserve(poly.size());
            for (const Pt& p : poly)
            {
                const double yy = (p.y - base) * s;
                double xx = pen + (p.x * s - xNorm) * widthFactor;
                xx += yy * tanObl;
                outPoly.push_back(Pt{xx, yy});
            }
            if (outPoly.size() >= 2) polys.push_back(std::move(outPoly));
        }

        if (cjk)
        {
            pen += height * widthFactor + spacing;   // 全角 cell
        }
        else
        {
            const double adv = (g->xmax - g->xmin) * s * widthFactor;
            pen += adv + height * 0.15 + spacing;
        }
    }
    return polys;
}

std::vector<std::vector<Pt>> Style::layoutMultiline(const std::wstring& text, double height,
                                                    double widthFactor, double obliqueDeg,
                                                    double lineGap) const
{
    std::vector<std::vector<Pt>> all;
    std::wstring line;
    double yOff = 0.0;
    const double dy = height * (lineGap > 0.0 ? lineGap : 1.8);

    auto emit = [&](const std::wstring& ln) {
        auto polys = layout(ln, height, widthFactor, obliqueDeg);
        for (auto& poly : polys)
        {
            for (auto& p : poly) p.y += yOff;
            all.push_back(std::move(poly));
        }
    };

    for (wchar_t ch : text)
    {
        if (ch == L'\n' || ch == L'\r')
        {
            if (!line.empty()) { emit(line); line.clear(); }
            yOff -= dy;
            continue;
        }
        line.push_back(ch);
    }
    if (!line.empty()) emit(line);
    return all;
}

// ---------------------------------------------------------------- 输出
Box bounds(const std::vector<std::vector<Pt>>& polys)
{
    Box b;
    bool first = true;
    for (const auto& poly : polys)
    {
        for (const Pt& p : poly)
        {
            if (first)
            {
                b.x0 = b.x1 = p.x;
                b.y0 = b.y1 = p.y;
                first = false;
            }
            else
            {
                b.x0 = std::min(b.x0, p.x);
                b.x1 = std::max(b.x1, p.x);
                b.y0 = std::min(b.y0, p.y);
                b.y1 = std::max(b.y1, p.y);
            }
        }
    }
    return b;
}

std::wstring toXScene(const std::vector<std::vector<Pt>>& polys, int w, int h,
                      const std::wstring& stroke, double strokeWidth)
{
    std::wstring xml;
    if (polys.empty() || w <= 0 || h <= 0) return xml;

    const Box b = bounds(polys);
    const double bw = b.x1 - b.x0, bh = b.y1 - b.y0;
    const double pad = 2.0;
    const double fitX = (bw > 1e-9) ? (w - 2.0 * pad) / bw : 1.0;
    const double fitY = (bh > 1e-9) ? (h - 2.0 * pad) / bh : 1.0;
    const double s = std::min(fitX, fitY);
    const double ox = (w - bw * s) * 0.5;
    const double oy = (h - bh * s) * 0.5;

    wchar_t buf[64];
    xml += L"<XScene Width=\"";
    swprintf_s(buf, L"%d", w); xml += buf;
    xml += L"\" Height=\"";
    swprintf_s(buf, L"%d", h); xml += buf;
    xml += L"\"><XGroup>";

    for (const auto& poly : polys)
    {
        if (poly.size() < 2) continue;
        xml += L"<XPolyline Points=\"";
        bool firstPt = true;
        for (const Pt& p : poly)
        {
            const double sx = (p.x - b.x0) * s + ox;
            const double sy = (b.y1 - p.y) * s + oy;   // y 翻转：世界 y 向上 → 场景 y 向下
            if (!firstPt) xml += L' ';
            swprintf_s(buf, L"%.1f,%.1f", sx, sy);
            xml += buf;
            firstPt = false;
        }
        xml += L"\" Stroke=\"";
        xml += stroke;
        xml += L"\" Width=\"";
        swprintf_s(buf, L"%.2f", strokeWidth);
        xml += buf;
        xml += L"\"/>";
    }

    xml += L"</XGroup></XScene>";
    return xml;
}

} // namespace xmlshx
