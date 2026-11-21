// ===========================================================================
// XmlVector.h —— 自研矢量绘制库（不依赖 SVG / 不依赖 Direct2D 几何管线）
//
// 设计口径：
//   * 场景模型是「路径 + 填充 + 描边」，不是 SVG 文档。SVG 只是其中一种输入格式，
//     本库不认识 SVG；谁要画图就自己构造 Path（moveTo/lineTo/quadTo/cubicTo/close）。
//   * 光栅化全部自己算：折线化 → 扫描线求交（nonzero / even-odd）→ 水平精确覆盖率
//     + 垂直 N 倍超采样 → src-over 混合。描边先扩成多边形（round join / cap）再填充。
//   * 变换复用 XmlMatrix4（4×4，涵盖所有 2D/3D 变换），不另起一套矩阵。
//   * 坐标与屏幕一致（左上原点、y 向下、单位像素）；颜色统一 0xAARRGGBB。
//
// 本库是"能用、能对账、能扩展"的第一版：填充/描边/抗锯齿/线性渐变/矩阵变换/裁剪盒。
// 未做的（明确留空，不假装支持）：径向渐变、虚线、混合模式、路径布尔运算。
// ===========================================================================
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

#include "XmlMatrix4.h"

namespace xmlvec
{

struct Vec2
{
    float x = 0.0f;
    float y = 0.0f;
};

struct RectF
{
    float l = 0.0f;
    float t = 0.0f;
    float r = 0.0f;
    float b = 0.0f;
    float width() const { return r - l; }
    float height() const { return b - t; }
    bool empty() const { return r <= l || b <= t; }
};

enum class PathCmd : std::uint8_t
{
    kMove = 0,     // 1 点
    kLine = 1,     // 1 点
    kQuad = 2,     // 2 点（控制点、终点）
    kCubic = 3,    // 3 点（控制点 1、控制点 2、终点）
    kClose = 4,    // 0 点
};

// ---------------------------------------------------------------------------
// 路径：只存命令 + 坐标，不做任何格式相关的事
// ---------------------------------------------------------------------------
class Path
{
public:
    std::vector<PathCmd> cmds;
    std::vector<Vec2> pts;

    void moveTo(float x, float y)
    {
        cmds.push_back(PathCmd::kMove);
        pts.push_back(Vec2{x, y});
    }
    void lineTo(float x, float y)
    {
        cmds.push_back(PathCmd::kLine);
        pts.push_back(Vec2{x, y});
    }
    void quadTo(float cx, float cy, float x, float y)
    {
        cmds.push_back(PathCmd::kQuad);
        pts.push_back(Vec2{cx, cy});
        pts.push_back(Vec2{x, y});
    }
    void cubicTo(float c1x, float c1y, float c2x, float c2y, float x, float y)
    {
        cmds.push_back(PathCmd::kCubic);
        pts.push_back(Vec2{c1x, c1y});
        pts.push_back(Vec2{c2x, c2y});
        pts.push_back(Vec2{x, y});
    }
    void close()
    {
        cmds.push_back(PathCmd::kClose);
    }

    // 常用图元糖：矩形 / 椭圆 / 圆角矩形 / 折线 / 圆环（描边用）
    void addRect(float x, float y, float w, float h)
    {
        moveTo(x, y);
        lineTo(x + w, y);
        lineTo(x + w, y + h);
        lineTo(x, y + h);
        close();
    }
    void addEllipse(float cx, float cy, float rx, float ry)
    {
        // 四段三次贝塞尔的整圆逼近（k = 0.5522847498）
        const float kx = rx * 0.5522847498f;
        const float ky = ry * 0.5522847498f;
        moveTo(cx + rx, cy);
        cubicTo(cx + rx, cy + ky, cx + kx, cy + ry, cx, cy + ry);
        cubicTo(cx - kx, cy + ry, cx - rx, cy + ky, cx - rx, cy);
        cubicTo(cx - rx, cy - ky, cx - kx, cy - ry, cx, cy - ry);
        cubicTo(cx + kx, cy - ry, cx + rx, cy - ky, cx + rx, cy);
        close();
    }
    void addCircle(float cx, float cy, float r) { addEllipse(cx, cy, r, r); }

    bool empty() const { return cmds.empty(); }

