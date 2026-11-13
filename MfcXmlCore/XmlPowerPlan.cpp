// 配电规划图实现：树布局（纵向/横向两种走向）+ 四个出口（XML / JSON / CSV / SVG）。
// 布局规则与 HTML 版 power-plan.html 完全同构，因此两边出图的坐标可直接对照。
#include "XmlPowerPlan.h"
#include "XmlGeometry.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <map>
#include <unordered_map>
#include <sstream>

namespace xmlplan
{
    namespace
    {
        // ---------------- 文本小工具 ----------------
        std::wstring trimWs(const std::wstring& s)
        {
            size_t b = 0, e = s.size();
            while (b < e && (s[b] == L' ' || s[b] == L'\t' || s[b] == L'\r' || s[b] == L'\n')) ++b;
            while (e > b && (s[e - 1] == L' ' || s[e - 1] == L'\t' || s[e - 1] == L'\r' || s[e - 1] == L'\n')) --e;
            return s.substr(b, e - b);
        }

        std::vector<std::wstring> splitLines(const std::wstring& text)
        {
            std::vector<std::wstring> lines;
            std::wstring cur;
            for (wchar_t c : text)
            {
                if (c == L'\r') continue;
                if (c == L'\n')
                {
                    lines.push_back(cur);
                    cur.clear();
                }
                else
                    cur += c;
            }
            if (!cur.empty() || !text.empty())
                lines.push_back(cur);
            return lines;
        }

        // CSV 拆分：支持双引号包住含逗号的字段，双引号内 "" 表示一个引号
        std::vector<std::wstring> splitCsvLine(const std::wstring& line, wchar_t sep)
        {
            std::vector<std::wstring> out;
            std::wstring cur;
            bool inQuote = false;
            for (size_t i = 0; i < line.size(); ++i)
            {
                const wchar_t c = line[i];
                if (inQuote)
                {
                    if (c == L'"')
                    {
                        if (i + 1 < line.size() && line[i + 1] == L'"') { cur += L'"'; ++i; }
                        else inQuote = false;
                    }
                    else cur += c;
                }
                else if (c == L'"') inQuote = true;
                else if (c == sep) { out.push_back(trimWs(cur)); cur.clear(); }
                else cur += c;
            }
            out.push_back(trimWs(cur));
            return out;
        }

        std::wstring xmlEscape(const std::wstring& s)
        {
            std::wstring r;
            r.reserve(s.size() + 8);
            for (wchar_t c : s)
            {
                switch (c)
                {
                case L'&': r += L"&amp;"; break;
                case L'<': r += L"&lt;"; break;
                case L'>': r += L"&gt;"; break;
                case L'"': r += L"&quot;"; break;
                default: r += c;
                }
            }
            return r;
        }

        std::wstring jsonEscape(const std::wstring& s)
        {
            std::wstring r;
            for (wchar_t c : s)
            {
                switch (c)
                {
                case L'"': r += L"\\\""; break;
                case L'\\': r += L"\\\\"; break;
                case L'\n': r += L"\\n"; break;
                case L'\r': r += L"\\r"; break;
                case L'\t': r += L"\\t"; break;
                default: r += c; break;
                }
            }
            return r;
        }

        // 文字显示宽度（按"半角单位"估）：CJK 记 2，其余记 1
        int textUnits(const std::wstring& s)
        {
            int u = 0;
            for (wchar_t c : s)
                u += (c >= 0x2E80) ? 2 : 1;
            return u;
        }

        // 上游可以写多个编号（双电源）：用 | , 、 ; / 分隔，第一个当主电源
        std::vector<std::wstring> splitUpstream(const std::wstring& raw)
        {
            std::vector<std::wstring> out;
            std::wstring cur;
            auto flush = [&]() {
                const std::wstring t = trimWs(cur);
                if (!t.empty()) out.push_back(t);
                cur.clear();
            };
            for (wchar_t c : raw)
            {
                if (c == L'|' || c == L',' || c == L'，' || c == L'、' || c == L';' ||
                    c == L'；' || c == L'/' || c == L'+')
                    flush();
                else
                    cur.push_back(c);
            }
            flush();
            return out;
        }

        // 电缆标注文字占用的画布宽度（10px 字号：半角 ≈5.6，CJK 记 2 单位）
        // 标注要画在"进入卡片的最后一段线"上，这一段线的长度必须由布局保证。
        int cableLabelWidth(const std::wstring& s)
        {
            return s.empty() ? 0 : textUnits(s) * 6 + 10;
        }

        std::wstring lowerAscii(const std::wstring& s)
        {
            std::wstring r = s;
            for (wchar_t& c : r)
            {
                if (c >= L'A' && c <= L'Z') c = (wchar_t)(c - L'A' + L'a');
                else if (c == L' ' || c == L'\t' || c == L'\r' || c == L'\n') c = L' ';
            }
            return r;
        }

        // 数字转文字：整数不带小数点（2 → "2"，2.5 → "2.5"）
        std::wstring numText(double v)
        {
            if (std::floor(v + 0.5) == v && std::fabs(v) < 1e12)
            {
                wchar_t buf[32] = { 0 };
                swprintf_s(buf, L"%ld", (long)v);
                return buf;
            }
            wchar_t buf[48] = { 0 };
            swprintf_s(buf, L"%.3g", v);
            return buf;
        }

        // 卡片尺寸：宽 = max(基准, 20 + 最大半角单位 × 8)，高 = 基准 + (有名称 ? nameH : 0)
        struct CardSize { int w = 0; int h = 0; };

        std::vector<std::wstring> nodeLines(const PowerNode& n)
        {
            // 卡片正文：设备名称 / 功率·电压等级（用户口径）
            std::vector<std::wstring> out;
            out.push_back(n.name.empty() ? n.code : n.name);
            std::wstring l2;
            if (n.loadKw > 0.0) l2 = numText(n.loadKw) + L" kW";
            const VoltageStyle vs = voltStyle(n.voltage);
            if (!vs.text.empty()) l2 += (l2.empty() ? L"" : L" · ") + vs.text;
            if (!l2.empty()) out.push_back(l2);
            // 第三行：电缆型号 + 合计长度（用户口径）
            std::wstring l3 = n.cable;
            if (n.length > 0.0)
            {
                if (!l3.empty()) l3 += L" · ";
                l3 += numText(n.length) + L" m";
            }
            if (!l3.empty()) out.push_back(l3);
            return out;
        }

        CardSize cardSizeFor(const PowerNode& n, const PowerPlanStyle& st)
        {
            const bool term = (n.role == L"terminal");
            int maxU = 0;
            for (const std::wstring& t : nodeLines(n))
                maxU = (std::max)(maxU, textUnits(t));
            CardSize s;
            s.w = (std::max)(term ? st.termW : st.boxW, 20 + maxU * 8);
            const int nl = static_cast<int>(nodeLines(n).size());
            s.h = (term ? st.termH : st.boxH) + (n.name.empty() ? 0 : st.nameH)
                + 13 * (std::max)(0, nl - 2);
            return s;
        }
    }

