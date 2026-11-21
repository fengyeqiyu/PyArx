// ===========================================================================
// outline_test.cpp —— 整数像素栅格 vs double 几何，"轮廓"这一环的偏差到底在哪
//
// 三个口径必须分开看，混用就会觉得"抖动"：
//   1) coverageAt：该像素格被盖住多少（0..255，灰值）。边缘天然是灰值，没有真值，
//      对它做二值判定必然在阈值附近跳来跳去 —— 这不是渲染器的错，是问题没定义。
//   2) centerHitAt：像素中心 (x+0.5, y+0.5) 是否在域内。二值、唯一、可重复。
//      整数像素与 double 几何的"交点偏差"在这里被彻底消掉：采样点固定，答案是确定的。
//   3) contourAt(0.5)：覆盖率 50% 等值线。对理想直边，覆盖率跨 1 像素线性上升，
//      50% 处正好是几何边界 —— 所以轮廓是亚像素精度，不是像素台阶。
// ===========================================================================
#include <cmath>
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
    // ---- 1. 矩形：覆盖率轮廓应贴住几何边 x=100/140、y=100/140 ----------------
    {
        xmlvec::Raster r(200, 200, 0xFFFFFFFFu);
        r.enablePicking(true);
        xmlvec::Shape s;
        s.path.addRect(100.0f, 100.0f, 40.0f, 40.0f);
        s.fill.color = 0xFF000000u;
        s.id = 1;
        r.fillShape(s);

        const std::vector<xmlvec::Polyline> c = r.contourAt(0.5f);
        check(!c.empty(), "矩形得到轮廓线段");
        float maxErr = 0.0f;
        for (const xmlvec::Polyline& pl : c)
            for (const xmlvec::Vec2& p : pl.pts)
            {
                // 每条线段端点必落在四条几何边之一上
                const float d[4] = {std::fabs(p.x - 100.0f), std::fabs(p.x - 140.0f),
                                    std::fabs(p.y - 100.0f), std::fabs(p.y - 140.0f)};
                float best = d[0];
                for (int i = 1; i < 4; ++i) best = (std::min)(best, d[i]);
                maxErr = (std::max)(maxErr, best);
            }
        std::printf("      rect outline pts=%zu maxErr=%.3f px\n", c.size() * 2, maxErr);
        check(maxErr < 0.7f, "矩形轮廓与 double 几何偏差 < 0.7px（亚像素级）");

        // ---- 2. 中心命中：整数格与 double 边的关系是确定的 -------------------
        check(r.centerHitAt(120, 120), "矩形内部像素中心命中");
        check(!r.centerHitAt(95, 120), "矩形左侧外部像素中心未命中");
        check(r.centerHitAt(100, 120), "x=100 像素中心(100.5)在矩形内 → 命中");
        check(!r.centerHitAt(99, 120), "x=99 像素中心(99.5)在矩形外 → 不命中");
        check(r.centerHitAt(139, 120) && !r.centerHitAt(140, 120), "右边界像素归属精确到 0.5px");
        // 覆盖率口径对照：同一批边界像素是灰值，二值化会抖
        std::printf("      cov(99,120)=%d cov(100,120)=%d cov(140,120)=%d\n", r.coverageAt(99, 120),
                    r.coverageAt(100, 120), r.coverageAt(140, 120));

        // ---- 3. 二值轮廓（整数格）：边界像素环 -------------------------------
        const std::vector<xmlvec::RectF> ring = r.outlineRects();
        check(!ring.empty(), "得到整数格边界像素集合");
        std::printf("      outlineRects=%zu (周长≈160 px)\n", ring.size());
    }

    // ---- 4. 圆：轮廓点到圆心的距离应 ≈ r -------------------------------------
    {
        xmlvec::Raster r(240, 240, 0xFFFFFFFFu);
        r.enablePicking(true);
        xmlvec::Shape s;
        s.path.addCircle(120.0f, 120.0f, 50.0f);
        s.fill.color = 0xFF114488u;
        s.id = 1;
        r.fillShape(s);

        const std::vector<xmlvec::Polyline> c = r.contourAt(0.5f);
        float maxErr = 0.0f;
        for (const xmlvec::Polyline& pl : c)
            for (const xmlvec::Vec2& p : pl.pts)
            {
                const float d = std::sqrt((p.x - 120.0f) * (p.x - 120.0f) +
                                          (p.y - 120.0f) * (p.y - 120.0f));
                maxErr = (std::max)(maxErr, std::fabs(d - 50.0f));
            }
        std::printf("      circle outline segs=%zu maxErr=%.3f px\n", c.size(), maxErr);
        check(maxErr < 0.8f, "圆轮廓半径偏差 < 0.8px");

        // 与"读像素颜色"对比：同色/抗锯齿会让颜色判据不可靠，中心命中不会
        check(r.centerHitAt(120, 120), "圆心命中");
        check(!r.centerHitAt(120, 60), "圆外（距圆心 59.5px）未命中");
    }

    // ---- 5. 细线：为什么"有没有被画"必须先定口径 ---------------------------------
    {
        xmlvec::Raster r(120, 40, 0xFFFFFFFFu);
        r.enablePicking(true);
        xmlvec::Stroke st;
        st.enabled = true;
        st.width = 1.0f;
        st.color = 0xFF000000u;
        st.cap = 1;
        st.join = 2;
        xmlvec::Path p;
        p.moveTo(5.0f, 5.0f);          // 斜率 1 的 45° 细线
        p.lineTo(35.0f, 35.0f);
        r.strokePath(p, st, nullptr, 7);

        int painted = 0, centers = 0;
        for (int y = 0; y < r.height(); ++y)
            for (int x = 0; x < r.width(); ++x)
            {
                if (r.paintedAt(x, y)) ++painted;
                if (r.centerHitAt(x, y)) ++centers;
            }
        std::printf("      thin line painted=%d centerHit=%d\n", painted, centers);
        check(painted > 0, "细线有覆盖率记录");
        // 45° 线每行恰好穿过一个单元格中心附近；覆盖率口径命中数会略多于中心口径
        check(centers > 20, "中心口径沿 45° 线仍连续命中（不因整数栅格丢失线段）");

        // 极细横线（0.6px）正好压在两条像素中心之间：覆盖率能看见，中心采样看不见
        xmlvec::Raster r2(120, 40, 0xFFFFFFFFu);
        r2.enablePicking(true);
        xmlvec::Stroke st2 = st;
        st2.width = 0.6f;
        st2.cap = 0;
        xmlvec::Path p2;
        p2.moveTo(5.0f, 20.0f);
        p2.lineTo(115.0f, 20.0f);
        r2.strokePath(p2, st2, nullptr, 8);
        int painted2 = 0, centers2 = 0;
        for (int y = 0; y < r2.height(); ++y)
            for (int x = 0; x < r2.width(); ++x)
            {
                if (r2.paintedAt(x, y)) ++painted2;
                if (r2.centerHitAt(x, y)) ++centers2;
            }
        std::printf("      flat thin line painted=%d centerHit=%d\n", painted2, centers2);
        check(painted2 > 100 && centers2 == 0,
              "落在行间的细线：覆盖率看得见、中心采样看不见 —— 这就是口径差异");
    }

    std::printf("outline_test: FAIL=%d\n", g_fail);
    return g_fail == 0 ? 0 : 1;
}