    RectF bounds() const
    {
        RectF r{1e30f, 1e30f, -1e30f, -1e30f};
        if (pts.empty())
            return RectF{0, 0, 0, 0};
        for (const Vec2& p : pts)
        {
            r.l = (std::min)(r.l, p.x);
            r.t = (std::min)(r.t, p.y);
            r.r = (std::max)(r.r, p.x);
            r.b = (std::max)(r.b, p.y);
        }
        return r;
    }
};

struct Polyline
{
    std::vector<Vec2> pts;
    bool closed = false;
};

// 贝塞尔细分量：按控制多边形长度与容差自适应（tol 是"拍平误差"，单位像素）
inline int SegmentCount(float len, float tol)
{
    if (len <= 0.0f)
        return 1;
    const float n = std::sqrt(len / (std::max)(tol, 0.05f));
    int c = static_cast<int>(std::ceil(n));
    if (c < 2) c = 2;
    if (c > 64) c = 64;
    return c;
}

inline float Dist(const Vec2& a, const Vec2& b)
{
    const float dx = b.x - a.x, dy = b.y - a.y;
    return std::sqrt(dx * dx + dy * dy);
}

// 把路径拍平成一组折线（三次/二次贝塞尔按容差细分）
inline std::vector<Polyline> FlattenPath(const Path& path, float tol = 0.3f)
{
    std::vector<Polyline> out;
    Polyline cur;
    auto flush = [&]() {
        if (cur.pts.size() >= 2)
            out.push_back(cur);
        cur.pts.clear();
        cur.closed = false;
    };

    Vec2 last{0, 0};
    Vec2 start{0, 0};
    bool hasStart = false;
    std::size_t i = 0;
    for (PathCmd c : path.cmds)
    {
        switch (c)
        {
        case PathCmd::kMove:
        {
            flush();
            last = path.pts[i++];
            start = last;
            hasStart = true;
            cur.pts.push_back(last);
            break;
        }
        case PathCmd::kLine:
        {
            last = path.pts[i++];
            if (cur.pts.empty())
                cur.pts.push_back(last);
            else
                cur.pts.push_back(last);
            break;
        }
        case PathCmd::kQuad:
        {
            const Vec2 c1 = path.pts[i++];
            const Vec2 p1 = path.pts[i++];
            const float len = Dist(last, c1) + Dist(c1, p1);
            const int n = SegmentCount(len, tol);
            for (int s = 1; s <= n; ++s)
            {
                const float t = static_cast<float>(s) / n;
                const float mt = 1.0f - t;
                const float bx = mt * mt * last.x + 2 * mt * t * c1.x + t * t * p1.x;
                const float by = mt * mt * last.y + 2 * mt * t * c1.y + t * t * p1.y;
                cur.pts.push_back(Vec2{bx, by});
            }
            last = p1;
            break;
        }
        case PathCmd::kCubic:
        {
            const Vec2 c1 = path.pts[i++];
            const Vec2 c2 = path.pts[i++];
            const Vec2 p1 = path.pts[i++];
            const float len = Dist(last, c1) + Dist(c1, c2) + Dist(c2, p1);
            const int n = SegmentCount(len, tol);
            for (int s = 1; s <= n; ++s)
            {
                const float t = static_cast<float>(s) / n;
                const float mt = 1.0f - t;
                const float b0 = mt * mt * mt;
                const float b1 = 3 * mt * mt * t;
                const float b2 = 3 * mt * t * t;
                const float b3 = t * t * t;
                cur.pts.push_back(Vec2{b0 * last.x + b1 * c1.x + b2 * c2.x + b3 * p1.x,
                                       b0 * last.y + b1 * c1.y + b2 * c2.y + b3 * p1.y});
            }
            last = p1;
            break;
        }
        case PathCmd::kClose:
        {
            if (hasStart && !cur.pts.empty())
            {
                if (Dist(cur.pts.front(), start) > 0.01f)
                    cur.pts.push_back(start);
                cur.closed = true;
            }
            flush();
            last = start;
            break;
        }
        }
    }
    flush();
    return out;
}

// ---------------------------------------------------------------------------
// 样式：填充 / 描边（线性渐变内建，其余渐变留空）
// ---------------------------------------------------------------------------
struct Paint
{
    bool enabled = true;
    std::uint32_t color = 0xFF000000u;     // ARGB
    int rule = 0;                          // 0 = nonzero，1 = even-odd
    bool gradient = false;
    Vec2 g0{0, 0};
    Vec2 g1{1, 0};
    std::uint32_t color0 = 0xFF000000u;
    std::uint32_t color1 = 0xFFFFFFFFu;
};

struct Stroke
{
    bool enabled = false;
    std::uint32_t color = 0xFF000000u;
    float width = 1.0f;
    int cap = 1;      // 0 = butt，1 = round，2 = square
    int join = 2;     // 0 = miter，1 = bevel，2 = round
    float miterLimit = 4.0f;
    std::vector<float> dash;   // 预留：空 = 实线
};

struct Shape
{
    Path path;
    Paint fill;
    Stroke stroke;
    // 拾取用的元素标识（0 = 匿名）。渲染时顺手写进 ID 缓冲，
    // 之后查"这个像素是哪个元素画的"，不需要再做任何几何反算。
    std::uint32_t id = 0;
};

struct Scene
{
    int width = 0;
    int height = 0;
    std::uint32_t clear = 0xFFFFFFFFu;
    std::vector<Shape> shapes;
};

// ---------------------------------------------------------------------------
// 光栅器：纯软件，不碰 D2D / GDI
// ---------------------------------------------------------------------------
class Raster
{
public:
    Raster(int w, int h, std::uint32_t clearColor = 0xFFFFFFFFu)
        : m_w((std::max)(1, w)), m_h((std::max)(1, h)),
          m_px(static_cast<std::size_t>((std::max)(1, w)) * static_cast<std::size_t>((std::max)(1, h)),
               clearColor)
    {
        m_cov.assign(static_cast<std::size_t>(m_w), 0.0f);
    }

    int width() const { return m_w; }
    int height() const { return m_h; }
    std::uint32_t* data() { return m_px.data(); }
    const std::uint32_t* data() const { return m_px.data(); }
    std::uint32_t pixel(int x, int y) const
    {
        if (x < 0 || y < 0 || x >= m_w || y >= m_h)
            return 0;
        return m_px[static_cast<std::size_t>(y) * m_w + x];
    }

    void clear(std::uint32_t c) { std::fill(m_px.begin(), m_px.end(), c); }

