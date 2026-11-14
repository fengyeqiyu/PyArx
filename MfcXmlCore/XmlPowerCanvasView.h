#pragma once
// XmlPowerCanvasView —— 配电规划图画布的控件模型（控件库，header-only，无 MFC 依赖）。
// 与 XRibbonModel 一样：只持有"数据 + 视图状态"，负责解析、布局、绘制、命中、缩放平移。
// 宿主（设计器表面 / Viewer / 运行时）只负责给它一个客户区矩形，不再自己算排版或另画一套。
//
// XML 用法：
//   <PowerCanvas Name="Plan" Left="0" Top="0" Width="900" Height="560"
//                Data="{...nodes+layout...}" ShowCable="1" Fit="1"/>
// Data 以 '{' 开头：优先当"已算好坐标的场景"（parseScene），失败再当 nodes 由 buildPlan 排版。
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include "XmlDraw.h"
#include "XmlPowerPlan.h"
#include "XmlPowerPlanDraw.h"

namespace xmlctl
{
    struct PowerCanvasView
    {
        xmlplan::PowerPlan plan;
        std::wstring data;              // Data 属性原文（JSON）
        std::wstring sel;               // 选中节点编号
        std::wstring error;             // 解析失败原因
        float zoom = 1.0f;              // 视图缩放
        int   ox = 0, oy = 0;           // 视口偏移（客户区像素）
        bool  showCable = true;         // 连线标注电缆
        bool  showGrid = false;         // 纸面网格（每 100 单位）
        bool  showRuler = false;        // 左/上标尺
        bool  loaded = false;
        bool  fitPending = true;        // 下一帧按客户区 fit
        int   lastW = 0, lastH = 0;     // 上次客户区尺寸（变了就重新 fit）

        // ---- 解析：'{' 开头按 JSON 处理（场景优先，其次 nodes 排版） ----
        bool load(const std::wstring& d)
        {
            data = d;
            loaded = false;
            error.clear();
            fitPending = true;
            if (d.empty())
            {
                error = L"Data 为空";
                return false;
            }
            if (d[0] != L'{')
            {
                error = L"Data 只支持 JSON（以 { 开头）";
                return false;
            }
            if (xmlplan::parseScene(d, plan, error) && !plan.boxes.empty())
            {
                loaded = true;
                return true;
            }
            xmlplan::PowerPlan p2;
            std::wstring t, err2;
            std::vector<xmlplan::PowerNode> nodes;
            if (xmlplan::parseJson(d, nodes, t, err2, nullptr))
            {
                std::wstring layoutErr;
                if (xmlplan::buildPlan(nodes, p2, xmlplan::PowerPlanStyle(), layoutErr, false))
                {
                    plan = p2;
                    loaded = true;
                    error.clear();
                    return true;
                }
                error = layoutErr;
                return false;
            }
            if (error.empty()) error = err2.empty() ? L"JSON 解析失败" : err2;
            return false;
        }

        // ---- 适应窗口：整张图放进客户区（留 8% 边），上下左右居中 ----
        void fitTo(const RECT& rc)
        {
            lastW = rc.right - rc.left;
            lastH = rc.bottom - rc.top;
            fitPending = false;
            if (plan.width <= 0 || plan.height <= 0 || lastW <= 32 || lastH <= 32)
            {
                zoom = 1.0f;
                ox = oy = 0;
                return;
            }
            const float fx = static_cast<float>(lastW - 24) / static_cast<float>(plan.width);
            const float fy = static_cast<float>(lastH - 24) / static_cast<float>(plan.height);
            float z = (fx < fy) ? fx : fy;
            z *= 0.92f;
            if (z < 0.05f) z = 0.05f;
            if (z > 4.0f) z = 4.0f;
            zoom = z;
            ox = rc.left + (lastW - static_cast<int>(lroundf(plan.width * zoom))) / 2;
            oy = rc.top + (lastH - static_cast<int>(lroundf(plan.height * zoom))) / 2;
        }

