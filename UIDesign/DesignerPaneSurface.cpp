#include "stdafx.h"
#include "../MfcXmlCore/controls/XmlMfcTags.h"   // 统一标签命名（X 前缀 -> 本意名）
#include "DesignerPaneView.h"
#include "DesignerPaneInternal.h"
#include "../MfcXmlCore/controls/XmlMfcOrder.h"   // Level 排序规则与运行时/预览器共用
#include <algorithm>
#include <functional>
#include <unordered_map>
#include <set>

using namespace uidesign;
using namespace uidesign::detail;

// ==== 临时性能诊断：PYARX_DESIGN_DEBUG=1 时写 G:\PyArx\log\paint_diag.log ====
static void paintLog(const char* fmt, ...)
{
    static int on = -1;
    if (on < 0)
    {
        wchar_t ev[8] = {};
        on = (::GetEnvironmentVariableW(L"PYARX_DESIGN_DEBUG", ev, 8) >= 1 && ev[0] == L'1') ? 1 : 0;
    }
    if (on != 1) return;
    va_list ap;
    va_start(ap, fmt);
    char buf[512];
    _vsnprintf_s(buf, sizeof(buf), _TRUNCATE, fmt, ap);
    va_end(ap);
    FILE* f = nullptr;
    _wfopen_s(&f, L"G:\\PyArx\\log\\paint_diag.log", L"a");
    if (f != nullptr) { fputs(buf, f); fclose(f); }
}

// 元素字号：优先读 XML 的 FontSize 属性，没写就回落到调用方给的默认值
static int fontPtOf(XmlElement* n, int def)
{
    if (n == nullptr) return def;
    const std::wstring s = n->get(L"FontSize");
    if (s.empty()) return def;
    const int v = _wtoi(s.c_str());
    return (v > 0 && v < 40) ? v : def;
}

// 临时诊断（PYARX_DESIGN_DEBUG=1 时写 G:\PyArx\log\menu_diag.log）
static void propDiag(const wchar_t* fmt, ...)
{
    if (::GetEnvironmentVariableW(L"PYARX_DESIGN_DEBUG", nullptr, 0) == 0) return;
    FILE* f = nullptr;
    if (_wfopen_s(&f, L"G:\\PyArx\\log\\menu_diag.log", L"a, ccs=UTF-8") != 0 || f == nullptr) return;
    va_list ap;
    va_start(ap, fmt);
    vfwprintf(f, fmt, ap);
    va_end(ap);
    fputws(L"\n", f);
    fclose(f);
}

// 面板内自绘的竖向滚动条：内容超出可视高度时才画，滑块位置与滚动量对应。
// 只做"看得见 + 能拖动"这一层，滚轮/拖动由 DesignerPaneSurfaceInput 处理。
static void drawPanelScrollBar(XmlCanvas& cv, const RECT& rc, int total, int viewH, int scroll)
{
    if (total <= viewH || total <= 0) return;
    const int trackW = 10;
    RECT sb = { rc.right - trackW, rc.top, rc.right, rc.bottom };
    cv.fillRect(sb, RGB(30, 36, 44));

    const int trackH = (int)(sb.bottom - sb.top);
    if (trackH <= 0) return;
    int thumbH = (int)((long long)trackH * viewH / total);
    if (thumbH < 28) thumbH = 28;
    if (thumbH > trackH) thumbH = trackH;

    const int maxS = total - viewH;
    int off = 0;
    if (maxS > 0)
        off = (int)((long long)(trackH - thumbH) * scroll / maxS);
    if (off < 0) off = 0;
    if (off > trackH - thumbH) off = trackH - thumbH;

    RECT thumb = { sb.left + 1, sb.top + off, sb.right - 1, sb.top + off + thumbH };
    cv.fillRect(thumb, RGB(98, 112, 132));
}

void DesignerPaneView::paintSurface(HDC hdc)
{
    CRect client;
    GetClientRect(&client);
    if (client.Width() <= 0 || client.Height() <= 0) return;

    // 整窗表面画到 WM_PAINT 传进来的 DC（DCRenderTarget）。
    // 原先用 HwndRenderTarget 直呈：它的呈现走 DWM 直接合成、不受 WS_CLIPCHILDREN 约束，
    // 会把所有真实子窗口（画布、源码编辑框）整个盖死 —— 这是"控件看不见"的唯一根因。
    // 必须 new：类内静态对象会为每个编译单元各建一份，渲染目标互相顶掉。
    // 同时按 HWND 校验：宿主重建窗口时旧 canvas 还绑着已销毁的 HWND，会画出黑屏。
    static D2DDcCanvas* cv = nullptr;
    static HWND cvHwnd = nullptr;
    if (cv == nullptr || cvHwnd != m_hWnd)
    {
        delete cv;
        cv = new D2DDcCanvas();
        cvHwnd = m_hWnd;
    }
    const ULONGLONG pt0 = ::GetTickCount64();
    const RECT target = {0, 0, client.Width(), client.Height()};
    cv->bind(hdc, target);
    if (!cv->begin(client.Width(), client.Height()))
        return;   // 硬件与 WARP 都起不来时这里什么都不画（不再回退 GDI 绘制）
    const ULONGLONG pt1 = ::GetTickCount64();

    cv->fillRect(client, xmlctl::shell().base);

    // 命中表随每帧重建（19 个按钮 + 几个分割条 + 树行指针，成本可忽略）
    m_surfToolRects.clear();
    m_surfToolNames.clear();
    m_surfMenuRects.clear();
    m_surfMenuNodes.clear();
    m_surfSplitterRects.clear();
    m_surfSplitterIdx.clear();
    m_surfDocNodes.clear();
    m_surfDocLevels.clear();
    m_surfWidgetTags.clear();
    m_surfWidgetCats.clear();
    m_surfWidgetLevels.clear();
    m_surfChipRects.clear();
    m_surfChipCats.clear();
    m_surfTabRects.clear();

    XmlElement* root = m_ui.root();
    if (root != nullptr)
    {
        for (auto& child : root->children)
        {
            const ULONGLONG a = ::GetTickCount64();
            paintSurfaceNode(*cv, &child);
            const ULONGLONG b = ::GetTickCount64();
            if (b - a >= 1)
                paintLog("[node] %ws name=%ws %llu ms\n", child.tag.c_str(), child.name.c_str(),
                         (unsigned long long)(b - a));
        }
    }
    const ULONGLONG pt2 = ::GetTickCount64();

    cv->end();
    const ULONGLONG pt3 = ::GetTickCount64();
    paintLog("[paint] %dx%d bind=%llu shell=%llu end=%llu\n", client.Width(), client.Height(),
             (unsigned long long)(pt1 - pt0), (unsigned long long)(pt2 - pt1),
             (unsigned long long)(pt3 - pt2));
}

