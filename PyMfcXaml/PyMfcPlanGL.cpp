#include "stdafx.h"
#include "PyMfcPlanGL.h"
#include <gl/GL.h>
#pragma comment(lib, "opengl32.lib")

namespace pymfcgl
{
    namespace
    {
        struct Ctx
        {
            HDC dc = nullptr;
            HGLRC rc = nullptr;
            int w = 0, h = 0;
        };
        std::map<HWND, Ctx> g_ctx;
        bool g_probed = false, g_ok = false;
        FILE* g_log = nullptr;

        void logf(const char* s)
        {
            if (g_log == nullptr)
            {
                g_log = ::fopen("G:\\PyArx\\docs\\_plan_gl.log", "a");
                if (g_log == nullptr) return;
            }
            ::fprintf(g_log, "%s\n", s);
            ::fflush(g_log);
        }

        // ---------------- 字形图集（GL_ALPHA，按需光栅化） ----------------
        const int ATLAS_W = 2048, ATLAS_H = 2048;
        GLuint g_atlas = 0;
        int g_ax = 1, g_ay = 1, g_rowH = 0;
        struct Glyph
        {
            int x = 0, y = 0, w = 0, h = 0, adv = 0, top = 0;
            bool valid = false;
        };
        struct FKey
        {
            std::wstring face; int pt = 12; bool bold = false;
            bool operator<(const FKey& o) const
            {
                if (pt != o.pt) return pt < o.pt;
                if (bold != o.bold) return bold < o.bold;
                return face < o.face;
            }
        };
        std::map<FKey, HFONT> g_fonts;
        std::map<std::pair<FKey, wchar_t>, Glyph> g_glyphs;

        HFONT fontOf(const FKey& k)
        {
            auto it = g_fonts.find(k);
            if (it != g_fonts.end()) return it->second;
            LOGFONTW lf = {};
            lf.lfHeight = -k.pt;
            lf.lfWeight = k.bold ? FW_SEMIBOLD : FW_NORMAL;
            lf.lfQuality = CLEARTYPE_QUALITY;
            lf.lfCharSet = DEFAULT_CHARSET;
            wcsncpy_s(lf.lfFaceName, k.face.c_str(), _TRUNCATE);
            HFONT f = ::CreateFontIndirectW(&lf);
            g_fonts[k] = f;
            return f;
        }

        void ensureAtlas()
        {
            if (g_atlas != 0) return;
            ::glGenTextures(1, &g_atlas);
            ::glBindTexture(GL_TEXTURE_2D, g_atlas);
            ::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            ::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            ::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
            ::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
            std::vector<unsigned char> zero(ATLAS_W * ATLAS_H, 0);
            ::glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, ATLAS_W, ATLAS_H, 0,
                           GL_ALPHA, GL_UNSIGNED_BYTE, zero.data());
        }

