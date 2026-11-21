#include "XmlSvgScene.h"

#include <windows.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"

namespace xmlsvg
{
    namespace
    {
        inline float distPointLine(float px, float py,
                                   float ax, float ay, float bx, float by)
        {
            const float dx = bx - ax;
            const float dy = by - ay;
            const float len2 = dx * dx + dy * dy;
            if (len2 < 1e-6f)
            {
                const float ex = px - ax;
                const float ey = py - ay;
                return sqrtf(ex * ex + ey * ey);
            }
            float t = ((px - ax) * dx + (py - ay) * dy) / len2;
            if (t < 0.0f) t = 0.0f;
            else if (t > 1.0f) t = 1.0f;
            const float cx = ax + t * dx;
            const float cy = ay + t * dy;
            const float ex = px - cx;
            const float ey = py - cy;
            return sqrtf(ex * ex + ey * ey);
        }

        // 三次贝塞尔自适应拍平：平坦度 = 两个控制点到弦的最大距离。
        // 用容差递归而不是固定段数，长了才多分，短段不会被无谓地切成碎片
        // （固定 16 段在 100px 段上会多出一倍顶点，放大后反而看出折角）。
        void flattenCubic(float x0, float y0, float x1, float y1,
                          float x2, float y2, float x3, float y3,
                          float tol, std::vector<SvgPoint>& out, int depth = 0)
        {
            const float d1 = distPointLine(x1, y1, x0, y0, x3, y3);
            const float d2 = distPointLine(x2, y2, x0, y0, x3, y3);
            if (depth >= 14 || (d1 <= tol && d2 <= tol))
            {
                out.push_back(SvgPoint{ x3, y3 });
                return;
            }
            // de Casteljau 在 t=0.5 处一分为二
            const float ax = (x0 + x1) * 0.5f, ay = (y0 + y1) * 0.5f;
            const float bx = (x1 + x2) * 0.5f, by = (y1 + y2) * 0.5f;
            const float cx = (x2 + x3) * 0.5f, cy = (y2 + y3) * 0.5f;
            const float dx = (ax + bx) * 0.5f, dy = (ay + by) * 0.5f;
            const float ex = (bx + cx) * 0.5f, ey = (by + cy) * 0.5f;
            const float mx = (dx + ex) * 0.5f, my = (dy + ey) * 0.5f;
            flattenCubic(x0, y0, ax, ay, dx, dy, mx, my, tol, out, depth + 1);
            flattenCubic(mx, my, ex, ey, cx, cy, x3, y3, tol, out, depth + 1);
        }

        COLORREF colorOf(unsigned int c)
        {
            // nanosvg 的 NSVG_RGB(r,g,b) = r | g<<8 | b<<16，与 COLORREF 同序
            return RGB(static_cast<BYTE>(c & 0xFF),
                       static_cast<BYTE>((c >> 8) & 0xFF),
                       static_cast<BYTE>((c >> 16) & 0xFF));
        }

        // ---------------- <text> 自解析（nanosvg 只管几何，文本得自己扫） ----------------
        // 只认工程图导出会用到的写法：绝对 x/y、text-anchor、font-size/weight/family、
        // fill、opacity，以及 <g transform="translate(...)"> 的偏移。其余变换记警告后忽略，
        // 不去猜——猜错的偏移比丢一个属性更难查。

        std::wstring decodeXml(const std::string& s)
        {
            std::string t;
            t.reserve(s.size());
            for (size_t i = 0; i < s.size(); ++i)
            {
                if (s[i] != '&')
                {
                    t.push_back(s[i]);
                    continue;
                }
                const size_t semi = s.find(';', i);
                if (semi == std::string::npos || semi - i > 10)
                {
                    t.push_back(s[i]);
                    continue;
                }
                const std::string ent = s.substr(i + 1, semi - i - 1);
                if (ent == "amp") t += '&';
                else if (ent == "lt") t += '<';
                else if (ent == "gt") t += '>';
                else if (ent == "quot") t += '"';
                else if (ent == "apos") t += '\'';
                else if (ent.size() > 1 && ent[0] == '#')
                {
                    const int code = (ent[1] == 'x' || ent[1] == 'X')
                        ? static_cast<int>(strtol(ent.c_str() + 2, nullptr, 16))
                        : atoi(ent.c_str() + 1);
                    // 数值实体不走原文的 UTF-8 流，这里按码点自己编 UTF-8 再拼回去
                    if (code > 0 && code < 0x80)
                        t.push_back(static_cast<char>(code));
                    else if (code < 0x800)
                    {
                        t.push_back(static_cast<char>(0xC0 | (code >> 6)));
                        t.push_back(static_cast<char>(0x80 | (code & 0x3F)));
                    }
                    else if (code >= 0xD800 && code < 0x10000)
                        ;   // 代理区单独出现：丢弃
                    else if (code < 0x10000)
                    {
                        t.push_back(static_cast<char>(0xE0 | (code >> 12)));
                        t.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
                        t.push_back(static_cast<char>(0x80 | (code & 0x3F)));
                    }
                    else if (code < 0x110000)
                    {
                        t.push_back(static_cast<char>(0xF0 | (code >> 18)));
                        t.push_back(static_cast<char>(0x80 | ((code >> 12) & 0x3F)));
                        t.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
                        t.push_back(static_cast<char>(0x80 | (code & 0x3F)));
                    }
                }
                else
                {
                    t.push_back(s[i]);
                    continue;   // 未知实体：原样保留 '&'
                }
                i = semi;
            }
            std::wstring out;
            if (!t.empty())
            {
                const int n = ::MultiByteToWideChar(CP_UTF8, 0, t.c_str(),
                                                    static_cast<int>(t.size()), nullptr, 0);
                if (n > 0)
                {
                    out.resize(static_cast<size_t>(n));
                    ::MultiByteToWideChar(CP_UTF8, 0, t.c_str(), static_cast<int>(t.size()),
                                          out.data(), n);
                }
            }
            return out;
        }

        bool attrOf(const std::string& tag, const char* name, std::string& out)
        {
            out.clear();
            const std::string key = std::string(name) + "=";
            size_t p = 0;
            while ((p = tag.find(key, p)) != std::string::npos)
            {
                // 往前一个字符必须是空白，避免匹配到 "font-size" 里的 "size"
                if (p > 0 && (tag[p - 1] == ' ' || tag[p - 1] == '\t' || tag[p - 1] == '\n'))
                {
                    const size_t q = p + key.size();
                    if (q < tag.size() && (tag[q] == '"' || tag[q] == '\''))
                    {
                        const char quote = tag[q];
                        const size_t end = tag.find(quote, q + 1);
                        if (end != std::string::npos)
                        {
                            out = tag.substr(q + 1, end - q - 1);
                            return true;
                        }
                    }
                    return false;
                }
                p += key.size();
            }
            return false;
        }

        float numOf(const std::string& s, float fallback)
        {
            if (s.empty()) return fallback;
            char* end = nullptr;
            const double v = strtod(s.c_str(), &end);
            if (end == s.c_str()) return fallback;
            return static_cast<float>(v);
        }