    // ======================= 电压工具 =======================
    std::wstring normVoltage(const std::wstring& raw)
    {
        const std::wstring s = trimWs(raw);
        if (s.empty()) return L"380";
        std::wstring lower;
        for (wchar_t c : s)
            if (c != L' ' && c != L'\t' && c != L'\r' && c != L'\n') lower += c;

        // 先认 kV / 千伏（如 10kV、35KV、110千伏）
        {
            const std::wstring lo = lowerAscii(lower);
            const size_t p = lo.find(L"kv");
            const size_t p2 = lower.find(L"千伏");
            size_t at = (p != std::wstring::npos) ? p : p2;
            if (at != std::wstring::npos)
            {
                // 向左取数字（含小数点）
                size_t b = at;
                while (b > 0 && (iswdigit(lower[b - 1]) || lower[b - 1] == L'.')) --b;
                if (b < at)
                {
                    std::wstring num = lower.substr(b, at - b);
                    while (!num.empty() && num.back() == L'0' && num.find(L'.') != std::wstring::npos)
                        num.pop_back();
                    if (!num.empty() && num.back() == L'.') num.pop_back();
                    return num + L"kV";
                }
            }
        }
        const std::wstring lo = lowerAscii(lower);
        if (lo.find(L"380") != std::wstring::npos || lower.find(L"三相") != std::wstring::npos ||
            lo.find(L"3ph") != std::wstring::npos)
            return L"380";
        if (lo.find(L"220") != std::wstring::npos || lower.find(L"单相") != std::wstring::npos ||
            lo.find(L"1ph") != std::wstring::npos)
            return L"220";
        return L"380";
    }

    int voltClass(const std::wstring& normalized)
    {
        if (normalized == L"220") return 1;
        if (normalized == L"380" || normalized.empty()) return 0;
        const double n = _wtof(normalized.c_str());
        if (!(n > 0)) return 0;
        return n >= 66.0 ? 3 : 2;
    }

    VoltageStyle voltStyle(const std::wstring& normalized)
    {
        VoltageStyle vs;
        switch (voltClass(normalized))
        {
        case 1: vs.text = L"220V"; vs.color = 0xB54708; vs.bg = 0xFEF6E7; break;
        case 2: vs.text = normalized; vs.color = 0x0F766E; vs.bg = 0xE6FFFA; break;
        case 3: vs.text = normalized; vs.color = 0xA8071A; vs.bg = 0xFEF3F2; break;
        default: vs.text = L"380V"; vs.color = 0x1F64B0; vs.bg = 0xE8F1FB; break;
        }
        return vs;
    }

    std::wstring cableLine(const PowerNode& n)
    {
        std::wstring s = n.cable;
        if (n.loadKw > 0.0)
            s += (s.empty() ? L"" : L" · ") + numText(n.loadKw) + L" kW";
        if (n.length > 0.0)
            s += (s.empty() ? L"" : L" · ") + numText(n.length) + L" m";
        return s;
    }

