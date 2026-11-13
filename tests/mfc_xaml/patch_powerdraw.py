# -*- coding: utf-8 -*-
"""一次性补丁：把配电图绘制抽成一套共用函数（屏幕自绘 + 离屏出图共用）。

替换三处：
1) 在 PyMfcHost 之前插入 drawPowerPlanContent（自由函数）
2) drawPlanCanvas 改为调用它
3) RenderPowerPlanPng 的内绘段改为调用它
"""
import io

SRC = r'G:\PyArx\PyMfcXaml\PyMfcXaml.cpp'

DRAW = r'''// -----------------------------------------------------------------------------
// 配电图绘制：屏幕自绘画布与离屏出图共用同一套画法。
// 参数 k = 缩放（离屏传 scale），(ox, oy) = 视口偏移（离屏传 0,0）。
// 与 HTML 版 power-plan.html 的 buildGraphString 逐项对齐：纸面 / 回路 / 电缆标注 /
// 卡片（root 深边加粗、terminal 浅蓝底）/ 电压角标 / 三行文字 / 折叠 +N。
// -----------------------------------------------------------------------------
static void drawPowerPlanContent(XamlCanvas& cv, const xamlplan::PowerPlan& plan,
                                 float k, int ox, int oy, const std::wstring& sel)
{
    if (k <= 0.0f) k = 1.0f;
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

    // 选中联动：父链 + 整棵子树的边高亮（与 HTML 的 applySelectionClasses 一致）
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

    // 回路：三次贝塞尔 → 折线（纵向=竖直 S，横向=水平 S）；先画线，卡片压上面
    for (const xamlplan::PowerLink& l : plan.links)
    {
        std::vector<xamlgeom::Point2> pts;
        xamlgeom::bezierPoints(xamlgeom::Point2(l.bez[0], l.bez[1]),
                               xamlgeom::Point2(l.bez[2], l.bez[3]),
                               xamlgeom::Point2(l.bez[4], l.bez[5]),
                               xamlgeom::Point2(l.bez[6], l.bez[7]), 16, pts);
        std::vector<POINT> poly;
        poly.reserve(pts.size());
        for (const xamlgeom::Point2& p : pts)
            poly.push_back(POINT{ X(p.x), Y(p.y) });

        const bool hot = isHot(l.from + L">" + l.to);
        const int lineW = hot ? (std::max)(2, PX(2.6)) : (std::max)(1, PX(2.0));
        cv.strokePath(poly, hot ? RGB(0xE1, 0x57, 0x59) : RGB(0x2F, 0x6F, 0xBA), lineW, false);

        // 连线标注电缆：横向挂在中点上方，纵向挂在中点右侧
        if (!l.cable.empty() && 10.0 * k >= 3.0)
        {
            cv.setTextStyle(L"Normal", L"Normal", L"NoWrap", L"Center");
            const int mx = X((l.x1 + l.x2) * 0.5);
            const int my = Y((l.y1 + l.y2) * 0.5);
            if (l.vertical)
            {
                const int h = PX(14);
                cv.text(RECT{ mx - PX(70), my - PX(6) - h, mx + PX(70), my - PX(6) },
                        l.cable, RGB(0x2F, 0x6F, 0xBA), DT_CENTER, ptOf(10.0), face);
            }
            else
            {
                const int h = PX(14);
                cv.text(RECT{ mx + PX(6), my - h / 2, mx + PX(6) + PX(170), my + h / 2 },
                        l.cable, RGB(0x2F, 0x6F, 0xBA), DT_LEFT, ptOf(10.0), face);
            }
        }
    }

    // 卡片：电压角标 → 三行文字 → 折叠标记
    for (size_t i = 0; i < plan.boxes.size() && i < plan.nodes.size(); ++i)
    {
        const xamlplan::PowerBox& b = plan.boxes[i];
        const xamlplan::PowerNode& nd = plan.nodes[i];
        const bool isRoot = (nd.role == L"root");
        const bool isTerm = (nd.role == L"terminal");
        const bool isSel = !sel.empty() && sel == nd.code;

        const RECT brc = { X(b.x), Y(b.y), X(b.x + b.w), Y(b.y + b.h) };
        const int radius = (std::max)(2, PX(2.0));
        COLORREF border = isRoot ? RGB(0x22, 0x22, 0x22)
                                 : (isTerm ? RGB(0x74, 0x86, 0x9C) : RGB(0x8A, 0x94, 0xA6));
        if (isSel) border = RGB(0xE1, 0x57, 0x59);
        const COLORREF fill = isSel ? RGB(0xFD, 0xF3, 0xF3)
                                    : (isTerm ? RGB(0xEA, 0xF2, 0xFB) : RGB(0xFF, 0xFF, 0xFF));
        cv.fillRoundRect(brc, fill, border, radius);
        cv.strokeRoundRect(brc, border, radius,
                           (std::max)(1, PX(isSel ? 2.0 : (isRoot ? 2.0 : 1.0))));

        // 电压角标（右上角）：380V 蓝 / 220V 棕 / 中压 teal / 高压 深红
        const xamlplan::VoltageStyle vs = xamlplan::voltStyle(nd.voltage);
        if (!vs.text.empty() && 8.5 * k >= 3.0)
        {
            const int bw = PX(8) + static_cast<int>(units(vs.text) * 4.6 * k);
            const int bh = (std::max)(1, PX(11.0));
            const RECT vrc = { X(b.x + b.w) - bw - PX(4), Y(b.y) + PX(3),
                               X(b.x + b.w) - PX(4), Y(b.y) + PX(3) + bh };
            cv.fillRoundRect(vrc, vs.bg, vs.color, (std::max)(1, PX(2.0)));
            cv.setTextStyle(L"SemiBold", L"Normal", L"NoWrap", L"Center");
            cv.text(vrc, vs.text, vs.color, DT_CENTER, ptOf(8.5), face);
        }

        // 三行文字：编号 / 名称 / 电缆·负载·长度
        if (nd.code.empty() || 14.0 * k < 3.0)
            continue;
        std::vector<std::wstring> lines;
        lines.push_back(nd.code);
        if (!nd.name.empty()) lines.push_back(nd.name);
        const std::wstring l3 = xamlplan::cableLine(nd);
        if (!l3.empty()) lines.push_back(l3);

        const int blockH = 16 + 13 * (static_cast<int>(lines.size()) - 1);
        const int top0 = static_cast<int>(lroundf((b.h - blockH) * 0.5f));
        for (size_t li = 0; li < lines.size(); ++li)
        {
            const bool first = (li == 0);
            if (!first && 10.5 * k < 3.0) break;
            const int bx = X(b.x + 3);
            const int bw = X(b.x + b.w - 3) - bx;
            const int top = Y(b.y + top0 + static_cast<int>(li) * 13);
            const int bot = Y(b.y + top0 + static_cast<int>(li) * 13 + 13);
            if (first) cv.setTextStyle(L"SemiBold", L"Normal", L"NoWrap", L"Center");
            else cv.setTextStyle(L"Normal", L"Normal", L"NoWrap", L"Center");
            const COLORREF color = first ? RGB(0x1F, 0x2A, 0x37)
                                         : (li == 1 ? RGB(0x5B, 0x66, 0x75) : RGB(0x1F, 0x64, 0xB0));
            cv.text(RECT{ bx, top, bx + bw, bot }, lines[li], color, DT_CENTER,
                    ptOf(first ? (isRoot ? 14.0 : 13.0) : 10.5), face);
        }

        // 折叠标记 "+ N"
        if (nd.collapsedCount > 0 && 9.0 * k >= 3.0)
        {
            wchar_t buf[32] = { 0 };
            swprintf_s(buf, L"+ %d", nd.collapsedCount);
            cv.setTextStyle(L"Normal", L"Normal", L"NoWrap", L"Center");
            cv.text(RECT{ X(b.x + b.w - 40), Y(b.y + 1), X(b.x + b.w - 5), Y(b.y + 14) },
                    buf, RGB(0x8A, 0x94, 0xA6), DT_RIGHT, ptOf(9.0), face);
        }
    }
}

'''