        const Glyph& glyphOf(const FKey& fk, wchar_t ch)
        {
            const std::pair<FKey, wchar_t> key(fk, ch);
            auto it = g_glyphs.find(key);
            if (it != g_glyphs.end()) return it->second;
            Glyph g;
            HFONT font = fontOf(fk);
            HDC dc = ::CreateCompatibleDC(nullptr);
            HGDIOBJ old = ::SelectObject(dc, font);
            SIZE sz = { 0, 0 };
            ::GetTextExtentPoint32W(dc, &ch, 1, &sz);
            TEXTMETRICW tm = {};
            ::GetTextMetricsW(dc, &tm);
            int gw = sz.cx > 0 ? sz.cx : 1;
            int gh = (tm.tmHeight > 0 ? tm.tmHeight : fk.pt) + 2;
            g.adv = sz.cx;
            g.top = tm.tmAscent;
            g.h = gh;
            g.w = gw;

            if (g_ax + gw + 2 >= ATLAS_W)
            {
                g_ax = 1;
                g_ay += g_rowH + 2;
                g_rowH = 0;
            }
            if (g_ay + gh + 2 < ATLAS_H)
            {
                // 8bpp 掩膜：黑底白字，上传成 GL_ALPHA
                BITMAPINFO bi = {};
                bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                bi.bmiHeader.biWidth = gw;
                bi.bmiHeader.biHeight = -gh;             // 自顶向下
                bi.bmiHeader.biPlanes = 1;
                bi.bmiHeader.biBitCount = 8;
                bi.bmiHeader.biCompression = BI_RGB;
                bi.bmiHeader.biClrUsed = 256;
                void* bits = nullptr;
                HBITMAP bmp = ::CreateDIBSection(dc, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
                if (bmp != nullptr)
                {
                    HGDIOBJ oldBmp = ::SelectObject(dc, bmp);
                    ::SetBkMode(dc, TRANSPARENT);
                    ::SetTextColor(dc, RGB(255, 255, 255));
                    ::ExtTextOutW(dc, 0, 0, 0, nullptr, &ch, 1, nullptr);
                    unsigned char* p = static_cast<unsigned char*>(bits);
                    std::vector<unsigned char> buf(static_cast<size_t>(gw) * gh);
                    for (int y = 0; y < gh; ++y)
                        for (int x = 0; x < gw; ++x)
                            buf[static_cast<size_t>(y) * gw + x] = p[static_cast<size_t>(y) * gw + x];
                    ensureAtlas();
                    ::glBindTexture(GL_TEXTURE_2D, g_atlas);
                    ::glTexSubImage2D(GL_TEXTURE_2D, 0, g_ax, g_ay, gw, gh, GL_ALPHA,
                                      GL_UNSIGNED_BYTE, buf.data());
                    g.x = g_ax;
                    g.y = g_ay;
                    g.valid = true;
                    g_ax += gw + 2;
                    if (gh > g_rowH) g_rowH = gh;
                    ::SelectObject(dc, oldBmp);
                    ::DeleteObject(bmp);
                }
            }
            ::SelectObject(dc, old);
            ::DeleteDC(dc);
            g_glyphs[key] = g;
            return g_glyphs[key];
        }

        // 对齐：0 左 / 1 中 / 2 右；返回整行宽度
        int textWidth(const FKey& fk, const std::wstring& s)
        {
            int wsum = 0;
            for (wchar_t c : s) wsum += glyphOf(fk, c).adv;
            return wsum;
        }

        void drawText(const FKey& fk, int x, int top, int align, const std::wstring& s,
                      COLORREF col, int clipL, int clipR)
        {
            const int tw = textWidth(fk, s);
            int cx = x;
            if (align == 1) cx = x - tw / 2;
            else if (align == 2) cx = x - tw;
            if (cx > clipR || cx + tw < clipL) return;
            ensureAtlas();
            ::glEnable(GL_TEXTURE_2D);
            ::glBindTexture(GL_TEXTURE_2D, g_atlas);
            ::glColor4ub(GetRValue(col), GetGValue(col), GetBValue(col), 255);
            ::glBegin(GL_QUADS);
            for (wchar_t c : s)
            {
                const Glyph& g = glyphOf(fk, c);
                if (g.valid && g.w > 0 && g.h > 0)
                {
                    const float u0 = static_cast<float>(g.x) / ATLAS_W;
                    const float v0 = static_cast<float>(g.y) / ATLAS_H;
                    const float u1 = static_cast<float>(g.x + g.w) / ATLAS_W;
                    const float v1 = static_cast<float>(g.y + g.h) / ATLAS_H;
                    const float y0 = static_cast<float>(top);
                    const float y1 = static_cast<float>(top + g.h);
                    const float x0 = static_cast<float>(cx);
                    const float x1 = static_cast<float>(cx + g.w);
                    ::glTexCoord2f(u0, v0); ::glVertex2f(x0, y0);
                    ::glTexCoord2f(u1, v0); ::glVertex2f(x1, y0);
                    ::glTexCoord2f(u1, v1); ::glVertex2f(x1, y1);
                    ::glTexCoord2f(u0, v1); ::glVertex2f(x0, y1);
                }
                cx += g.adv;
            }
            ::glEnd();
            ::glDisable(GL_TEXTURE_2D);
        }

        void fillQuad(int x0, int y0, int x1, int y1, COLORREF c)
        {
            ::glColor4ub(GetRValue(c), GetGValue(c), GetBValue(c), 255);
            ::glBegin(GL_QUADS);
            ::glVertex2f(static_cast<float>(x0), static_cast<float>(y0));
            ::glVertex2f(static_cast<float>(x1), static_cast<float>(y0));
            ::glVertex2f(static_cast<float>(x1), static_cast<float>(y1));
            ::glVertex2f(static_cast<float>(x0), static_cast<float>(y1));
            ::glEnd();
        }

        void strokeBox(int x0, int y0, int x1, int y1, COLORREF c, int lw)
        {
            if (lw < 1) lw = 1;
            ::glColor4ub(GetRValue(c), GetGValue(c), GetBValue(c), 255);
            ::glBegin(GL_QUADS);
            auto q = [](float a, float b, float cc, float d) {
                ::glVertex2f(a, b); ::glVertex2f(cc, b); ::glVertex2f(cc, d); ::glVertex2f(a, d);
            };
            q(static_cast<float>(x0), static_cast<float>(y0), static_cast<float>(x1),
              static_cast<float>(y0 + lw));
            q(static_cast<float>(x0), static_cast<float>(y1 - lw), static_cast<float>(x1),
              static_cast<float>(y1));
            q(static_cast<float>(x0), static_cast<float>(y0 + lw), static_cast<float>(x0 + lw),
              static_cast<float>(y1 - lw));
            q(static_cast<float>(x1 - lw), static_cast<float>(y0 + lw), static_cast<float>(x1),
              static_cast<float>(y1 - lw));
            ::glEnd();
        }
    }

