// ===========================================================================
// outline_png.cpp —— 对一张已有位图求轮廓（纯像素法，不知道也不问原几何）
//
// 输入 24bpp BMP（由 PNG 转来），输出三张对照图：
//   *_ink.bmp    覆盖率 50% 等值线：细线自身的两条边（亚像素）
//   *_union.bmp  flood fill 外部背景后，几个圆并集的整体外轮廓（含内孔）
//   *_step.bmp   二值化后的像素台阶轮廓（1px 绿点，看清"像素法"的极限在哪）
//
// 关键：全程没有任何点-多边形判定，也没有原几何（没有圆心/半径可用）。
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
    std::vector<std::uint32_t> px;   // 0xAARRGGBB
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

static void copySrc(xmlvec::Raster& r, const Img& img)
{
    std::memcpy(r.data(), img.px.data(), sizeof(std::uint32_t) * img.px.size());
}

static std::int64_t g_seg = 0;
static void drawContours(xmlvec::Raster& r, const std::vector<xmlvec::Polyline>& cs,
                         std::uint32_t color, float width)
{
    xmlvec::Stroke st;
    st.enabled = true;
    st.width = width;
    st.color = color;
    st.cap = 1;
    st.join = 2;
    for (const xmlvec::Polyline& pl : cs)
    {
        if (pl.pts.size() < 2)
            continue;
        xmlvec::Path p;
        p.moveTo(pl.pts.front().x, pl.pts.front().y);
        for (std::size_t i = 1; i < pl.pts.size(); ++i)
            p.lineTo(pl.pts[i].x, pl.pts[i].y);
        r.strokePath(p, st, nullptr);
        ++g_seg;
    }
}

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        std::fprintf(stderr, "usage: outline_png in.bmp outprefix\n");
        return 1;
    }
    Img img;
    if (!loadBmp24(argv[1], img))
    {
        std::fprintf(stderr, "cannot read %s\n", argv[1]);
        return 2;
    }
    const int W = img.w, H = img.h;

    // ---- 1. 位图 -> 覆盖率场（墨迹量，不需要知道原几何） -------------------
    xmlvec::Raster r(W, H, 0xFF000000u);
    r.enablePicking(true);
    copySrc(r, img);
    int peak = 1;
    for (int i = 0; i < W * H; ++i)
    {
        const std::uint32_t c = img.px[static_cast<std::size_t>(i)];
        const int rr = (c >> 16) & 0xFF, gg = (c >> 8) & 0xFF, bb = c & 0xFF;
        const int luma = (std::max)(rr, (std::max)(gg, bb));   // 取最大通道：彩线（洋红）与白线都识别
        peak = (std::max)(peak, luma);
    }
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x)
        {
            const std::uint32_t c = img.px[static_cast<std::size_t>(y) * W + x];
            const int rr = (c >> 16) & 0xFF, gg = (c >> 8) & 0xFF, bb = c & 0xFF;
            const int luma = (std::max)(rr, (std::max)(gg, bb));   // 取最大通道：彩线（洋红）与白线都识别
            const int cov = (std::min)(255, luma * 255 / peak);
            r.setCoverageAt(x, y, static_cast<std::uint8_t>(cov));
        }

    // ---- 2. 细线自身的两条边轮廓（覆盖率 50% 等值线） -----------------------
    const std::vector<xmlvec::Polyline> ink = r.contourAt(0.5f);
    xmlvec::Raster outInk(W, H, 0xFF000000u);
    outInk.enablePicking(false);
    copySrc(outInk, img);
    drawContours(outInk, ink, 0xFFFF3020u, 1.0f);
    {
        char p[512];
        std::snprintf(p, sizeof p, "%s_ink.bmp", argv[2]);
        outInk.saveBmp(std::wstring(p, p + std::strlen(p)));
    }
    std::printf("INK outline segs=%zu\n", ink.size());

    // ---- 3. flood fill 外部背景 -> 并集区域 -> 整体外轮廓 -------------------
    std::vector<std::uint8_t> outside(static_cast<std::size_t>(W) * H, 0);
    std::vector<int> stack;
    stack.reserve(static_cast<std::size_t>(W) * 4);
    for (int x = 0; x < W; ++x)
    {
        stack.push_back(x);
        stack.push_back((H - 1) * W + x);
    }
    for (int y = 0; y < H; ++y)
    {
        stack.push_back(y * W);
        stack.push_back(y * W + W - 1);
    }
    while (!stack.empty())
    {
        const int idx = stack.back();
        stack.pop_back();
        if (outside[static_cast<std::size_t>(idx)])
            continue;
        if (r.coverageAt(idx % W, idx / W) >= 24)   // 墨迹挡住
            continue;
        outside[static_cast<std::size_t>(idx)] = 1;
        const int x = idx % W, y = idx / W;
        if (x > 0) stack.push_back(idx - 1);
        if (x + 1 < W) stack.push_back(idx + 1);
        if (y > 0) stack.push_back(idx - W);
        if (y + 1 < H) stack.push_back(idx + W);
    }
    xmlvec::Raster solid(W, H, 0xFF000000u);
    solid.enablePicking(true);
    std::size_t interior = 0;
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x)
        {
            const bool in = !outside[static_cast<std::size_t>(y) * W + x];
            if (in && r.coverageAt(x, y) < 24)
                ++interior;
            solid.setCoverageAt(x, y, in ? 255 : 0);
        }
    const std::vector<xmlvec::Polyline> uni = solid.contourAt(0.5f);
    xmlvec::Raster outUni(W, H, 0xFF000000u);
    outUni.enablePicking(false);
    copySrc(outUni, img);
    // 内部区域淡淡铺一层白，证明"并集"是填出来的，不是算出来的
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x)
        {
            if (outside[static_cast<std::size_t>(y) * W + x])
                continue;
            if (r.coverageAt(x, y) >= 24)
                continue;
            std::uint32_t c = outUni.pixel(x, y);
            const int rr = (c >> 16) & 0xFF, gg = (c >> 8) & 0xFF, bb = c & 0xFF;
            const int nr = rr + (255 - rr) / 5, ng = gg + (255 - gg) / 5,
                      nb = bb + (255 - bb) / 5;
            outUni.data()[static_cast<std::size_t>(y) * W + x] =
                0xFF000000u | (static_cast<std::uint32_t>(nr) << 16) |
                (static_cast<std::uint32_t>(ng) << 8) | static_cast<std::uint32_t>(nb);
        }
    drawContours(outUni, uni, 0xFF30A0FFu, 1.6f);
    {
        char p[512];
        std::snprintf(p, sizeof p, "%s_union.bmp", argv[2]);
        outUni.saveBmp(std::wstring(p, p + std::strlen(p)));
    }
    std::printf("UNION outline segs=%zu interior_px=%zu\n", uni.size(), interior);

    // ---- 4. 像素台阶轮廓（二值化后逐像素，看清 0.5px 台阶的形态） ----------
    r.thresholdToCenter(128);
    xmlvec::Raster outStep(W, H, 0xFF000000u);
    outStep.enablePicking(false);
    copySrc(outStep, img);
    std::size_t ring = 0;
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x)
        {
            if (!r.centerHitAt(x, y))
                continue;
            const bool edge = !r.centerHitAt(x - 1, y) || !r.centerHitAt(x + 1, y) ||
                              !r.centerHitAt(x, y - 1) || !r.centerHitAt(x, y + 1);
            if (!edge)
                continue;
            ++ring;
            outStep.data()[static_cast<std::size_t>(y) * W + x] = 0xFF40FF40u;
        }
    {
        char p[512];
        std::snprintf(p, sizeof p, "%s_step.bmp", argv[2]);
        outStep.saveBmp(std::wstring(p, p + std::strlen(p)));
    }
    std::printf("STEP outline px=%zu\n", ring);

    std::printf("OK size=%dx%d segs_total=%lld\n", W, H, static_cast<long long>(g_seg));
    return 0;
}
