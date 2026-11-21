// ===========================================================================
// contour2dxf.cpp —— 位图 -> 像素法轮廓 -> 矢量闭合环 -> DXF
//
// 输出的是真正的矢量图元：LWPOLYLINE（闭合，顶点 double），不是贴图。
// 在 DWG/CAD 画布上就是可缩放、可捕捉、可偏移、可分层的一条曲线。
// ===========================================================================
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>
#include "XmlVector.h"

struct Img
{
    int w = 0, h = 0;
    std::vector<std::uint32_t> px;
};

static bool loadBmp24(const char* path, Img& out)
{
    std::FILE* f = std::fopen(path, "rb");
    if (!f)
        return false;
    unsigned char hdr[54];
    if (std::fread(hdr, 1, 54, f) != 54)
    {
        std::fclose(f);
        return false;
    }
    const int w = *reinterpret_cast<int*>(hdr + 18);
    const int hRaw = *reinterpret_cast<int*>(hdr + 22);
    const int bpp = *reinterpret_cast<short*>(hdr + 28);
    const unsigned off = *reinterpret_cast<unsigned*>(hdr + 10);
    if (bpp != 24)
    {
        std::fclose(f);
        return false;
    }
    const int h = hRaw < 0 ? -hRaw : hRaw;
    const bool bottomUp = hRaw > 0;
    std::fseek(f, static_cast<long>(off), SEEK_SET);
    const int stride = ((w * 3 + 3) / 4) * 4;
    std::vector<unsigned char> row(static_cast<std::size_t>(stride));
    out.w = w;
    out.h = h;
    out.px.assign(static_cast<std::size_t>(w) * h, 0xFF000000u);
    for (int i = 0; i < h; ++i)
    {
        const int y = bottomUp ? (h - 1 - i) : i;
        if (std::fread(row.data(), 1, row.size(), f) != row.size())
            break;
        for (int x = 0; x < w; ++x)
        {
            const unsigned char b = row[x * 3 + 0], g = row[x * 3 + 1], r = row[x * 3 + 2];
            out.px[static_cast<std::size_t>(y) * w + x] =
                0xFF000000u | (static_cast<std::uint32_t>(r) << 16) |
                (static_cast<std::uint32_t>(g) << 8) | b;
        }
    }
    std::fclose(f);
    return true;
}

static void emit(std::FILE* f, int code, const char* value)
{
    std::fprintf(f, "%d\n%s\n", code, value);
}
static void emitD(std::FILE* f, int code, double v)
{
    std::fprintf(f, "%d\n%.6f\n", code, v);
}
static void emitI(std::FILE* f, int code, int v)
{
    std::fprintf(f, "%d\n%d\n", code, v);
}

// Kasa 最小二乘圆拟合：解 [Sxx Sxy Sx; Sxy Syy Sy; Sx Sy n]·[a b c]ᵀ = [..]ᵀ
struct Fit
{
    bool ok = false;
    double cx = 0.0, cy = 0.0, r = 0.0, resid = 0.0;
};