    bool enabled()
    {
        if (g_probed) return g_ok;
        g_probed = true;
        wchar_t v[8] = { 0 };
        g_ok = (::GetEnvironmentVariableW(L"PYARX_CANVAS_GL", v, 8) > 0 && v[0] == L'1');
        logf(g_ok ? "canvas-gl: enabled" : "canvas-gl: disabled (env)");
        return g_ok;
    }

    bool attach(HWND hwnd, int w, int h)
    {
        if (hwnd == nullptr || !::IsWindow(hwnd)) return false;
        auto it = g_ctx.find(hwnd);
        if (it != g_ctx.end())
        {
            if (w > 0 && h > 0 && (it->second.w != w || it->second.h != h))
            {
                it->second.w = w; it->second.h = h;
                if (::wglMakeCurrent(it->second.dc, it->second.rc))
                    ::glViewport(0, 0, w, h);
            }
            return it->second.rc != nullptr;
        }

        Ctx c;
        c.dc = ::GetDC(hwnd);
        if (c.dc == nullptr) { logf("canvas-gl: GetDC failed"); return false; }

        PIXELFORMATDESCRIPTOR pfd = {};
        pfd.nSize = sizeof(pfd);
        pfd.nVersion = 1;
        pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
        pfd.iPixelType = PFD_TYPE_RGBA;
        pfd.cColorBits = 32;
        pfd.cAlphaBits = 8;
        pfd.iLayerType = PFD_MAIN_PLANE;
        const int fmt = ::ChoosePixelFormat(c.dc, &pfd);
        if (fmt == 0 || !::SetPixelFormat(c.dc, fmt, &pfd))
        {
            logf("canvas-gl: SetPixelFormat failed");
            ::ReleaseDC(hwnd, c.dc);
            return false;
        }
        c.rc = ::wglCreateContext(c.dc);
        if (c.rc == nullptr || !::wglMakeCurrent(c.dc, c.rc))
        {
            logf("canvas-gl: wglCreateContext failed");
            if (c.rc) ::wglDeleteContext(c.rc);
            ::ReleaseDC(hwnd, c.dc);
            return false;
        }
        c.w = w > 0 ? w : 1;
        c.h = h > 0 ? h : 1;
        g_ctx[hwnd] = c;
        char buf[128];
        ::sprintf_s(buf, "canvas-gl: ok %dx%d ver=%s", c.w, c.h,
                    reinterpret_cast<const char*>(::glGetString(GL_VERSION)));
        logf(buf);
        return true;
    }

