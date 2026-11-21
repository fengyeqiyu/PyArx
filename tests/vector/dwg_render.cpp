// ===========================================================================
// dww_render.cpp —— DWG 的真正渲染管线（DWG → DXF → 自研矢量光栅，不走 SVG）
//
// 为什么是 DXF 中间层：DWG 是封闭二进制格式，版本之间有压缩与对象映射，
// 直接解码等于重写一个 CAD 内核；本机自带 AutoCAD 核心控制台（accoreconsole），
// 用它把 DWG 批量转成公开规范的 ASCII DXF，再由本程序做几何解析 + 自研光栅化。
// 渲染侧完全不依赖 SVG：几何进 xmlvec::Path，像素由 xmlvec::Raster 自己算。
//
// 支持实体：LINE / LWPOLYLINE / POLYLINE(+VERTEX) / CIRCLE / ARC / INSERT(块引用，
// 含均匀缩放与旋转) / POINT。文字暂不入图（TEXT/MTEXT 需要字体度量，属另一条通道）。
// ===========================================================================
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#include "XmlVector.h"

namespace
{

struct P2
{
    double x = 0.0;
    double y = 0.0;
};

struct Seg
{
    P2 a, b;
};

struct ArcSeg
{
    P2 c;
    double r = 0.0;
    double a0 = 0.0;   // 度
    double a1 = 0.0;   // 度（逆时针从 a0 到 a1）
};

struct Drawn
{
    std::vector<Seg> segs;
    std::vector<ArcSeg> arcs;
};

struct DxfPair
{
    int code = 0;
    std::string value;
};

bool readPairs(const std::string& path, std::vector<DxfPair>& out)
{
    FILE* f = fopen(path.c_str(), "rb");
    if (f == nullptr)
        return false;
    std::string line;
    char buf[1024];
    int pendingCode = -1;
    while (fgets(buf, sizeof(buf), f) != nullptr)
    {
        std::string s(buf);
        while (!s.empty() && (s.back() == '\n' || s.back() == '\r'))
            s.pop_back();
        // 去掉首尾空白（DXF 的 code 行常见前导空格）
        std::size_t b = s.find_first_not_of(" \t");
        std::size_t e = s.find_last_not_of(" \t");
        s = (b == std::string::npos) ? std::string() : s.substr(b, e - b + 1);
        if (pendingCode < 0)
        {
            if (s.empty())
                continue;
            pendingCode = std::atoi(s.c_str());
            continue;
        }
        DxfPair p;
        p.code = pendingCode;
        p.value = s;
        out.push_back(p);
        pendingCode = -1;
    }
    fclose(f);
    return true;
}

double num(const std::vector<DxfPair>& v, std::size_t& i, int code, double def)
{
    // 在实体范围内找第一个 code（找不到就保持游标不动）
    std::size_t j = i;
    while (j < v.size())
    {
        if (v[j].code == 0)
            break;
        if (v[j].code == code)
        {
            const double d = std::atof(v[j].value.c_str());
            i = j + 1;
            return d;
        }
        ++j;
    }
    return def;
}

struct Ent
{
    std::string type;
    std::size_t begin = 0;   // 指向 type 那条 (code 0)
    std::size_t end = 0;
};

std::string upper(std::string s)
{
    for (char& c : s)
        c = static_cast<char>(::toupper(static_cast<unsigned char>(c)));
    return s;
}

struct Block
{
    std::vector<Ent> ents;
    P2 base{0, 0};
};

// 从一段实体里抽点（10/20 为主，11/21 为次）
P2 point(const std::vector<DxfPair>& v, std::size_t b, std::size_t e, int cx, int cy, P2 def)
{
    P2 p = def;
    for (std::size_t i = b; i < e; ++i)
    {
        if (v[i].code == cx)
            p.x = std::atof(v[i].value.c_str());
        else if (v[i].code == cy)
            p.y = std::atof(v[i].value.c_str());
    }
    return p;
}

double value1(const std::vector<DxfPair>& v, std::size_t b, std::size_t e, int code, double def)
{
    for (std::size_t i = b; i < e; ++i)
        if (v[i].code == code)
            return std::atof(v[i].value.c_str());
    return def;
}

void addArcApprox(Drawn& d, const P2& c, double r, double a0, double a1)
{
    // 长弧直接进光栅器（由 Raster 的贝塞尔/扫描线处理），这里换算成折线段即可；
    // 段数按弧长自适应（每 6° 一段，至少 4 段）
    double sweep = a1 - a0;
    if (sweep <= 0)
        sweep += 360.0;
    int n = static_cast<int>(std::ceil(sweep / 6.0));
    if (n < 4) n = 4;
    P2 prev{c.x + r * std::cos(a0 * 3.14159265358979323846 / 180.0),
            c.y + r * std::sin(a0 * 3.14159265358979323846 / 180.0)};
    for (int i = 1; i <= n; ++i)
    {
        const double t = a0 + sweep * i / n;
        P2 cur{c.x + r * std::cos(t * 3.14159265358979323846 / 180.0),
               c.y + r * std::sin(t * 3.14159265358979323846 / 180.0)};
        d.segs.push_back(Seg{prev, cur});
        prev = cur;
    }
}

// 把一段实体解析成线段（块引用递归一层，含插入点/比例/旋转）
void parseEntity(const std::vector<DxfPair>& v, const Ent& en, Drawn& out,
                 const std::map<std::string, Block>& blocks, const P2& ins, double sx,
                 double sy, double rot, int depth)
{
    const std::string type = en.type;
    const std::size_t b = en.begin, e = en.end;

    auto mapPt = [&](const P2& w) -> P2 {
        // 世界 → 当前块实例坐标：先按块基点平移、再缩放、再绕插入点旋转、再平移到插入点
        double x = (w.x) * sx;
        double y = (w.y) * sy;
        const double c = std::cos(rot * 3.14159265358979323846 / 180.0);
        const double s = std::sin(rot * 3.14159265358979323846 / 180.0);
        return P2{ins.x + x * c - y * s, ins.y + x * s + y * c};
    };

    if (type == "LINE")
    {
        const P2 a = mapPt(point(v, b, e, 10, 20, P2{}));
        const P2 c = mapPt(point(v, b, e, 11, 21, P2{}));
        out.segs.push_back(Seg{a, c});
    }
    else if (type == "CIRCLE")
    {
        const P2 c = point(v, b, e, 10, 20, P2{});
        const double r = value1(v, b, e, 40, 0.0);
        addArcApprox(out, mapPt(c), r * ((std::fabs)(sx) + (std::fabs)(sy)) * 0.5, 0.0, 360.0);
    }
    else if (type == "ARC")
    {
        const P2 c = point(v, b, e, 10, 20, P2{});
        const double r = value1(v, b, e, 40, 0.0);
        const double a0 = value1(v, b, e, 50, 0.0);
        const double a1 = value1(v, b, e, 51, 360.0);
        addArcApprox(out, mapPt(c), r * ((std::fabs)(sx) + (std::fabs)(sy)) * 0.5, a0, a1);
    }
    else if (type == "LWPOLYLINE")
    {
        std::vector<P2> pts;
        double x = 0.0, y = 0.0;
        for (std::size_t i = b; i < e; ++i)
        {
            if (v[i].code == 10)
                x = std::atof(v[i].value.c_str());
            else if (v[i].code == 20)
            {
                y = std::atof(v[i].value.c_str());
                pts.push_back(P2{x, y});
            }
        }
        // 闭合标志：code 70 的 bit0
        const bool closed = (static_cast<int>(value1(v, b, e, 70, 0.0)) & 1) != 0;
        for (std::size_t i = 0; i + 1 < pts.size(); ++i)
            out.segs.push_back(Seg{mapPt(pts[i]), mapPt(pts[i + 1])});
        if (closed && pts.size() > 2)
            out.segs.push_back(Seg{mapPt(pts.back()), mapPt(pts.front())});
    }
    else if (type == "POLYLINE")
    {
        // 老式多段线：顶点在后续 VERTEX 实体里，parseEntities 已经把 VERTEX 单独列出
        // 这里只处理标志位，实际线段由 VERTEX 顺序拼（见 parseEntities 的收尾逻辑）
    }
    else if (type == "INSERT" && depth < 3)
    {
        const P2 at = point(v, b, e, 10, 20, P2{});
        const double scx = value1(v, b, e, 41, 1.0);
        const double scy = value1(v, b, e, 42, 1.0);
        const double ang = value1(v, b, e, 50, 0.0);
        std::string name;
        for (std::size_t i = b; i < e; ++i)
            if (v[i].code == 2)
            {
                name = v[i].value;
                break;
            }
        auto it = blocks.find(name);
        if (it == blocks.end())
            return;
        // 块内实体先按"块自身坐标"解析（不施加任何变换），再由这里一次性
        // 套 (基点修正 → 比例 → 旋转 → 插入点平移)。分两步是为了避免双重变换。
        Drawn sub;
        for (const Ent& be : it->second.ents)
            parseEntity(v, be, sub, blocks, P2{0, 0}, 1.0, 1.0, 0.0, depth + 1);
        const double c = std::cos(rot * 3.14159265358979323846 / 180.0);
        const double sn = std::sin(rot * 3.14159265358979323846 / 180.0);
        const P2 base = it->second.base;
        auto put = [&](P2 p) -> P2 {
            const double bx = (p.x - base.x) * scx;
            const double by = (p.y - base.y) * scy;
            const double rx = bx * c - by * sn;
            const double ry = bx * sn + by * c;
            return P2{ins.x + at.x + rx, ins.y + at.y + ry};
        };
        for (const Seg& sg : sub.segs)
            out.segs.push_back(Seg{put(sg.a), put(sg.b)});
    }
    else if (type == "VERTEX")
    {
        const P2 p = mapPt(point(v, b, e, 10, 20, P2{}));
        (void)p;
    }
}

void parseEntities(const std::vector<DxfPair>& v, std::size_t b, std::size_t e,
                   const std::map<std::string, Block>& blocks, Drawn& out)
{
    // 先把实体切成 [code0 .. 下一个 code0) 的段
    std::vector<Ent> ents;
    std::size_t i = b;
    while (i < e)
    {
        if (v[i].code == 0)
        {
            Ent en;
            en.type = upper(v[i].value);
            en.begin = i + 1;
            std::size_t j = i + 1;
            while (j < e && v[j].code != 0)
                ++j;
            en.end = j;
            ents.push_back(en);
            i = j;
        }
        else
            ++i;
    }
    // POLYLINE 与紧随的 VERTEX 拼成折线
    for (std::size_t k = 0; k < ents.size(); ++k)
    {
        if (ents[k].type == "POLYLINE")
        {
            const bool closed = (static_cast<int>(value1(v, ents[k].begin, ents[k].end, 70, 0.0)) & 1) != 0;
            std::vector<P2> pts;
            std::size_t m = k + 1;
            while (m < ents.size() && ents[m].type == "VERTEX")
            {
                pts.push_back(point(v, ents[m].begin, ents[m].end, 10, 20, P2{}));
                ++m;
            }
            for (std::size_t q = 0; q + 1 < pts.size(); ++q)
                out.segs.push_back(Seg{pts[q], pts[q + 1]});
            if (closed && pts.size() > 2)
                out.segs.push_back(Seg{pts.back(), pts.front()});
            k = m - 1;
        }
    }
    for (const Ent& en : ents)
    {
        if (en.type == "POLYLINE" || en.type == "VERTEX" || en.type == "SEQEND")
            continue;
        parseEntity(v, en, out, blocks, P2{0, 0}, 1.0, 1.0, 0.0, 0);
    }
}

} // namespace

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        std::fprintf(stderr, "usage: dwg_render <in.dxf> <out.bmp> [width] [height]\n");
        return 2;
    }
    std::vector<DxfPair> pairs;
    if (!readPairs(argv[1], pairs))
    {
        std::fprintf(stderr, "cannot open %s\n", argv[1]);
        return 2;
    }

    // 切 SECTION：ENTITIES 与 BLOCKS
    std::map<std::string, Block> blocks;
    struct Section
    {
        std::size_t b = 0, e = 0;
    };
    Section entSec, blkSec;
    for (std::size_t i = 0; i + 1 < pairs.size(); ++i)
    {
        if (pairs[i].code == 0 && upper(pairs[i].value) == "SECTION")
        {
            std::string name;
            if (pairs[i + 1].code == 2)
                name = upper(pairs[i + 1].value);
            std::size_t j = i + 1;
            while (j < pairs.size() && !(pairs[j].code == 0 && upper(pairs[j].value) == "ENDSEC"))
                ++j;
            if (name == "ENTITIES")
            {
                entSec.b = i + 1;
                entSec.e = j;
            }
            else if (name == "BLOCKS")
            {
                blkSec.b = i + 1;
                blkSec.e = j;
            }
            i = j;
        }
    }

    // 块表
    if (blkSec.e > blkSec.b)
    {
        std::size_t i = blkSec.b;
        while (i < blkSec.e)
        {
            if (pairs[i].code == 0 && upper(pairs[i].value) == "BLOCK")
            {
                std::string name;
                std::size_t j = i + 1;
                P2 base{0, 0};
                while (j < blkSec.e && pairs[j].code != 0)
                {
                    if (pairs[j].code == 2)
                        name = pairs[j].value;
                    else if (pairs[j].code == 10)
                        base.x = std::atof(pairs[j].value.c_str());
                    else if (pairs[j].code == 20)
                        base.y = std::atof(pairs[j].value.c_str());
                    ++j;
                }
                std::size_t k = j;
                while (k < blkSec.e && !(pairs[k].code == 0 && upper(pairs[k].value) == "ENDBLK"))
                    ++k;
                Block blk;
                blk.base = base;
                // 块内实体段：从 j 到 k
                std::size_t s = j;
                while (s < k)
                {
                    if (pairs[s].code == 0 && upper(pairs[s].value) != "ENDBLK")
                    {
                        Ent en;
                        en.type = upper(pairs[s].value);
                        en.begin = s + 1;
                        std::size_t t = s + 1;
                        while (t < k && pairs[t].code != 0)
                            ++t;
                        en.end = t;
                        blk.ents.push_back(en);
                        s = t;
                    }
                    else
                        ++s;
                }
                if (!name.empty())
                    blocks[name] = blk;
                i = k + 1;
            }
            else
                ++i;
        }
    }

    Drawn d;
    if (entSec.e > entSec.b)
        parseEntities(pairs, entSec.b, entSec.e, blocks, d);

    if (d.segs.empty())
    {
        std::fprintf(stderr, "no geometry parsed\n");
        return 3;
    }

    // 自适应视图：等比铺满、留 6% 边距，y 轴翻转（DXF 是数学坐标，屏幕 y 向下）
    double l = 1e300, t = 1e300, r = -1e300, bo = -1e300;
    for (const Seg& s : d.segs)
    {
        l = (std::min)(l, (std::min)(s.a.x, s.b.x));
        r = (std::max)(r, (std::max)(s.a.x, s.b.x));
        t = (std::min)(t, (std::min)(s.a.y, s.b.y));
        bo = (std::max)(bo, (std::max)(s.a.y, s.b.y));
    }
    const int W = (argc > 3) ? std::atoi(argv[3]) : 1200;
    const int H = (argc > 4) ? std::atoi(argv[4]) : 900;
    const double vw = (std::max)(r - l, 1e-6);
    const double vh = (std::max)(bo - t, 1e-6);
    const double sc = (std::min)(W * 0.88 / vw, H * 0.88 / vh);
    const double ox = (W - vw * sc) * 0.5 - l * sc;
    const double oy = (H - vh * sc) * 0.5 + bo * sc;

    xmlvec::Raster raster(W, H, 0xFFFFFFFFu);
    raster.enablePicking(true);   // 渲染顺便记覆盖率 + 段 id，查询时不做任何几何反算
    xmlvec::Stroke st;
    st.enabled = true;
    st.width = 1.4f;
    st.color = 0xFF1A1A1A;
    st.cap = 1;
    st.join = 2;

    std::uint32_t segId = 0;
    for (const Seg& s : d.segs)
    {
        xmlvec::Path p;
        p.moveTo(static_cast<float>(s.a.x * sc + ox), static_cast<float>(-s.a.y * sc + oy));
        p.lineTo(static_cast<float>(s.b.x * sc + ox), static_cast<float>(-s.b.y * sc + oy));
        raster.strokePath(p, st, nullptr, ++segId);   // id = 段序号，后画覆盖先画
    }

    if (!raster.saveBmp(std::wstring(argv[2], argv[2] + std::strlen(argv[2]))))
    {
        std::fprintf(stderr, "cannot write %s\n", argv[2]);
        return 4;
    }
    std::printf("OK segs=%zu arcs=%zu size=%dx%d scale=%.3f\n", d.segs.size(), d.arcs.size(), W, H,
                sc);

    // 可选：--pick x y —— 直接问"这个像素有没有被画到"，不跑任何几何运算
    if (argc >= 7 && std::strcmp(argv[5], "--pick") == 0)
    {
        const int px = std::atoi(argv[6]);
        const int py = (argc >= 8) ? std::atoi(argv[7]) : -1;
        if (px < 0 || py < 0)
        {
            // --pick -1：全图统计，并给出若干命中示例
            std::size_t painted = 0, typed = 0;
            int shown = 0;
            for (int y = 0; y < H; ++y)
                for (int x = 0; x < W; ++x)
                {
                    if (raster.paintedAt(x, y))
                    {
                        ++painted;
                        if (raster.idAt(x, y))
                            ++typed;
                        if (shown < 6)
                        {
                            std::printf("HIT (%d,%d) coverage=%d seg=%u\n", x, y,
                                        raster.coverageAt(x, y), raster.idAt(x, y));
                            ++shown;
                        }
                    }
                }
            std::printf("SCAN painted=%zu/%d typed=%zu\n", painted, W * H, typed);
        }
        else
        {
            const int cov = raster.coverageAt(px, py);
            const std::uint32_t hit = raster.idAt(px, py);
            std::printf("PICK (%d,%d) painted=%d coverage=%d/%d seg=%u\n", px, py, cov > 0 ? 1 : 0,
                        cov, 255, hit);
            if (hit > 0 && hit <= d.segs.size())
                std::printf("SEG %u: (%.2f,%.2f)-(%.2f,%.2f)\n", hit, d.segs[hit - 1].a.x,
                            d.segs[hit - 1].a.y, d.segs[hit - 1].b.x, d.segs[hit - 1].b.y);
        }
    }
    return 0;
}