        // ---- 绘制：纸面 / 回路 / 卡片，画布底色先铺满客户区 ----
        void paint(XmlCanvas& cv, const RECT& rc, unsigned bg = 0x00000000)
        {
            if (rc.right <= rc.left || rc.bottom <= rc.top) return;
            if (fitPending || (rc.right - rc.left) != lastW || (rc.bottom - rc.top) != lastH)
                fitTo(rc);
            cv.fillRect(rc, bg);
            if (!loaded) return;
            // 真裁剪：放大后卡片不许溢出画布矩形（此前后端只做了"整项不可见就跳过"）
            cv.clipPush(rc);
            RECT clip = rc;
            xmlplan::drawPlan(cv, plan, zoom, ox, oy, sel, showCable, &clip, showGrid);
            if (showRuler) xmlplan::drawRuler(cv, plan, zoom, ox, oy, rc);
            cv.clipPop();
        }

        // ---- 命中：先卡片，再回路（点到线 = 选中它的下游节点） ----
        std::wstring hit(int x, int y) const
        {
            if (!loaded || zoom <= 0.0f) return {};
            const float mx = (static_cast<float>(x) - ox) / zoom;
            const float my = (static_cast<float>(y) - oy) / zoom;
            for (const xmlplan::PowerBox& b : plan.boxes)
            {
                if (mx >= b.x && mx <= b.x + b.w && my >= b.y && my <= b.y + b.h)
                    return b.code;
            }
            const float tol = 8.0f / zoom;
            float best = 3.4e38f;
            std::wstring bestCode;
            for (const xmlplan::PowerLink& l : plan.links)
            {
                float px[4], py[4];
                int n = 0;
                for (int q = 0; q < 4; ++q)
                {
                    const float cx = static_cast<float>(l.bez[q * 2]);
                    const float cy = static_cast<float>(l.bez[q * 2 + 1]);
                    if (n > 0 && px[n - 1] == cx && py[n - 1] == cy) continue;
                    px[n] = cx; py[n] = cy; ++n;
                }
                for (int i = 1; i < n; ++i)
                {
                    const float ax = px[i - 1], ay = py[i - 1];
                    const float bx = px[i], by = py[i];
                    const float vx = bx - ax, vy = by - ay;
                    const float len2 = vx * vx + vy * vy;
                    float t0 = 0.0f;
                    if (len2 > 1e-6f)
                        t0 = ((mx - ax) * vx + (my - ay) * vy) / len2;
                    if (t0 < 0.0f) t0 = 0.0f;
                    if (t0 > 1.0f) t0 = 1.0f;
                    const float qx = ax + vx * t0, qy = ay + vy * t0;
                    const float d2 = (mx - qx) * (mx - qx) + (my - qy) * (my - qy);
                    if (d2 < best) { best = d2; bestCode = l.to; }
                }
            }
            if (best <= tol * tol) return bestCode;
            return {};
        }

        // ---- 滚轮缩放：以光标为锚点，缩放前后同一模型点仍落在光标下 ----
        void wheelAt(int x, int y, short delta)
        {
            if (!loaded) return;
            const float z0 = (zoom > 0.001f) ? zoom : 1.0f;
            float z1 = z0 * static_cast<float>(std::pow(1.15, static_cast<double>(delta) / 120.0));
            setZoomAbout(x, y, z1);
        }

        // 指定倍率缩放（按钮 100% / 放大 / 缩小共用），锚点同样锁在 (x, y)
        void setZoomAbout(int x, int y, float z1)
        {
            if (!loaded) return;
            if (z1 < 0.05f) z1 = 0.05f;
            if (z1 > 4.0f) z1 = 4.0f;
            const float z0 = (zoom > 0.001f) ? zoom : 1.0f;
            if (z1 == z0) return;
            const float mx = (static_cast<float>(x) - ox) / z0;
            const float my = (static_cast<float>(y) - oy) / z0;
            zoom = z1;
            ox = x - static_cast<int>(lroundf(mx * z1));
            oy = y - static_cast<int>(lroundf(my * z1));
        }

        void panBy(int dx, int dy) { ox += dx; oy += dy; }
    };
}