    // ======================= 建树 + 布局 =======================
    bool buildPlan(const std::vector<PowerNode>& nodesIn, PowerPlan& out, const PowerPlanStyle& style,
                   std::wstring& err, bool horizontal)
    {
        out = PowerPlan();
        out.dir = horizontal ? L"h" : L"v";
        err.clear();
        if (nodesIn.empty())
        {
            err = L"没有可用的节点";
            return false;
        }

        // 编号唯一性
        std::map<std::wstring, size_t> index;
        out.nodes.reserve(nodesIn.size());
        for (size_t i = 0; i < nodesIn.size(); ++i)
        {
            PowerNode n = nodesIn[i];
            n.code = trimWs(n.code);
            n.name = trimWs(n.name);
            n.upstream = trimWs(n.upstream);
            n.cable = trimWs(n.cable);
            n.voltage = normVoltage(n.voltage);
            if (n.loadKw < 0.0) n.loadKw = 0.0;
            if (n.length < 0.0) n.length = 0.0;
            if (n.collapsedCount < 0) n.collapsedCount = 0;
            if (n.code.empty())
            {
                err = L"第 " + std::to_wstring(i + 1) + L" 行缺少编号";
                return false;
            }
            if (index.count(n.code))
            {
                err = L"编号重复：" + n.code;
                return false;
            }
            index[n.code] = out.nodes.size();
            out.nodes.push_back(n);
        }

        const size_t n = out.nodes.size();
        const bool HORIZ = horizontal;

        // 父子关系：上游为空或找不到 → 视为起点
        // 双电源：upstream 里第一个有效编号当主电源（参与树排布），其余只补进线。
        std::vector<int> parent(n, -1);
        std::vector<std::vector<size_t>> children(n);
        std::vector<std::vector<size_t>> extraUp(n);
        for (size_t i = 0; i < n; ++i)
        {
            const std::wstring& up = out.nodes[i].upstream;
            if (up.empty()) continue;
            const std::vector<std::wstring> ups = splitUpstream(up);
            bool gotPrimary = false;
            for (size_t ui = 0; ui < ups.size(); ++ui)
            {
                auto it = index.find(ups[ui]);
                if (it == index.end())
                {
                    out.warnings.push_back(L"节点 " + out.nodes[i].code + L" 的上游 " + ups[ui] +
                                           L" 不存在，已忽略");
                    continue;
                }
                if (it->second == i)
                {
                    err = L"上游关系成环，节点：" + out.nodes[i].code;
                    return false;
                }
                if (!gotPrimary)
                {
                    parent[i] = (int)it->second;   // 主电源：参与树排布
                    gotPrimary = true;
                }
                else
                {
                    extraUp[i].push_back(it->second);   // 备用电源：只补进线（虚线）
                }
            }
            if (!gotPrimary)
                out.nodes[i].upstream.clear();   // 上游全都不存在 → 按起点处理
        }
        // 成环检测（两步走会漏，直接沿链走）
        for (size_t i = 0; i < n; ++i)
        {
            size_t steps = 0;
            int p = parent[i];
            while (p >= 0)
            {
                if (++steps > n)
                {
                    err = L"上游关系成环，节点：" + out.nodes[i].code;
                    return false;
                }
                p = parent[(size_t)p];
            }
        }
        for (size_t i = 0; i < n; ++i)
            if (parent[i] >= 0) children[(size_t)parent[i]].push_back(i);

        // 兄弟按编号排序，保证同一份数据每次出图一致
        for (size_t i = 0; i < n; ++i)
        {
            std::sort(children[i].begin(), children[i].end(), [&](size_t a, size_t b) {
                return out.nodes[a].code < out.nodes[b].code;
            });
        }
        std::vector<size_t> roots;
        for (size_t i = 0; i < n; ++i)
            if (parent[i] < 0) roots.push_back(i);
        std::sort(roots.begin(), roots.end(), [&](size_t a, size_t b) {
            return out.nodes[a].code < out.nodes[b].code;
        });

        // 角色：无上游 = root；有可视子或折叠计数 = branch；其余 = terminal
        for (size_t i = 0; i < n; ++i)
        {
            PowerNode& nd = out.nodes[i];
            if (nd.upstream.empty()) nd.role = L"root";
            else if (!children[i].empty() || nd.collapsedCount > 0) nd.role = L"branch";
            else nd.role = L"terminal";
        }

        // 卡片尺寸 + 深度
        std::vector<int> bw(n), bh(n), depth(n, 0);
        for (size_t i = 0; i < n; ++i)
        {
            const CardSize cs = cardSizeFor(out.nodes[i], style);
            bw[i] = cs.w;
            bh[i] = cs.h;
        }
        {
            std::vector<std::pair<size_t, int>> stack;
            for (size_t r : roots) stack.push_back({ r, 0 });
            size_t visited = 0;
            while (!stack.empty())
            {
                auto pr = stack.back();
                stack.pop_back();
                depth[pr.first] = pr.second;
                ++visited;
                for (size_t c : children[pr.first]) stack.push_back({ c, pr.second + 1 });
            }
            if (visited != n)
            {
                err = L"上游关系成环，存在无法归属的节点";
                return false;
            }
        }

        // 子树在主轴上的尺寸（后序）：纵向按宽累加，横向按高累加
        std::vector<int> subW(n, 0);
        {
            std::function<int(size_t)> calc = [&](size_t i) -> int {
                const int own = HORIZ ? bh[i] : bw[i];
                if (children[i].empty())
                {
                    subW[i] = own;
                    return own;
                }
                int sum = 0;
                for (size_t k = 0; k < children[i].size(); ++k)
                    sum += calc(children[i][k]) + (k ? style.gapX : 0);
                subW[i] = (std::max)(own, sum);
                return subW[i];
            };
            for (size_t r : roots) calc(r);
        }

        // 摆放（前序）：子树占 [start, start+subW)；父卡片居中于首末子中心之间
        out.boxes.assign(n, PowerBox());
        for (size_t i = 0; i < n; ++i)
        {
            out.boxes[i].code = out.nodes[i].code;
            out.boxes[i].w = bw[i];
            out.boxes[i].h = bh[i];
            out.boxes[i].depth = depth[i];
        }
        {
            std::function<void(size_t, int)> place = [&](size_t i, int start) {
                PowerBox& box = out.boxes[i];
                if (children[i].empty())
                {
                    if (HORIZ) box.y = start + (subW[i] - box.h) / 2;
                    else box.x = start + (subW[i] - box.w) / 2;
                    return;
                }
                int kidsExt = 0;
                for (size_t k = 0; k < children[i].size(); ++k)
                    kidsExt += subW[children[i][k]] + (k ? style.gapX : 0);
                int cur = start + (subW[i] - kidsExt) / 2;
                const size_t firstKid = children[i].front();
                const size_t lastKid = children[i].back();
                for (size_t c : children[i])
                {
                    place(c, cur);
                    cur += subW[c] + style.gapX;
                }
                if (HORIZ)
                {
                    const PowerBox& f = out.boxes[firstKid];
                    const PowerBox& l = out.boxes[lastKid];
                    const int midY = (f.y + f.h / 2 + l.y + l.h / 2) / 2;
                    box.y = midY - box.h / 2;
                }
                else
                {
                    const PowerBox& f = out.boxes[firstKid];
                    const PowerBox& l = out.boxes[lastKid];
                    const int midX = (f.x + f.w / 2 + l.x + l.w / 2) / 2;
                    box.x = midX - box.w / 2;
                }
            };
            int rootStart = style.margin;
            for (size_t r : roots)
            {
                place(r, rootStart);
                rootStart += subW[r] + style.rootGap;
            }
        }

        // 层级坐标：纵向 = 每层一行（行高取该层最高卡片），横向 = 每层一列（列宽取该层最宽卡片）
        {
            int maxDepth = 0;
            for (size_t i = 0; i < n; ++i) maxDepth = (std::max)(maxDepth, depth[i]);
            std::vector<std::vector<size_t>> byDepth((size_t)maxDepth + 1);
            for (size_t i = 0; i < n; ++i) byDepth[(size_t)depth[i]].push_back(i);

            int cursor = style.margin;
            for (int d = 0; d <= maxDepth; ++d)
            {
                const std::vector<size_t>& bucket = byDepth[(size_t)d];
                if (HORIZ)
                {
                    int colW = style.boxW;
                    int colGap = style.colGap;
                    for (size_t i : bucket)
                    {
                        colW = (std::max)(colW, bw[i]);
                        // 这一层的电缆标注画在进入本层卡片的横线上，层间距不够就会压到卡片
                        colGap = (std::max)(colGap, cableLabelWidth(out.nodes[i].cable) + 16);
                    }
                    for (size_t i : bucket) out.boxes[i].x = cursor;
                    cursor += colW + colGap;
                }
                else
                {
                    int rowH = style.boxH;
                    int rowGap = style.gapY;
                    for (size_t i : bucket)
                    {
                        rowH = (std::max)(rowH, bh[i]);
                        // 上下布局时电缆标注竖排画在进入卡片的竖线上，行间距同样要留够
                        rowGap = (std::max)(rowGap, cableLabelWidth(out.nodes[i].cable) + 16);
                    }
                    for (size_t i : bucket)
                        out.boxes[i].y = cursor + (rowH - bh[i]) / 2;
                    cursor += rowH + rowGap;
                }
            }
        }

        // 连接线 + 贝塞尔控制点
        for (size_t i = 0; i < n; ++i)
        {
            if (parent[i] < 0) continue;
            const PowerBox& pb = out.boxes[(size_t)parent[i]];
            const PowerBox& cb = out.boxes[i];
            PowerLink lk;
            lk.from = pb.code;
            lk.to = cb.code;
            lk.cable = out.nodes[i].cable;
            lk.vertical = HORIZ ? 0 : 1;
            if (HORIZ)
            {
                lk.x1 = pb.x + pb.w;
                lk.y1 = pb.y + pb.h / 2;
                lk.x2 = cb.x;
                lk.y2 = cb.y + cb.h / 2;
                // 拐点尽量靠近卡片：留下 x2 之前这一段横线专门放电缆标注（左对齐、
                // 线长随文字长度走）。层间距已在上面按标注宽度加过，这里只做下限保护。
                const double seg = (std::max)((lk.x2 - lk.x1) * 0.5,
                                              static_cast<double>(cableLabelWidth(lk.cable) + 10));
                double mx = lk.x2 - seg;
                if (mx < lk.x1 + 6) mx = lk.x1 + 6;
                lk.bez[0] = lk.x1; lk.bez[1] = lk.y1;
                lk.bez[2] = mx; lk.bez[3] = lk.y1;
                lk.bez[4] = mx; lk.bez[5] = lk.y2;
                lk.bez[6] = lk.x2; lk.bez[7] = lk.y2;
            }
            else
            {
                lk.x1 = pb.x + pb.w / 2;
                lk.y1 = pb.y + pb.h;
                lk.x2 = cb.x + cb.w / 2;
                lk.y2 = cb.y;
                // 同横向：拐点靠近卡片，y2 之前这段竖线留给竖排的电缆标注
                const double seg = (std::max)((lk.y2 - lk.y1) * 0.5,
                                              static_cast<double>(cableLabelWidth(lk.cable) + 10));
                double my = lk.y2 - seg;
                if (my < lk.y1 + 6) my = lk.y1 + 6;
                lk.bez[0] = lk.x1; lk.bez[1] = lk.y1;
                lk.bez[2] = lk.x1; lk.bez[3] = my;
                lk.bez[4] = lk.x2; lk.bez[5] = my;
                lk.bez[6] = lk.x2; lk.bez[7] = lk.y2;
            }
            out.links.push_back(lk);
        }

        // 双电源的备用进线：主电源之外的上游各补一条虚线进线，不参与布局、不标电缆
        for (size_t i = 0; i < n; ++i)
        {
            if (extraUp[i].empty()) continue;
            const PowerBox& cb = out.boxes[i];
            for (size_t pi : extraUp[i])
            {
                const PowerBox& pb = out.boxes[pi];
                PowerLink lk;
                lk.from = pb.code;
                lk.to = cb.code;
                lk.dashed = 1;
                lk.vertical = HORIZ ? 0 : 1;
                if (HORIZ)
                {
                    // 接在卡片左边靠上一点，避免和主进线完全重合
                    lk.x1 = pb.x + pb.w;
                    lk.y1 = pb.y + pb.h / 2;
                    lk.x2 = cb.x;
                    lk.y2 = cb.y + (std::min)(10, cb.h / 3);
                    const double seg = (std::max)((lk.x2 - lk.x1) * 0.5, 24.0);
                    double mx = lk.x2 - seg;
                    if (mx < lk.x1 + 6) mx = lk.x1 + 6;
                    lk.bez[0] = lk.x1; lk.bez[1] = lk.y1;
                    lk.bez[2] = mx; lk.bez[3] = lk.y1;
                    lk.bez[4] = mx; lk.bez[5] = lk.y2;
                    lk.bez[6] = lk.x2; lk.bez[7] = lk.y2;
                }
                else
                {
                    lk.x1 = pb.x + pb.w / 2;
                    lk.y1 = pb.y + pb.h;
                    lk.x2 = cb.x + (std::min)(10, cb.w / 3);
                    lk.y2 = cb.y;
                    const double seg = (std::max)((lk.y2 - lk.y1) * 0.5, 24.0);
                    double my = lk.y2 - seg;
                    if (my < lk.y1 + 6) my = lk.y1 + 6;
                    lk.bez[0] = lk.x1; lk.bez[1] = lk.y1;
                    lk.bez[2] = lk.x1; lk.bez[3] = my;
                    lk.bez[4] = lk.x2; lk.bez[5] = my;
                    lk.bez[6] = lk.x2; lk.bez[7] = lk.y2;
                }
                out.links.push_back(lk);
            }
        }        std::sort(out.links.begin(), out.links.end(), [](const PowerLink& a, const PowerLink& b) {
            if (a.from != b.from) return a.from < b.from;
            return a.to < b.to;
        });

        // 画布尺寸
        int maxX = 0, maxY = 0;
        for (const PowerBox& b : out.boxes)
        {
            if (b.x + b.w > maxX) maxX = b.x + b.w;
            if (b.y + b.h > maxY) maxY = b.y + b.h;
        }
        out.width = maxX + style.margin;
        out.height = maxY + style.margin;
        return true;
    }