    void detach(HWND hwnd)
    {
        auto it = g_ctx.find(hwnd);
        if (it == g_ctx.end()) return;
        ::wglMakeCurrent(nullptr, nullptr);
        if (it->second.rc) ::wglDeleteContext(it->second.rc);
        if (it->second.dc) ::ReleaseDC(hwnd, it->second.dc);
        g_ctx.erase(it);
    }

    bool drawPlan(HWND hwnd, const xamlplan::PowerPlan& plan, float k, int ox, int oy,
                  const std::wstring& sel, bool showCable, int w, int h)
    {
        if (!attach(hwnd, w, h)) return false;
        auto it = g_ctx.find(hwnd);
        if (it == g_ctx.end() || it->second.rc == nullptr) return false;
        if (!::wglMakeCurrent(it->second.dc, it->second.rc)) return false;
        if (k <= 0.0f) k = 1.0f;

        ::glViewport(0, 0, w, h);
        ::glMatrixMode(GL_PROJECTION);
        ::glLoadIdentity();
        ::glOrtho(0.0, static_cast<GLdouble>(w), static_cast<GLdouble>(h), 0.0, -1.0, 1.0);
        ::glMatrixMode(GL_MODELVIEW);
        ::glLoadIdentity();
        ::glDisable(GL_DEPTH_TEST);
        ::glDisable(GL_TEXTURE_2D);
        ::glEnable(GL_BLEND);
        ::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        ::glClearColor(0xF3 / 255.0f, 0xF5 / 255.0f, 0xF8 / 255.0f, 1.0f);
        ::glClear(GL_COLOR_BUFFER_BIT);

        auto X = [&](double v) { return ox + static_cast<int>(lroundf(static_cast<float>(v) * k)); };
        auto Y = [&](double v) { return oy + static_cast<int>(lroundf(static_cast<float>(v) * k)); };
        auto PX = [&](double v) { return static_cast<int>(lroundf(static_cast<float>(v) * k)); };
        auto ptOf = [&](double px) { int p = static_cast<int>(px * k * 0.75 + 0.5); return p > 0 ? p : 1; };

        // 纸面
        const int px0 = X(0), py0 = Y(0), px1 = X(plan.width), py1 = Y(plan.height);
        fillQuad(px0, py0, px1, py1, RGB(0xFF, 0xFF, 0xFF));

        // 选中联动：父链 + 子树高亮
        std::map<std::wstring, size_t> idx;
        std::vector<size_t> up(plan.nodes.size(), (size_t)-1);
        std::vector<std::vector<size_t>> kids(plan.nodes.size());
        for (size_t i = 0; i < plan.nodes.size(); ++i) idx[plan.nodes[i].code] = i;
        for (size_t i = 0; i < plan.nodes.size(); ++i)
        {
            auto f = idx.find(plan.nodes[i].upstream);
            if (f != idx.end() && f->second != i) { up[i] = f->second; kids[f->second].push_back(i); }
        }
        std::set<std::wstring> hotEdges;
        if (!sel.empty())
        {
            auto f = idx.find(sel);
            if (f != idx.end())
            {
                size_t cur = f->second;
                while (up[cur] != (size_t)-1)
                {
                    hotEdges.insert(plan.nodes[up[cur]].code + L">" + plan.nodes[cur].code);
                    cur = up[cur];
                }
                std::vector<size_t> st2(1, f->second);
                while (!st2.empty())
                {
                    const size_t i = st2.back(); st2.pop_back();
                    for (size_t c : kids[i])
                    {
                        hotEdges.insert(plan.nodes[i].code + L">" + plan.nodes[c].code);
                        st2.push_back(c);
                    }
                }
            }
        }

        // 连线（折线，直线段；虚线按步长切段）
        ::glLineWidth(static_cast<float>(PX(2.0) > 1 ? PX(2.0) : 1));
        for (const xamlplan::PowerLink& l : plan.links)
        {
            const bool hot = hotEdges.count(l.from + L">" + l.to) > 0;
            COLORREF clr = hot ? RGB(0xE1, 0x57, 0x59)
                : (l.magenta ? RGB(0xC8, 0x00, 0xB4) : RGB(0x2F, 0x6F, 0xBA));
            const int pts[4][2] = { { X(l.bez[0]), Y(l.bez[1]) }, { X(l.bez[2]), Y(l.bez[3]) },
                                    { X(l.bez[4]), Y(l.bez[5]) }, { X(l.bez[6]), Y(l.bez[7]) } };
            int minx = pts[0][0], maxx = pts[0][0], miny = pts[0][1], maxy = pts[0][1];
            for (int i = 1; i < 4; ++i)
            {
                if (pts[i][0] < minx) minx = pts[i][0];
                if (pts[i][0] > maxx) maxx = pts[i][0];
                if (pts[i][1] < miny) miny = pts[i][1];
                if (pts[i][1] > maxy) maxy = pts[i][1];
            }
            if (maxx < 0 || minx > w || maxy < 0 || miny > h) continue;
            ::glColor4ub(GetRValue(clr), GetGValue(clr), GetBValue(clr), 255);
            ::glLineWidth(static_cast<float>(hot ? (PX(2.6) > 2 ? PX(2.6) : 2) : (PX(2.0) > 1 ? PX(2.0) : 1)));
            if (l.dashed)
            {
                const double dash = PX(6.0) > 1 ? PX(6.0) : 2.0;
                const double gapLen = PX(4.0) > 1 ? PX(4.0) : 2.0;
                double sPos = 0.0;
                ::glBegin(GL_LINES);
                for (int si = 1; si < 4; ++si)
                {
                    const double dx = pts[si][0] - pts[si - 1][0];
                    const double dy = pts[si][1] - pts[si - 1][1];
                    const double len = std::sqrt(dx * dx + dy * dy);
                    if (len <= 0.0) continue;
                    double t = 0.0;
                    while (t < len)
                    {
                        const double phase = std::fmod(sPos, dash + gapLen);
                        const double remain = (phase < dash) ? (dash - phase) : (dash + gapLen - phase);
                        double t2 = t + remain;
                        if (t2 > len) t2 = len;
                        const double u0 = t / len, u1 = t2 / len;
                        ::glVertex2f(static_cast<float>(pts[si - 1][0] + dx * u0),
                                     static_cast<float>(pts[si - 1][1] + dy * u0));
                        ::glVertex2f(static_cast<float>(pts[si - 1][0] + dx * u1),
                                     static_cast<float>(pts[si - 1][1] + dy * u1));
                        sPos += (t2 - t) + gapLen;
                        t = t2 + gapLen;
                    }
                }
                ::glEnd();
            }
            else
            {
                ::glBegin(GL_LINE_STRIP);
                for (int i = 0; i < 4; ++i)
                    ::glVertex2f(static_cast<float>(pts[i][0]), static_cast<float>(pts[i][1]));
                ::glEnd();
            }

            // 电缆标注
            if (showCable && !l.cable.empty() && 10.0 * k >= 1.5)
            {
                FKey fk; fk.face = L"Microsoft YaHei"; fk.pt = ptOf(10.0); fk.bold = false;
                const int lx = (std::min)(pts[2][0], pts[3][0]) + PX(6);
                if (!l.vertical)
                    drawText(fk, lx, pts[3][1] - PX(13), 0, l.cable, RGB(0x1F, 0x64, 0xB0), 0, w);
            }
        }

        // 卡片
        for (size_t i = 0; i < plan.boxes.size() && i < plan.nodes.size(); ++i)
        {
            const xamlplan::PowerBox& b = plan.boxes[i];
            const xamlplan::PowerNode& nd = plan.nodes[i];
            const int bx0 = X(b.x), by0 = Y(b.y), bx1 = X(b.x + b.w), by1 = Y(b.y + b.h);
            if (bx1 < 0 || bx0 > w || by1 < 0 || by0 > h) continue;

            const bool isRoot = (nd.role == L"root");
            const bool isTerm = (nd.role == L"terminal");
            const bool isSel = !sel.empty() && sel == nd.code;
            const bool isDual = nd.dual != 0;
            COLORREF border = isRoot ? RGB(0x22, 0x22, 0x22)
                : (isTerm ? RGB(0x74, 0x86, 0x9C) : RGB(0x8A, 0x94, 0xA6));
            if (isDual) border = RGB(0xE0, 0x24, 0x24);
            if (isSel) border = RGB(0xE1, 0x57, 0x59);
            COLORREF fill = isTerm ? RGB(0xEA, 0xF2, 0xFB)
                : (isDual ? RGB(0xFF, 0xF3, 0xF3) : RGB(0xFF, 0xFF, 0xFF));
            fillQuad(bx0, by0, bx1, by1, fill);

            const int span = bx1 - bx0;
            if (span < 10) continue;
            strokeBox(bx0, by0, bx1, by1, border,
                      PX(isSel ? 2.2 : (isRoot || isDual ? 2.0 : 1.0)));

            const xamlplan::VoltageStyle vs = xamlplan::voltStyle(nd.voltage);
            if (!vs.text.empty() && 8.5 * k >= 1.5)
            {
                FKey fk; fk.face = L"Microsoft YaHei"; fk.pt = ptOf(8.5); fk.bold = true;
                const int tw = textWidth(fk, vs.text) + PX(8);
                const int bh = (std::max)(1, PX(11.0));
                const int vx1 = bx1 - PX(4), vy0 = by0 + PX(3);
                fillQuad(vx1 - tw, vy0, vx1, vy0 + bh, static_cast<COLORREF>(vs.bg));
                drawText(fk, vx1 - tw / 2, vy0 + (bh - fk.pt) / 2, 1, vs.text,
                         static_cast<COLORREF>(vs.color), 0, w);
            }

            if (nd.code.empty() || 14.0 * k < 1.5) continue;
            std::vector<std::wstring> lines;
            lines.push_back(nd.name.empty() ? nd.code : nd.name);
            std::wstring l2;
            {
                wchar_t buf[48] = { 0 };
                if (nd.loadKw > 0.0) { swprintf_s(buf, L"%.3g kW", nd.loadKw); l2 = buf; }
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

            const int blockH = PX(16) + PX(13) * (static_cast<int>(lines.size()) - 1);
            const int top0 = by0 + static_cast<int>(lroundf((by1 - by0 - blockH) * 0.5f));
            for (size_t li = 0; li < lines.size(); ++li)
            {
                if (li > 0 && 10.5 * k < 1.5) break;
                FKey fk; fk.face = L"Microsoft YaHei";
                fk.pt = ptOf(li == 0 ? (isRoot ? 14.0 : 13.0) : 10.5);
                fk.bold = (li == 0);
                COLORREF col = li == 0 ? (isDual ? RGB(0xC0, 0x1B, 0x1B) : RGB(0x1F, 0x2A, 0x37))
                                       : RGB(0x1F, 0x64, 0xB0);
                const int cx = (bx0 + bx1) / 2;
                const int ty = top0 + PX(static_cast<int>(li) * 13);
                drawText(fk, cx, ty, 1, lines[li], col, 0, w);
            }

            if (nd.collapsedCount > 0 && 9.0 * k >= 1.5)
            {
                wchar_t buf[32] = { 0 };
                swprintf_s(buf, L"+ %d", nd.collapsedCount);
                FKey fk; fk.face = L"Microsoft YaHei"; fk.pt = ptOf(9.0); fk.bold = false;
                drawText(fk, bx1 - PX(6), by0 + PX(1), 2, buf, RGB(0x8A, 0x94, 0xA6), 0, w);
            }
        }

        ::glFlush();
        ::SwapBuffers(it->second.dc);
        return true;
    }
}