        COLORREF parseSvgColor(const std::string& s)
        {
            std::string v = s;
            while (!v.empty() && v[0] == ' ') v.erase(0, 1);
            while (!v.empty() && v[v.size() - 1] == ' ') v.erase(v.size() - 1);
            if (v.empty() || v == "none") return 0;
            if (v[0] == '#')
            {
                unsigned int r = 0, g = 0, b = 0;
                if (v.size() >= 7 && sscanf_s(v.c_str(), "#%2x%2x%2x", &r, &g, &b) == 3)
                    return RGB(r, g, b);
                if (v.size() >= 4 && sscanf_s(v.c_str(), "#%1x%1x%1x", &r, &g, &b) == 3)
                    return RGB(r * 17, g * 17, b * 17);
                return 0;
            }
            if (v.compare(0, 4, "rgb(") == 0)
            {
                unsigned int r = 0, g = 0, b = 0;
                if (sscanf_s(v.c_str(), "rgb(%u,%u,%u)", &r, &g, &b) != 3 &&
                    sscanf_s(v.c_str(), "rgb(%u, %u, %u)", &r, &g, &b) != 3)
                    return 0;
                return RGB(r & 0xFF, g & 0xFF, b & 0xFF);
            }
            struct Named { const char* n; COLORREF c; };
            static const Named kNames[] = {
                { "black", RGB(0,0,0) }, { "white", RGB(255,255,255) },
                { "red", RGB(255,0,0) }, { "blue", RGB(0,0,255) },
                { "green", RGB(0,128,0) }, { "gray", RGB(128,128,128) },
                { "grey", RGB(128,128,128) }, { "orange", RGB(255,165,0) },
                { "yellow", RGB(255,255,0) }, { "silver", RGB(192,192,192) },
            };
            for (const Named& n : kNames)
                if (_stricmp(v.c_str(), n.n) == 0)
                    return n.c;
            return 0;
        }

        // 只取 font-family 的首个名字：导出里常写成 "Microsoft YaHei, PingFang SC, sans-serif"
        std::wstring firstFamily(const std::string& s)
        {
            std::string v = s;
            const size_t comma = v.find(',');
            if (comma != std::string::npos) v = v.substr(0, comma);
            while (!v.empty() && (v[0] == ' ' || v[0] == '\'' || v[0] == '"'))
                v.erase(0, 1);
            while (!v.empty() && (v[v.size() - 1] == ' ' || v[v.size() - 1] == '\'' ||
                                  v[v.size() - 1] == '"'))
                v.erase(v.size() - 1);
            return decodeXml(v);
        }

        // 从 transform 里抽 translate 的 (dx,dy)；其余变换形式不支持
        bool translateOf(const std::string& tr, float& dx, float& dy, bool& unsupported)
        {
            unsupported = false;
            dx = dy = 0.0f;
            const size_t p = tr.find("translate(");
            if (p == std::string::npos)
            {
                if (!tr.empty() && tr.find("matrix(") == std::string::npos &&
                    tr.find("scale(") == std::string::npos &&
                    tr.find("rotate(") == std::string::npos &&
                    tr.find("skew") == std::string::npos)
                    return false;   // 空 transform
                unsupported = true;
                return false;
            }
            const size_t q = p + 10;
            const size_t end = tr.find(')', q);
            if (end == std::string::npos) { unsupported = true; return false; }
            std::string body = tr.substr(q, end - q);
            for (char& c : body) if (c == ',') c = ' ';
            char* e = nullptr;
            dx = static_cast<float>(strtod(body.c_str(), &e));
            dy = 0.0f;
            if (e != nullptr && *e != '\0')
                dy = static_cast<float>(strtod(e, nullptr));
            return true;
        }