    // ======================= XML 出口 =======================
    namespace
    {
        std::wstring xmlAttrEscape(const std::wstring& s)
        {
            std::wstring out;
            out.reserve(s.size() + 16);
            for (wchar_t ch : s)
            {
                switch (ch)
                {
                case L'&': out += L"&amp;"; break;
                case L'<': out += L"&lt;"; break;
                case L'>': out += L"&gt;"; break;
                case L'"': out += L"&quot;"; break;
                case L'\'': out += L"&apos;"; break;
                case L'\r': break;
                case L'\n': out += L' '; break;
                default: out += ch; break;
                }
            }
            return out;
        }
    }

    std::wstring toXml(const PowerPlan& plan, const std::wstring& title)
    {
        std::wstringstream ss;
        const std::wstring t = title.empty() ? std::wstring(L"配电规划图") : title;
        const std::wstring data = xmlAttrEscape(toJson(plan));
        // 窗口尺寸不跟着画布走：宽图（如 4878px）会把窗口撑出屏幕。
        // 画布自己会按客户区 fit，所以给一个"看得全"的窗口尺寸即可。
        const int winW = (plan.width + 48 > 1140) ? 1140 : (plan.width + 48);
        const int winH = (plan.height + 48 > 480) ? 480 : (plan.height + 48);
        ss << L"<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n";
        ss << L"<!-- 由配电规划图自动生成：一张 PowerCanvas 承载全部卡片与回路，"
           << L"滚轮缩放 / 拖动平移 / 单击卡片可命中 -->\r\n";
        ss << L"<Window xmlns:x=\"http://schemas.microsoft.com/winfx/2006/xml\"\r\n";
        ss << L"        Name=\"PowerPlan\" Width=\"" << winW << L"\" Height=\""
           << winH << L"\" Title=\"" << xmlEscape(t) << L"\" Background=\"#FFF3F5F8\">\r\n";
        ss << L"  <PowerCanvas Name=\"Plan\" Left=\"0\" Top=\"0\" Width=\"" << winW
           << L"\" Height=\"" << winH << L"\" Data=\""
           << data << L"\"/>\r\n";
        ss << L"</Window>\r\n";
        return ss.str();
    }

    // ======================= JSON 出口 =======================
    std::wstring toJson(const PowerPlan& plan)
    {
        std::wstringstream ss;
        ss << L"{\r\n";
        ss << L"  \"dir\": \"" << (plan.dir == L"h" ? L"h" : L"v") << L"\",\r\n";
        ss << L"  \"nodes\": [\r\n";
        for (size_t i = 0; i < plan.nodes.size(); ++i)
        {
            const PowerNode& n = plan.nodes[i];
            ss << L"    { \"code\": \"" << jsonEscape(n.code)
               << L"\", \"name\": \"" << jsonEscape(n.name)
               << L"\", \"upstream\": \"" << jsonEscape(n.upstream)
               << L"\", \"cable\": \"" << jsonEscape(n.cable)
               << L"\", \"loadKw\": " << n.loadKw
               << L", \"voltage\": \"" << jsonEscape(n.voltage)
               << L"\", \"length\": " << n.length
               << L", \"role\": \"" << jsonEscape(n.role)
               << L"\", \"collapsedCount\": " << n.collapsedCount << L" }";
            ss << (i + 1 < plan.nodes.size() ? L",\r\n" : L"\r\n");
        }
        ss << L"  ],\r\n";
        ss << L"  \"layout\": {\r\n";
        ss << L"    \"width\": " << plan.width << L",\r\n";
        ss << L"    \"height\": " << plan.height << L",\r\n";
        ss << L"    \"nodes\": [\r\n";
        for (size_t i = 0; i < plan.boxes.size(); ++i)
        {
            const PowerBox& b = plan.boxes[i];
            ss << L"      { \"code\": \"" << jsonEscape(b.code)
               << L"\", \"x\": " << b.x << L", \"y\": " << b.y
               << L", \"w\": " << b.w << L", \"h\": " << b.h
               << L", \"depth\": " << b.depth << L" }";
            ss << (i + 1 < plan.boxes.size() ? L",\r\n" : L"\r\n");
        }
        ss << L"    ],\r\n";
        ss << L"    \"links\": [\r\n";
        for (size_t i = 0; i < plan.links.size(); ++i)
        {
            const PowerLink& l = plan.links[i];
            ss << L"      { \"from\": \"" << jsonEscape(l.from) << L"\", \"to\": \"" << jsonEscape(l.to)
               << L"\", \"cable\": \"" << jsonEscape(l.cable)
               << L"\", \"x1\": " << l.x1 << L", \"y1\": " << l.y1
               << L", \"x2\": " << l.x2 << L", \"y2\": " << l.y2
               << L", \"vertical\": " << l.vertical << L", \"dashed\": " << l.dashed << L" }";
            ss << (i + 1 < plan.links.size() ? L",\r\n" : L"\r\n");
        }
        ss << L"    ]\r\n";
        ss << L"  }\r\n";
        ss << L"}\r\n";
        return ss.str();
    }

    // ======================= CSV 出口 =======================
    std::wstring toCsv(const std::vector<PowerNode>& nodes)
    {
        std::wstringstream ss;
        ss << L"编号,名称,上游,电缆型号,负载kW,电压,长度m\r\n";
        auto cell = [](const std::wstring& v) {
            if (v.find_first_of(L",\"\r\n") == std::wstring::npos) return v;
            std::wstring q = L"\"";
            for (wchar_t c : v)
            {
                if (c == L'"') q += L"\"\"";
                else q += c;
            }
            return q + L"\"";
        };
        for (const PowerNode& n : nodes)
        {
            ss << cell(n.code) << L"," << cell(n.name) << L"," << cell(n.upstream) << L","
               << cell(n.cable) << L"," << (n.loadKw > 0 ? numText(n.loadKw) : std::wstring())
               << L"," << cell(n.voltage)
               << L"," << (n.length > 0 ? numText(n.length) : std::wstring()) << L"\r\n";
        }
        return ss.str();
    }