// XML 代码行：行号栏 + 逐 token 着色。
// 共享绘制层只有"整段一个颜色"的文字接口，所以要按量出来的宽度顺次排出多个色段；
// 量宽用 GDI 的 Consolas 同尺寸字体，与 D2D 的 DWrite 同族，误差在 1px 内。
static void drawCodeLine(XmlCanvas& cv, HDC hdc, HFONT font, const RECT& lr, int lineNo,
                         const std::wstring& code)
{
    CString num;
    num.Format(L"%4d", lineNo);
    RECT nr = lr;
    nr.right = lr.left + 36;
    cv.text(nr, std::wstring(num.GetString()), RGB(88, 99, 113), DT_RIGHT, 8, L"Consolas");
    if (code.empty()) return;

    int x = lr.left + 44;
    auto put = [&](const std::wstring& s, COLORREF c) {
        if (s.empty() || x >= (int)lr.right) return;
        RECT r = { x, lr.top, lr.right, lr.bottom };
        cv.text(r, s, c, DT_LEFT, 9, L"Consolas");
        SIZE sz = { 0, 0 };
        ::SelectObject(hdc, font);
        ::GetTextExtentPoint32W(hdc, s.c_str(), (int)s.size(), &sz);
        x += sz.cx;
    };

    const COLORREF cText = RGB(203, 213, 224);
    const COLORREF cTag = RGB(126, 178, 255);
    const COLORREF cAttr = RGB(214, 186, 125);
    const COLORREF cVal = RGB(150, 206, 150);
    const COLORREF cComment = RGB(106, 153, 85);
    const COLORREF cPunct = RGB(138, 150, 166);

    // 注释行整体上色（行内注释也只染注释段，够用且不会染色错位）
    const size_t cmt = code.find(L"<!--");
    if (cmt != std::wstring::npos)
    {
        put(code.substr(0, cmt), cText);
        put(code.substr(cmt), cComment);
        return;
    }

    std::wstring run;
    int mode = 0;          // 0 文本 / 1 标签名 / 2 属性名 / 3 等号后 / 4 值 / 5 标签内空白
    wchar_t quote = 0;
    auto flush = [&](COLORREF c) {
        if (!run.empty()) { put(run, c); run.clear(); }
    };

    for (size_t i = 0; i < code.size(); ++i)
    {
        const wchar_t ch = code[i];
        if (mode == 0)
        {
            if (ch == L'<')
            {
                flush(cText);
                if (i + 1 < code.size() && code[i + 1] == L'/') { put(L"</", cPunct); ++i; }
                else put(L"<", cPunct);
                mode = 1;
            }
            else run += ch;
            continue;
        }
        if (mode == 1)          // 标签名
        {
            if (ch == L'>') { flush(cTag); put(L">", cPunct); mode = 0; continue; }
            if (ch == L'/') { flush(cTag); put(L"/", cPunct); mode = 0; continue; }
            if (ch == L' ' || ch == L'\t') { flush(cTag); run += ch; mode = 5; continue; }
            run += ch;
            continue;
        }
        if (mode == 5)          // 标签内空白 / 属性之间
        {
            if (ch == L' ' || ch == L'\t') { run += ch; continue; }
            flush(cText);
            if (ch == L'>') { put(L">", cPunct); mode = 0; continue; }
            if (ch == L'/') { put(L"/", cPunct); mode = 0; continue; }
            if (ch == L'=') { put(L"=", cPunct); mode = 3; continue; }
            mode = 2;
            run += ch;
            continue;
        }
        if (mode == 2)          // 属性名
        {
            if (ch == L'=') { flush(cAttr); put(L"=", cPunct); mode = 3; continue; }
            if (ch == L' ' || ch == L'\t') { flush(cAttr); run += ch; mode = 5; continue; }
            if (ch == L'>') { flush(cAttr); put(L">", cPunct); mode = 0; continue; }
            run += ch;
            continue;
        }
        if (mode == 3)          // 等号后：引号或裸值
        {
            flush(cAttr);
            if (ch == L' ') { run += ch; continue; }
            if (ch == L'"' || ch == L'\'') { quote = ch; run += ch; mode = 4; }
            else { quote = 0; mode = 4; run += ch; }
            continue;
        }
        if (mode == 4)          // 值
        {
            if (quote != 0 && ch == quote) { run += ch; flush(cVal); mode = 5; continue; }
            if (quote == 0 && (ch == L' ' || ch == L'>' || ch == L'/'))
            {
                flush(cVal);
                if (ch == L' ') { run += ch; mode = 5; }
                else { put(std::wstring(1, ch), cPunct); mode = (ch == L'>') ? 0 : 0; }
                continue;
            }
            run += ch;
            continue;
        }
    }
    flush(mode == 1 ? cTag : (mode == 2 ? cAttr : (mode == 4 ? cVal : cText)));
}