static Fit fitCircle(const std::vector<xmlvec::Vec2>& pts)
{
    Fit ft;
    const double n = static_cast<double>(pts.size());
    if (n < 12)
        return ft;
    // 先平移到质心再做 Kasa：坐标量级从 ~400 降到 ~半径，否则正规方程会因大数相减丢精度
    double mx = 0.0, my = 0.0;
    for (const xmlvec::Vec2& p : pts)
    {
        mx += p.x;
        my += p.y;
    }
    mx /= n;
    my /= n;
    double Sx = 0, Sy = 0, Sxx = 0, Syy = 0, Sxy = 0;
    double b1 = 0, b2 = 0, b3 = 0;
    for (const xmlvec::Vec2& p : pts)
    {
        const double x = p.x - mx, y = p.y - my;
        Sx += x, Sy += y, Sxx += x * x, Syy += y * y, Sxy += x * y;
        b1 += x * (x * x + y * y);
        b2 += y * (x * x + y * y);
        b3 += x * x + y * y;
    }
    const double m[3][4] = {{Sxx, Sxy, Sx, b1}, {Sxy, Syy, Sy, b2}, {Sx, Sy, n, b3}};
    auto det3 = [](const double a[3][4], int skip) -> double {
        double r[3][3];
        for (int i = 0; i < 3; ++i)
        {
            int k = 0;
            for (int j = 0; j < 4; ++j)
            {
                if (j == skip)
                    continue;
                r[i][k++] = a[i][j];
            }
        }
        return r[0][0] * (r[1][1] * r[2][2] - r[1][2] * r[2][1]) -
               r[0][1] * (r[1][0] * r[2][2] - r[1][2] * r[2][0]) +
               r[0][2] * (r[1][0] * r[2][1] - r[1][1] * r[2][0]);
    };
    const double D = det3(m, 3);
    if (std::fabs(D) < 1e-9)
        return ft;
    // x²+y² = A·x + B·y + C  → 圆心 (A/2, B/2)，r² = C + A²/4 + B²/4
    const double A = det3(m, 0) / D, B = det3(m, 1) / D, Cc = det3(m, 2) / D;
    const double r2 = Cc + 0.25 * (A * A + B * B);
    if (r2 <= 0.0)
        return ft;
    const double a = A * 0.5, b = B * 0.5;
    const double r = std::sqrt(r2);
    double err = 0.0;
    for (const xmlvec::Vec2& p : pts)
    {
        const double dx = p.x - mx - a, dy = p.y - my - b;
        err += std::fabs(std::sqrt(dx * dx + dy * dy) - r);
    }
    ft.ok = true;
    ft.cx = a + mx;
    ft.cy = b + my;
    ft.r = r + 0.5;   // 二值轮廓沿像素中心连线，真实边界还要向外推半像素
    ft.resid = err / n;
    return ft;
}

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        std::fprintf(stderr, "usage: contour2dxf in.bmp out.dxf\n");
        return 1;
    }
    Img img;
    if (!loadBmp24(argv[1], img))
    {
        std::fprintf(stderr, "cannot read %s\n", argv[1]);
        return 2;
    }
    const int W = img.w, H = img.h;

    xmlvec::Raster r(W, H, 0xFF000000u);
    r.enablePicking(true);
    int peak = 1;
    std::vector<int> luma(static_cast<std::size_t>(W) * H, 0);
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x)
        {
            const std::uint32_t c = img.px[static_cast<std::size_t>(y) * W + x];
            const int rr = (c >> 16) & 0xFF, gg = (c >> 8) & 0xFF, bb = c & 0xFF;
            const int L = (std::max)(rr, (std::max)(gg, bb));   // 最大通道：彩线/白线同一口径
            luma[static_cast<std::size_t>(y) * W + x] = L;
            peak = (std::max)(peak, L);
        }
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x)
            r.setCoverageAt(x, y, static_cast<std::uint8_t>(
                (std::min)(255, luma[static_cast<std::size_t>(y) * W + x] * 255 / peak)));

    // 覆盖率直方图（诊断"轮廓段数为什么是这个量"）
    int hi = 0, mid = 0, lo = 0;
    for (int i = 0; i < W * H; ++i)
    {
        const int v = luma[static_cast<std::size_t>(i)];
        if (v * 255 / peak >= 200) ++hi;
        else if (v * 255 / peak >= 40) ++mid;
        else if (v * 255 / peak > 0) ++lo;
    }
    std::printf("LUMA peak=%d hi=%d mid=%d lo=%d\n", peak, hi, mid, lo);

    // 模式一：覆盖率亚像素等值线（贴原图元，但阈值附近会出碎片环）
    std::vector<xmlvec::Polyline> loops = r.contourLoops(0.5f);
    {
        int small = 0;
        for (const xmlvec::Polyline& lp : loops)
            if (lp.pts.size() < 12) ++small;
        std::printf("GRAD loops=%zu small(n<12)=%d\n", loops.size(), small);
    }

    // 模式二：先二值化（阈值 128）再求轮廓 —— 像素法标准口径，无阈值抖动
    r.thresholdToCenter(128);
    // 连通域过滤（4 邻域）：抗锯齿边缘会把阈值切成一堆 1~3 像素的碎点，
    // 只保留 >= 40 像素的连通块，轮廓才是"图元本身"而不是噪声。
    {
        std::vector<int> lab(static_cast<std::size_t>(W) * H, 0);
        std::vector<int> stk;
        std::vector<std::size_t> cnt(1, 0);
        for (int y = 0; y < H; ++y)
            for (int x = 0; x < W; ++x)
            {
                const int idx = y * W + x;
                if (lab[static_cast<std::size_t>(idx)] || !r.centerHitAt(x, y))
                    continue;
                const int cur = static_cast<int>(cnt.size());
                cnt.push_back(0);
                stk.clear();
                stk.push_back(idx);
                lab[static_cast<std::size_t>(idx)] = cur;
                while (!stk.empty())
                {
                    const int i = stk.back();
                    stk.pop_back();
                    ++cnt[static_cast<std::size_t>(cur)];
                    const int xx = i % W, yy = i / W;
                    auto push = [&](int nx, int ny) {
                        if (nx < 0 || ny < 0 || nx >= W || ny >= H)
                            return;
                        const int ni = ny * W + nx;
                        if (lab[static_cast<std::size_t>(ni)] || !r.centerHitAt(nx, ny))
                            return;
                        lab[static_cast<std::size_t>(ni)] = cur;
                        stk.push_back(ni);
                    };
                    push(xx - 1, yy);
                    push(xx + 1, yy);
                    push(xx, yy - 1);
                    push(xx, yy + 1);
                }
            }
        std::printf("BLOCKS=%zu keep=%zu\n", cnt.size() - 1,
                    static_cast<std::size_t>(std::count_if(cnt.begin(), cnt.end(),
                                                           [](std::size_t c) { return c >= 40; })));
        for (int y = 0; y < H; ++y)
            for (int x = 0; x < W; ++x)
            {
                const int lb = lab[static_cast<std::size_t>(y) * W + x];
                const bool on = lb > 0 && cnt[static_cast<std::size_t>(lb)] >= 40;
                r.setCoverageAt(x, y, on ? 255 : 0);
            }
    }
    r.thresholdToCenter(128);   // 中心位图与保留的连通块对齐，像素边界环才不会被碎片污染

    // --- 轮廓边集（像素法最可靠的表示）：每条边 1 单位长，整数端点 ---
    const std::vector<std::pair<xmlvec::Vec2, xmlvec::Vec2>> edges = r.pixelEdges();
    double peri = 0.0;
    for (const auto& e : edges)
        peri += std::sqrt((e.second.x - e.first.x) * (e.second.x - e.first.x) +
                          (e.second.y - e.first.y) * (e.second.y - e.first.y));

    // --- 顺带串环，给出"能闭合多少"：闭合数不作为正确性判据 ---
    loops = r.pixelLoops();
    std::size_t closed = 0;
    for (const xmlvec::Polyline& lp : loops)
        if (lp.closed)
            ++closed;

    std::FILE* f = std::fopen(argv[2], "w");
    if (!f)
    {
        std::fprintf(stderr, "cannot write %s\n", argv[2]);
        return 3;
    }
    emit(f, 0, "SECTION");
    emit(f, 2, "HEADER");
    emit(f, 9, "$ACADVER");
    emit(f, 1, "AC1009");
    emit(f, 9, "$INSUNITS");
    emitI(f, 70, 0);
    emit(f, 0, "ENDSEC");
    emit(f, 0, "SECTION");
    emit(f, 2, "TABLES");
    emit(f, 0, "TABLE");
    emit(f, 2, "LTYPE");
    emitI(f, 70, 1);
    emit(f, 0, "LTYPE");
    emit(f, 2, "CONTINUOUS");
    emitI(f, 70, 0);
    emit(f, 3, "Solid line");
    emitI(f, 72, 65);
    emitI(f, 73, 0);
    emitD(f, 40, 0.0);
    emit(f, 0, "ENDTAB");
    emit(f, 0, "TABLE");
    emit(f, 2, "LAYER");
    emitI(f, 70, 2);
    emit(f, 0, "LAYER");
    emit(f, 2, "0");
    emitI(f, 70, 0);
    emitI(f, 62, 7);
    emit(f, 6, "CONTINUOUS");
    emit(f, 0, "LAYER");
    emit(f, 2, "CONTOUR");
    emitI(f, 70, 0);
    emitI(f, 62, 3);
    emit(f, 6, "CONTINUOUS");
    emit(f, 0, "ENDTAB");
    emit(f, 0, "ENDSEC");
    emit(f, 0, "SECTION");
    emit(f, 2, "BLOCKS");
    emit(f, 0, "ENDSEC");
    emit(f, 0, "SECTION");
    emit(f, 2, "ENTITIES");
    for (const auto& e : edges)
    {
        emit(f, 0, "LINE");
        emit(f, 8, "CONTOUR");
        emitD(f, 10, e.first.x);
        emitD(f, 20, static_cast<double>(H) - e.first.y);
        emitD(f, 30, 0.0);
        emitD(f, 11, e.second.x);
        emitD(f, 21, static_cast<double>(H) - e.second.y);
        emitD(f, 31, 0.0);
    }
    emit(f, 0, "ENDSEC");
    emit(f, 0, "EOF");
    std::fclose(f);

    std::printf("EDGES=%zu perimeter=%.1f loops=%zu closed=%zu -> %s\n", edges.size(), peri,
                loops.size(), closed, argv[2]);
    return 0;
}