    // -------------------------------------------------------------------
    // 拾取（picking）：渲染时顺手记录，不额外做点-多边形判定。
    //   coverage = 该像素被"任何图元"画到的累计覆盖率（0..255，饱和）
    //   id       = 最后画到该像素的元素 id（后画覆盖先画 = 屏幕最终可见语义）
    // 打开记录才会分配缓冲；不开就是原来的纯像素路径，零额外开销。
    // -------------------------------------------------------------------
    void enablePicking(bool on = true)
    {
        m_record = on;
        if (!on)
            return;
        if (m_mask.size() != m_px.size())
            m_mask.assign(m_px.size(), 0);
        if (m_ids.size() != m_px.size())
            m_ids.assign(m_px.size(), 0u);
        if (m_center.size() != m_px.size())
            m_center.assign(m_px.size(), 0);
    }

    void clearPicking()
    {
        std::fill(m_mask.begin(), m_mask.end(), 0);
        std::fill(m_ids.begin(), m_ids.end(), 0u);
        std::fill(m_center.begin(), m_center.end(), 0);
    }

    bool pickingEnabled() const { return m_record; }

    // 外部位图输入：把一张已有图的灰度/墨迹量直接灌进覆盖率场。
    // 这样"对一张位图求轮廓"完全不需要重画几何，也不需要知道它原来是圆还是方。
    void setCoverageAt(int x, int y, std::uint8_t v)
    {
        if (!m_record || x < 0 || y < 0 || x >= m_w || y >= m_h)
            return;
        m_mask[static_cast<std::size_t>(y) * m_w + x] = v;
    }

    // 覆盖率二值化到中心命中位图（位图输入没有真正的"中心采样"，这就是"算不算墨迹"的约定）
    void thresholdToCenter(std::uint8_t thr = 128)
    {
        if (!m_record)
            return;
        for (std::size_t i = 0; i < m_mask.size(); ++i)
            m_center[i] = m_mask[i] >= thr ? 1 : 0;
    }

    std::uint8_t coverageAt(int x, int y) const
    {
        if (!m_record || x < 0 || y < 0 || x >= m_w || y >= m_h)
            return 0;
        return m_mask[static_cast<std::size_t>(y) * m_w + x];
    }

    std::uint32_t idAt(int x, int y) const
    {
        if (!m_record || x < 0 || y < 0 || x >= m_w || y >= m_h)
            return 0;
        return m_ids[static_cast<std::size_t>(y) * m_w + x];
    }

    // 这个像素到底有没有被画过（默认要求覆盖率 ≥ 1/255，即至少一点点）
    bool paintedAt(int x, int y, std::uint8_t minCover = 1) const
    {
        return coverageAt(x, y) >= minCover;
    }