    // ======================= SVG 出口 =======================
    namespace
    {
        // 与屏幕绘制同一套数值：三行文字基线、电压角标、折叠标记
        void svgNode(std::wstringstream& ss, const PowerPlan& plan, size_t i)
        {
            const PowerNode& nd = plan.nodes[i];
            const PowerBox& b = plan.boxes[i];
            const bool isRoot = (nd.role == L"root");
            const bool isTerm = (nd.role == L"terminal");
            const std::wstring fill = isTerm ? L"#EAF2FB" : L"#FFFFFF";
            const std::wstring stroke = isRoot ? L"#222222" : (isTerm ? L"#74869C" : L"#8A94A6");
            const int sw = isRoot ? 2 : 1;

            ss << L"<g class=\"node\" data-code=\"" << xmlEscape(nd.code) << L"\">";
            ss << L"<rect x=\"" << b.x << L"\" y=\"" << b.y << L"\" width=\"" << b.w
               << L"\" height=\"" << b.h << L"\" rx=\"2\" fill=\"" << fill
               << L"\" stroke=\"" << stroke << L"\" stroke-width=\"" << sw << L"\"/>";

            // 电压角标
            const VoltageStyle vs = voltStyle(nd.voltage);
            if (!vs.text.empty())
            {
                const int bw = 8 + textUnits(vs.text) * 4.6;
                const int bh = 11;
                const int bx = b.x + b.w - bw - 4;
                const int by = b.y + 3;
                wchar_t col[16] = { 0 }, bg[16] = { 0 };
                swprintf_s(col, L"#%06X", vs.color);
                swprintf_s(bg, L"#%06X", vs.bg);
                ss << L"<rect x=\"" << bx << L"\" y=\"" << by << L"\" width=\"" << bw
                   << L"\" height=\"" << bh << L"\" rx=\"2\" fill=\"" << bg
                   << L"\" stroke=\"" << col << L"\" stroke-width=\"0.7\"/>";
                ss << L"<text x=\"" << (bx + bw / 2) << L"\" y=\"" << (by + 8.6)
                   << L"\" text-anchor=\"middle\" font-size=\"8.5\" font-weight=\"600\" fill=\""
                   << col << L"\">" << xmlEscape(vs.text) << L"</text>";
            }

            // 三行文字
            const std::vector<std::wstring> lines = nodeLines(nd);
            const int blockH = 16 + 13 * ((int)lines.size() - 1);
            int by = b.y + (b.h - blockH) / 2 + 12;
            const int cx = b.x + b.w / 2;
            for (size_t k = 0; k < lines.size(); ++k)
            {
                const bool first = (k == 0);
                ss << L"<text x=\"" << cx << L"\" y=\"" << by
                   << L"\" text-anchor=\"middle\" font-size=\"" << (first ? (isRoot ? 14 : 13) : 10.5)
                   << L"\" font-weight=\"" << (first ? 600 : 400) << L"\" fill=\""
                   << (first ? L"#1F2A37" : (k == 1 ? L"#5B6675" : L"#1F64B0"))
                   << L"\">" << xmlEscape(lines[k]) << L"</text>";
                by += 13;
            }
            if (nd.collapsedCount > 0)
            {
                // 与 HTML 一致：卡片右上角、右对齐（x = 右边缘 - 6，y = 顶边 + 12）
                ss << L"<text x=\"" << (b.x + b.w - 6) << L"\" y=\"" << (b.y + 12)
                   << L"\" text-anchor=\"end\" font-size=\"9\" fill=\"#8A94A6\">+ "
                   << nd.collapsedCount << L"</text>";
            }
            ss << L"</g>";
        }

        void svgTitleBlock(std::wstringstream& ss, int x, int y, int w, int h,
                           const std::wstring& title)
        {
            const int c1 = (int)(w * 0.52), c2 = (int)(w * 0.24);
            ss << L"<g stroke=\"#667085\" stroke-width=\"1\" fill=\"none\">";
            ss << L"<rect x=\"" << x << L"\" y=\"" << y << L"\" width=\"" << w
               << L"\" height=\"" << h << L"\"/>";
            ss << L"<line x1=\"" << (x + c1) << L"\" y1=\"" << y << L"\" x2=\"" << (x + c1)
               << L"\" y2=\"" << (y + h) << L"\"/>";
            ss << L"<line x1=\"" << (x + c1 + c2) << L"\" y1=\"" << y << L"\" x2=\"" << (x + c1 + c2)
               << L"\" y2=\"" << (y + h) << L"\"/>";
            ss << L"<line x1=\"" << x << L"\" y1=\"" << (y + h / 2) << L"\" x2=\"" << (x + w)
               << L"\" y2=\"" << (y + h / 2) << L"\"/>";
            ss << L"</g>";
            ss << L"<g font-size=\"11\" fill=\"#1F2A37\">";
            ss << L"<text x=\"" << (x + 6) << L"\" y=\"" << (y + h / 3) << L"\">图名："
               << xmlEscape(title) << L"</text>";
            ss << L"<text x=\"" << (x + c1 + 6) << L"\" y=\"" << (y + h / 3)
               << L"\">工程：-</text>";
            ss << L"<text x=\"" << (x + c1 + c2 + 6) << L"\" y=\"" << (y + h / 3)
               << L"\">图号：-</text>";
            ss << L"<text x=\"" << (x + 6) << L"\" y=\"" << (y + h - 10) << L"\">日期：-</text>";
            ss << L"<text x=\"" << (x + c1 + 6) << L"\" y=\"" << (y + h - 10) << L"\">单位：-</text>";
            ss << L"<text x=\"" << (x + c1 + c2 + 6) << L"\" y=\"" << (y + h - 10)
               << L"\">比例：示意</text>";
            ss << L"</g>";
        }
    }

    std::wstring toSvg(const PowerPlan& plan, const std::wstring& title, bool withTitleBlock)
    {
        const std::wstring t = title.empty() ? std::wstring(L"配电干线图") : title;
        // withTitleBlock=true 是"设计稿导出"（标题带 + 边距 + 页脚），
        // false 是"屏上视图导出"：尺寸必须等于 plan 的宽高，否则和画布对不上。
        const int titleH = withTitleBlock ? 36 : 0;
        const int footH = withTitleBlock ? 64 : 0;
        const int margin = withTitleBlock ? 24 : 0;
        const int W = (std::max)(plan.width, 720);
        const int H = plan.height + titleH + footH;

        std::wstringstream ss;
        ss << L"<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
        ss << L"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << W << L"\" height=\"" << H
           << L"\" viewBox=\"0 0 " << W << L" " << H << L"\">";
        ss << L"<rect x=\"0\" y=\"0\" width=\"" << W << L"\" height=\"" << H << L"\" fill=\"#FFFFFF\"/>";
        ss << L"<rect x=\"" << margin << L"\" y=\"" << titleH << L"\" width=\"" << plan.width
           << L"\" height=\"" << plan.height << L"\" fill=\"#FFFFFF\" stroke=\"#CFD6DF\"/>";
        if (withTitleBlock)
        {
            ss << L"<text x=\"" << margin << L"\" y=\"22\" font-size=\"14\" font-weight=\"600\" "
               << L"fill=\"#1F2A37\" font-family=\"Microsoft YaHei\">" << xmlEscape(t) << L"</text>";
        }
        ss << L"<g transform=\"translate(" << margin << L" " << (titleH + 0) << L")\">";

        // 连线（先画线，卡片压在上面）
        for (const PowerLink& l : plan.links)
        {
            // 与屏上绘制一致：4 个拐点连成正交折线（L 命令），不再输出三次曲线 C。
            ss << L"<path d=\"M " << l.bez[0] << L" " << l.bez[1];
            for (int q = 1; q < 4; ++q)
            {
                const double qx = l.bez[q * 2], qy = l.bez[q * 2 + 1];
                const double px = l.bez[(q - 1) * 2], py = l.bez[(q - 1) * 2 + 1];
                if (qx != px || qy != py)
                    ss << L" L " << qx << L" " << qy;
            }
            ss << L"\" fill=\"none\" stroke=\"#2F6FBA\" stroke-width=\"2\""
               << (l.dashed ? L" stroke-dasharray=\"6 4\"" : L"") << L"/>";
            if (!l.dashed && !l.cable.empty())
            {
                // 与屏上一致：标注贴在"进入卡片的那一段线"上，左对齐、坐在线上沿
                if (l.vertical)
                {
                    ss << L"<text transform=\"rotate(-90 " << l.x2 << L" " << l.y2
                       << L")\" x=\"" << (l.x2 + 6) << L"\" y=\"" << (l.y2 - 3)
                       << L"\" font-size=\"10\" fill=\"#1F64B0\">" << xmlEscape(l.cable) << L"</text>";
                }
                else
                {
                    ss << L"<text x=\"" << (l.bez[2] + 6) << L"\" y=\"" << (l.y2 - 3)
                       << L"\" font-size=\"10\" fill=\"#1F64B0\">" << xmlEscape(l.cable) << L"</text>";
                }
            }
        }
        for (size_t i = 0; i < plan.boxes.size(); ++i) svgNode(ss, plan, i);
        ss << L"</g>";
        if (withTitleBlock)
            svgTitleBlock(ss, W - 520 - margin, H - footH + 4, 520, footH - 8, t);
        ss << L"</svg>";
        return ss.str();
    }