NEW_DRAWPLAN = r'''    // 画整体内容：页面底 → （SVG 场景 | 配电图）全部交给同一套绘制函数
    void drawPlanCanvas(XamlCanvas& cv, int id, const RECT& rc)
    {
        auto it = m_planCv.find(id);
        if (it == m_planCv.end())
            return;
        PlanCanvasState& st = it->second;
        cv.fillRect(rc, RGB(0xF3, 0xF5, 0xF8));
        if (st.mode == 0)
        {
            // 空画布：给一句提示，免得看起来像"没画出来"
            cv.setTextStyle(L"Normal", L"Normal", L"NoWrap", L"Center");
            cv.text(rc, L"（配电画布：等待数据）", RGB(0x8A, 0x94, 0xA6), DT_CENTER, 10, L"Microsoft YaHei");
            return;
        }
        if (st.mode == 2)
        {
            xamlsvg::drawSvgScene(cv, st.svg, st.zoom, st.ox, st.oy);
            return;
        }
        drawPowerPlanContent(cv, st.plan, st.zoom, st.ox, st.oy, st.sel);
    }


'''


def main():
    with io.open(SRC, 'r', encoding='utf-8-sig', newline='') as f:
        text = f.read()

    # 1) 插入共用绘制函数（放在 PyMfcHost 之前的文件作用域）
    anchor1 = '// 重画这个控件自己（自绘控件在自己的 WM_PAINT 里画，所以只失效自己）'
    assert text.count(anchor1) == 1, 'anchor1 不唯一'
    text = text.replace(anchor1, DRAW + anchor1, 1)

    # 2) 换掉 drawPlanCanvas
    start2 = '    // 画整体内容：底 → 纸面 → 曲线 → 卡片 → 文字。与离屏渲染（RenderPowerPlanPng）'
    end2 = '    // GridView 格子尺寸覆盖（0 = 自适应'
    i = text.find(start2)
    j = text.find(end2, i)
    assert i > 0 and j > i, 'drawPlanCanvas 区间定位失败'
    text = text[:i] + NEW_DRAWPLAN + text[j:]

    # 3) 离屏渲染改为调用共用绘制（锚点要落在 RenderPowerPlanPng 内，
    #    因为新插入的共用函数里也有同名的 face / ptOf）
    rpn = text.find('bool RenderPowerPlanPng(')
    assert rpn > 0, '找不到 RenderPowerPlanPng'
    start3 = text.find('            const std::wstring face = L"Microsoft YaHei";', rpn)
    end3 = text.find('            canvas.end();', start3)
    assert start3 > rpn and end3 > start3, 'RenderPowerPlanPng 区间定位失败'
    # 3) 离屏渲染改为调用共用绘制（锚点要落在 RenderPowerPlanPng 内，
    #    因为新插入的共用函数里也有同名的 face / ptOf）
    rpn = text.find('bool RenderPowerPlanPng(')
    assert rpn > 0, '找不到 RenderPowerPlanPng'
    start3 = text.find('            const std::wstring face = L"Microsoft YaHei";', rpn)
    end3 = text.find('            canvas.end();', start3)
    assert start3 > rpn and end3 > start3, 'RenderPowerPlanPng 区间定位失败'
    text = text[:start3] + ('            drawPowerPlanContent(canvas, plan, static_cast<float>(scale), 0, 0,\r\n'
                            '                                std::wstring());\r\n\r\n') + text[end3:]

    with io.open(SRC, 'w', encoding='utf-8-sig', newline='') as f:
        f.write(text)
    print('补丁完成：%d 字符' % len(text))


if __name__ == '__main__':
    main()