    // 把等值线线段拼成闭合环（矢量视图要的就是这个：一串 double 顶点，不是一个位图）
    std::vector<Polyline> contourLoops(float level = 0.5f) const
    {
        std::vector<Polyline> segs = contourAt(level);
        std::vector<Polyline> loops;
        if (segs.empty())
            return loops;
        auto bucketKey = [](float v) -> std::int64_t {
            return static_cast<std::int64_t>(std::floor(static_cast<double>(v) * 0.25));   // 4 格/px
        };
        auto keyOf = [&](const Vec2& p) -> std::uint64_t {
            const std::int64_t bx = bucketKey(p.x) + (1 << 20);
            const std::int64_t by = bucketKey(p.y) + (1 << 20);
            return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(bx)) << 32) |
                   static_cast<std::uint64_t>(static_cast<std::uint32_t>(by));
        };
        // 端点装桶：同一点由相邻单元各自插值得到，浮点上可能差 1e-6~1e-3，
        // 所以不能用"精确相同 key"配对，要用容差近邻（3×3 桶 + 0.06px 半径）。
        std::unordered_map<std::uint64_t, std::vector<int>> ends;
        ends.reserve(segs.size() * 2);
        for (std::size_t i = 0; i < segs.size(); ++i)
        {
            ends[keyOf(segs[i].pts[0])].push_back(static_cast<int>(i) * 2 + 0);
            ends[keyOf(segs[i].pts[1])].push_back(static_cast<int>(i) * 2 + 1);
        }
        std::vector<char> used(segs.size(), 0);
        auto findNext = [&](const Vec2& tail) -> int {
            const std::int64_t bx = bucketKey(tail.x), by = bucketKey(tail.y);
            int bestSeg = -1;
            float bestD = 0.06f;
            for (std::int64_t dy = -1; dy <= 1; ++dy)
                for (std::int64_t dx = -1; dx <= 1; ++dx)
                {
                    const std::uint64_t k =
                        (static_cast<std::uint64_t>(static_cast<std::uint32_t>(bx + dx + (1 << 20)))
                         << 32) |
                        static_cast<std::uint64_t>(static_cast<std::uint32_t>(by + dy + (1 << 20)));
                    auto it = ends.find(k);
                    if (it == ends.end())
                        continue;
                    for (int e : it->second)
                    {
                        const int si = e / 2;
                        if (used[static_cast<std::size_t>(si)])
                            continue;
                        const Vec2 p = segs[static_cast<std::size_t>(si)].pts[e & 1];
                        const float d = std::fabs(p.x - tail.x) + std::fabs(p.y - tail.y);
                        if (d < bestD)
                        {
                            bestD = d;
                            bestSeg = si;
                        }
                    }
                }
            return bestSeg;
        };
        for (std::size_t i = 0; i < segs.size(); ++i)
        {
            if (used[i])
                continue;
            used[i] = 1;
            Polyline loop;
            loop.closed = true;
            loop.pts.push_back(segs[i].pts[0]);
            loop.pts.push_back(segs[i].pts[1]);
            while (true)
            {
                const Vec2 tail = loop.pts.back();
                const int js = findNext(tail);
                if (js < 0)
                    break;
                used[static_cast<std::size_t>(js)] = 1;
                const Vec2 a = segs[static_cast<std::size_t>(js)].pts[0];
                const Vec2 b = segs[static_cast<std::size_t>(js)].pts[1];
                loop.pts.push_back((std::fabs(a.x - tail.x) + std::fabs(a.y - tail.y) <=
                                    std::fabs(b.x - tail.x) + std::fabs(b.y - tail.y))
                                       ? b
                                       : a);
            }
            // 收尾：等值线本来闭合，走回起点附近时因为启程段已被使用而停住，
            // 末段端点距起点不足一段长（<2.5px）就直接吸附成闭合环。
            if (loop.pts.size() > 3 && Dist(loop.pts.front(), loop.pts.back()) < 2.5f)
            {
                loop.pts.back() = loop.pts.front();   // 吸附成闭环（首尾同点）
                loop.closed = true;
            }
            else
                loop.closed = false;
            if (loop.pts.size() >= 2)
                loops.push_back(loop);
        }
        return loops;
    }

    // 像素边界边集合：每个墨迹像素与空邻居之间的那条格边（1 单位长，整数端点）。
    // 这是"像素法轮廓"最原始也最可靠的表示：不断链、不插值、可逐条核对；
    // 需要闭合环时再按端点串接，串接失败也不影响这些边本身的正确性。
    std::vector<std::pair<Vec2, Vec2>> pixelEdges() const
    {
        std::vector<std::pair<Vec2, Vec2>> out;
        if (!m_record)
            return out;
        for (int y = 0; y < m_h; ++y)
            for (int x = 0; x < m_w; ++x)
            {
                if (!centerHitAt(x, y))
                    continue;
                const Vec2 a{static_cast<float>(x), static_cast<float>(y)};
                const Vec2 b{static_cast<float>(x + 1), static_cast<float>(y)};
                const Vec2 c{static_cast<float>(x + 1), static_cast<float>(y + 1)};
                const Vec2 d{static_cast<float>(x), static_cast<float>(y + 1)};
                if (!centerHitAt(x, y - 1))
                    out.push_back({a, b});   // 上
                if (!centerHitAt(x + 1, y))
                    out.push_back({b, c});   // 右
                if (!centerHitAt(x, y + 1))
                    out.push_back({c, d});   // 下
                if (!centerHitAt(x - 1, y))
                    out.push_back({d, a});   // 左
            }
        return out;
    }

    // 像素边界环：二值位图的轮廓标准解法。
    // 把每个墨迹像素与空邻居之间的那条"格边"收进来，串成顺时针闭环；
    // 顶点是整数格点，闭合精确、周长正确，不需要任何浮点插值。
    // 鞍点（对角相接的两个墨迹像素）按右手规则选下一条，保证不交叉。
    std::vector<Polyline> pixelLoops() const
    {
        std::vector<Polyline> out;
        if (!m_record)
            return out;
        struct E
        {
            int x0, y0, x1, y1;
        };
        std::vector<E> es;
        es.reserve(static_cast<std::size_t>(m_w) * m_h);
        for (int y = 0; y < m_h; ++y)
            for (int x = 0; x < m_w; ++x)
            {
                if (!centerHitAt(x, y))
                    continue;
                if (!centerHitAt(x, y - 1))
                    es.push_back(E{x, y, x + 1, y});           // 上边 →
                if (!centerHitAt(x + 1, y))
                    es.push_back(E{x + 1, y, x + 1, y + 1});   // 右边 ↓
                if (!centerHitAt(x, y + 1))
                    es.push_back(E{x + 1, y + 1, x, y + 1});   // 下边 ←
                if (!centerHitAt(x - 1, y))
                    es.push_back(E{x, y + 1, x, y});           // 左边 ↑
            }
        if (es.empty())
            return out;
        auto keyOf = [](int x, int y) -> std::uint64_t {
            return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(x + (1 << 20))) << 32) |
                   static_cast<std::uint64_t>(static_cast<std::uint32_t>(y + (1 << 20)));
        };
        std::unordered_map<std::uint64_t, std::vector<int>> from;
        from.reserve(es.size());
        for (std::size_t i = 0; i < es.size(); ++i)
            from[keyOf(es[i].x0, es[i].y0)].push_back(static_cast<int>(i));
        std::vector<char> used(es.size(), 0);
        for (std::size_t i = 0; i < es.size(); ++i)
        {
            if (used[i])
                continue;
            Polyline lp;
            lp.closed = true;
            const int sx = es[i].x0, sy = es[i].y0;
            int cur = static_cast<int>(i);
            lp.pts.push_back(Vec2{static_cast<float>(sx), static_cast<float>(sy)});
            std::size_t guard = 0;
            while (guard++ < es.size() + 4)
            {
                used[static_cast<std::size_t>(cur)] = 1;
                lp.pts.push_back(Vec2{static_cast<float>(es[static_cast<std::size_t>(cur)].x1),
                                      static_cast<float>(es[static_cast<std::size_t>(cur)].y1)});
                const int nx = es[static_cast<std::size_t>(cur)].x1;
                const int ny = es[static_cast<std::size_t>(cur)].y1;
                if (nx == sx && ny == sy)
                    break;
                auto it = from.find(keyOf(nx, ny));
                if (it == from.end())
                    break;
                const int inx = es[static_cast<std::size_t>(cur)].x1 - es[static_cast<std::size_t>(cur)].x0;
                const int iny = es[static_cast<std::size_t>(cur)].y1 - es[static_cast<std::size_t>(cur)].y0;
                int best = -1, bestScore = -1;
                for (int j : it->second)
                {
                    if (used[static_cast<std::size_t>(j)])
                        continue;
                    const int dx = es[static_cast<std::size_t>(j)].x1 - es[static_cast<std::size_t>(j)].x0;
                    const int dy = es[static_cast<std::size_t>(j)].y1 - es[static_cast<std::size_t>(j)].y0;
                    // 屏幕 y 向下：cross = inx*dy - iny*dx，正值 = 右转，按最右优先
                    const int score = inx * dy - iny * dx > 0 ? 2
                                      : (inx * dx + iny * dy > 0 ? 1 : 0);
                    if (score > bestScore)
                    {
                        bestScore = score;
                        best = j;
                    }
                }
                if (best < 0)
                    break;
                cur = best;
            }
            if (lp.pts.size() > 3)
            {
                lp.pts.pop_back();   // 末点与起点重复
                out.push_back(lp);
            }
            else
            {
                lp.closed = false;
                out.push_back(lp);
            }
        }
        return out;
    }

    // 可见元素：该像素最终显示的元素 id（0 表示露出底色）
    std::uint32_t visibleIdAt(int x, int y) const { return idAt(x, y); }

    // 像素中心命中：点 (x+0.5, y+0.5) 是否落在填充域内。
    // 与覆盖率口径的区别 ——
    //   覆盖率 = "这个像素格子被画到了多少"，边缘像素天然是 0..1 的灰值，没有唯一真值；
    //   中心命中 = "采样点在里面吗"，二值、唯一、可重复，不受多采样相位影响。
    // 需要"到底算不算画过"的硬判定（例如拾取、布尔命中）就用这个。
    bool centerHitAt(int x, int y) const
    {
        if (!m_record || x < 0 || y < 0 || x >= m_w || y >= m_h)
            return false;
        return m_center[static_cast<std::size_t>(y) * m_w + x] != 0;
    }

    // 二值轮廓：中心命中的像素，凡 4 邻域里有未命中的，就是这个集合的边界像素。
    // 返回的是像素方格（整数格 [x,x+1]×[y,y+1]），完全由采样结果决定，不做任何几何求交。
    std::vector<RectF> outlineRects() const
    {
        std::vector<RectF> out;
        if (!m_record)
            return out;
        for (int y = 0; y < m_h; ++y)
            for (int x = 0; x < m_w; ++x)
            {
                if (!centerHitAt(x, y))
                    continue;
                const bool edge = !centerHitAt(x - 1, y) || !centerHitAt(x + 1, y) ||
                                  !centerHitAt(x, y - 1) || !centerHitAt(x, y + 1);
                if (edge)
                    out.push_back(RectF{static_cast<float>(x), static_cast<float>(y),
                                        static_cast<float>(x + 1), static_cast<float>(y + 1)});
            }
        return out;
    }

    // 亚像素轮廓：在覆盖率场上取等值线（marching squares，端点用覆盖率线性插值）。
    // level=0.5 时，"半覆盖"的位置就是几何边界本身（理想直边下误差≈0），
    // 因此这条轮廓的精度是亚像素级，而不是像素台阶。返回值是一段段线段（每条 2 点）。
    std::vector<Polyline> contourAt(float level = 0.5f) const
    {
        std::vector<Polyline> out;
        if (!m_record || m_w < 2 || m_h < 2)
            return out;
        const float lv = level * 255.0f;
        auto v = [&](int x, int y) -> float {
            if (x < 0 || y < 0 || x >= m_w || y >= m_h)
                return 0.0f;
            return static_cast<float>(m_mask[static_cast<std::size_t>(y) * m_w + x]);
        };
        for (int y = 0; y < m_h - 1; ++y)
            for (int x = 0; x < m_w - 1; ++x)
            {
                const float v00 = v(x, y), v10 = v(x + 1, y), v11 = v(x + 1, y + 1),
                            v01 = v(x, y + 1);
                // 四条边的交点（端点跨过 level 才插值）
                Vec2 p[4];
                int n = 0;
                auto add = [&](float a, float b, float ax, float ay, float bx, float by) {
                    if ((a >= lv) == (b >= lv))
                        return;
                    const float t = (lv - a) / (b - a);
                    p[n++] = Vec2{ax + t * (bx - ax), ay + t * (by - ay)};
                };
                add(v00, v10, static_cast<float>(x), static_cast<float>(y), static_cast<float>(x + 1),
                    static_cast<float>(y));
                add(v10, v11, static_cast<float>(x + 1), static_cast<float>(y),
                    static_cast<float>(x + 1), static_cast<float>(y + 1));
                add(v01, v11, static_cast<float>(x), static_cast<float>(y + 1),
                    static_cast<float>(x + 1), static_cast<float>(y + 1));
                add(v00, v01, static_cast<float>(x), static_cast<float>(y), static_cast<float>(x),
                    static_cast<float>(y + 1));
                if (n == 2)
                {
                    Polyline pl;
                    pl.pts.push_back(p[0]);
                    pl.pts.push_back(p[1]);
                    out.push_back(pl);
                }
                else if (n == 4)
                {
                    // 鞍点：按中心值决定两段怎么连，避免交叉连接
                    const float vc = (v00 + v10 + v11 + v01) * 0.25f;
                    Polyline a, b;
                    if (vc >= lv)
                    {
                        a.pts = {p[0], p[3]};
                        b.pts = {p[1], p[2]};
                    }
                    else
                    {
                        a.pts = {p[0], p[1]};
                        b.pts = {p[2], p[3]};
                    }
                    out.push_back(a);
                    out.push_back(b);
                }
            }
        return out;
    }

    void fillShape(const Shape& s, const xmlgeo::XmlMatrix4* xf = nullptr)
    {
        fillPath(s.path, s.fill, xf, s.id);
        if (s.stroke.enabled)
            strokePath(s.path, s.stroke, xf, s.id);
    }

    void drawScene(const Scene& sc, const xmlgeo::XmlMatrix4* xf = nullptr)
    {
        clear(sc.clear);
        for (const Shape& s : sc.shapes)
            fillShape(s, xf);
    }

    // 填充：nonzero / even-odd 两种规则都自己算
    // id != 0 时，本次填充覆盖到的像素会记进 ID 缓冲（拾取用）
    void fillPath(const Path& path, const Paint& paint, const xmlgeo::XmlMatrix4* xf = nullptr,
                  std::uint32_t id = 0)
    {
        if (!paint.enabled || path.empty())
            return;
        std::vector<Polyline> polys = FlattenPath(path, 0.3f);
        transformPolys(polys, xf);
        fillPolys(polys, paint, paint.rule != 0, id);
    }

    // 描边：路径按半宽扩成多边形（round join/cap 用圆盘补齐），再按填充规则画
    void strokePath(const Path& path, const Stroke& st, const xmlgeo::XmlMatrix4* xf = nullptr,
                    std::uint32_t id = 0)
    {
        if (!st.enabled || path.empty() || st.width <= 0.0f)
            return;
        std::vector<Polyline> polys = FlattenPath(path, 0.3f);
        transformPolys(polys, xf);
        const float hw = st.width * 0.5f;
        std::vector<Polyline> out;
        out.reserve(polys.size() * 4);
        for (const Polyline& pl : polys)
        {
            if (pl.pts.size() < 2)
                continue;
            const bool closed = pl.closed;
            std::vector<Vec2> p = pl.pts;
            if (closed && Dist(p.front(), p.back()) > 0.01f)
                p.push_back(p.front());
            // 每一段：两侧各偏移半宽，构成一个梯形（端点用同向法线，接头处由圆盘盖住）
            for (std::size_t i = 0; i + 1 < p.size(); ++i)
            {
                const Vec2 a = p[i], b = p[i + 1];
                const float dx = b.x - a.x, dy = b.y - a.y;
                const float len = std::sqrt(dx * dx + dy * dy);
                if (len < 1e-4f)
                    continue;
                const float nx = -dy / len * hw, ny = dx / len * hw;
                Polyline q;
                q.pts = {Vec2{a.x + nx, a.y + ny}, Vec2{b.x + nx, b.y + ny},
                         Vec2{b.x - nx, b.y - ny}, Vec2{a.x - nx, a.y - ny}};
                q.closed = true;
                out.push_back(q);
            }
            // 接头与端点：join/cap 都用圆盘/方盘补齐（miter/bevel 也先按圆盘，
            // 差在尖角处少一块，不改变"线宽 = stroke-width"这一硬指标）
            const std::size_t first = 0;
            const std::size_t lastIndex = p.size() - 1;
            for (std::size_t i = 0; i < p.size(); ++i)
            {
                const bool isEnd = (i == first || i == lastIndex);
                if (isEnd && !closed)
                {
                    if (st.cap == 1)
                        out.push_back(disc(p[i], hw, 24));
                    else if (st.cap == 2)
                        out.push_back(squareDisc(p[i], hw));
                    // butt：不补
                }
                else
                {
                    out.push_back(disc(p[i], hw, 24));
                }
            }
        }
        Paint fillPaint;
        fillPaint.color = st.color;
        fillPaint.rule = 0;
        fillPolys(out, fillPaint, false, id);
    }

    // 直接给一段折线描边（CAD 视图这类"逐段画线"的调用方用）
    void strokePolyline(const std::vector<Vec2>& pts, bool closed, const Stroke& st)
    {
        Path p;
        if (pts.empty())
            return;
        p.moveTo(pts[0].x, pts[0].y);
        for (std::size_t i = 1; i < pts.size(); ++i)
            p.lineTo(pts[i].x, pts[i].y);
        if (closed)
            p.close();
        strokePath(p, st, nullptr);
    }

    bool saveBmp(const std::wstring& path) const
    {
        // 24bpp、bottom-up，够任何看图工具打开；不需要 PNG 编码依赖
        const int rowBytes = ((m_w * 3 + 3) / 4) * 4;
        const std::uint32_t dataSize = static_cast<std::uint32_t>(rowBytes) * m_h;
        std::uint8_t header[54] = {0};
        header[0] = 'B'; header[1] = 'M';
        const std::uint32_t fileSize = 54 + dataSize;
        std::memcpy(header + 2, &fileSize, 4);
        const std::uint32_t offBits = 54;
        std::memcpy(header + 10, &offBits, 4);
        const std::uint32_t biSize = 40;
        std::memcpy(header + 14, &biSize, 4);
        const std::int32_t bw = m_w, bh = m_h;
        std::memcpy(header + 18, &bw, 4);
        std::memcpy(header + 22, &bh, 4);
        const std::uint16_t planes = 1, bpp = 24;
        std::memcpy(header + 26, &planes, 2);
        std::memcpy(header + 28, &bpp, 2);
        std::memcpy(header + 34, &dataSize, 4);

        FILE* f = ::_wfopen(path.c_str(), L"wb");
        if (f == nullptr)
            return false;
        fwrite(header, 1, 54, f);
        std::vector<std::uint8_t> row(static_cast<std::size_t>(rowBytes), 0);
        for (int y = m_h - 1; y >= 0; --y)
        {
            for (int x = 0; x < m_w; ++x)
            {
                const std::uint32_t c = m_px[static_cast<std::size_t>(y) * m_w + x];
                row[static_cast<std::size_t>(x) * 3 + 0] = static_cast<std::uint8_t>((c)&0xFF);
                row[static_cast<std::size_t>(x) * 3 + 1] = static_cast<std::uint8_t>((c >> 8) & 0xFF);
                row[static_cast<std::size_t>(x) * 3 + 2] = static_cast<std::uint8_t>((c >> 16) & 0xFF);
            }
            fwrite(row.data(), 1, static_cast<std::size_t>(rowBytes), f);
        }
        fclose(f);
        return true;
    }