    // ======================= CSV 读入 =======================
    bool parseCsv(const std::wstring& text, std::vector<PowerNode>& out, std::wstring& title,
                  std::wstring& err)
    {
        out.clear();
        title.clear();
        err.clear();
        const std::vector<std::wstring> lines = splitLines(text);
        if (lines.empty())
        {
            err = L"文件为空";
            return false;
        }

        // 分隔符：优先逗号，整行没有逗号时按制表符
        wchar_t sep = L',';
        {
            const std::wstring& l = lines[0];
            if (l.find(L',') == std::wstring::npos && l.find(L'\t') != std::wstring::npos) sep = L'\t';
        }

        size_t startRow = 0;
        int colCode = 0, colName = 1, colUp = 2, colCable = 3, colLoad = 4, colVolt = 5, colLen = 6;
        {
            const std::wstring& head = lines[0];
            if (head.find(L"编号") != std::wstring::npos || head.find(L"code") != std::wstring::npos)
            {
                const std::vector<std::wstring> cells = splitCsvLine(head, sep);
                for (size_t i = 0; i < cells.size(); ++i)
                {
                    const std::wstring& c = cells[i];
                    if (c.find(L"编号") != std::wstring::npos || c == L"code") colCode = (int)i;
                    else if (c.find(L"名称") != std::wstring::npos || c == L"name") colName = (int)i;
                    else if (c.find(L"上游") != std::wstring::npos || c == L"parent" ||
                             c == L"upstream") colUp = (int)i;
                    else if (c.find(L"电缆") != std::wstring::npos || c == L"cable") colCable = (int)i;
                    else if (c.find(L"负载") != std::wstring::npos || c == L"load") colLoad = (int)i;
                    else if (c.find(L"电压") != std::wstring::npos || c == L"voltage") colVolt = (int)i;
                    else if (c.find(L"长度") != std::wstring::npos || c == L"length") colLen = (int)i;
                }
                startRow = 1;
            }
        }

        auto cellOf = [](const std::vector<std::wstring>& cells, int idx) -> std::wstring {
            if (idx < 0 || (size_t)idx >= cells.size()) return std::wstring();
            return cells[(size_t)idx];
        };

        for (size_t r = startRow; r < lines.size(); ++r)
        {
            if (trimWs(lines[r]).empty()) continue;
            const std::vector<std::wstring> cells = splitCsvLine(lines[r], sep);
            PowerNode n;
            n.code = cellOf(cells, colCode);
            n.name = cellOf(cells, colName);
            n.upstream = cellOf(cells, colUp);
            n.cable = cellOf(cells, colCable);
            const std::wstring load = cellOf(cells, colLoad);
            if (!load.empty()) n.loadKw = _wtof(load.c_str());
            n.voltage = normVoltage(cellOf(cells, colVolt));
            const std::wstring len = cellOf(cells, colLen);
            if (!len.empty()) n.length = _wtof(len.c_str());
            if (n.code.empty()) continue;
            out.push_back(n);
        }

        if (out.empty())
        {
            err = L"没有解析到任何节点（第一行可以是表头：编号,名称,上游,电缆型号,负载kW,电压,长度m）";
            return false;
        }
        return true;
    }

    // ======================= JSON 读入 =======================
    namespace
    {
        bool findJsonString(const std::wstring& obj, const wchar_t* key, std::wstring& out)
        {
            const std::wstring pat = std::wstring(L"\"") + key + L"\"";
            const size_t k = obj.find(pat);
            if (k == std::wstring::npos) return false;
            size_t c = obj.find(L':', k + pat.size());
            if (c == std::wstring::npos) return false;
            ++c;
            while (c < obj.size() && (obj[c] == L' ' || obj[c] == L'\t')) ++c;
            if (c >= obj.size()) return false;
            if (obj[c] == L'"')
            {
                std::wstring v;
                for (size_t i = c + 1; i < obj.size(); ++i)
                {
                    if (obj[i] == L'\\' && i + 1 < obj.size())
                    {
                        const wchar_t e = obj[++i];
                        if (e == L'n') v += L'\n';
                        else if (e == L't') v += L'\t';
                        else if (e == L'r') v += L'\r';
                        else v += e;
                        continue;
                    }
                    if (obj[i] == L'"') break;
                    v += obj[i];
                }
                out = v;
                return true;
            }
            return false;
        }

        bool findJsonNumber(const std::wstring& obj, const wchar_t* key, double& out)
        {
            const std::wstring pat = std::wstring(L"\"") + key + L"\"";
            const size_t k = obj.find(pat);
            if (k == std::wstring::npos) return false;
            size_t c = obj.find(L':', k + pat.size());
            if (c == std::wstring::npos) return false;
            ++c;
            while (c < obj.size() && (obj[c] == L' ' || obj[c] == L'\t')) ++c;
            const size_t b = c;
            while (c < obj.size() && (iswdigit(obj[c]) || obj[c] == L'.' || obj[c] == L'-' ||
                                      obj[c] == L'+' || obj[c] == L'e' || obj[c] == L'E'))
                ++c;
            if (c == b) return false;
            out = _wtof(obj.substr(b, c - b).c_str());
            return true;
        }

        std::vector<std::wstring> jsonObjectsOf(const std::wstring& arr)
        {
            std::vector<std::wstring> objs;
            bool inStr = false;
            int depth = 0;
            std::wstring cur;
            for (size_t i = 0; i < arr.size(); ++i)
            {
                const wchar_t c = arr[i];
                if (inStr)
                {
                    cur += c;
                    if (c == L'\\' && i + 1 < arr.size()) { cur += arr[++i]; continue; }
                    if (c == L'"') inStr = false;
                    continue;
                }
                if (c == L'"') { inStr = true; if (depth > 0) cur += c; continue; }
                if (c == L'{')
                {
                    ++depth;
                    if (depth == 1) { cur.clear(); continue; }
                }
                if (c == L'}')
                {
                    --depth;
                    if (depth == 0) { objs.push_back(cur); continue; }
                }
                if (depth > 0) cur += c;
            }
            return objs;
        }

        // 只记对象的字符区间，不再逐对象拷贝成 std::wstring：
        // 10221 个节点 × 每个对象 11 次全串查找，原来是边扫边 += 拼字符串（上万个 realloc）
        struct JsonSpan { size_t b, e; };   // [b,e)，不含外层大括号

        void jsonObjectSpans(const std::wstring& arr, std::vector<JsonSpan>& out)
        {
            bool inStr = false;
            int depth = 0;
            size_t start = 0;
            for (size_t i = 0; i < arr.size(); ++i)
            {
                const wchar_t c = arr[i];
                if (inStr)
                {
                    if (c == L'\\') { ++i; continue; }
                    if (c == L'"') inStr = false;
                    continue;
                }
                if (c == L'"') { inStr = true; continue; }
                if (c == L'{') { if (++depth == 1) start = i + 1; }
                else if (c == L'}') { if (depth > 0 && --depth == 0) out.push_back(JsonSpan{ start, i }); }
            }
        }