        void collectTexts(const std::string& src, std::vector<SvgText>& out,
                          std::vector<std::wstring>& warns)
        {
            // group translate 栈：进入 <g> 压入累计偏移，遇到 </g> 弹出
            std::vector<std::pair<float, float>> stack;
            float curX = 0.0f, curY = 0.0f;
            bool warnedMatrix = false;

            size_t i = 0;
            while ((i = src.find('<', i)) != std::string::npos)
            {
                // 注释 / CDATA / 处理指令 / DOCTYPE 直接跳过
                if (src.compare(i, 4, "<!--") == 0)
                {
                    const size_t e = src.find("-->", i);
                    i = (e == std::string::npos) ? src.size() : e + 3;
                    continue;
                }
                if (src.compare(i, 9, "<![CDATA[") == 0)
                {
                    const size_t e = src.find("]]>", i);
                    i = (e == std::string::npos) ? src.size() : e + 3;
                    continue;
                }
                if (src.compare(i, 2, "<?") == 0 || src.compare(i, 2, "<!") == 0)
                {
                    const size_t e = src.find('>', i);
                    i = (e == std::string::npos) ? src.size() : e + 1;
                    continue;
                }

                const size_t gt = src.find('>', i);
                if (gt == std::string::npos) break;
                const std::string tag = src.substr(i + 1, gt - i - 1);
                const bool selfClose = !tag.empty() && tag[tag.size() - 1] == '/';
                const bool closing = !tag.empty() && tag[0] == '/';

                // 标签名
                std::string name;
                for (size_t k = closing ? 1 : 0; k < tag.size(); ++k)
                {
                    const char c = tag[k];
                    if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '/' || c == '>')
                        break;
                    name.push_back(c);
                }

                if (name == "g")
                {
                    if (closing)
                    {
                        if (!stack.empty())
                        {
                            stack.pop_back();
                            if (!stack.empty()) { curX = stack.back().first; curY = stack.back().second; }
                            else { curX = curY = 0.0f; }
                        }
                    }
                    else
                    {
                        float dx = 0.0f, dy = 0.0f;
                        std::string tr;
                        bool bad = false;
                        if (attrOf(tag, "transform", tr))
                            translateOf(tr, dx, dy, bad);
                        if (bad && !warnedMatrix)
                        {
                            warns.push_back(L"<g transform> 只支持 translate，其余变换已忽略");
                            warnedMatrix = true;
                        }
                        stack.push_back(std::make_pair(curX + dx, curY + dy));
                        if (selfClose) stack.pop_back();
                        curX += dx;
                        curY += dy;
                        if (selfClose) { curX = stack.back().first; curY = stack.back().second; }
                    }
                    i = gt + 1;
                    continue;
                }

                if (name == "text" && !closing && !selfClose)
                {
                    SvgText t;
                    std::string v;
                    if (!attrOf(tag, "x", v)) { i = gt + 1; continue; }
                    t.x = numOf(v, 0.0f) + curX;
                    t.y = attrOf(tag, "y", v) ? numOf(v, 0.0f) + curY : curY;
                    t.sizePx = attrOf(tag, "font-size", v) ? numOf(v, 12.0f) : 12.0f;
                    t.weight = attrOf(tag, "font-weight", v) ? static_cast<int>(numOf(v, 400.0f)) : 400;
                    t.color = attrOf(tag, "fill", v) ? parseSvgColor(v) : RGB(0, 0, 0);
                    t.opacity = attrOf(tag, "opacity", v) ? numOf(v, 1.0f) : 1.0f;
                    if (attrOf(tag, "text-anchor", v))
                    {
                        if (v == "middle") t.anchor = 1;
                        else if (v == "end") t.anchor = 2;
                    }
                    // rotate="角度[ 中心x 中心y]"，只取首个数（逐字旋转列表按整段角度画）
                    if (attrOf(tag, "rotate", v))
                        t.rotate = numOf(v, 0.0f);
                    else if (attrOf(tag, "transform", v))
                    {
                        // 兼容 transform="rotate(a cx cy)"：绕自身锚点旋转时可忽略中心
                        size_t rp = v.find("rotate(");
                        if (rp != std::string::npos)
                        {
                            const char* p = v.c_str() + rp + 7;
                            char* end = nullptr;
                            const double a = strtod(p, &end);
                            if (end != p) t.rotate = static_cast<float>(a);
                        }
                    }
                    if (attrOf(tag, "font-family", v))
                        t.family = firstFamily(v);

                    const size_t close = src.find("</text>", gt + 1);
                    if (close == std::string::npos) { i = gt + 1; continue; }
                    t.text = decodeXml(src.substr(gt + 1, close - gt - 1));
                    if (!t.text.empty())
                        out.push_back(t);
                    i = close + 7;
                    continue;
                }

                i = gt + 1;
            }
        }
    }

    bool parseSvgText(const std::string& utf8, SvgScene& out, std::wstring& err)
    {
        out = SvgScene();
        err.clear();
        if (utf8.empty())
        {
            err = L"SVG 内容为空";
            return false;
        }

        // nsvgParse 会就地改写传入的字符串（它按 XML 词法边走边截断），
        // 所以必须给一份可写的拷贝；const 引用不能直接传。
        std::string buf = utf8;
        NSVGimage* img = nsvgParse(buf.data(), "px", 96.0f);
        if (img == nullptr)
        {
            err = L"SVG 解析失败（不是合法 SVG，或只含不受支持的图元）";
            return false;
        }

        out.width = img->width;
        out.height = img->height;

        for (NSVGshape* s = img->shapes; s != nullptr; s = s->next)
        {
            if (s->opacity <= 0.001f)
                continue;

            SvgShape sh;
            sh.opacity = s->opacity;
            sh.id = std::wstring(s->id, s->id + strlen(s->id));

            if (s->fill.type == NSVG_PAINT_COLOR)
            {
                sh.hasFill = true;
                sh.fill = colorOf(s->fill.color);
            }
            else if (s->fill.type == NSVG_PAINT_LINEAR_GRADIENT ||
                     s->fill.type == NSVG_PAINT_RADIAL_GRADIENT)
            {
                // 渐变按首色近似填充，并在警告里留痕（工程图很少用到渐变）
                NSVGgradient* g = s->fill.gradient;
                if (g != nullptr && g->nstops > 0)
                {
                    sh.hasFill = true;
                    sh.fill = colorOf(g->stops[0].color);
                    out.warnings.push_back(L"渐变填充按首色近似");
                }
            }

            if (s->stroke.type == NSVG_PAINT_COLOR)
            {
                sh.hasStroke = true;
                sh.stroke = colorOf(s->stroke.color);
                sh.strokeWidth = s->strokeWidth > 0.0f ? s->strokeWidth : 1.0f;
                if (s->strokeDashArray != nullptr && s->strokeDashCount > 0)
                {
                    sh.dashOn = (s->strokeDashArray[0] > 0.0f) ? s->strokeDashArray[0] : 1.0f;
                    sh.dashOff = (s->strokeDashCount > 1 && s->strokeDashArray[1] > 0.0f)
                                     ? s->strokeDashArray[1]
                                     : sh.dashOn;
                }
            }

            if (!sh.hasFill && !sh.hasStroke)
                continue;   // 无可视样式（如仅用于分组的占位 path）

            for (NSVGpath* p = s->paths; p != nullptr; p = p->next)
            {
                if (p->npts < 2)
                    continue;
                SvgContour ct;
                ct.closed = p->closed != 0;
                ct.pts.push_back(SvgPoint{ p->pts[0], p->pts[1] });

                // pts 布局：x0,y0, [c1x,c1y,c2x,c2y,x,y]...，一段三次贝塞尔占 6 个 float
                for (int i = 0; i + 3 < p->npts; i += 3)
                {
                    const float* q = p->pts + i * 2;
                    flattenCubic(q[0], q[1], q[2], q[3], q[4], q[5], q[6], q[7],
                                 0.35f, ct.pts);
                }
                if (ct.pts.size() >= 2)
                {
                    sh.contours.push_back(std::move(ct));
                    out.pathCount++;
                }
            }

            if (!sh.contours.empty())
                out.shapes.push_back(std::move(sh));
        }

        nsvgDelete(img);

        // 几何解析完再扫文本：nanosvg 会丢弃 <text>，但它不会改我们传进去的原串
        // （它改的是上面的 buf 拷贝），所以这里可以安全地再扫一遍 utf8。
        collectTexts(utf8, out.texts, out.warnings);

        if (out.shapes.empty() && out.texts.empty())
        {
            err = L"SVG 里没有可绘制的图元（都是 image 或不支持的标签）";
            return false;
        }
        if (out.width <= 0.0f || out.height <= 0.0f)
        {
            // 没有 width/height/viewBox 时，退回按几何包围盒推算，避免画布是 0
            float mx = 0.0f, my = 0.0f;
            for (const SvgShape& sh : out.shapes)
                for (const SvgContour& ct : sh.contours)
                    for (const SvgPoint& pt : ct.pts)
                    {
                        if (pt.x > mx) mx = pt.x;
                        if (pt.y > my) my = pt.y;
                    }
            out.width = mx + 1.0f;
            out.height = my + 1.0f;
            out.warnings.push_back(L"SVG 未声明 width/height，按几何包围盒推算");
        }
        return true;
    }

    bool loadSvgFile(const std::wstring& path, SvgScene& out, std::wstring& err)
    {
        err.clear();
        FILE* f = _wfopen(path.c_str(), L"rb");
        if (f == nullptr)
        {
            err = L"打不开 SVG 文件";
            return false;
        }
        std::string text;
        char chunk[4096];
        size_t n = 0;
        while ((n = fread(chunk, 1, sizeof(chunk), f)) > 0)
            text.append(chunk, n);
        fclose(f);
        return parseSvgText(text, out, err);
    }

    // 虚线：画布没有 dash 样式，按弧长把折线切成实线段（与配电图虚线的做法一致）
    static void strokeDashedPath(XmlCanvas& canvas, const std::vector<POINT>& pts,
                                 COLORREF color, int thickness, bool closed,
                                 float dashOn, float dashOff)
    {
        if (dashOn <= 0.0f || dashOff <= 0.0f || pts.size() < 2)
            return;
        // 像素级下限：dash 太密时不能把一条线切成上千段（那是"卡死"而不是"虚线"）
        if (dashOn < 1.0f) dashOn = 1.0f;
        if (dashOff < 0.5f) dashOff = 0.5f;
        const float period = dashOn + dashOff;
        float phase = 0.0f;                        // 当前在 dash 周期里的位置
        std::vector<POINT> poly = pts;
        if (closed && pts.size() >= 3)
            poly.push_back(pts[0]);

        for (size_t i = 1; i < poly.size(); ++i)
        {
            const POINT a = poly[i - 1];
            const POINT b = poly[i];
            const float dx = static_cast<float>(b.x - a.x);
            const float dy = static_cast<float>(b.y - a.y);
            const float len = sqrtf(dx * dx + dy * dy);
            if (len <= 0.0f)
                continue;

            float t = 0.0f;
            while (t < len)
            {
                const float remain = period - fmodf(phase, period);
                float t2 = t + remain;
                if (t2 > len)
                    t2 = len;
                if (t2 < t + 1e-4f)            // 浮点兜底：保证每轮都前进，绝不死循环
                    t2 = t + 1e-4f;
                if (fmodf(phase, period) < dashOn)
                {
                    POINT pa = { static_cast<LONG>(lroundf(a.x + dx * (t / len))),
                                 static_cast<LONG>(lroundf(a.y + dy * (t / len))) };
                    POINT pb = { static_cast<LONG>(lroundf(a.x + dx * (t2 / len))),
                                 static_cast<LONG>(lroundf(a.y + dy * (t2 / len))) };
                    canvas.strokeLine(pa, pb, color, thickness);
                }
                phase += (t2 - t);
                t = t2;
            }
        }
    }

    void drawSvgScene(XmlCanvas& canvas, const SvgScene& scene,
                      float scale, int offsetX, int offsetY)
    {
        if (scale <= 0.0f)
            scale = 1.0f;

        for (const SvgShape& sh : scene.shapes)
        {
            const int alpha = static_cast<int>(sh.opacity * 255.0f);
            const bool fade = sh.opacity < 0.999f;

            for (const SvgContour& ct : sh.contours)
            {
                std::vector<POINT> pts;
                pts.reserve(ct.pts.size());
                for (const SvgPoint& p : ct.pts)
                {
                    POINT q;
                    q.x = static_cast<LONG>(lroundf(p.x * scale)) + offsetX;
                    q.y = static_cast<LONG>(lroundf(p.y * scale)) + offsetY;
                    pts.push_back(q);
                }
                if (pts.size() < 2)
                    continue;

                if (sh.hasFill)
                {
                    // 圆整后可能退化成共线点，fillPath 对 < 3 点的轮廓无意义
                    if (pts.size() >= 3)
                    {
                        if (fade)
                            canvas.pushOpacity(alpha);
                        canvas.fillPath(pts, sh.fill);
                        if (fade)
                            canvas.popOpacity();
                    }
                }
                if (sh.hasStroke)
                {
                    int w = static_cast<int>(lroundf(sh.strokeWidth * scale));
                    // 笔宽下限 1px：细线在缩小时不能消失（这是之前 QML 线踩过的坑）
                    if (w < 1) w = 1;
                    if (fade)
                        canvas.pushOpacity(alpha);
                    if (sh.dashOn > 0.0f && sh.dashOff > 0.0f)
                        strokeDashedPath(canvas, pts, sh.stroke, w, ct.closed,
                                         sh.dashOn * scale, sh.dashOff * scale);
                    else
                        canvas.strokePath(pts, sh.stroke, w, ct.closed);
                    if (fade)
                        canvas.popOpacity();
                }
            }
        }

        // 文本按"框内居中"画：DWrite 的段落对齐是矩形居中，所以先把基线上移
        // 约 0.34em 折算成视觉中心，再用 textWidth 把锚点换成左右边界。
        for (const SvgText& t : scene.texts)
        {
            const int pt = static_cast<int>(lroundf(t.sizePx * scale * 0.75f));
            if (pt < 1) continue;

            const std::wstring weight = (t.weight >= 700) ? L"Bold"
                                     : (t.weight >= 600) ? L"SemiBold"
                                     : L"Normal";
            const std::wstring family = t.family.empty() ? L"Microsoft YaHei" : t.family;
            canvas.setTextStyle(weight, L"Normal", L"NoWrap", L"Center");

            float wpx = canvas.textWidth(t.text, pt, family);
            if (wpx < 0.0f)
                wpx = static_cast<float>(t.text.size()) * t.sizePx * scale * 0.62f;   // 无测量能力时估算

            const float hpix = t.sizePx * scale * 1.3f;      // 留出上下余量，避免被 CLIP 切边
            const float cx = t.x * scale + offsetX;
            const float cy = (t.y - t.sizePx * 0.34f) * scale + offsetY;

            RECT rc = {};
            if (t.anchor == 1)      { rc.left = static_cast<LONG>(lroundf(cx - wpx * 0.5f));
                                      rc.right = static_cast<LONG>(lroundf(cx + wpx * 0.5f + 1.0f)); }
            else if (t.anchor == 2) { rc.left = static_cast<LONG>(lroundf(cx - wpx));
                                      rc.right = static_cast<LONG>(lroundf(cx + 1.0f)); }
            else                    { rc.left = static_cast<LONG>(lroundf(cx));
                                      rc.right = static_cast<LONG>(lroundf(cx + wpx + 1.0f)); }
            rc.top = static_cast<LONG>(lroundf(cy - hpix * 0.5f));
            rc.bottom = static_cast<LONG>(lroundf(cy + hpix * 0.5f));

            const int alpha = static_cast<int>(t.opacity * 255.0f);
            if (t.opacity < 0.999f)
                canvas.pushOpacity(alpha);
            // 旋转：绕锚点 (cx, cy)。CAD 旋转角是逆时针、SVG 是顺时针，导出侧已换算成负角，
            // 这里只按 SVG 语义交给后端（D2D 正值顺时针）。
            const bool rotated = fabsf(t.rotate) > 0.01f;
            if (rotated)
            {
                POINT piv = { static_cast<LONG>(lroundf(cx)), static_cast<LONG>(lroundf(cy)) };
                canvas.pushRotate(static_cast<int>(lroundf(t.rotate)), piv);
            }
            canvas.text(rc, t.text, t.color, DT_LEFT, pt, family);
            if (rotated)
                canvas.popTransform();
            if (t.opacity < 0.999f)
                canvas.popOpacity();
        }
    }

    // ======================== XML 场景（<XScene> 声明式显示列表）========================
    //
    // 和 SVG 那条路共用 SvgScene / drawSvgScene，只是换前端描述语言：
    //     <XScene Width="950" Height="613">
    //       <XLine X1="10" Y1="20" X2="300" Y2="20" Stroke="#F2F4F6" Width="1"/>
    //       <XPolyline Points="10,10 200,10 200,80" Closed="1" Stroke="#F2F4F6"/>
    //       <XCircle Cx="100" Cy="100" R="40" Stroke="#FF0000"/>
    //       <XArc Cx="100" Cy="100" R="40" Start="0" End="90" Stroke="#FF0000"/>
    //       <XRect X="10" Y="10" W="80" H="40" Stroke="#F2F4F6"/>
    //       <XGroup Transform="translate(300 200) rotate(-30) scale(0.5 0.5)">
    //         <XCircle .../>
    //       </XGroup>
    //       <XText X="10" Y="30" Size="14" Color="#F2F4F6" Anchor="start">门厅说明</XText>
    //     </XScene>
    //
    // 为什么不是"把 XML 转成 SVG 再喂 nanosvg"：那样等于在同一份内容上做两次词法
    // 解析 + 一次字符串重写；万级图元时这段开销比绘制本身还大。
    namespace
    {
        struct Xf
        {
            float a = 1.0f, b = 0.0f, c = 0.0f, d = 1.0f, e = 0.0f, f = 0.0f;
        };

        inline Xf xfMul(const Xf& m, const Xf& n)   // m * n（n 后应用，即先 n 再 m）
        {
            Xf r;
            r.a = m.a * n.a + m.c * n.b;
            r.b = m.b * n.a + m.d * n.b;
            r.c = m.a * n.c + m.c * n.d;
            r.d = m.b * n.c + m.d * n.d;
            r.e = m.a * n.e + m.c * n.f + m.e;
            r.f = m.b * n.e + m.d * n.f + m.f;
            return r;
        }

        inline SvgPoint xfApply(const Xf& m, float x, float y)
        {
            return SvgPoint{ m.a * x + m.c * y + m.e, m.b * x + m.d * y + m.f };
        }

        inline float xfScale(const Xf& m)
        {
            const float sx = sqrtf(m.a * m.a + m.b * m.b);
            return sx > 1e-6f ? sx : 1.0f;
        }

        inline char lowerAscii(char ch)
        {
            return (ch >= 'A' && ch <= 'Z') ? static_cast<char>(ch - 'A' + 'a') : ch;
        }

        inline bool startsWithNoCase(const std::string& s, const char* p)
        {
            size_t i = 0;
            while (*p != 0)
            {
                if (i >= s.size() || lowerAscii(s[i]) != lowerAscii(*p))
                    return false;
                ++i;
                ++p;
            }
            return i == s.size();
        }

        // XML 实体解码（&amp; &lt; &gt; &quot; &apos; &#xNN; &#NN;）
        std::string xmlDecodeEntities(const std::string& s);

        std::wstring utf8ToW(const std::string& s)
        {
            if (s.empty())
                return std::wstring();
            const int n = ::MultiByteToWideChar(CP_UTF8, 0, s.c_str(),
                                                static_cast<int>(s.size()), nullptr, 0);
            std::wstring w(static_cast<size_t>(n), L'\0');
            if (n > 0)
                ::MultiByteToWideChar(CP_UTF8, 0, s.c_str(), static_cast<int>(s.size()),
                                      w.data(), n);
            return w;
        }

        // 属性取值：tag 形如 `<XLine X1="1" X2="2"`，名字大小写不敏感
        bool xAttr(const std::string& tag, const char* name, std::string& out)
        {
            out.clear();
            size_t i = 0;
            while (i < tag.size() && tag[i] != ' ' && tag[i] != '\t' && tag[i] != '\n' &&
                   tag[i] != '\r')
                ++i;                                    // 跳过标签名
            while (i < tag.size())
            {
                while (i < tag.size() && (tag[i] == ' ' || tag[i] == '\t' || tag[i] == '\n' ||
                                          tag[i] == '\r' || tag[i] == '/'))
                    ++i;
                if (i >= tag.size())
                    break;
                const size_t ns = i;
                while (i < tag.size() && tag[i] != '=' && tag[i] != ' ' && tag[i] != '/' &&
                       tag[i] != '>')
                    ++i;
                const std::string key = tag.substr(ns, i - ns);
                while (i < tag.size() && (tag[i] == ' ' || tag[i] == '\t'))
                    ++i;
                if (i >= tag.size() || tag[i] != '=')
                    continue;
                ++i;
                while (i < tag.size() && (tag[i] == ' ' || tag[i] == '\t'))
                    ++i;
                if (i >= tag.size())
                    break;
                const char quote = tag[i];
                if (quote != '"' && quote != '\'')
                    continue;
                ++i;
                const size_t vs = i;
                while (i < tag.size() && tag[i] != quote)
                    ++i;
                const std::string val = tag.substr(vs, i < tag.size() ? i - vs : 0);
                if (i < tag.size())
                    ++i;
                if (key.size() == strlen(name) && startsWithNoCase(key, name))
                {
                    out = xmlDecodeEntities(val);
                    return true;
                }
            }
            return false;
        }

        // XML 实体解码（&amp; &lt; &gt; &quot; &apos; &#xNN; &#NN;）
        std::string xmlDecodeEntities(const std::string& s)
        {
            if (s.find('&') == std::string::npos)
                return s;
            std::string out;
            out.reserve(s.size());
            for (size_t i = 0; i < s.size(); ++i)
            {
                if (s[i] != '&')
                {
                    out.push_back(s[i]);
                    continue;
                }
                const size_t semi = s.find(';', i);
                if (semi == std::string::npos || semi - i > 12)
                {
                    out.push_back(s[i]);
                    continue;
                }
                const std::string ent = s.substr(i + 1, semi - i - 1);
                if (ent == "amp") out.push_back('&');
                else if (ent == "lt") out.push_back('<');
                else if (ent == "gt") out.push_back('>');
                else if (ent == "quot") out.push_back('"');
                else if (ent == "apos") out.push_back('\'');
                else if (!ent.empty() && (ent[0] == '#'))
                {
                    const bool hex = ent.size() > 1 && (ent[1] == 'x' || ent[1] == 'X');
                    const unsigned long cp = hex ? strtoul(ent.c_str() + 2, nullptr, 16)
                                                 : strtoul(ent.c_str() + 1, nullptr, 10);
                    if (cp < 0x80)
                        out.push_back(static_cast<char>(cp));
                    else
                    {
                        char buf[8] = {};
                        const int n = ::WideCharToMultiByte(
                            CP_UTF8, 0, reinterpret_cast<const wchar_t*>(&cp), 1, buf,
                            static_cast<int>(sizeof(buf)), nullptr, nullptr);
                        if (n > 0)
                            out.append(buf, n);
                    }
                }
                else
                {
                    out.push_back(s[i]);
                    continue;
                }
                i = semi;
            }
            return out;
        }

        bool numOf(const std::string& tag, const char* name, float& out)
        {
            std::string v;
            if (!xAttr(tag, name, v) || v.empty())
                return false;
            out = static_cast<float>(atof(v.c_str()));
            return true;
        }

        // 颜色：#RRGGBB（不透明）/ #AARRGGBB（带 alpha，与 XAML 的 Background/ForeColor
        // 同一套写法）/ #RGB。alpha 在前，因为本项目 XML 侧一律是 AARRGGBB。
        bool colorOf(const std::string& v, COLORREF& out, float& opacity)
        {
            opacity = 1.0f;
            if (v.empty() || v == "none")
                return false;
            if (v[0] != '#')
                return false;
            const std::string hx = v.substr(1);
            unsigned long rgb = 0, alpha = 255;
            if (hx.size() >= 8)
            {
                alpha = strtoul(hx.substr(0, 2).c_str(), nullptr, 16);
                rgb = strtoul(hx.substr(2, 6).c_str(), nullptr, 16);
            }
            else if (hx.size() >= 6)
                rgb = strtoul(hx.substr(0, 6).c_str(), nullptr, 16);
            else if (hx.size() == 3)
            {
                const unsigned long r = strtoul(hx.substr(0, 1).c_str(), nullptr, 16);
                const unsigned long g = strtoul(hx.substr(1, 1).c_str(), nullptr, 16);
                const unsigned long b = strtoul(hx.substr(2, 1).c_str(), nullptr, 16);
                rgb = (r * 17u << 16) | (g * 17u << 8) | (b * 17u);
            }
            else
                return false;
            out = RGB((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
            opacity = static_cast<float>(alpha) / 255.0f;
            return true;
        }

        // Transform="translate(x y) rotate(deg) scale(sx sy)"，按 SVG 顺序左乘
        Xf parseTransform(const std::string& s)
        {
            Xf m;
            size_t i = 0;
            while (i < s.size())
            {
                while (i < s.size() && (s[i] == ' ' || s[i] == ',' || s[i] == '\t' || s[i] == ';'))
                    ++i;
                const size_t ns = i;
                while (i < s.size() && ((s[i] >= 'a' && s[i] <= 'z') ||
                                        (s[i] >= 'A' && s[i] <= 'Z')))
                    ++i;
                if (i == ns)
                    break;
                const std::string fn = s.substr(ns, i - ns);
                while (i < s.size() && (s[i] == ' ' || s[i] == '\t'))
                    ++i;
                if (i >= s.size() || s[i] != '(')
                    break;
                ++i;
                const size_t as = i;
                while (i < s.size() && s[i] != ')')
                    ++i;
                const std::string args = s.substr(as, i < s.size() ? i - as : 0);
                if (i < s.size())
                    ++i;
                float a[6] = { 0, 0, 0, 0, 0, 0 };
                int cnt = 0;
                {
                    size_t k = 0;
                    while (k < args.size() && cnt < 6)
                    {
                        while (k < args.size() && (args[k] == ' ' || args[k] == ',' || args[k] == '\t'))
                            ++k;
                        if (k >= args.size())
                            break;
                        a[cnt++] = static_cast<float>(atof(args.c_str() + k));
                        while (k < args.size() && args[k] != ' ' && args[k] != ',' && args[k] != '\t')
                            ++k;
                    }
                }
                Xf n;
                if (fn == "translate")
                {
                    n.e = a[0];
                    n.f = cnt > 1 ? a[1] : 0.0f;
                }
                else if (fn == "scale")
                {
                    n.a = a[0];
                    n.d = cnt > 1 ? a[1] : a[0];
                }
                else if (fn == "rotate")
                {
                    const float rad = static_cast<float>(a[0] * 3.14159265358979 / 180.0);
                    n.a = cosf(rad);
                    n.b = sinf(rad);
                    n.c = -sinf(rad);
                    n.d = cosf(rad);
                    if (cnt >= 3)        // rotate(a cx cy) = T(cx,cy) R T(-cx,-cy)
                    {
                        Xf t1; t1.e = a[1]; t1.f = a[2];
                        Xf t2; t2.e = -a[1]; t2.f = -a[2];
                        n = xfMul(xfMul(t1, n), t2);
                    }
                }
                else
                    continue;
                m = xfMul(m, n);
            }
            return m;
        }

        // 圆弧拍平（度，逆时针；画布 y 向下，所以角度取反再采样）
        void flattenArc(float cx, float cy, float r, float a1deg, float a2deg,
                        const Xf& m, std::vector<SvgPoint>& out)
        {
            float sweep = a2deg - a1deg;
            while (sweep <= -360.0f) sweep += 360.0f;
            while (sweep > 360.0f) sweep -= 360.0f;   // 整圆 = 360，不能归一成 0
            if (sweep < 0.0f)
                sweep += 360.0f;                       // CAD 的起止角按逆时针走
            if (sweep < 0.5f)
                sweep = 360.0f;                        // start==end 视为整圆
            const int segs = static_cast<int>(ceilf(fabsf(sweep) / 4.0f));
            const int n = segs < 2 ? 2 : (segs > 180 ? 180 : segs);
            for (int i = 0; i <= n; ++i)
            {
                const float t = a1deg + sweep * (static_cast<float>(i) / n);
                const float rad = static_cast<float>(t * 3.14159265358979 / 180.0);
                out.push_back(xfApply(m, cx + r * cosf(rad), cy - r * sinf(rad)));
            }
        }

        // 点列 "x,y x,y" 或 "x y x y"
        void parsePoints(const std::string& s, std::vector<SvgPoint>& out)
        {
            std::vector<float> nums;
            size_t i = 0;
            while (i < s.size())
            {
                while (i < s.size() && (s[i] == ' ' || s[i] == ',' || s[i] == '\t' ||
                                        s[i] == '\n' || s[i] == '\r'))
                    ++i;
                if (i >= s.size())
                    break;
                nums.push_back(static_cast<float>(atof(s.c_str() + i)));
                while (i < s.size() && s[i] != ' ' && s[i] != ',' && s[i] != '\t' &&
                       s[i] != '\n' && s[i] != '\r')
                    ++i;
            }
            for (size_t k = 0; k + 1 < nums.size(); k += 2)
                out.push_back(SvgPoint{ nums[k], nums[k + 1] });
        }


        // 虚线：Dash="实线段,空档"（也接受 DashArray / dasharray）
        void readDash(const std::string& tag, SvgShape& sh)
        {
            std::string v;
            if (!xAttr(tag, "dash", v) && !xAttr(tag, "dasharray", v))
                return;
            std::vector<SvgPoint> nums;
            parsePoints(v, nums);
            if (nums.empty())
                return;
            sh.dashOn = nums[0].x;
            sh.dashOff = nums.size() > 1 ? nums[1].x : nums[0].x;
            if (sh.dashOn < 0.5f) sh.dashOn = 0.5f;
            if (sh.dashOff < 0.5f) sh.dashOff = 0.5f;
        }

        bool truthy(const std::string& v)
        {
            return v == "1" || v == "true" || v == "True" || v == "yes";
        }
    }

    bool parseXmlSceneText(const std::string& utf8, SvgScene& out, std::wstring& err)
    {
        err.clear();
        out.shapes.clear();
        out.texts.clear();
        out.warnings.clear();
        out.width = 0.0f;
        out.height = 0.0f;
        out.pathCount = 0;

        if (utf8.empty())
        {
            err = L"XML 场景内容为空";
            return false;
        }

        Xf stack[16];
        int depth = 0;
        stack[0] = Xf();

        size_t i = 0;
        while (i < utf8.size())
        {
            const size_t lt = utf8.find('<', i);
            if (lt == std::string::npos)
                break;

            // 注释 / 声明 / DOCTYPE 跳过
            if (utf8.compare(lt, 4, "<!--") == 0)
            {
                const size_t end = utf8.find("-->", lt + 4);
                i = (end == std::string::npos) ? utf8.size() : end + 3;
                continue;
            }
            if (utf8.compare(lt, 2, "<?") == 0 || utf8.compare(lt, 2, "<!") == 0)
            {
                const size_t end = utf8.find('>', lt + 2);
                i = (end == std::string::npos) ? utf8.size() : end + 1;
                continue;
            }

            // 结束标签：</XGroup> 之类，弹一层变换
            if (lt + 1 < utf8.size() && utf8[lt + 1] == '/')
            {
                size_t end = utf8.find('>', lt + 2);
                if (end == std::string::npos)
                    end = utf8.size() - 1;
                std::string name;
                size_t k = lt + 2;
                while (k < utf8.size() && (isalnum(static_cast<unsigned char>(utf8[k]))))
                {
                    name.push_back(lowerAscii(utf8[k]));
                    ++k;
                }
                if (depth > 0 && (name == "xgroup" || name == "group"))
                    --depth;
                i = end + 1;
                continue;
            }

            // 起标签：读到 '>'，注意属性值里可能有 '>'（用引号状态机）
            size_t end = lt + 1;
            bool inQuote = false;
            char qch = 0;
            while (end < utf8.size())
            {
                const char ch = utf8[end];
                if (inQuote)
                {
                    if (ch == qch)
                        inQuote = false;
                }
                else if (ch == '"' || ch == '\'')
                {
                    inQuote = true;
                    qch = ch;
                }
                else if (ch == '>')
                    break;
                ++end;
            }
            if (end >= utf8.size())
                break;
            const std::string tag = utf8.substr(lt + 1, end - lt - 1);
            const bool selfClose = tag.find_last_not_of(" \t\r\n") != std::string::npos &&
                                   tag[tag.find_last_not_of(" \t\r\n")] == '/';

            size_t ns = 0;
            while (ns < tag.size() && (tag[ns] == ' ' || tag[ns] == '\t' || tag[ns] == '\n'))
                ++ns;
            std::string name;
            size_t k = ns;
            while (k < tag.size() && (isalnum(static_cast<unsigned char>(tag[k])) ||
                                      tag[k] == '_' || tag[k] == '-'))
            {
                name.push_back(lowerAscii(tag[k]));
                ++k;
            }

            // 去掉 X 前缀：XLine -> line，XScene -> scene
            std::string kind = name;
            if (kind.size() > 1 && kind[0] == 'x')
            {
                const std::string rest = kind.substr(1);
                if (rest == "scene" || rest == "line" || rest == "circle" || rest == "arc" ||
                    rest == "rect" || rest == "polyline" || rest == "polygon" ||
                    rest == "text" || rest == "path" || rest == "group")
                    kind = rest;
            }

            const Xf cur = stack[depth];

            if (kind == "scene")
            {
                float w = 0.0f, h = 0.0f;
                if (numOf(tag, "width", w))
                    out.width = w;
                if (numOf(tag, "height", h))
                    out.height = h;
            }
            else if (kind == "group")
            {
                std::string tf;
                if (xAttr(tag, "transform", tf))
                {
                    const Xf m = xfMul(cur, parseTransform(tf));
                    if (depth + 1 < 16)
                    {
                        stack[depth + 1] = m;
                        ++depth;
                    }
                }
                else if (depth + 1 < 16)
                {
                    stack[depth + 1] = cur;
                    ++depth;
                }
            }
            else if (kind == "line")
            {
                float x1 = 0, y1 = 0, x2 = 0, y2 = 0;
                if (numOf(tag, "x1", x1) && numOf(tag, "y1", y1) && numOf(tag, "x2", x2) &&
                    numOf(tag, "y2", y2))
                {
                    SvgShape sh;
                    sh.opacity = 1.0f;
                    std::string col;
                    if (xAttr(tag, "stroke", col) || xAttr(tag, "color", col))
                    {
                        COLORREF c = 0;
                        float op = 1.0f;
                        if (colorOf(col, c, op))
                        {
                            sh.hasStroke = true;
                            sh.stroke = c;
                            sh.opacity = op;
                        }
                    }
                    float w = 1.0f;
                    if (!numOf(tag, "width", w))
                        numOf(tag, "strokewidth", w);
                    sh.strokeWidth = w > 0.0f ? w : 1.0f;
                    readDash(tag, sh);
                    if (sh.hasStroke)
                    {
                        SvgContour ct;
                        ct.pts.push_back(xfApply(cur, x1, y1));
                        ct.pts.push_back(xfApply(cur, x2, y2));
                        sh.contours.push_back(std::move(ct));
                        out.shapes.push_back(std::move(sh));
                        out.pathCount++;
                    }
                }
            }
            else if (kind == "circle")
            {
                float cx = 0, cy = 0, r = 0;
                if (numOf(tag, "cx", cx) && numOf(tag, "cy", cy) && numOf(tag, "r", r) && r > 0)
                {
                    SvgShape sh;
                    sh.opacity = 1.0f;
                    std::string col;
                    if (xAttr(tag, "stroke", col) || xAttr(tag, "color", col))
                    {
                        COLORREF c = 0;
                        float op = 1.0f;
                        if (colorOf(col, c, op))
                        {
                            sh.hasStroke = true;
                            sh.stroke = c;
                            sh.opacity = op;
                        }
                    }
                    float w = 1.0f;
                    if (!numOf(tag, "width", w))
                        numOf(tag, "strokewidth", w);
                    sh.strokeWidth = w > 0.0f ? w : 1.0f;
                    readDash(tag, sh);
                    if (sh.hasStroke)
                    {
                        SvgContour ct;
                        ct.closed = true;
                        flattenArc(cx, cy, r, 0.0f, 360.0f, cur, ct.pts);
                        sh.contours.push_back(std::move(ct));
                        out.shapes.push_back(std::move(sh));
                        out.pathCount++;
                    }
                }
            }
            else if (kind == "arc")
            {
                float cx = 0, cy = 0, r = 0, a1 = 0, a2 = 0;
                if (numOf(tag, "cx", cx) && numOf(tag, "cy", cy) && numOf(tag, "r", r) &&
                    numOf(tag, "start", a1) && numOf(tag, "end", a2) && r > 0)
                {
                    SvgShape sh;
                    sh.opacity = 1.0f;
                    std::string col;
                    if (xAttr(tag, "stroke", col) || xAttr(tag, "color", col))
                    {
                        COLORREF c = 0;
                        float op = 1.0f;
                        if (colorOf(col, c, op))
                        {
                            sh.hasStroke = true;
                            sh.stroke = c;
                            sh.opacity = op;
                        }
                    }
                    float w = 1.0f;
                    if (!numOf(tag, "width", w))
                        numOf(tag, "strokewidth", w);
                    sh.strokeWidth = w > 0.0f ? w : 1.0f;
                    readDash(tag, sh);
                    if (sh.hasStroke)
                    {
                        SvgContour ct;
                        flattenArc(cx, cy, r, a1, a2, cur, ct.pts);
                        sh.contours.push_back(std::move(ct));
                        out.shapes.push_back(std::move(sh));
                        out.pathCount++;
                    }
                }
            }
            else if (kind == "rect")
            {
                float x = 0, y = 0, w = 0, h = 0;
                const bool bySize = numOf(tag, "x", x) && numOf(tag, "y", y) &&
                                    numOf(tag, "w", w) && numOf(tag, "h", h);
                if (!bySize)
                {
                    float x2 = 0, y2 = 0;
                    if (numOf(tag, "x1", x) && numOf(tag, "y1", y) && numOf(tag, "x2", x2) &&
                        numOf(tag, "y2", y2))
                    {
                        w = x2 - x;
                        h = y2 - y;
                    }
                }
                if (w > 0.0f || h > 0.0f)
                {
                    SvgShape sh;
                    sh.opacity = 1.0f;
                    std::string col;
                    if (xAttr(tag, "stroke", col) || xAttr(tag, "color", col))
                    {
                        COLORREF c = 0;
                        float op = 1.0f;
                        if (colorOf(col, c, op))
                        {
                            sh.hasStroke = true;
                            sh.stroke = c;
                            sh.opacity = op;
                        }
                    }
                    float sw = 1.0f;
                    if (!numOf(tag, "width", sw))
                        numOf(tag, "strokewidth", sw);
                    sh.strokeWidth = sw > 0.0f ? sw : 1.0f;
                    readDash(tag, sh);

                    std::string fcol;
                    if (xAttr(tag, "fill", fcol))
                    {
                        COLORREF fc = 0;
                        float fo = 1.0f;
                        if (colorOf(fcol, fc, fo))
                        {
                            sh.hasFill = true;
                            sh.fill = fc;
                            if (fo < sh.opacity)
                                sh.opacity = fo;
                        }
                    }
                    if (sh.hasStroke || sh.hasFill)
                    {
                        SvgContour ct;
                        ct.closed = true;
                        ct.pts.push_back(xfApply(cur, x, y));
                        ct.pts.push_back(xfApply(cur, x + w, y));
                        ct.pts.push_back(xfApply(cur, x + w, y + h));
                        ct.pts.push_back(xfApply(cur, x, y + h));
                        sh.contours.push_back(std::move(ct));
                        out.shapes.push_back(std::move(sh));
                        out.pathCount++;
                    }
                }
            }
            else if (kind == "polyline" || kind == "polygon")
            {
                std::string pts;
                if (xAttr(tag, "points", pts) || xAttr(tag, "d", pts))
                {
                    std::vector<SvgPoint> raw;
                    parsePoints(pts, raw);
                    if (raw.size() >= 2)
                    {
                        SvgShape sh;
                        sh.opacity = 1.0f;
                        std::string col;
                        if (xAttr(tag, "stroke", col) || xAttr(tag, "color", col))
                        {
                            COLORREF c = 0;
                            float op = 1.0f;
                            if (colorOf(col, c, op))
                            {
                                sh.hasStroke = true;
                                sh.stroke = c;
                                sh.opacity = op;
                            }
                        }
                        float w = 1.0f;
                        if (!numOf(tag, "width", w))
                            numOf(tag, "strokewidth", w);
                        sh.strokeWidth = w > 0.0f ? w : 1.0f;
                        readDash(tag, sh);

                        std::string fcol;
                        if (xAttr(tag, "fill", fcol))
                        {
                            COLORREF fc = 0;
                            float fo = 1.0f;
                            if (colorOf(fcol, fc, fo))
                            {
                                sh.hasFill = true;
                                sh.fill = fc;
                                if (fo < sh.opacity)
                                    sh.opacity = fo;
                            }
                        }
                        std::string closed;
                        SvgContour ct;
                        ct.closed = (kind == "polygon") ||
                                    (xAttr(tag, "closed", closed) && truthy(closed));
                        for (const SvgPoint& p : raw)
                            ct.pts.push_back(xfApply(cur, p.x, p.y));
                        if (sh.hasStroke || sh.hasFill)
                        {
                            sh.contours.push_back(std::move(ct));
                            out.shapes.push_back(std::move(sh));
                        }
                        else
                        {
                            sh.hasStroke = false;
                        }
                        out.pathCount++;
                    }
                }
            }
            else if (kind == "path")
            {
                std::string d;
                if (xAttr(tag, "d", d))
                {
                    // 只吃机器生成的 M/L/Z：CAD 侧的圆弧改走 <XArc>，这里不做 A 拟合
                    std::vector<SvgContour> contours;
                    SvgContour cur2;
                    std::vector<float> nums;
                    char cmd = 0;
                    size_t k2 = 0;
                    bool bad = false;
                    while (k2 < d.size() && !bad)
                    {
                        while (k2 < d.size() && (d[k2] == ' ' || d[k2] == ',' || d[k2] == '\t'))
                            ++k2;
                        if (k2 >= d.size())
                            break;
                        const char ch = d[k2];
                        if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z'))
                        {
                            cmd = ch;
                            ++k2;
                            if (cmd == 'Z' || cmd == 'z')
                            {
                                cur2.closed = true;
                                if (cur2.pts.size() >= 2)
                                    contours.push_back(cur2);
                                cur2 = SvgContour();
                            }
                            else if (cmd == 'A' || cmd == 'a')
                                bad = true;             // 不支持弧命令（见上）
                            continue;
                        }
                        nums.clear();
                        while (k2 < d.size() && ((d[k2] >= '0' && d[k2] <= '9') || d[k2] == '.' ||
                                                 d[k2] == '-' || d[k2] == '+' || d[k2] == 'e' ||
                                                 d[k2] == 'E'))
                        {
                            nums.push_back(static_cast<float>(atof(d.c_str() + k2)));
                            while (k2 < d.size() && ((d[k2] >= '0' && d[k2] <= '9') ||
                                                     d[k2] == '.' || d[k2] == '-' ||
                                                     d[k2] == '+' || d[k2] == 'e' ||
                                                     d[k2] == 'E'))
                                ++k2;
                            while (k2 < d.size() && (d[k2] == ' ' || d[k2] == ',' || d[k2] == '\t'))
                                ++k2;
                        }
                        if (nums.size() >= 2)
                        {
                            if (cmd == 'M' || cmd == 'm' || cmd == 'L' || cmd == 'l' || cmd == 0)
                            {
                                cur2.pts.push_back(xfApply(cur, nums[0], nums[1]));
                                cmd = 'L';
                            }
                        }
                    }
                    if (cur2.pts.size() >= 2)
                        contours.push_back(cur2);

                    SvgShape sh;
                    sh.opacity = 1.0f;
                    std::string col;
                    if (xAttr(tag, "stroke", col) || xAttr(tag, "color", col))
                    {
                        COLORREF c = 0;
                        float op = 1.0f;
                        if (colorOf(col, c, op))
                        {
                            sh.hasStroke = true;
                            sh.stroke = c;
                            sh.opacity = op;
                        }
                    }
                    float w = 1.0f;
                    if (!numOf(tag, "width", w))
                        numOf(tag, "strokewidth", w);
                    sh.strokeWidth = w > 0.0f ? w : 1.0f;
                    readDash(tag, sh);
                    std::string fcol;
                    if (xAttr(tag, "fill", fcol))
                    {
                        COLORREF fc = 0;
                        float fo = 1.0f;
                        if (colorOf(fcol, fc, fo))
                        {
                            sh.hasFill = true;
                            sh.fill = fc;
                            if (fo < sh.opacity)
                                sh.opacity = fo;
                        }
                    }
                    if ((sh.hasStroke || sh.hasFill) && !contours.empty())
                    {
                        sh.contours = std::move(contours);
                        out.shapes.push_back(std::move(sh));
                        out.pathCount++;
                    }
                    if (bad)
                        out.warnings.push_back(L"<XPath> 含 A 命令，已跳过该段（圆弧请用 <XArc>）");
                }
            }
            else if (kind == "text")
            {
                float x = 0, y = 0, size = 0;
                if (numOf(tag, "x", x) && numOf(tag, "y", y))
                {
                    if (!numOf(tag, "size", size))
                        numOf(tag, "fontsize", size);
                    if (size <= 0.0f)
                        size = 12.0f;

                    SvgText t;
                    t.sizePx = size;
                    t.weight = 400;
                    t.family = L"Microsoft YaHei";
                    t.opacity = 1.0f;
                    const SvgPoint p = xfApply(cur, x, y);
                    t.x = p.x;
                    t.y = p.y;

                    std::string col;
                    if (xAttr(tag, "color", col) || xAttr(tag, "fill", col))
                    {
                        COLORREF c = 0;
                        float op = 1.0f;
                        if (colorOf(col, c, op))
                        {
                            t.color = c;
                            t.opacity = op;
                        }
                    }
                    std::string anchor;
                    if (xAttr(tag, "anchor", anchor))
                        t.anchor = (anchor == "middle") ? 1 : (anchor == "end") ? 2 : 0;
                    std::string rotate;
                    if (xAttr(tag, "rotate", rotate))
                        t.rotate = static_cast<float>(atof(rotate.c_str()));
                    std::string fam;
                    if (xAttr(tag, "family", fam) || xAttr(tag, "fontfamily", fam))
                        t.family = utf8ToW(fam);
                    std::string weight;
                    if (xAttr(tag, "weight", weight))
                        t.weight = atoi(weight.c_str());

                    // 文字内容：优先 Text 属性，其次标签内文本
                    std::string text;
                    if (xAttr(tag, "text", text) && !text.empty())
                        t.text = utf8ToW(text);
                    else if (!selfClose)
                    {
                        const size_t gt = end + 1;
                        const size_t close = utf8.find("</", gt);
                        if (close != std::string::npos && close > gt)
                        {
                            text = xmlDecodeEntities(utf8.substr(gt, close - gt));
                            t.text = utf8ToW(text);
                            end = utf8.find('>', close);
                            if (end == std::string::npos)
                                end = close + 1;
                        }
                    }
                    if (!t.text.empty() && t.color != 0)
                        out.texts.push_back(std::move(t));
                    else if (!t.text.empty())
                    {
                        t.color = RGB(255, 255, 255);
                        out.texts.push_back(std::move(t));
                    }
                }
            }

            i = end + 1;
        }

        if (out.shapes.empty() && out.texts.empty())
        {
            err = L"XML 场景里没有可绘制的图元";
            return false;
        }
        if (out.width <= 0.0f || out.height <= 0.0f)
        {
            float mx = 0.0f, my = 0.0f;
            for (const SvgShape& sh : out.shapes)
                for (const SvgContour& ct : sh.contours)
                    for (const SvgPoint& pt : ct.pts)
                    {
                        if (pt.x > mx) mx = pt.x;
                        if (pt.y > my) my = pt.y;
                    }
            for (const SvgText& t : out.texts)
            {
                if (t.x > mx) mx = t.x;
                if (t.y > my) my = t.y;
            }
            out.width = mx + 1.0f;
            out.height = my + 1.0f;
        }
        return true;
    }
}