private:
    static void transformPolys(std::vector<Polyline>& polys, const xmlgeo::XmlMatrix4* xf)
    {
        if (xf == nullptr)
            return;
        for (Polyline& pl : polys)
            for (Vec2& p : pl.pts)
            {
                float ox = 0.0f, oy = 0.0f;
                xf->TransformPoint(p.x, p.y, 0.0f, ox, oy);
                p.x = ox;
                p.y = oy;
            }
    }

    static Polyline disc(const Vec2& c, float r, int seg)
    {
        Polyline q;
        q.closed = true;
        for (int i = 0; i < seg; ++i)
        {
            const float a = 6.283185307f * static_cast<float>(i) / seg;
            q.pts.push_back(Vec2{c.x + r * std::cos(a), c.y + r * std::sin(a)});
        }
        return q;
    }

    static Polyline squareDisc(const Vec2& c, float r)
    {
        Polyline q;
        q.closed = true;
        q.pts = {Vec2{c.x - r, c.y - r}, Vec2{c.x + r, c.y - r},
                 Vec2{c.x + r, c.y + r}, Vec2{c.x - r, c.y + r}};
        return q;
    }

    // 扫描线填充：垂直 4 子采样 + 水平精确覆盖率
    void fillPolys(const std::vector<Polyline>& polys, const Paint& paint, bool evenOdd,
                   std::uint32_t id)
    {
        struct Edge
        {
            float x0, y0, x1, y1;
        };
        std::vector<Edge> edges;
        RectF bb{1e30f, 1e30f, -1e30f, -1e30f};
        for (const Polyline& pl : polys)
        {
            if (pl.pts.size() < 3)
                continue;
            for (std::size_t i = 0; i < pl.pts.size(); ++i)
            {
                const Vec2 a = pl.pts[i];
                const Vec2 b = pl.pts[(i + 1) % pl.pts.size()];
                if (a.y == b.y)
                    continue;
                edges.push_back(Edge{a.x, a.y, b.x, b.y});
                bb.l = (std::min)(bb.l, (std::min)(a.x, b.x));
                bb.r = (std::max)(bb.r, (std::max)(a.x, b.x));
                bb.t = (std::min)(bb.t, (std::min)(a.y, b.y));
                bb.b = (std::max)(bb.b, (std::max)(a.y, b.y));
            }
        }
        if (edges.empty())
            return;
        const int y0 = (std::max)(0, static_cast<int>(std::floor(bb.t)));
        const int y1 = (std::min)(m_h - 1, static_cast<int>(std::ceil(bb.b)));
        const int x0 = (std::max)(0, static_cast<int>(std::floor(bb.l)));
        const int x1 = (std::min)(m_w - 1, static_cast<int>(std::ceil(bb.r)));
        if (y1 < y0 || x1 < x0)
            return;

        const int samples = 4;
        const int passes = m_record ? samples + 1 : samples;   // 最后一遍 = 像素中心采样
        struct Cross
        {
            float x;
            int wind;
        };
        std::vector<Cross> xs;
        for (int y = y0; y <= y1; ++y)
        {
            std::fill(m_cov.begin(), m_cov.end(), 0.0f);
            for (int s = 0; s < passes; ++s)
            {
                const bool centerPass = (s == samples);
                const float sy = centerPass ? (static_cast<float>(y) + 0.5f)
                                            : static_cast<float>(y) + (static_cast<float>(s) + 0.5f) / samples;
                xs.clear();
                for (const Edge& e : edges)
                {
                    const float lo = (std::min)(e.y0, e.y1), hi = (std::max)(e.y0, e.y1);
                    if (sy < lo || sy >= hi)
                        continue;
                    const float t = (sy - e.y0) / (e.y1 - e.y0);
                    xs.push_back(Cross{e.x0 + t * (e.x1 - e.x0), e.y0 < e.y1 ? 1 : -1});
                }
                if (xs.size() < 2)
                    continue;
                std::sort(xs.begin(), xs.end(), [](const Cross& a, const Cross& b) { return a.x < b.x; });
                int wind = 0;
                for (std::size_t i = 0; i + 1 < xs.size(); ++i)
                {
                    wind += xs[i].wind;
                    const bool inside = evenOdd ? ((wind & 1) != 0) : (wind != 0);
                    if (!inside)
                        continue;
                    float xa = xs[i].x, xb = xs[i + 1].x;
                    if (xb <= static_cast<float>(x0) || xa >= static_cast<float>(x1 + 1))
                        continue;
                    xa = (std::max)(xa, static_cast<float>(x0));
                    xb = (std::min)(xb, static_cast<float>(x1 + 1));
                    if (xb <= xa)
                        continue;
                    const float inv = 1.0f / samples;
                    int ia = static_cast<int>(std::floor(xa));
                    int ib = static_cast<int>(std::ceil(xb)) - 1;
                    if (centerPass)
                    {
                        // 中心采样：不做覆盖率加权，只判像素中心 (ix+0.5, y+0.5) 在不在 span 里
                        for (int ix = ia; ix <= ib; ++ix)
                        {
                            if (ix < x0 || ix > x1)
                                continue;
                            const float cx = static_cast<float>(ix) + 0.5f;
                            if (cx >= xa && cx < xb)
                                m_center[static_cast<std::size_t>(y) * m_w + ix] = 1;
                        }
                        continue;
                    }
                    for (int ix = ia; ix <= ib; ++ix)
                    {
                        if (ix < x0 || ix > x1)
                            continue;
                        const float l = (std::max)(xa, static_cast<float>(ix));
                        const float r = (std::min)(xb, static_cast<float>(ix + 1));
                        if (r > l)
                            m_cov[static_cast<std::size_t>(ix)] += (r - l) * inv;
                    }
                }
            }

            for (int x = x0; x <= x1; ++x)
            {
                float a = m_cov[static_cast<std::size_t>(x)];
                if (a <= 0.0005f)
                    continue;
                if (a > 1.0f) a = 1.0f;
                std::uint32_t c = paint.color;
                if (paint.gradient)
                {
                    const float gx = paint.g1.x - paint.g0.x;
                    const float gy = paint.g1.y - paint.g0.y;
                    const float denom = gx * gx + gy * gy;
                    float t = 0.0f;
                    if (denom > 1e-6f)
                        t = ((static_cast<float>(x) + 0.5f - paint.g0.x) * gx +
                             (static_cast<float>(y) + 0.5f - paint.g0.y) * gy) / denom;
                    t = (std::max)(0.0f, (std::min)(1.0f, t));
                    c = lerpColor(paint.color0, paint.color1, t);
                }
                // 拾取记录：先按覆盖率记"这里被画过"，再按绘制顺序记元素 id
                if (m_record)
                {
                    const std::size_t off = static_cast<std::size_t>(y) * m_w + x;
                    const std::uint32_t cov = static_cast<std::uint32_t>(a * 255.0f + 0.5f);
                    const std::uint32_t sum = static_cast<std::uint32_t>(m_mask[off]) + cov;
                    m_mask[off] = static_cast<std::uint8_t>(sum > 255 ? 255 : sum);
                    if (id != 0)
                        m_ids[off] = id;
                }
                blendPixel(x, y, c, a);
            }
        }
    }

    static std::uint32_t lerpColor(std::uint32_t c0, std::uint32_t c1, float t)
    {
        const auto ch = [](std::uint32_t c, int shift) { return static_cast<float>((c >> shift) & 0xFF); };
        const auto mix = [t](float a, float b) { return static_cast<std::uint32_t>(a + (b - a) * t + 0.5f); };
        const std::uint32_t a = mix(ch(c0, 24), ch(c1, 24));
        const std::uint32_t r = mix(ch(c0, 16), ch(c1, 16));
        const std::uint32_t g = mix(ch(c0, 8), ch(c1, 8));
        const std::uint32_t b = mix(ch(c0, 0), ch(c1, 0));
        return (a << 24) | (r << 16) | (g << 8) | b;
    }

    void blendPixel(int x, int y, std::uint32_t src, float cover)
    {
        const float sa = static_cast<float>((src >> 24) & 0xFF) / 255.0f * cover;
        if (sa <= 0.0005f)
            return;
        std::uint32_t& dst = m_px[static_cast<std::size_t>(y) * m_w + x];
        const float dr = static_cast<float>((dst >> 16) & 0xFF);
        const float dg = static_cast<float>((dst >> 8) & 0xFF);
        const float db = static_cast<float>(dst & 0xFF);
        const float da = static_cast<float>((dst >> 24) & 0xFF) / 255.0f;
        const float sr = static_cast<float>((src >> 16) & 0xFF);
        const float sg = static_cast<float>((src >> 8) & 0xFF);
        const float sb = static_cast<float>(src & 0xFF);
        // src-over（非预乘）：
        const float outA = sa + da * (1.0f - sa);
        float outR = (sr * sa + dr * da * (1.0f - sa));
        float outG = (sg * sa + dg * da * (1.0f - sa));
        float outB = (sb * sa + db * da * (1.0f - sa));
        if (outA > 1e-6f)
        {
            outR /= outA;
            outG /= outA;
            outB /= outA;
        }
        const auto clamp255 = [](float v) {
            if (v < 0.0f) v = 0.0f;
            if (v > 255.0f) v = 255.0f;
            return static_cast<std::uint32_t>(v + 0.5f);
        };
        dst = (clamp255(outA * 255.0f) << 24) | (clamp255(outR) << 16) |
              (clamp255(outG) << 8) | clamp255(outB);
    }

    int m_w = 0;
    int m_h = 0;
    std::vector<std::uint32_t> m_px;
    std::vector<float> m_cov;
    // 拾取缓冲（enablePicking 才分配）：覆盖率 + 最后绘制的元素 id
    bool m_record = false;
    std::vector<std::uint8_t> m_mask;
    std::vector<std::uint32_t> m_ids;
    std::vector<std::uint8_t> m_center;
};

} // namespace xmlvec