        bool spanString(const std::wstring& t, size_t b, size_t e, const wchar_t* key, std::wstring& out)
        {
            const std::wstring pat = std::wstring(1, L'"') + key + L'"';
            const size_t k = t.find(pat, b);
            if (k == std::wstring::npos || k >= e) return false;
            size_t c = t.find(L':', k + pat.size());
            if (c == std::wstring::npos || c >= e) return false;
            ++c;
            while (c < e && (t[c] == L' ' || t[c] == L'\t')) ++c;
            if (c >= e || t[c] != L'"') return false;
            ++c;
            size_t j = c;
            bool esc = false;
            while (j < e && t[j] != L'"')
            {
                if (t[j] == L'\\') { esc = true; ++j; }
                ++j;
            }
            if (j > e) j = e;
            if (!esc) { out.assign(t, c, j - c); return true; }
            std::wstring v;
            v.reserve(j - c);
            for (size_t i = c; i < j; ++i)
            {
                if (t[i] == L'\\' && i + 1 < j)
                {
                    const wchar_t x = t[++i];
                    if (x == L'n') v += L'\n';
                    else if (x == L't') v += L'\t';
                    else if (x == L'r') v += L'\r';
                    else v += x;
                    continue;
                }
                v += t[i];
            }
            out = v;
            return true;
        }

        bool spanDouble(const std::wstring& t, size_t b, size_t e, const wchar_t* key, double& out)
        {
            const std::wstring pat = std::wstring(1, L'"') + key + L'"';
            const size_t k = t.find(pat, b);
            if (k == std::wstring::npos || k >= e) return false;
            size_t c = t.find(L':', k + pat.size());
            if (c == std::wstring::npos || c >= e) return false;
            ++c;
            while (c < e && (t[c] == L' ' || t[c] == L'\t')) ++c;
            const size_t n0 = c;
            while (c < e && (iswdigit(t[c]) || t[c] == L'.' || t[c] == L'-' || t[c] == L'+' ||
                             t[c] == L'e' || t[c] == L'E'))
                ++c;
            if (c == n0) return false;
            out = _wtof(t.substr(n0, c - n0).c_str());
            return true;
        }

        // 取 key 对应的数组文本（含方括号）
        std::wstring jsonArrayOf(const std::wstring& text, const wchar_t* key)
        {
            const std::wstring pat = std::wstring(L"\"") + key + L"\"";
            const size_t k = text.find(pat);
            if (k == std::wstring::npos) return std::wstring();
            const size_t lb = text.find(L'[', k);
            if (lb == std::wstring::npos) return std::wstring();
            int depth = 0;
            bool inStr = false;
            for (size_t i = lb; i < text.size(); ++i)
            {
                const wchar_t c = text[i];
                if (inStr)
                {
                    if (c == L'\\') { ++i; continue; }
                    if (c == L'"') inStr = false;
                    continue;
                }
                if (c == L'"') { inStr = true; continue; }
                if (c == L'[') ++depth;
                else if (c == L']')
                {
                    if (--depth == 0) return text.substr(lb, i - lb + 1);
                }
            }
            return std::wstring();
        }
    }

    bool parseJson(const std::wstring& text, std::vector<PowerNode>& out, std::wstring& title,
                   std::wstring& err, bool* horizontal)
    {
        out.clear();
        title.clear();
        err.clear();

        findJsonString(text, L"title", title);

        // "dir":"h" 决定布局走向：画布/导图要按 JSON 自己的方向重排，否则横向数据会被摆成纵向
        if (horizontal != nullptr)
        {
            std::wstring dir;
            if (findJsonString(text, L"dir", dir))
            {
                const std::wstring d = trimWs(dir);
                *horizontal = (d == L"h" || d == L"H");
            }
        }

        const std::wstring arr = jsonArrayOf(text, L"nodes");
        if (arr.empty())
        {
            err = L"JSON 里没有 nodes 数组";
            return false;
        }
        const std::vector<std::wstring> objs = jsonObjectsOf(arr);
        for (const std::wstring& o : objs)
        {
            PowerNode n;
            findJsonString(o, L"code", n.code);
            findJsonString(o, L"name", n.name);
            findJsonString(o, L"upstream", n.upstream);
            findJsonString(o, L"cable", n.cable);
            double v = 0;
            if (findJsonNumber(o, L"loadKw", v)) n.loadKw = v;
            std::wstring volt;
            if (findJsonString(o, L"voltage", volt)) n.voltage = normVoltage(volt);
            if (findJsonNumber(o, L"length", v)) n.length = v;
            double cc = 0;
            if (findJsonNumber(o, L"collapsedCount", cc)) n.collapsedCount = (int)cc;
            n.code = trimWs(n.code);
            if (!n.code.empty()) out.push_back(n);
        }

        if (out.empty())
        {
            err = L"没有解析到任何节点";
            return false;
        }
        return true;
    }

