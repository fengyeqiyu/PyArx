#include "XmlSvgScene.h"

#include <windows.h>
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
}
