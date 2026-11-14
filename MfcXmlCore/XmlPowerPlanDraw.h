#pragma once
// 配电规划图的绘制层（控件库 render 层，header-only）。
// 屏幕自绘画布、Viewer 预览、设计器表面、离屏出 PNG 全部走这一份画法 ——
// 以前这套画法只住在 PyMfcXml 的 Python 绑定里，设计器和预览器根本取不到，
// 三端自然对不上；下沉到 MfcXmlCore 之后只依赖 XmlCanvas（XmlDraw.h）。
#include <algorithm>
#include <cmath>
#include <map>
#include <string>
#include <vector>

#include "XmlPowerPlan.h"

namespace xmlplan
{
    // k = 缩放，(ox, oy) = 视口偏移；clip 为客户区矩形（视口裁剪用），nullptr = 不裁剪。
    // 与 HTML 版 power-plan.html 逐项对齐：纸面 / 回路 / 电缆标注 / 卡片（root 深边加粗、
    // terminal 浅蓝底）/ 电压角标 / 三行文字 / 折叠 +N。
    inline void drawPlan(XmlCanvas& cv, const PowerPlan& plan,
                         float k, int ox, int oy, const std::wstring& sel,
                         bool showCable = true, const RECT* clip = nullptr)
    {
        if (k <= 0.0f) k = 1.0f;
        auto offClip = [&](int x0, int y0, int x1, int y1) {
            if (clip == nullptr) return false;
            return x1 < clip->left || x0 > clip->right || y1 < clip->top || y0 > clip->bottom;
        };
        const std::wstring face = L"Microsoft YaHei";
        auto X = [&](double v) { return ox + static_cast<int>(lroundf(static_cast<float>(v) * k)); };
        auto Y = [&](double v) { return oy + static_cast<int>(lroundf(static_cast<float>(v) * k)); };
        auto PX = [&](double v) { return static_cast<int>(lroundf(static_cast<float>(v) * k)); };
        auto ptOf = [&](double px) {
            const int pt = static_cast<int>(px * k * 0.75 + 0.5);
            return pt > 0 ? pt : 1;
        };
        auto units = [](const std::wstring& s) {
            int u = 0;
            for (wchar_t c : s) u += (c >= 0x2E80) ? 2 : 1;
            return u;
        };

        // 纸面
        const RECT paper = { ox, oy, X(plan.width), Y(plan.height) };
        cv.fillRect(paper, RGB(0xFF, 0xFF, 0xFF));
        cv.strokeRect(paper, RGB(0xCF, 0xD6, 0xDF), 1);

        // 选中联动：父链 + 整棵子树的边高亮
        std::map<std::wstring, size_t> idx;
        std::vector<size_t> up(plan.nodes.size(), (size_t)-1);
        std::vector<std::vector<size_t>> kids(plan.nodes.size());
        for (size_t i = 0; i < plan.nodes.size(); ++i) idx[plan.nodes[i].code] = i;
        for (size_t i = 0; i < plan.nodes.size(); ++i)
        {
            auto it = idx.find(plan.nodes[i].upstream);
            if (it != idx.end() && it->second != i)
            {
                up[i] = it->second;
                kids[it->second].push_back(i);
            }
        }
        std::vector<std::wstring> hiEdges;
        if (!sel.empty())
        {
            auto it = idx.find(sel);
            if (it != idx.end())
            {
                size_t cur = it->second;
                while (up[cur] != (size_t)-1)
                {
                    hiEdges.push_back(plan.nodes[up[cur]].code + L">" + plan.nodes[cur].code);
                    cur = up[cur];
                }
                std::vector<size_t> stack(1, it->second);
                while (!stack.empty())
                {
                    const size_t i = stack.back();
                    stack.pop_back();
                    for (size_t c : kids[i])
                    {
                        hiEdges.push_back(plan.nodes[i].code + L">" + plan.nodes[c].code);
                        stack.push_back(c);
                    }
                }
            }
        }
        auto isHot = [&](const std::wstring& key) {
            for (const std::wstring& e : hiEdges) if (e == key) return true;
            return false;
        };

        // 回路：正交折线（布局给出的四个点就是拐点序列），先画线，卡片压上面
        for (const PowerLink& l : plan.links)
        {
            std::vector<POINT> poly;
            const double raw[4][2] = { { l.bez[0], l.bez[1] }, { l.bez[2], l.bez[3] },
                                       { l.bez[4], l.bez[5] }, { l.bez[6], l.bez[7] } };
            for (int i = 0; i < 4; ++i)
            {
                const POINT pt = { X(raw[i][0]), Y(raw[i][1]) };
                if (!poly.empty() && poly.back().x == pt.x && poly.back().y == pt.y)
                    continue;
                poly.push_back(pt);
            }
            if (!poly.empty())
            {
                int x0 = poly[0].x, x1 = poly[0].x, y0 = poly[0].y, y1 = poly[0].y;
                for (const POINT& pt : poly)
                {
                    if (pt.x < x0) x0 = pt.x; if (pt.x > x1) x1 = pt.x;
                    if (pt.y < y0) y0 = pt.y; if (pt.y > y1) y1 = pt.y;
                }
                if (offClip(x0, y0, x1, y1)) continue;
            }

            const bool hot = isHot(l.from + L">" + l.to);
            const int lineW = hot ? (std::max)(2, PX(2.6)) : (std::max)(1, PX(2.0));
            const COLORREF lineClr = hot ? RGB(0xE1, 0x57, 0x59)
                : (l.magenta ? RGB(0xC8, 0x00, 0xB4) : RGB(0x2F, 0x6F, 0xBA));
            if (l.dashed)
            {
                // 备用电源进线：沿折线走虚线（画布没有 dash 样式，自己按步长切段）
                const double dash = PX(6.0) > 1 ? PX(6.0) : 2.0;
                const double gapLen = PX(4.0) > 1 ? PX(4.0) : 2.0;
                double sPos = 0.0;
                for (size_t si = 1; si < poly.size(); ++si)
                {
                    const POINT a = poly[si - 1];
                    const POINT b = poly[si];
                    const double dx = b.x - a.x, dy = b.y - a.y;
                    const double len = std::sqrt(dx * dx + dy * dy);
                    if (len <= 0.0) continue;
                    double t = 0.0;
                    while (t < len)
                    {
                        const double phase = std::fmod(sPos, dash + gapLen);
                        const double remain = (phase < dash) ? (dash - phase) : (dash + gapLen - phase);
                        double t2 = t + remain;
                        if (t2 > len) t2 = len;
                        if (phase < dash)
                        {
                            const POINT pa = { a.x + static_cast<long>(lround(dx * (t / len))),
                                               a.y + static_cast<long>(lround(dy * (t / len))) };
                            const POINT pb = { a.x + static_cast<long>(lround(dx * (t2 / len))),
                                               a.y + static_cast<long>(lround(dy * (t2 / len))) };
                            cv.strokeLine(pa, pb, lineClr, lineW);
                        }
                        sPos += (t2 - t);
                        t = t2;
                    }
                }
            }
            else
            {
                cv.strokePath(poly, lineClr, lineW, false);
            }

            // 连线标注电缆：贴在"进入卡片的那一段线"上（竖排转 90°，横排压线上沿）
            if (showCable && !l.cable.empty() && 10.0 * k >= 1.5)
            {
                const POINT to = { X(l.x2), Y(l.y2) };
                const POINT prev = poly.size() >= 2 ? poly[poly.size() - 2] : to;
                if (l.vertical)
                {
                    cv.setTextStyle(L"Normal", L"Normal", L"NoWrap", L"Left");
                    POINT pivot = to;
                    cv.pushRotate(-90, pivot);
                    cv.text(RECT{ pivot.x + PX(6), pivot.y - PX(13), pivot.x + PX(226),
                                  pivot.y - PX(1) },
                            l.cable, RGB(0x1F, 0x64, 0xB0), DT_LEFT, ptOf(10.0), face);
                    cv.popTransform();
                }
                else
                {
                    const int lx = (std::min)(prev.x, to.x) + PX(6);
                    cv.setTextStyle(L"Normal", L"Normal", L"NoWrap", L"Left");
                    cv.text(RECT{ lx, to.y - PX(13), lx + PX(220), to.y - PX(1) },
                            l.cable, RGB(0x1F, 0x64, 0xB0), DT_LEFT, ptOf(10.0), face);
                }
            }
        }

        // 卡片：电压角标 → 三行文字 → 折叠标记
        for (size_t i = 0; i < plan.boxes.size() && i < plan.nodes.size(); ++i)
        {
            const PowerBox& b = plan.boxes[i];
            const PowerNode& nd = plan.nodes[i];
            const bool isRoot = (nd.role == L"root");
            const bool isTerm = (nd.role == L"terminal");
            const bool isSel = !sel.empty() && sel == nd.code;

            const RECT brc = { X(b.x), Y(b.y), X(b.x + b.w), Y(b.y + b.h) };
            if (offClip(brc.left, brc.top, brc.right, brc.bottom)) continue;
            const int radius = (std::max)(1, PX(2.0));
            COLORREF border = isRoot ? RGB(0x22, 0x22, 0x22)
                                     : (isTerm ? RGB(0x74, 0x86, 0x9C) : RGB(0x8A, 0x94, 0xA6));
            const bool isDual = nd.dual != 0;
            if (isDual) border = RGB(0xE0, 0x24, 0x24);
            if (isSel) border = RGB(0xE1, 0x57, 0x59);
            const COLORREF fill = isTerm ? RGB(0xEA, 0xF2, 0xFB)
                                         : (isDual ? RGB(0xFF, 0xF3, 0xF3) : RGB(0xFF, 0xFF, 0xFF));
            cv.fillRoundRect(brc, fill, border, radius);
            const int spanPx = brc.right - brc.left;
            if (spanPx < 10)
                continue;
            cv.strokeRoundRect(brc, border, radius,
                               (std::max)(1, PX(isSel ? 2.2 : (isRoot || isDual ? 2.0 : 1.0))));

            // 电压角标（右上角）：380V 蓝 / 220V 棕 / 中压 teal / 高压 深红
            const VoltageStyle vs = voltStyle(nd.voltage);
            if (!vs.text.empty() && 8.5 * k >= 1.5)
            {
                const int bw = PX(8) + static_cast<int>(units(vs.text) * 4.6 * k);
                const int bh = (std::max)(1, PX(11.0));
                const RECT vrc = { X(b.x + b.w) - bw - PX(4), Y(b.y) + PX(3),
                                   X(b.x + b.w) - PX(4), Y(b.y) + PX(3) + bh };
                cv.fillRoundRect(vrc, vs.bg, vs.color, (std::max)(1, PX(2.0)));
                cv.setTextStyle(L"SemiBold", L"Normal", L"NoWrap", L"Center");
                cv.text(vrc, vs.text, vs.color, DT_CENTER, ptOf(8.5), face);
            }

            // 正文：设备名称 / 功率·电压等级 / 电缆型号·长度
            if (nd.code.empty() || 14.0 * k < 1.5)
                continue;
            std::vector<std::wstring> lines;
            lines.push_back(nd.name.empty() ? nd.code : nd.name);
            std::wstring l2;
            {
                wchar_t buf[48] = { 0 };
                if (nd.loadKw > 0.0)
                {
                    swprintf_s(buf, L"%.3g kW", nd.loadKw);
                    l2 = buf;
                }
            }
            if (!vs.text.empty()) l2 += (l2.empty() ? L"" : L" · ") + vs.text;
            if (!l2.empty()) lines.push_back(l2);
            std::wstring l3 = nd.cable;
            if (nd.length > 0.0)
            {
                wchar_t buf[48] = { 0 };
                swprintf_s(buf, L"%.3g m", nd.length);
                if (!l3.empty()) l3 += L" · ";
                l3 += buf;
            }
            if (!l3.empty()) lines.push_back(l3);

            const int blockH = 16 + 13 * (static_cast<int>(lines.size()) - 1);
            const int top0 = static_cast<int>(lroundf((b.h - blockH) * 0.5f));
            for (size_t li = 0; li < lines.size(); ++li)
            {
                const bool first = (li == 0);
                if (!first && 10.5 * k < 1.5) break;
                const int bx = X(b.x + 3);
                const int bw = X(b.x + b.w - 3) - bx;
                const int top = Y(b.y + top0 + static_cast<int>(li) * 13);
                const int bot = Y(b.y + top0 + static_cast<int>(li) * 13 + 13);
                if (first) cv.setTextStyle(L"SemiBold", L"Normal", L"NoWrap", L"Center");
                else cv.setTextStyle(L"Normal", L"Normal", L"NoWrap", L"Center");
                const COLORREF color = first
                    ? (isDual ? RGB(0xC0, 0x1B, 0x1B) : RGB(0x1F, 0x2A, 0x37))
                    : RGB(0x1F, 0x64, 0xB0);
                cv.text(RECT{ bx, top, bx + bw, bot }, lines[li], color, DT_CENTER,
                        ptOf(first ? (isRoot ? 14.0 : 13.0) : 10.5), face);
            }

            // 折叠标记 "+ N"（与既有实现一致：卡片右上角右对齐）
            if (nd.collapsedCount > 0 && 9.0 * k >= 1.5)
            {
                wchar_t buf[32] = { 0 };
                swprintf_s(buf, L"+ %d", nd.collapsedCount);
                cv.setTextStyle(L"Normal", L"Normal", L"NoWrap", L"Right");
                cv.text(RECT{ X(b.x + b.w - 46), Y(b.y + 1), X(b.x + b.w - 6), Y(b.y + 14) },
                        buf, RGB(0x8A, 0x94, 0xA6), DT_RIGHT, ptOf(9.0), face);
            }
        }
    }
}