    // ======================= 场景 JSON（坐标由外部算好）=======================
    bool parseScene(const std::wstring& text, PowerPlan& out, std::wstring& err)
    {
        out = PowerPlan();
        err.clear();

        const std::wstring dirText = [&]() {
            std::wstring d;
            findJsonString(text, L"dir", d);
            return trimWs(d);
        }();
        out.dir = (dirText == L"h" || dirText == L"H") ? L"h" : L"v";

        const std::wstring nodesArr = jsonArrayOf(text, L"nodes");
        if (nodesArr.empty())
        {
            err = L"场景里没有 nodes 数组";
            return false;
        }
        std::vector<JsonSpan> nodeSpans;
        jsonObjectSpans(nodesArr, nodeSpans);

        // 坐标段：layout.nodes
        const std::wstring layoutText = [&]() {
            const std::wstring pat = L"\"layout\"";
            const size_t k = text.find(pat);
            if (k == std::wstring::npos) return std::wstring();
            const size_t lb = text.find(L'{', k);
            if (lb == std::wstring::npos) return std::wstring();
            int depth = 0;
            bool inStr = false;
            for (size_t i = lb; i < text.size(); ++i)
            {
                const wchar_t c = text[i];
                if (inStr)
                {
                    if (c == L'\\') { ++i; continue; }
                    if (c == L'"') inStr = false;
                    continue;
                }
                if (c == L'"') { inStr = true; continue; }
                if (c == L'{') ++depth;
                else if (c == L'}')
                {
                    if (--depth == 0) return text.substr(lb, i - lb + 1);
                }
            }
            return std::wstring();
        }();

        double wv = 0, hv = 0;
        if (!layoutText.empty())
        {
            findJsonNumber(layoutText, L"width", wv);
            findJsonNumber(layoutText, L"height", hv);
        }
        out.width = (int)wv;
        out.height = (int)hv;

        // 节点：先按 nodes 建全字段，再用 layout.nodes 的坐标（按 code 关联）
        std::unordered_map<std::wstring, PowerBox> boxesByCode;
        std::wstring boxesArr;
        if (!layoutText.empty())
        {
            boxesArr = jsonArrayOf(layoutText, L"nodes");
            std::vector<JsonSpan> boxSpans;
            jsonObjectSpans(boxesArr, boxSpans);
            boxesByCode.reserve(boxSpans.size() * 2);
            for (const JsonSpan& sp : boxSpans)
            {
                PowerBox b;
                if (!spanString(boxesArr, sp.b, sp.e, L"code", b.code)) continue;
                b.code = trimWs(b.code);
                double v = 0;
                if (spanDouble(boxesArr, sp.b, sp.e, L"x", v)) b.x = (int)v;
                if (spanDouble(boxesArr, sp.b, sp.e, L"y", v)) b.y = (int)v;
                if (spanDouble(boxesArr, sp.b, sp.e, L"w", v)) b.w = (int)v;
                if (spanDouble(boxesArr, sp.b, sp.e, L"h", v)) b.h = (int)v;
                if (spanDouble(boxesArr, sp.b, sp.e, L"depth", v)) b.depth = (int)v;
                if (!b.code.empty()) boxesByCode.emplace(b.code, b);
            }
        }
        if (boxesByCode.empty())
        {
            err = L"场景里没有 layout.nodes 坐标段";
            return false;
        }

        out.nodes.reserve(nodeSpans.size());
        out.boxes.reserve(nodeSpans.size());
        for (const JsonSpan& sp : nodeSpans)
        {
            PowerNode n;
            spanString(nodesArr, sp.b, sp.e, L"code", n.code);
            spanString(nodesArr, sp.b, sp.e, L"name", n.name);
            spanString(nodesArr, sp.b, sp.e, L"upstream", n.upstream);
            spanString(nodesArr, sp.b, sp.e, L"cable", n.cable);
            std::wstring volt, role;
            if (spanString(nodesArr, sp.b, sp.e, L"voltage", volt)) n.voltage = normVoltage(volt);
            if (spanString(nodesArr, sp.b, sp.e, L"role", role)) n.role = trimWs(role);
            double v = 0;
            if (spanDouble(nodesArr, sp.b, sp.e, L"loadKw", v)) n.loadKw = v;
            if (spanDouble(nodesArr, sp.b, sp.e, L"length", v)) n.length = v;
            double cc = 0;
            if (spanDouble(nodesArr, sp.b, sp.e, L"collapsedCount", cc)) n.collapsedCount = (int)cc;
            double du = 0;
            if (spanDouble(nodesArr, sp.b, sp.e, L"dual", du)) n.dual = (du != 0) ? 1 : 0;
            n.code = trimWs(n.code);
            if (n.code.empty()) continue;
            // 坐标缺一不可：外部没算出这张卡片就不画，避免画出 0,0 的假卡片
            auto it = boxesByCode.find(n.code);
            if (it == boxesByCode.end()) continue;
            if (n.role.empty())
                n.role = n.upstream.empty() ? L"root" : L"branch";
            out.nodes.push_back(n);
            out.boxes.push_back(it->second);
        }
        if (out.nodes.empty())
        {
            err = L"场景里节点与坐标对不上";
            return false;
        }

        // 连线：坐标由外部给，贝塞尔控制点按走向就地补（纯几何，不是布局）
        if (!layoutText.empty())
        {
            const std::wstring linksArr = jsonArrayOf(layoutText, L"links");
            std::vector<JsonSpan> linkSpans;
            jsonObjectSpans(linksArr, linkSpans);
            out.links.reserve(linkSpans.size());
            for (const JsonSpan& sp : linkSpans)
            {
                PowerLink l;
                spanString(linksArr, sp.b, sp.e, L"from", l.from);
                spanString(linksArr, sp.b, sp.e, L"to", l.to);
                spanString(linksArr, sp.b, sp.e, L"cable", l.cable);
                l.from = trimWs(l.from);
                l.to = trimWs(l.to);
                double v = 0;
                if (spanDouble(linksArr, sp.b, sp.e, L"x1", v)) l.x1 = (int)v;
                if (spanDouble(linksArr, sp.b, sp.e, L"y1", v)) l.y1 = (int)v;
                if (spanDouble(linksArr, sp.b, sp.e, L"x2", v)) l.x2 = (int)v;
                if (spanDouble(linksArr, sp.b, sp.e, L"y2", v)) l.y2 = (int)v;
                if (spanDouble(linksArr, sp.b, sp.e, L"dashed", v)) l.dashed = (v != 0) ? 1 : 0;
                if (spanDouble(linksArr, sp.b, sp.e, L"magenta", v)) l.magenta = (v != 0) ? 1 : 0;
                // 拐点与 buildPlan 同一套规则：靠近卡片，给电缆标注留出那段线
                if (out.dir == L"h")
                {
                    const double seg = l.dashed
                        ? (std::max)((l.x2 - l.x1) * 0.5, 24.0)
                        : (std::max)((l.x2 - l.x1) * 0.5,
                                     static_cast<double>(cableLabelWidth(l.cable) + 10));
                    double mx = l.x2 - seg;
                    if (mx < l.x1 + 6) mx = l.x1 + 6;
                    l.vertical = 0;
                    l.bez[0] = l.x1; l.bez[1] = l.y1;
                    l.bez[2] = mx;   l.bez[3] = l.y1;
                    l.bez[4] = mx;   l.bez[5] = l.y2;
                    l.bez[6] = l.x2; l.bez[7] = l.y2;
                }
                else
                {
                    const double seg = l.dashed
                        ? (std::max)((l.y2 - l.y1) * 0.5, 24.0)
                        : (std::max)((l.y2 - l.y1) * 0.5,
                                     static_cast<double>(cableLabelWidth(l.cable) + 10));
                    double my = l.y2 - seg;
                    if (my < l.y1 + 6) my = l.y1 + 6;
                    l.vertical = 1;
                    l.bez[0] = l.x1; l.bez[1] = l.y1;
                    l.bez[2] = l.x1; l.bez[3] = my;
                    l.bez[4] = l.x2; l.bez[5] = my;
                    l.bez[6] = l.x2; l.bez[7] = l.y2;
                }
                if (!l.from.empty() && !l.to.empty()) out.links.push_back(l);
            }
        }

        if (out.width <= 0 || out.height <= 0)
        {
            // 没给画布尺寸就按坐标兜底，别退化成 1×1
            int mx = 0, my = 0;
            for (const PowerBox& b : out.boxes)
            {
                mx = (std::max)(mx, b.x + b.w);
                my = (std::max)(my, b.y + b.h);
            }
            out.width = mx + 24;
            out.height = my + 24;
        }
        return true;
    }

    // ======================= 交叉自检 =======================
    int countCrossings(const PowerPlan& plan)
    {
        auto cross = [](const xmlgeom::Point2& a, const xmlgeom::Point2& b,
                        const xmlgeom::Point2& c) {
            return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
        };
        auto segCross = [&](const xmlgeom::Point2& p1, const xmlgeom::Point2& p2,
                            const xmlgeom::Point2& p3, const xmlgeom::Point2& p4) {
            const double d1 = cross(p3, p4, p1);
            const double d2 = cross(p3, p4, p2);
            const double d3 = cross(p1, p2, p3);
            const double d4 = cross(p1, p2, p4);
            const bool s1 = (d1 > 1e-9 && d2 < -1e-9) || (d1 < -1e-9 && d2 > 1e-9);
            const bool s2 = (d3 > 1e-9 && d4 < -1e-9) || (d3 < -1e-9 && d4 > 1e-9);
            return s1 && s2;   // 只算"真穿过"，共端点/相切不算
        };

        std::vector<std::vector<xmlgeom::Point2>> polys;
        for (const PowerLink& l : plan.links)
        {
            std::vector<xmlgeom::Point2> pts;
            xmlgeom::bezierPoints(xmlgeom::Point2(l.bez[0], l.bez[1]),
                                   xmlgeom::Point2(l.bez[2], l.bez[3]),
                                   xmlgeom::Point2(l.bez[4], l.bez[5]),
                                   xmlgeom::Point2(l.bez[6], l.bez[7]), 16, pts);
            polys.push_back(pts);
        }

        int crossings = 0;
        for (size_t i = 0; i < polys.size(); ++i)
        {
            for (size_t j = i + 1; j < polys.size(); ++j)
            {
                bool hit = false;
                for (size_t a = 0; a + 1 < polys[i].size() && !hit; ++a)
                    for (size_t b = 0; b + 1 < polys[j].size() && !hit; ++b)
                        if (segCross(polys[i][a], polys[i][a + 1], polys[j][b], polys[j][b + 1]))
                            hit = true;
                if (hit) ++crossings;
            }
        }
        return crossings;
    }
}
