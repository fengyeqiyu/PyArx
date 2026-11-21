// ===========================================================================
// pick_test.cpp —— 验证"不靠几何运算也能知道某个像素有没有被绘制"
//
// 口径：渲染时顺手把覆盖率与元素 id 写进缓冲，之后查询像素即可；
// 不做点在多边形内判定，也不回读几何。
//   覆盖率：该像素被任何图元画到的累计量（抗锯齿边缘是中间值）
//   元素 id：最后画到该像素的元素（后画覆盖先画 = 屏幕最终可见语义）
// ===========================================================================
#include <cstdio>
#include "XmlVector.h"

static int g_fail = 0;
static void check(bool ok, const char* what)
{
    std::printf("%s %s\n", ok ? "PASS" : "FAIL", what);
    if (!ok) ++g_fail;
}

int main()
{
    xmlvec::Raster r(240, 200, 0xFFFFFFFFu);
    r.enablePicking(true);

    // A：左圆，id=1；B：右圆，id=2，两者在 x∈[100,140] 相交
    xmlvec::Shape a;
    a.path.addCircle(100, 100, 40);
    a.fill.color = 0xFFCC2222u;
    a.id = 1;

    xmlvec::Shape b;
    b.path.addCircle(140, 100, 40);
    b.fill.color = 0xFF2244CCu;
    b.id = 2;

    r.fillShape(a);
    r.fillShape(b);

    // 1) A 独占区：被画过，且属于 1 号元素
    check(r.paintedAt(80, 100), "A 独占像素被绘制");
    check(r.idAt(80, 100) == 1, "A 独占像素 id=1");

    // 2) B 独占区
    check(r.paintedAt(160, 100), "B 独占像素被绘制");
    check(r.idAt(160, 100) == 2, "B 独占像素 id=2");

    // 3) 重叠区：后画的 B 覆盖 A —— 与屏幕最终可见一致
    check(r.paintedAt(120, 100), "重叠像素被绘制");
    check(r.idAt(120, 100) == 2, "重叠像素归属后画的元素（id=2）");

    // 4) 空白区：既没画过也不属于任何元素
    check(!r.paintedAt(10, 10), "空白像素未被绘制");
    check(r.idAt(10, 10) == 0, "空白像素 id=0");
    check(r.coverageAt(10, 10) == 0, "空白像素覆盖率=0");

    // 5) 抗锯齿：圆心区域覆盖率饱和，圆边缘出现中间覆盖率
    check(r.coverageAt(100, 100) == 255, "实心区覆盖率饱和 255");
    int partial = 0;
    for (int y = 0; y < r.height(); ++y)
        for (int x = 0; x < r.width(); ++x)
        {
            const int c = r.coverageAt(x, y);
            if (c > 0 && c < 255)
                ++partial;
        }
    check(partial > 50, "边缘存在中间覆盖率（抗锯齿像素）");
    std::printf("      antialias partial px = %d\n", partial);

    // 6) 不开记录时零采样，避免误以为"默认就有"
    xmlvec::Raster plain(240, 200, 0xFFFFFFFFu);
    plain.fillShape(a);
    plain.fillShape(b);
    check(!plain.pickingEnabled(), "默认不开启拾取记录");
    check(plain.coverageAt(100, 100) == 0 && plain.idAt(100, 100) == 0, "未开启时查询恒为 0");

    // 7) 直接读像素也能判"画没画"（同色风险见注释）：这里用与底色不同的填充验证
    const std::uint32_t px = r.pixel(80, 100);
    check(px == 0xFFCC2222u || px == 0xFF2244CCu, "像素值可直接读回（A 区是 A 的颜色）");

    std::printf("pick_test: FAIL=%d\n", g_fail);
    return g_fail == 0 ? 0 : 1;
}