// ===========================================================================
// 菜单条：XML 的 MenuBar 由这张表面自绘（外壳构件，与 PanelHeader/TabStrip 同类）。
// 顶层 MenuItem 横排，点击用系统下拉菜单弹出，子项命令复用同一套 ensureAction 分发。
// ===========================================================================
void DesignerPaneView::paintMenuBar(XmlCanvas& cv, const RECT& rc, XmlElement* node)
{
    // 外观全部取 MenuBar 节点自身属性（与画布、预览器同源），外壳不再自己定颜色/字号
    const xmlctl::ShellLook lk = xmlctl::shellLookOf(*node);
    cv.fillRect(rc, lk.back);
    cv.fillRect(RECT{ rc.left, rc.bottom - 1, rc.right, rc.bottom }, lk.line);
    m_surfTitleStrip = rc.bottom;      // 这一行就是无边框窗体的拖动区

    HDC hdc = ::GetDC(nullptr);
    HFONT font = ::CreateFontW(-13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                               OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                               DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI");
    HGDIOBJ oldFont = ::SelectObject(hdc, font);

    int x = rc.left + 8;
    int rightX = rc.right;          // 窗口按钮从右往左排：文档顺序第一个贴着最右边
    for (auto& c : node->children)
    {
        const std::wstring bare = xmlmfc::BareTag(c.tag);

        // ---- 标题：无边框窗体的名字，画在菜单条左端 ----
        // 文案优先取顶层窗口标题（打开/保存文档时它会被更新），缺省用 XML 里写的 Text；
        // 固定占一列宽度并省略号收尾，长路径不会把菜单挤到屏幕外。
        if (_wcsicmp(bare.c_str(), L"ShellTitle") == 0)
        {
            // 所见即所得：标题文案与宽度都按 XML 声明，不再换成窗口路径
            std::wstring t = c.get(L"Title");
            if (t.empty()) t = c.get(L"Text");
            if (t.empty()) t = c.text;
            int w = (c.previewW > 0) ? c.previewW : lk.titleW;
            if (w <= 0 || w > (rc.right - rc.left) / 2) w = lk.titleW;   // 标题只占固定一列，不能吃掉整条菜单
            RECT tr = { x, rc.top, x + w, rc.bottom };
            cv.text(tr, t, lk.fore,
                    DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS, lk.fontPt, L"");
            x += w;
            continue;
        }

        // ---- 窗口按钮：自绘最小化 / 最大化 / 关闭（不依赖系统标题栏）----
        if (_wcsicmp(bare.c_str(), L"Button") == 0)
        {
            const std::wstring role = c.get(L"Role");
            if (_wcsicmp(role.c_str(), L"WinBtn") == 0 || _wcsicmp(role.c_str(), L"WinClose") == 0)
            {
                const int w = c.previewW > 0 ? c.previewW : 46;
                rightX -= w;
                RECT br = { rightX, rc.top, rightX + w, rc.bottom };
                m_surfToolRects.push_back(br);
                m_surfToolNames.push_back(c.name);
                const int idx = (int)m_surfToolNames.size() - 1;
                const bool hot = idx == m_surfHotTool;
                const bool isClose = _wcsicmp(role.c_str(), L"WinClose") == 0;
                if (hot)
                    cv.fillRect(br, isClose ? RGB(196, 43, 28) : RGB(58, 64, 72));
                const int cx = (br.left + br.right) / 2;
                const int cy = (br.top + br.bottom) / 2;
                const COLORREF g = hot ? RGB(255, 255, 255) : RGB(206, 212, 220);
                if (isClose)
                {
                    cv.strokeLine({ cx - 5, cy - 5 }, { cx + 5, cy + 5 }, g, 1);
                    cv.strokeLine({ cx - 5, cy + 5 }, { cx + 5, cy - 5 }, g, 1);
                }
                else if (_wcsicmp(c.name.c_str(), L"btnWinMax") == 0)
                    cv.strokeRect(RECT{ cx - 5, cy - 5, cx + 5, cy + 5 }, g, 1);
                else
                    cv.strokeLine({ cx - 5, cy }, { cx + 5, cy }, g, 1);
                continue;
            }
        }

        // ---- 菜单项 ----
        std::wstring hdr = c.get(L"Header");
        if (hdr.empty()) hdr = c.text;
        if (hdr.empty()) continue;
        SIZE sz = {};
        ::GetTextExtentPoint32W(hdc, hdr.c_str(), (int)hdr.size(), &sz);
        const int w = sz.cx + 26;
        RECT ir = { x, rc.top, x + w, rc.bottom };
        cv.text(ir, hdr, lk.fore, DT_CENTER | DT_VCENTER | DT_SINGLELINE, lk.fontPt, L"");
        m_surfMenuRects.push_back(ir);
        m_surfMenuNodes.push_back(&c);
        x += w;
    }

    ::SelectObject(hdc, oldFont);
    ::DeleteObject(font);
    ::ReleaseDC(nullptr, hdc);
}

void DesignerPaneView::surfaceMenuPopup(XmlElement* top, POINT screenPt)
{
    if (top == nullptr) return;

    HMENU m = ::CreatePopupMenu();
    for (auto& c : top->children)
    {
        std::wstring hdr = c.get(L"Header");
        if (hdr.empty()) hdr = c.text;
        const UINT id = ensureAction(CString(c.name.c_str()));
        ::AppendMenuW(m, MF_STRING, id, hdr.c_str());
    }
    ::TrackPopupMenu(m, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON,
                     screenPt.x, screenPt.y, 0, m_hWnd, nullptr);
    ::DestroyMenu(m);
    Invalidate(FALSE);
}

void DesignerPaneView::paintSurfaceNode(XmlCanvas& cv, XmlElement* node)
{
    if (node == nullptr) return;
    CRect rc(node->previewX, node->previewY,
             node->previewX + node->previewW, node->previewY + node->previewH);
    if (rc.right <= rc.left || rc.bottom <= rc.top) return;

    const std::wstring tag = node->tag;
    // 统一命名：X + 本意名，外壳构件按本意名分派
    const std::wstring bareTag = xmlmfc::BareTag(tag);

    // ---- 画布：内容直接画进父表面 ----
    // 父表面的 HwndRenderTarget 不透明呈现会盖住画布子窗口（DWM 合成不受
    // WS_CLIPCHILDREN 约束），所以画布内容必须由这张唯一的表面自己画。
    if (m_canvas != nullptr && node->name == L"canvasHost")
    {
        const ULONGLONG ca = ::GetTickCount64();
        m_canvas->paintInto(cv, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
        paintLog("[canvas] %llu ms %dx%d\n", (unsigned long long)(::GetTickCount64() - ca),
                 rc.right - rc.left, rc.bottom - rc.top);
        return;
    }

    int rowH = 24;                     // 行高可被 XML 的 LineHeight 覆盖

    // ---- 菜单条（单表面自绘：顶层 MenuItem 横排，点击弹系统下拉菜单）----
    if (_wcsicmp(bareTag.c_str(), L"MenuBar") == 0)
    {
        paintMenuBar(cv, rc, node);
        return;
    }

    // ---- 左侧 Draw 工具条（直线 / 矩形 / 圆 / 弧）----
    // 外观与命中都在外壳这一侧；画布只保存"待放置图元"的状态
    if (_wcsicmp(bareTag.c_str(), L"DrawBar") == 0)
    {
        // Draw 竖条外观全部来自 XML 节点属性，和画布、预览器用同一套数值
        const xmlctl::ShellLook dl = xmlctl::shellLookOf(*node);
        const int itemH = xmlctl::shellInt(*node, L"ItemHeight", 32);
        const int pad = xmlctl::shellInt(*node, L"ItemPadding", 6);
        const int gap = xmlctl::shellInt(*node, L"ItemSpacing", 4);
        const int capH = xmlctl::shellInt(*node, L"CaptionHeight", 24);
        const int capPt = xmlctl::shellInt(*node, L"CaptionFontSize", dl.fontPt);

        // 布局引擎给外壳自定义容器的矩形可能只按内容高度算，这里按工具数撑开
        const int needH = capH + pad + (int)node->children.size() * (itemH + gap) + pad;
        if (rc.bottom - rc.top < needH) rc.bottom = rc.top + needH;
        cv.fillRect(rc, dl.back);
        cv.fillRect(RECT{ rc.left, rc.top, rc.left + 1, rc.bottom }, dl.line);

        std::wstring cap = node->get(L"Title");
        if (cap.empty()) cap = node->get(L"Text");
        RECT hr{ rc.left, rc.top, rc.right, rc.top + capH };
        cv.text(hr, cap, dl.dim, DT_CENTER | DT_VCENTER | DT_SINGLELINE, capPt,
                L"Microsoft YaHei UI");

        const std::wstring pending = (m_canvas != nullptr) ? m_canvas->PendingShapeTag()
                                                           : std::wstring();
        int y = rc.top + capH + pad;
        // 工具清单以 XML 的 DrawBar 子节点为准；子节点缺失时退回四个内置图元
        std::vector<std::wstring> tools;
        for (auto& c : node->children)
        {
            std::wstring shape = c.get(L"Tool");
            if (shape.empty()) continue;
            tools.push_back(shape);
        }
        if (tools.empty())
        {
            tools.push_back(L"Line");
            tools.push_back(L"Rectangle");
            tools.push_back(L"Ellipse");
            tools.push_back(L"Arc");
        }
        for (auto& shape : tools)
        {
            RECT b{ rc.left + pad, y, rc.right - pad, y + itemH };
            if (b.bottom > rc.bottom - pad) break;
            const bool on = (_wcsicmp(pending.c_str(), shape.c_str()) == 0);
            cv.fillRect(b, on ? dl.accent : dl.back);
            cv.strokeRect(b, on ? dl.accent : dl.line, 1);
            const COLORREF ink = on ? RGB(255, 255, 255) : dl.fore;

            if (_wcsicmp(shape.c_str(), L"Line") == 0)
                cv.strokeLine(POINT{ b.left + 8, b.bottom - 8 }, POINT{ b.right - 8, b.top + 8 },
                              ink, 2);
            else if (_wcsicmp(shape.c_str(), L"Rectangle") == 0)
                cv.strokeRect(RECT{ b.left + 7, b.top + 6, b.right - 7, b.bottom - 6 }, ink, 1);
            else if (_wcsicmp(shape.c_str(), L"Ellipse") == 0)
                cv.fillEllipse(RECT{ b.left + 7, b.top + 6, b.right - 7, b.bottom - 6 },
                               on ? dl.accent : dl.back, ink);
            else
            {
                const int cx = (b.left + b.right) / 2;
                const int cy = b.bottom - 8;
                const int rx = (b.right - b.left) / 2 - 7;
                const int ry = b.bottom - b.top - 14;
                std::vector<POINT> pts;
                for (int k = 0; k <= 24; ++k)
                {
                    const double t = 3.14159265 * (1.0 - k / 24.0);
                    pts.push_back(POINT{ (LONG)(cx + rx * cos(t)), (LONG)(cy - ry * sin(t)) });
                }
                cv.strokePath(pts, ink, 2, false);
            }

            m_surfToolRects.push_back(RECT{ b.left, b.top, b.right, b.bottom });
            m_surfToolNames.push_back(std::wstring(L"shape:") + shape);
            y += itemH + gap;
        }
        return;
    }

    // ---- 工具栏 ----
    if (_wcsicmp(bareTag.c_str(), L"ToolBar") == 0)
    {
        const xmlctl::ShellLook tl = xmlctl::shellLookOf(*node);
        cv.fillRect(rc, tl.back);
        cv.fillRect(RECT{ rc.left, rc.bottom - 1, rc.right, rc.bottom }, tl.line);
        const int savedPt = m_surfInheritPt;
        m_surfInheritPt = fontPtOf(node, 0);       // 工具栏声明的字号传给每个按钮
        for (auto& c : node->children)
            paintSurfaceNode(cv, &c);
        m_surfInheritPt = savedPt;
        return;
    }
    if (_wcsicmp(bareTag.c_str(), L"Button") == 0)
    {
        // 按钮自己登记命中矩形：绘制顺序即 XML 顺序，不依赖 ToolBar 分支是否先跑
        m_surfToolRects.push_back(RECT{ rc.left, rc.top, rc.right, rc.bottom });
        m_surfToolNames.push_back(node->name);
        int idx = (int)m_surfToolNames.size() - 1;
        const std::wstring icon = node->get(L"Icon");
        std::wstring path;
        if (!icon.empty())
            path = surfaceExeDir() + L"\\" + icon;

        const std::wstring role = node->get(L"Role");
        const bool hot = idx == m_surfHotTool;
        if (_wcsicmp(role.c_str(), L"Primary") == 0 || _wcsicmp(role.c_str(), L"Ghost") == 0)
        {
            xmlctl::xmlDrawPillButton(cv, rc, std::wstring(uiNodeText(node).GetString()),
                                        _wcsicmp(role.c_str(), L"Primary") == 0, hot, fontPtOf(node, m_surfInheritPt > 0 ? m_surfInheritPt : 10));
            return;
        }
        if (_wcsicmp(role.c_str(), L"Active") == 0)
        {
            xmlctl::xmlDrawPillButton(cv, rc, std::wstring(uiNodeText(node).GetString()), false,
                                        true, 9);
            return;
        }

        // 工具栏图标按钮：声明了 Content（且带高度）时画成"图标 + 下方标题"
        const std::wstring content = uiNodeText(node).GetString();
        if (!path.empty() && !content.empty() && node->previewH >= 36)
        {
            xmlctl::xmlDrawToolButtonCaption(cv, rc, path, content, hot, false, false,
                                               xmlctl::ToolButtonStyle(),
                                               fontPtOf(node, m_surfInheritPt > 0 ? m_surfInheritPt : 10));
            return;
        }

        xmlctl::ToolButtonStyle tbs;
        tbs.back = xmlctl::shell().header;   // 常态与工具栏同色，只有悬停才显块
        tbs.hot = xmlctl::shell().panelAlt;
        xmlctl::xmlDrawToolButton(cv, rc, path, hot, false, tbs);
        return;
    }
    if (_wcsicmp(bareTag.c_str(), L"Separator") == 0)
    {
        xmlctl::xmlDrawSeparator(cv, rc, node->previewW <= node->previewH);
        return;
    }

    // ---- 面板标题条（XML 声明的 <PanelHeader Text=".." Hint="..">）----
    if (_wcsicmp(bareTag.c_str(), L"PanelHeader") == 0)
    {
        std::wstring hint = node->get(L"Hint");
        if (node->name == L"hdrWidgetBox")
        {
            int n = 0;
            for (auto& cat : m_catalog.categories())
            {
                if (!m_widgetFilter.IsEmpty() && CString(cat) != m_widgetFilter) continue;
                n += (int)m_catalog.templates(cat).size();
            }
            CString s; s.Format(L"%d 个控件", n);
            hint = std::wstring(s.GetString());
        }
        else if (node->name == L"hdrInspector")
        {
            int n = 0;
            uidesign::Walk(m_doc.root(), [&](const Path&, XmlElement&) { ++n; });
            CString s; s.Format(L"%d 节点", n);
            hint = std::wstring(s.GetString());
        }
        else if (node->name == L"hdrProps")
        {
            XmlElement* sel = Resolve(m_doc.root(), m_selected);
            hint = (sel != nullptr) ? std::wstring(L"<") + sel->tag + L">"
                                    : std::wstring(L"未选中对象");
        }
        else if (node->name == L"hdrCode")
        {
            const std::wstring xml = m_doc.toXml();
            int lines = 1;
            for (wchar_t ch : xml) if (ch == L'\n') ++lines;
            CString s; s.Format(L"%d 行 · UTF-8", lines);
            hint = std::wstring(s.GetString());
        }

        xmlctl::xmlDrawPanelHeader(cv, rc, std::wstring(uiNodeText(node).GetString()), hint, fontPtOf(node, 10));

        RECT savedParentH = m_surfParentRc;
        m_surfParentRc = rc;
        for (auto& c : node->children)
            paintSurfaceNode(cv, &c);
        m_surfParentRc = savedParentH;
        return;
    }

    // ---- 面板标题条（旧写法：带 Background 的 TextBlock）----
    if (_wcsicmp(bareTag.c_str(), L"Label") == 0)
    {
        std::wstring content = node->get(L"Text");
        if (content.empty()) content = node->get(L"Title");
        if (content.empty()) content = node->get(L"Content");
        if (content.empty()) content = node->text;
        COLORREF bg = ParseColor(CString(node->get(L"Background").c_str()));
        COLORREF fg = ParseColor(CString(node->get(L"Foreground").c_str()));
        if ((int)bg == -1) bg = xmlctl::shell().header;
        if ((int)fg == -1) fg = xmlctl::shell().text;
        xmlctl::xmlDrawPanelCaption(cv, rc, content, bg, fg, 6);
        return;
    }

    // ---- 分割条 ----
    if (_wcsicmp(bareTag.c_str(), L"SplitView") == 0)
    {
        int si = -1;
        for (size_t i = 0; i < m_splitters.size(); ++i)
            if (m_splitters[i].el == node) { si = (int)i; break; }

        // 分割条自身的高度可能只算了 200（Grid 轨道高度偏小），但父容器是完整的，
        // 视觉与命中都向父容器拉伸，否则只能拖住一小段。
        const bool vertical = rc.Width() <= rc.Height();
        RECT dr = rc;
        if (vertical)
        {
            if (m_surfParentRc.bottom > dr.bottom) dr.bottom = m_surfParentRc.bottom;
        }
        else
        {
            if (m_surfParentRc.right > dr.right) dr.right = m_surfParentRc.right;
        }

        m_surfSplitterRects.push_back(dr);
        m_surfSplitterIdx.push_back(si);
        if (!vertical)
        {
            // 水平分隔条（主体 ↔ 代码坞）：整条铺一层比背景亮一点的带子 + 中间一条亮线，
            // 否则和深色背景糊成一片，看不出来这里能拖。
            const bool hot = si >= 0 && (si == m_surfHotSplit || si == m_dragSplit);
            cv.fillRect(dr, hot ? RGB(44, 52, 64) : RGB(30, 35, 43));
            const int mid = (dr.top + dr.bottom) / 2;
            cv.fillRect(RECT{ dr.left, mid, dr.right, mid + 1 },
                        hot ? RGB(120, 148, 184) : RGB(66, 76, 90));
        }
        else
        {
            xmlctl::xmlDrawSplitter(cv, dr, vertical,
                                      si >= 0 && si == m_surfHotSplit,
                                      si >= 0 && si == m_dragSplit);
        }
        return;
    }

    // ---- 控件箱 / 对象树 ----
    if (_wcsicmp(bareTag.c_str(), L"TreeView") == 0)
    {
        // 深色面板里不能再用系统白底（那是"未声明外观"的默认），否则整块泛白刺眼
        // 底色/文字色/选中色/引导线颜色全部取节点属性：和预览器、画布用同一份 XML
        xmlctl::TreeStyle st;
        st.back = RGB(23, 27, 33);
        st.fore = RGB(201, 210, 222);
        st.selBack = RGB(44, 74, 121);
        st.selFore = RGB(255, 255, 255);
        st.guide = RGB(80, 92, 106);
        {
            COLORREF c;
            if (xmlctl::shellColor(node->get(L"Background"), c)) st.back = c;
            if (xmlctl::shellColor(node->get(L"Foreground"), c)) st.fore = c;
            if (xmlctl::shellColor(node->get(L"SelectionColor"), c)) st.selBack = c;
            if (xmlctl::shellColor(node->get(L"BorderColor"), c)) st.guide = c;
            const std::wstring lh = node->get(L"LineHeight");
            if (!lh.empty()) rowH = _wtoi(lh.c_str());
        }
        std::vector<xmlctl::TreeRow> rows;
        if (node->name == L"widgetBox")
        {
            m_surfWidgetRc = RECT{ rc.left, rc.top, rc.right, rc.bottom };
            buildWidgetRows(rows);
            m_surfWidgetTotal = (int)rows.size() * rowH;
            clampSurfaceScroll();
            // 内容比可视区高时右侧留出滚动条，并把它画出来（否则树被裁掉一半且没有任何提示）
            RECT treeRc = rc;
            const int viewH = (int)(rc.bottom - rc.top);
            if (m_surfWidgetTotal > viewH) treeRc.right -= 10;
            xmlctl::xmlDrawTree(cv, treeRc, rows, st, m_surfWidgetScroll, fontPtOf(node, 0));
            drawPanelScrollBar(cv, rc, m_surfWidgetTotal, viewH, m_surfWidgetScroll);
        }
        else
        {
            m_surfDocRc = RECT{ rc.left, rc.top, rc.right, rc.bottom };
            XmlElement* sel = Resolve(m_doc.root(), m_selected);
            m_surfDocNodes.clear();
            m_surfDocLevels.clear();
            // 多选时对象检查器要把每个被选中的节点都标蓝：先按路径集合展开成节点指针集合
            m_docSelPtrs.clear();
            for (const auto& p : m_selSet)
            {
                XmlElement* n = Resolve(m_doc.root(), p);
                if (n != nullptr) m_docSelPtrs.insert(n);
            }
            if (sel != nullptr) m_docSelPtrs.insert(sel);
            buildDocRows(m_doc.root(), 0, rows, sel);
            m_surfDocTotal = (int)rows.size() * rowH;
            // 画布点选 / 树里选行后，把对象检查器滚到选中行（第 5 条：立刻定位到所选对象）
            if (m_docRevealSel)
            {
                int idx = -1;
                for (size_t k = 0; k < m_surfDocNodes.size(); ++k)
                    if (m_surfDocNodes[k] == sel) { idx = (int)k; break; }
                if (idx >= 0)
                {
                    const int viewH = (int)(rc.bottom - rc.top) - 26;      // 扣掉表头
                    const int rowTop = idx * rowH;
                    if (m_surfDocScroll + rowTop < 0) m_surfDocScroll = -rowTop;
                    else if (rowTop + rowH > m_surfDocScroll + viewH) m_surfDocScroll = rowTop + rowH - viewH;
                    if (m_surfDocScroll > 0) m_surfDocScroll = 0;
                }
                m_docRevealSel = false;
            }
            clampSurfaceScroll();
            RECT treeRc = rc;
            const int viewH = (int)(rc.bottom - rc.top);
            if (m_surfDocTotal > viewH) treeRc.right -= 10;
            xmlctl::xmlDrawTree(cv, treeRc, rows, st, m_surfDocScroll, fontPtOf(node, 0));
            drawPanelScrollBar(cv, rc, m_surfDocTotal, viewH, m_surfDocScroll);
        }
        return;
    }

    // ---- 属性表 ----
    if (_wcsicmp(bareTag.c_str(), L"PropertyGrid") == 0)
    {
        // 属性表配色/列宽/行高同样取节点属性
        xmlctl::PropStyle st;
        st.back = RGB(23, 27, 33);
        st.keyBack = RGB(29, 34, 42);
        st.groupBack = RGB(35, 41, 50);
        st.keyFore = RGB(150, 198, 238);
        st.valFore = RGB(216, 224, 234);
        st.line = RGB(42, 48, 57);
        st.selBack = RGB(44, 74, 121);
        st.editBack = RGB(33, 39, 48);
        st.keyW = 104;
        st.headH = 26;
        {
            COLORREF c;
            if (xmlctl::shellColor(node->get(L"Background"), c)) st.back = c;
            if (xmlctl::shellColor(node->get(L"Foreground"), c)) st.valFore = c;
            if (xmlctl::shellColor(node->get(L"KeyColor"), c)) st.keyFore = c;
            if (xmlctl::shellColor(node->get(L"BorderColor"), c)) st.line = c;
            if (xmlctl::shellColor(node->get(L"SelectionColor"), c)) st.selBack = c;
            if (xmlctl::shellColor(node->get(L"GroupColor"), c)) st.groupBack = c;
            const std::wstring lh = node->get(L"LineHeight");
            if (!lh.empty()) rowH = _wtoi(lh.c_str());
        }
        std::vector<xmlctl::PropRow> rows;
        m_surfPropRc = RECT{ rc.left, rc.top, rc.right, rc.bottom };
        buildPropRows(Resolve(m_doc.root(), m_selected), rows);
        if (m_surfSelPropRow >= 0 && m_surfSelPropRow < (int)rows.size())
            rows[m_surfSelPropRow].selected = true;
        m_surfPropTotal = (int)rows.size() * rowH + st.headH;
        clampSurfaceScroll();
        m_surfPropRows = rows;   // 供双击/右键取 key，不必再算一遍
        RECT gridRc = rc;
        const int viewH = (int)(rc.bottom - rc.top);
        int totalH = m_surfPropTotal;
        if (totalH > viewH) gridRc.right -= 12;
        propDiag(L"[prop] rc=%d,%d,%d,%d total=%d viewH=%d scroll=%d rows=%d", rc.left, rc.top, rc.right, rc.bottom,
                 totalH, viewH, m_surfPropScroll, (int)rows.size());
        xmlctl::xmlDrawPropertyGrid(cv, gridRc, rows, st, m_surfPropScroll, fontPtOf(node, 0));
        drawPanelScrollBar(cv, rc, totalH, viewH, m_surfPropScroll);
        return;
    }

    // ---- Canvas：主画布交给真实窗口；代码区交给 CodeEditor 控件 ----
    if (_wcsicmp(bareTag.c_str(), L"Canvas") == 0)
    {
        if (node->name == L"xmlCodeHost")
        {
            m_surfCodeRc = RECT{ rc.left, rc.top, rc.right, rc.bottom };
            // 代码区现在由 CodeEditor 控件自己画（行号槽 / 语法着色 / 光标 / 提示），
            // 父表面这里只铺一层底色，防止控件还没摆好时露出旧像素。
            cv.fillRect(rc, xmlctl::shell().code);
            cv.fillRect(RECT{ rc.left, rc.top, rc.right, rc.top + 1 }, xmlctl::shell().lineSoft);
            syncCodeEditor();
        }
        return;
    }

    // ---- 文档标签条 ----
    if (_wcsicmp(bareTag.c_str(), L"TabStrip") == 0)
    {
        xmlctl::xmlDrawTabBar(cv, rc);
        int ti = 0;
        for (auto& c : node->children)
        {
            if (_wcsicmp(c.tag.c_str(), L"Tab") != 0)
            {
                paintSurfaceNode(cv, &c);   // 右侧的小动作按钮
                continue;
            }
            RECT tr = { c.previewX, c.previewY, c.previewX + c.previewW,
                        c.previewY + c.previewH };
            CString title = uiNodeText(&c);
            if (c.name == L"tabDoc")
                title = m_docPath.IsEmpty() ? CString(L"未命名.xml") : uiFileName(m_docPath);
            const bool active = (_wcsicmp(c.get(L"Active").c_str(), L"True") == 0);
            xmlctl::xmlDrawTab(cv, tr, std::wstring(title.GetString()), active,
                                 ti == m_surfHotTab, 9);
            m_surfTabRects.push_back(tr);
            ++ti;
        }
        return;
    }
    if (_wcsicmp(bareTag.c_str(), L"Tab") == 0)
    {
        xmlctl::xmlDrawTab(cv, rc, std::wstring(uiNodeText(node).GetString()),
                             _wcsicmp(node->get(L"Active").c_str(), L"True") == 0, false, 9);
        return;
    }

    // ---- 控件箱分类过滤（<ChipBar> + <Chip Category="..">）----
    if (_wcsicmp(bareTag.c_str(), L"ChipBar") == 0)
    {
        for (auto& c : node->children)
        {
            if (_wcsicmp(c.tag.c_str(), L"Chip") != 0)
            {
                paintSurfaceNode(cv, &c);
                continue;
            }
            RECT cr = { c.previewX, c.previewY, c.previewX + c.previewW, c.previewY + c.previewH };
            CString cat = CString(c.get(L"Category").c_str());
            const bool checked = (cat == m_widgetFilter);
            const int ci = (int)m_surfChipRects.size();
            xmlctl::xmlDrawChip(cv, cr, std::wstring(uiNodeText(&c).GetString()), checked,
                                  ci == m_surfHotChip, 9);
            m_surfChipRects.push_back(cr);
            m_surfChipCats.push_back(cat);
        }
        return;
    }
    if (_wcsicmp(bareTag.c_str(), L"Chip") == 0)
    {
        const CString cat = CString(node->get(L"Category").c_str());
        xmlctl::xmlDrawChip(cv, rc, std::wstring(uiNodeText(node).GetString()),
                              cat == m_widgetFilter, false, 9);
        return;
    }

    // ---- 画布舞台：深色台面 + 四角定位标，画布（真实窗口）叠在上面 ----
    if (_wcsicmp(bareTag.c_str(), L"Stage") == 0)
    {
        xmlctl::xmlDrawStage(cv, rc);
        RECT savedParentS = m_surfParentRc;
        m_surfParentRc = rc;
        for (auto& c : node->children)
            paintSurfaceNode(cv, &c);
        m_surfParentRc = savedParentS;
        return;
    }

    // ---- 状态栏 ----
    if (_wcsicmp(bareTag.c_str(), L"StatusBar") == 0)
    {
        std::vector<xmlctl::StatusCell2> cells;
        int i = 0;
        for (auto& c : node->children)
        {
            xmlctl::StatusCell2 cell;
            if (i == 0)
            {
                // 首格是品牌格：实心强调色块，固定文案（XML 里写死）
                cell.text = std::wstring(uiNodeText(&c).GetString());
                cell.accent = true;
            }
            else
            {
                const int si = i - 1;
                if (si == 3 && m_canvas != nullptr)
                {
                    // 缩放格直接取画布实时倍率，避免状态栏和画布对不上
                    CString z; z.Format(L"%d%%", (int)(m_canvas->zoom() * 100.0f + 0.5f));
                    cell.text = std::wstring(z.GetString());
                }
                else if (si < 4 && !m_surfStatus[si].IsEmpty())
                    cell.text = std::wstring(m_surfStatus[si].GetString());
                else
                    cell.text = std::wstring(uiNodeText(&c).GetString());
            }
            cell.width = c.previewW;
            cells.push_back(cell);
            ++i;
        }
        xmlctl::xmlDrawStatusBarRich(cv, rc, cells, 9);
        return;
    }

    // ---- 其余容器：先吃 XML 声明的 Background，再递归 ----
    // 单表面模式下容器没有子窗口，底色必须在这里补上；否则 XML 里写的面板背景
    // 永远看不见（以前只有 Window Background 生效这一条"看不见的规则"）。
    const bool plainContainer =
        _wcsicmp(bareTag.c_str(), L"Grid") == 0 || _wcsicmp(bareTag.c_str(), L"DockPanel") == 0 ||
        _wcsicmp(bareTag.c_str(), L"StackPanel") == 0 || _wcsicmp(bareTag.c_str(), L"WrapPanel") == 0 ||
        _wcsicmp(bareTag.c_str(), L"Page") == 0 || _wcsicmp(bareTag.c_str(), L"PaletteSet") == 0;
    if (plainContainer)
    {
        COLORREF bg = ParseColor(CString(node->get(L"Background").c_str()));
        if ((int)bg != -1)
            cv.fillRect(rc, bg);
    }

    RECT savedParent = m_surfParentRc;
    m_surfParentRc = rc;
    // 兄弟的绘制先后按 Level 定（Level 大的后画 = 压在小的上面），
    // 同级仍按声明序 —— 与运行时 z 序、预览器叠放是同一套语义。
    if (node->children.empty())
    {
        m_surfParentRc = savedParent;
        return;
    }
    {
        std::vector<int> levels;
        levels.reserve(node->children.size());
        for (auto& c : node->children)
        {
            const std::wstring lv = c.get(L"Level");
            int v = 0;
            if (lv.empty() || lv == L"0")
            {
                const std::wstring zi = c.get(L"ZIndex");
                v = zi.empty() ? 0 : _wtoi(zi.c_str());
            }
            else
                v = _wtoi(lv.c_str());
            levels.push_back(v);
        }
        const std::vector<int> order = xmlmfc::sortDrawOrder(levels);
        for (int idx : order)
            paintSurfaceNode(cv, &node->children[idx]);
    }
    m_surfParentRc = savedParent;
}

void DesignerPaneView::buildWidgetRows(std::vector<xmlctl::TreeRow>& rows)
{
    rows.clear();
    m_surfWidgetTags.clear();
    int idx = 0;

    // Draw 图元并进控件箱树：放在最前面一组（直线/矩形/圆/圆弧），
    // 选中即进入"待放置"状态，拖画布即按矩形落下，与控件放置同一条路径。
    {
        xmlctl::TreeRow r;
        r.text = L"Draw";
        r.level = 0;
        r.hasChildren = true;
        r.expanded = (m_collapsedCats.find(CString(L"Draw")) == m_collapsedCats.end());
        r.selected = (idx == m_surfSelWidgetRow);
        rows.push_back(r);
        m_surfWidgetTags.push_back(CString());
        m_surfWidgetCats.push_back(CString(L"Draw"));
        m_surfWidgetLevels.push_back(0);
        ++idx;
        if (r.expanded)
        {
            static const wchar_t* const kNames[4] = { L"直线", L"矩形", L"圆", L"圆弧" };
            static const wchar_t* const kTags[4]  = { L"Line", L"Rectangle", L"Ellipse", L"Arc" };
            for (int k = 0; k < 4; ++k)
            {
                xmlctl::TreeRow c;
                c.text = kNames[k];
                c.level = 1;
                c.selected = (idx == m_surfSelWidgetRow);
                rows.push_back(c);
                m_surfWidgetTags.push_back(CString(kTags[k]));
                m_surfWidgetCats.push_back(CString());
                m_surfWidgetLevels.push_back(1);
                ++idx;
            }
        }
    }

    for (auto& cat : m_catalog.categories())
    {
        const CString catName(cat);
        if (!m_widgetFilter.IsEmpty() && catName != m_widgetFilter) continue;
        auto tpls = m_catalog.templates(cat);
        if (tpls.empty()) continue;

        xmlctl::TreeRow r;
        r.text = std::wstring(catName.GetString());
        r.level = 0;
        r.hasChildren = true;
        r.expanded = (m_collapsedCats.find(catName) == m_collapsedCats.end());
        r.selected = (idx == m_surfSelWidgetRow);
        rows.push_back(r);
        m_surfWidgetTags.push_back(CString());   // 分类行不可放置
        m_surfWidgetCats.push_back(catName);
        m_surfWidgetLevels.push_back(0);
        ++idx;

        if (!r.expanded) continue;   // 收起的分类不列模板行

        for (auto* t : tpls)
        {
            xmlctl::TreeRow c;
            c.text = std::wstring(t->display.GetString());
            c.level = 1;
            c.selected = (idx == m_surfSelWidgetRow);
            rows.push_back(c);
            m_surfWidgetTags.push_back(t->tag);
            m_surfWidgetCats.push_back(CString());
            m_surfWidgetLevels.push_back(1);
            ++idx;
        }
    }
}

void DesignerPaneView::buildDocRows(XmlElement* node, int level, std::vector<xmlctl::TreeRow>& rows,
                             XmlElement* sel)
{
    if (node == nullptr) return;
    xmlctl::TreeRow r;
    r.text = std::wstring(L"<") + node->tag + std::wstring(L"> ") +
             (node->name.empty() ? std::wstring(L"(unnamed)") : node->name);
    r.level = level;
    r.hasChildren = !node->children.empty();
    const bool collapsed = m_collapsedNodes.find(node) != m_collapsedNodes.end();
    r.expanded = !collapsed;
    r.selected = (node == sel) || (m_docSelPtrs.find(node) != m_docSelPtrs.end());
    rows.push_back(r);
    m_surfDocNodes.push_back(node);
    m_surfDocLevels.push_back(level);
    if (collapsed) return;   // 收起的子树不列出来
    for (auto& c : node->children)
        buildDocRows(&c, level + 1, rows, sel);
}

void DesignerPaneView::buildPropRows(XmlElement* node, std::vector<xmlctl::PropRow>& rows)
{
    rows.clear();
    if (node == nullptr) return;

    auto Int = [](int v) { CString s; s.Format(L"%d", v); return s; };
    auto Add = [&](const CString& grp, const CString& key, const CString& val) {
        if (!grp.IsEmpty())
        {
            xmlctl::PropRow g;
            g.key = std::wstring(grp.GetString());
            g.val.clear();
            g.group = true;
            rows.push_back(g);
        }
        xmlctl::PropRow p;
        p.key = std::wstring(key.GetString());
        p.val = std::wstring(val.GetString());
        p.editable = false;
        rows.push_back(p);
    };

    Add(L"通用", L"Level", node->get(L"Level").empty() ? CString(L"0") : CString(node->get(L"Level").c_str()));
    Add(L"", L"TabIndex", node->get(L"TabIndex").empty() ? CString(L"-1") : CString(node->get(L"TabIndex").c_str()));
    CString themeVal(node->get(L"Theme").c_str());
    if (themeVal.IsEmpty()) themeVal = L"（默认）";
    Add(L"", L"Theme", themeVal);

    // 布局组用 XML 里的属性名（Left/Top），不再用内部叫法 X/Y
    Add(L"布局", L"Left", Int(node->x));
    Add(L"", L"Top", Int(node->y));
    Add(L"", L"Width", Int(node->w));
    Add(L"", L"Height", Int(node->h));
    // Row/Column/RowSpan/ColumnSpan 是 Grid 的定位属性，只有父容器确实是 <Grid> 时才列出来。
    // 挂 Panel / Window / StackPanel / 绝对定位布局上没有任何意义（属性面板是给源码服务的，
    // 那里也读不到这几项），一律不显示，避免"Grid 跟 Panel 挂钩"的错觉。
    {
        XmlElement* par = nullptr;
        {
            XmlElement* r = m_doc.root();
            std::function<void(XmlElement*)> findPar = [&](XmlElement* cur) {
                if (cur == nullptr || par != nullptr) return;
                for (auto& c : cur->children)
                {
                    if (&c == node) { par = cur; return; }
                    findPar(&c);
                    if (par != nullptr) return;
                }
            };
            if (r != nullptr) findPar(r);
        }
        if (par != nullptr && _wcsicmp(par->tag.c_str(), L"Grid") == 0)
        {
            Add(L"", L"Row", Int(node->row));
            Add(L"", L"Column", Int(node->col));
            Add(L"", L"RowSpan", Int(node->rowSpan));
            Add(L"", L"ColumnSpan", Int(node->colSpan));
        }
    }

    // 文本类属性的真实名字随节点而变：XML 里写的是 Header（GroupBox）就显示 Header，
    // 其余一律 Text。新建控件写入的也是 Text。
    CString textKey = L"Text";
    if (!node->has(L"Text") && node->has(L"Header")) textKey = L"Header";

    const uidesign::CtrlTemplate* t = m_catalog.byTag(CString(node->tag.c_str()));
    if (t != nullptr)
    {
        CString grp = L"属性";
        for (auto& p : t->props)
        {
            CString key = p.name;
            CString val;
            if (key == L"Name") val = CString(node->name.c_str());
            else if (key == L"Text") { key = textKey; val = CString(node->text.c_str()); }
            else val = CString(node->get(key.GetString()).c_str());
            if (val.IsEmpty()) val = L"（默认）";       // 未显式写的属性标出"走默认"，避免值列留一大片空白
            Add(grp, key, val);
            grp.Empty();
        }
    }
    else
    {
        Add(L"属性", L"Name", CString(node->name.c_str()));
        CString textVal(node->text.c_str());
        if (textVal.IsEmpty()) textVal = L"（默认）";
        Add(L"", textKey, textVal);
    }

    // 补齐：模板属性清单之外，节点上真实存在的属性也全部列出。
    // 否则 XML 里写了的属性（Background/FontSize/Icon 之外的自定义项）在面板上看不到，
    // 用户会以为"属性显示不全"。已在上面展示过的 key 不重复，Left/Top/Width/Height 归布局组。
    {
        std::set<std::wstring> shown;
        for (auto& r : rows)
            if (!r.group) shown.insert(r.key);
        auto already = [&](const std::wstring& k) -> bool {
            for (auto& s : shown)
                if (_wcsicmp(s.c_str(), k.c_str()) == 0) return true;
            return false;
        };
        CString grp = L"其他属性";
        int added = 0;
        for (auto& kv : node->attrs)
        {
            const std::wstring& k = kv.first;
            if (_wcsnicmp(k.c_str(), L"xmlns", 5) == 0) continue;   // 命名空间声明不算可视属性
            if (_wcsicmp(k.c_str(), L"Name") == 0 || _wcsicmp(k.c_str(), L"Text") == 0 ||
                _wcsicmp(k.c_str(), L"Content") == 0 || _wcsicmp(k.c_str(), L"Header") == 0)
                continue;
            if (_wcsicmp(k.c_str(), L"Left") == 0 || _wcsicmp(k.c_str(), L"Top") == 0 ||
                _wcsicmp(k.c_str(), L"Width") == 0 || _wcsicmp(k.c_str(), L"Height") == 0)
                continue;
            // Grid.Row / Grid.Column / Grid.RowSpan / Grid.ColumnSpan 已在布局组以
            // Row/Column/RowSpan/ColumnSpan 显示，这里不再重复列一遍
            if (_wcsicmp(k.c_str(), L"Grid.Row") == 0 || _wcsicmp(k.c_str(), L"Grid.Column") == 0 ||
                _wcsicmp(k.c_str(), L"Grid.RowSpan") == 0 || _wcsicmp(k.c_str(), L"Grid.ColumnSpan") == 0)
                continue;
            if (already(k)) continue;
            Add(grp, CString(k.c_str()), CString(kv.second.c_str()));
            grp.Empty();
            shown.insert(k);
            ++added;
        }
        (void)added;
    }

    // 诊断：把最终行内容写进 menu_diag.log，便于核对"属性是否补齐"
    if (::GetEnvironmentVariableW(L"PYARX_DESIGN_DEBUG", nullptr, 0) > 0)
    {
        propDiag(L"[rows] tag=%s count=%d", node->tag.c_str(), (int)rows.size());
        for (size_t i = 0; i < rows.size(); ++i)
        {
            const xmlctl::PropRow& r = rows[i];
            if (r.group) propDiag(L"[row%d] <%s>", (int)i, r.key.c_str());
            else         propDiag(L"[row%d] %s = %s", (int)i, r.key.c_str(), r.val.c_str());
        }
    }
}