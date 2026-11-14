#pragma once
// CDesignCanvas：设计器预览画布
// 基于统一运行时 XmlElement 树渲染（GDI 预览 + 命中测试 + 选中控制点 + 网格点阵）。
// 选中节点用路径（uidesign::Path，根到节点的 child 索引链）表达，避免值语义下指针失效。
#include "stdafx.h"
#include "XmlTheme.h"
#include <stdio.h>
#include <stdarg.h>
#include <algorithm>
#include "XmlCore.h"
#include "../MfcXmlCore/controls/XmlMfcTags.h"   // 统一标签命名（X 前缀 -> 本意名）
#include "XmlGeometry.h"      // 绘制与夹点用的纯几何库（正圆弧、直线端点、各类夹点）
#include "UiDocUtil.h"
#include "XmlDraw.h"
#include "XmlLeafPaint.h"   // 叶子控件统一渲染器（预览器同一份）
#include "XmlD2D.h"
#include "XmlControls.h"
#include "DesignerTools.h"
#include "../MfcXmlCore/controls/XmlMfcOrder.h"   // Level 排序与运行时/预览器同源

namespace uidesign
{
    // 画布上控件用的浅色调色板：值统一来自 MfcXmlCore/XmlTheme.h，
    // 预览器（Viewer）用同一份——这是"预览器与设计器显示不一样"的根因修复。
    namespace cvdoc
    {
        const COLORREF kSurface = xmlmfc::theme::surface;   // 容器 / 空壳
        const COLORREF kButton  = xmlmfc::theme::button;    // 按钮底
        const COLORREF kField   = xmlmfc::theme::field;     // 输入、列表、表格
        const COLORREF kBorder  = xmlmfc::theme::border;    // 边框
        const COLORREF kInk     = xmlmfc::theme::ink;       // 正文
        const COLORREF kMuted   = xmlmfc::theme::muted;     // 次要文字、占位
        const COLORREF kAccent  = xmlmfc::theme::accent;    // 进度、勾选
        const COLORREF kTrack   = xmlmfc::theme::track;     // 轨道、空槽
        const COLORREF kHead    = xmlmfc::theme::head;      // 表头
        const COLORREF kGrid    = xmlmfc::theme::grid;      // 表格线

        // 画布解析 Items / Nodes / Pages 这类"竖线分隔"属性（与运行时同一语法）
        inline std::vector<std::wstring> splitBar(const std::wstring& s)
        {
            std::vector<std::wstring> out;
            size_t start = 0;
            while (start <= s.size())
            {
                const size_t i = s.find(L'|', start);
                const std::wstring seg = (i == std::wstring::npos) ? s.substr(start) : s.substr(start, i - start);
                if (!seg.empty()) out.push_back(seg);
                if (i == std::wstring::npos) break;
                start = i + 1;
            }
            return out;
        }

        // 树形：seg 里 '>' 表示父子层级；返回 {depth, text}
        inline void flattenNodes(const std::wstring& nodes,
                                 std::vector<std::pair<int, std::wstring>>& out)
        {
            for (auto& seg : splitBar(nodes))
            {
                int depth = 0;
                size_t start = 0;
                while (start <= seg.size())
                {
                    const size_t gt = seg.find(L'>', start);
                    const std::wstring part = (gt == std::wstring::npos) ? seg.substr(start)
                                                                        : seg.substr(start, gt - start);
                    if (!part.empty()) out.push_back({ depth++, part });
                    if (gt == std::wstring::npos) break;
                    start = gt + 1;
                }
            }
        }
    }

    // 画布消息：选中节点（lParam 为 new Path*，由接收方拷贝/释放）
    constexpr UINT WM_CANVAS_SELECT = WM_APP + 1;
    // 多选：lParam 为 std::vector<Path>*（首项是主选中，其余是附加选择）
    constexpr UINT WM_CANVAS_SELECT_MULTI = WM_APP + 3;
    constexpr UINT WM_DOC_CHANGED   = WM_APP + 2;   // 画布上直接改了文档（就地编辑/下拉/勾选）

    // ==== 临时诊断：只在 PYARX_DESIGN_DEBUG=1 时写 G:\PyArx\log\canvas_diag.log ====
    inline void diagLog(const char* fmt, ...)
    {
        static int on = -1;
        if (on < 0)
        {
            wchar_t ev[8] = {};
            on = (::GetEnvironmentVariableW(L"PYARX_DESIGN_DEBUG", ev, 8) >= 1 && ev[0] == L'1') ? 1 : 0;
        }
        if (on != 1) return;
        static bool first = true;
        FILE* f = nullptr;
        _wfopen_s(&f, L"G:\\PyArx\\log\\canvas_diag.log", first ? L"w" : L"a");
        first = false;
        if (f == nullptr) return;
        va_list ap; va_start(ap, fmt);
        vfprintf(f, fmt, ap);
        va_end(ap);
        fclose(f);
    }

    inline std::string diagNarrow(const std::wstring& s)
    {
        std::string r;
        for (size_t i = 0; i < s.size(); ++i)
            r.push_back(s[i] < 128 ? (char)s[i] : '?');
        return r;
    }

    class DesignCanvas : public CWnd
    {
    public:
        friend class DesignerPaneView;   // 画布内容由设计器整窗表面代画（paintInto）
        DesignCanvas() = default;

        // 绑定编辑文档（XmlRuntime 持有 XmlElement 树）；node 为空表示未选中

        void setDocument(XmlRuntime* doc)
        {
            m_doc = doc;
            m_docBmpDirty = true;
            m_sel.clear();
            m_selPath = nullptr;
            fitToView();
            Invalidate();
            // 启动路径（InitInstance 里 openFile）时消息泵还没跑到 WM_PAINT，
            // 单靠 Invalidate 标记的无效区会被吞掉：画布一直停在"首帧空文档"的样子。
            // 这里强制立刻画一次，文档一加载就看得见。
            UpdateWindow();
        }
        void setSelected(const Path& p)
        {
            m_sel = p;
            m_selPath = m_doc ? Resolve(m_doc->root(), p) : nullptr;
            m_selSet.clear();
            m_selNodes.clear();
            if (m_selPath != nullptr) { m_selSet.push_back(p); m_selNodes.push_back(m_selPath); }
            Invalidate();
        }
        // 多选：v 首项为主选中；用于对齐 / 均匀分布
        void setSelectionSet(const std::vector<Path>& v)
        {
            m_selSet = v;
            m_selNodes.clear();
            XmlElement* root = m_doc ? m_doc->root() : nullptr;
            for (auto& p : v) m_selNodes.push_back(Resolve(root, p));
            m_selPath = m_selNodes.empty() ? nullptr : m_selNodes.front();
            if (!v.empty()) m_sel = v.front(); else { m_sel.clear(); }
            Invalidate();
        }
        const std::vector<Path>& selectionSet() const { return m_selSet; }
        void clearSelection() { m_sel.clear(); m_selPath = nullptr; m_selSet.clear(); m_selNodes.clear(); Invalidate(); }

        void dumpDiag(XmlElement* n, int depth, int maxDepth)
        {
            if (n == nullptr || depth > maxDepth) return;
            const std::string tag = diagNarrow(n->tag);
            diagLog("%*s%s w=%d h=%d pv=%d,%d %dx%d kids=%d\n",
                    depth * 2, "", tag.c_str(), n->w, n->h,
                    n->previewX, n->previewY, n->previewW, n->previewH,
                    (int)n->children.size());
            for (size_t i = 0; i < n->children.size(); ++i)
                dumpDiag(&n->children[i], depth + 1, maxDepth);
        }

    public:
        ~DesignCanvas()
        {
            if (m_bgD2DBmp != nullptr) { m_bgD2DBmp->Release(); m_bgD2DBmp = nullptr; }
            if (m_docBmp != nullptr) { m_docBmp->Release(); m_docBmp = nullptr; }
            delete m_d2dCv; m_d2dCv = nullptr;
        }

        void setShowGrid(bool on) { m_showGrid = on; m_bgD2DW = 0; Invalidate(); }

        // 缩放：整个文档树 + 装饰层统一按倍率绘制，鼠标坐标进事件前按同一倍率反算
        float zoom() const { return m_zoom; }
        void setZoom(float z)
        {
            if (z < 0.25f) z = 0.25f;
            if (z > 4.0f) z = 4.0f;
            if (z == m_zoom) return;
            m_zoom = z;
            Invalidate(FALSE);
        }
        // 视图平移（窗口像素）。画布内容整体随 pan 走，鼠标事件入口按同一偏移反算，
        // 于是"拖动查看"和"命中测试"用的是同一套坐标，不会出现点到别处的问题。
        float panX() const { return m_panX; }
        float panY() const { return m_panY; }
        void setPan(float px, float py) { m_panX = px; m_panY = py; invalidateSurface(); }
        void resetView() { m_panX = 0; m_panY = 0; Invalidate(FALSE); }

        // 以画布客户坐标 localPt 为锚点缩放：缩放前后同一文档点仍落在光标下（鼠标滚轮用）
        void zoomAt(CPoint localPt, short delta)
        {
            const float z0 = (m_zoom <= 0.0f) ? 1.0f : m_zoom;
            float z1 = z0 * ((delta > 0) ? 1.2f : (1.0f / 1.2f));
            if (z1 < 0.05f) z1 = 0.05f;
            if (z1 > 8.0f) z1 = 8.0f;
            if (z1 == z0) return;
            const float dx = (float)localPt.x - m_panX;
            const float dy = (float)localPt.y - m_panY;
            m_zoom = z1;
            m_panX = (float)localPt.x - dx * (z1 / z0);
            m_panY = (float)localPt.y - dy * (z1 / z0);
            invalidateSurface();
        }

        // 标尺：窗口坐标下画顶部/左侧刻度条，不参与缩放
        void setShowRuler(bool on) { m_showRuler = on; Invalidate(FALSE); }

        BOOL create(CWnd* parent, UINT id)
        {
            // 不用 CS_HREDRAW|CS_VREDRAW，避免尺寸变化整窗擦白导致闪烁（配合内存 DC 双缓冲）
            // 背景用 WHITE_BRUSH，不设 WS_BORDER，避免画布左上角出现边框黑线
            CString cls = AfxRegisterWndClass(0,
                ::LoadCursor(nullptr, IDC_ARROW), (HBRUSH)::GetStockObject(WHITE_BRUSH), nullptr);
            return CWnd::Create(cls, _T("canvas"), WS_CHILD | WS_VISIBLE,
                CRect(0, 0, 100, 100), parent, id);
        }

    protected:
        // 双缓冲已经覆盖整个客户区，禁止系统先擦背景（否则白刷与离屏位图交替 -> 明显闪烁）
        afx_msg BOOL OnEraseBkgnd(CDC* pDC) { return TRUE; }

        // 悬浮光标：夹点 -> 对应方向的缩放光标；控件内部 -> 移动光标；绘制中 -> 十字
        afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
        {
            if (nHitTest == HTCLIENT)
            {
                CPoint pt; ::GetCursorPos(&pt); ScreenToClient(&pt);
                if (m_jig) { ::SetCursor(::LoadCursor(nullptr, IDC_CROSS)); return TRUE; }
                if (m_dragMode == kMove) { ::SetCursor(::LoadCursor(nullptr, IDC_SIZEALL)); return TRUE; }
                if (m_dragMode == kResize)
                {
                    if (m_lineDragIdx >= 0 || m_arcDragIdx >= 0)
                        ::SetCursor(::LoadCursor(nullptr, IDC_SIZEALL));
                    else
                        ::SetCursor(::LoadCursor(nullptr, handleCursor(m_resizeHandle)));
                    return TRUE;
                }
                if (m_selPath != nullptr && (m_doc == nullptr || m_selPath != m_doc->root()))
                {
                    XmlElement* hn = nullptr;
                    int h = hitHandleAny(pt, hn);        // 多选时其余成员的夹点同样可拖
                    if (h >= 0)
                    {
                        // 直线端点、椭圆圆心是"跟着鼠标走"，用四向移动光标
                        const bool moveish = isOpenShapeNode(hn) || (isEllipseNode(hn) && h == 0);
                        if (moveish)
                            ::SetCursor(::LoadCursor(nullptr, IDC_SIZEALL));
                        else
                            ::SetCursor(::LoadCursor(nullptr, handleCursor(boxHandleOf(hn, h))));
                        return TRUE;
                    }
                    CRect rc(m_selPath->previewX, m_selPath->previewY,
                             m_selPath->previewX + m_selPath->previewW,
                             m_selPath->previewY + m_selPath->previewH);
                    if (rc.PtInRect(pt)) { ::SetCursor(::LoadCursor(nullptr, IDC_SIZEALL)); return TRUE; }
                }
            }
            return CWnd::OnSetCursor(pWnd, nHitTest, message);
        }

        // 夹点索引 -> 光标资源：0=TL 1=TC 2=TR 3=ML 4=MR 5=BL 6=BC 7=BR
        static LPCWSTR handleCursor(int h)
        {
            switch (h)
            {
            case 0: case 7: return IDC_SIZENWSE;   // 左上 / 右下：西北-东南
            case 2: case 5: return IDC_SIZENESW;   // 右上 / 左下：东北-西南
            case 1: case 6: return IDC_SIZENS;     // 上中 / 下中：垂直
            case 3: case 4: return IDC_SIZEWE;     // 左中 / 右中：水平
            default: return IDC_ARROW;
            }
        }

        afx_msg void OnSize(UINT nType, int cx, int cy)
        {
            CWnd::OnSize(nType, cx, cy);
            // 画布是直呈窗口，宿主改尺寸（停靠面板拖动、窗口最大化）后必须自己重画，
            // 否则旧一帧的像素留在客户区，看起来就是"画布整块变黑"。
            if (cx > 0 && cy > 0)
            {
                if (m_autoFit) fitToView();
                Invalidate(FALSE);
            }
        }

        // 文档比视口大时按视口等比缩小：外壳版式（1440x900）在 1360 屏幕上也能整幅可见
        void fitToView()
        {
            if (!m_autoFit || m_doc == nullptr || m_doc->root() == nullptr) return;
            XmlElement* r = m_doc->root();
            CRect cr{ 0, 0, 0, 0 };
            GetClientRect(&cr);
            if (cr.Width() <= 64 || cr.Height() <= 64 || r->w <= 0 || r->h <= 0) return;
            const float fx = static_cast<float>(cr.Width() - 32) / static_cast<float>(r->w);
            const float fy = static_cast<float>(cr.Height() - 32) / static_cast<float>(r->h);
            float fit = fx < fy ? fx : fy;
            if (fit > 1.0f) fit = 1.0f;
            if (fit < 0.2f) fit = 0.2f;
            m_zoom = fit;
        }

        afx_msg void OnPaint()
        {
            CPaintDC dc(this);
            CRect client;
            GetClientRect(&client);
            const int W = client.Width(), H = client.Height();
            if (W <= 0 || H <= 0) return;

            // 画布是独立子窗口，内容由自己这套 D2D 路径上屏（父表面那块只是兜底）。
            // 中键平移/滚轮缩放的每帧重绘就落在这里，父表面不参与 —— 这就是"丝滑"的关键。
            if (!paintD2D(dc, W, H))
                dc.FillSolidRect(client, RGB(160, 160, 160));
        }

        // 背景（白底 + 网格）烘焙成 ID2D1Bitmap：直接写 DIB 像素，比 SetPixel 快两个数量级，
        // 只在尺寸/网格开关变化时重建，之后每帧一次 DrawBitmap
        void ensureBgBitmap(int W, int H, ID2D1RenderTarget* rt)
        {
            if (rt == nullptr || W <= 0 || H <= 0) return;
            if (m_bgD2DBmp != nullptr && m_bgD2DW == W && m_bgD2DH == H && m_bgD2DGrid == m_showGrid)
                return;
            if (m_bgD2DBmp != nullptr) { m_bgD2DBmp->Release(); m_bgD2DBmp = nullptr; }
            m_bgD2DW = m_bgD2DH = 0;

            BITMAPINFO bmi = {};
            bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bmi.bmiHeader.biWidth = W;
            bmi.bmiHeader.biHeight = -H;          // top-down
            bmi.bmiHeader.biPlanes = 1;
            bmi.bmiHeader.biBitCount = 32;
            bmi.bmiHeader.biCompression = BI_RGB;

            void* bits = nullptr;
            HDC screen = ::GetDC(nullptr);
            HDC memDC = ::CreateCompatibleDC(screen);
            HBITMAP hbm = ::CreateDIBSection(memDC, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
            if (hbm == nullptr || bits == nullptr)
            {
                if (hbm != nullptr) ::DeleteObject(hbm);
                ::DeleteDC(memDC);
                ::ReleaseDC(nullptr, screen);
                return;
            }

            unsigned int* px = static_cast<unsigned int*>(bits);
            const int total = W * H;
            // 画布底色：窗体（根窗口）以外的区域是"工作台"，用灰色；窗体本身仍按白底绘制。
            for (int i = 0; i < total; ++i) px[i] = 0x00A0A0A0u;   // RGB(160,160,160)
            if (m_showGrid)
            {
                const unsigned int dot = 0x00949494u;              // RGB(148,148,148)，灰色底上的浅点
                for (int y = 8; y < H; y += 8)
                {
                    unsigned int* row = px + static_cast<size_t>(y) * W;
                    for (int x = 8; x < W; x += 8) row[x] = dot;
                }
            }

            D2D1_BITMAP_PROPERTIES bp = D2D1::BitmapProperties(
                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE), 96.0f, 96.0f);
            const HRESULT hr = rt->CreateBitmap(D2D1::SizeU(static_cast<UINT32>(W),
                                                            static_cast<UINT32>(H)),
                                                bits, static_cast<UINT32>(W * 4), bp, &m_bgD2DBmp);
            ::DeleteObject(hbm);
            ::DeleteDC(memDC);
            ::ReleaseDC(nullptr, screen);
            if (FAILED(hr))
            {
                if (m_bgD2DBmp != nullptr) { m_bgD2DBmp->Release(); m_bgD2DBmp = nullptr; }
                return;
            }
            m_bgD2DW = W;
            m_bgD2DH = H;
            m_bgD2DGrid = m_showGrid;
        }

        // 把整棵文档树烘成一张"自然尺寸"的 D2D 位图（不乘倍率、不做平移）。
        // 之后平移/缩放都靠这张走位图变换；文档内容没变就不重建，避免每帧遍历节点树。
        void ensureDocCache(XmlElement* root, ID2D1RenderTarget* rt)
        {
            if (root == nullptr || rt == nullptr) { m_docBmpDirty = true; return; }
            const int dw = root->previewW;
            const int dh = root->previewH;
            // 超大文档 / 超高倍率下位图会爆内存：放弃烘焙，退回逐帧直画
            if (dw <= 0 || dh <= 0 || dw > 4096 || dh > 4096) { m_docBmpDirty = true; return; }

            if (!m_docBmpDirty && m_docBmp != nullptr && m_docBmpRoot == root &&
                m_docBmpW == dw && m_docBmpH == dh)
                return;

            if (m_docBmp != nullptr) { m_docBmp->Release(); m_docBmp = nullptr; }
            m_docBmpRoot = nullptr;

            BITMAPINFO bmi = {};
            bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bmi.bmiHeader.biWidth = dw;
            bmi.bmiHeader.biHeight = -dh;          // top-down
            bmi.bmiHeader.biPlanes = 1;
            bmi.bmiHeader.biBitCount = 32;
            bmi.bmiHeader.biCompression = BI_RGB;

            void* bits = nullptr;
            HDC screen = ::GetDC(nullptr);
            HDC memDC = ::CreateCompatibleDC(screen);
            HBITMAP hbm = ::CreateDIBSection(memDC, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
            if (hbm == nullptr || bits == nullptr)
            {
                if (hbm != nullptr) ::DeleteObject(hbm);
                ::DeleteDC(memDC);
                ::ReleaseDC(nullptr, screen);
                m_docBmpDirty = true;
                return;
            }

            // 烘焙画布：白底起手，再用文档自己的绘制路径把树画一遍（单位变换）
            D2DDcCanvas bake;
            RECT bakeRc = { 0, 0, dw, dh };
            if (bake.bind(memDC, bakeRc) && bake.begin(dw, dh))
            {
                RECT wr = { 0, 0, dw, dh };
                bake.fillRect(wr, RGB(255, 255, 255));
                drawNode(bake, root, CPoint(0, 0), true);
                bake.end();
            }

            D2D1_BITMAP_PROPERTIES bp = D2D1::BitmapProperties(
                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE), 96.0f, 96.0f);
            const HRESULT hr = rt->CreateBitmap(D2D1::SizeU(static_cast<UINT32>(dw),
                                                            static_cast<UINT32>(dh)),
                                                bits, static_cast<UINT32>(dw * 4), bp, &m_docBmp);
            ::DeleteObject(hbm);
            ::DeleteDC(memDC);
            ::ReleaseDC(nullptr, screen);
            if (FAILED(hr))
            {
                if (m_docBmp != nullptr) { m_docBmp->Release(); m_docBmp = nullptr; }
                m_docBmpDirty = true;
                return;
            }
            m_docBmpRoot = root;
            m_docBmpW = dw;
            m_docBmpH = dh;
            m_docBmpDirty = false;
        }

        bool paintD2D(CDC& dc, int W, int H)
        {
            // 画布是停靠面板里的子窗口：实测 HwndRenderTarget 直呈在本窗口上 EndDraw 返回 S_OK
            // 但内容不上屏（同一窗口的 GDI 双缓冲路径却能正常显示），
            // 因此改成"D2D 画到内存 DC（DCRenderTarget，仍是硬件绘制）+ BitBlt 上屏"，
            // 呈现方式与已验证可靠的 GDI 双缓冲一致，不再有"画了但看不见"的静默失败。
            if (m_d2dCv == nullptr) m_d2dCv = new D2DDcCanvas();

            if (m_memDC.GetSafeHdc() == nullptr)
                m_memDC.CreateCompatibleDC(&dc);
            if (m_memBmp.GetSafeHandle() == nullptr || m_memW != W || m_memH != H)
            {
                if (m_memBmp.GetSafeHandle() != nullptr) m_memBmp.DeleteObject();
                if (!m_memBmp.CreateCompatibleBitmap(&dc, W, H)) return false;
                m_memW = W; m_memH = H;
            }

            CBitmap* oldBmp = m_memDC.SelectObject(&m_memBmp);
            RECT bindRc = {0, 0, W, H};
            const bool bindOk = m_d2dCv->bind(m_memDC.GetSafeHdc(), bindRc);
            const bool began = bindOk && m_d2dCv->begin(W, H);
            XmlElement* root = m_doc ? m_doc->root() : nullptr;
            {
                static int logFrames = 0;
                if (logFrames < 3)
                {
                    ++logFrames;
                    diagLog("[paintD2D] W=%d H=%d bind=%d begin=%d root=%p kids=%d\n", W, H,
                            bindOk ? 1 : 0, began ? 1 : 0, (void*)root,
                            root ? (int)root->children.size() : 0);
                }
            }
            if (!began)
            {
                m_memDC.SelectObject(oldBmp);
                return false;
            }

            ID2D1RenderTarget* rt = m_d2dCv->target();
            ensureBgBitmap(W, H, rt);
            if (rt != nullptr && m_bgD2DBmp != nullptr)
                rt->DrawBitmap(m_bgD2DBmp, D2D1::RectF(0.0f, 0.0f,
                                                       static_cast<float>(W), static_cast<float>(H)));

            // 缩放 + 平移：背景位图已经画过（不缩放/不平移），
            // 从这里开始文档树与装饰层统一乘倍率并跟随 m_panX/m_panY 走。
            if (rt != nullptr)
                rt->SetTransform(D2D1::Matrix3x2F::Scale(m_zoom, m_zoom) *
                                 D2D1::Matrix3x2F::Translation(m_panX, m_panY));

            if (root != nullptr)
            {
                const RECT vp = docViewport(W, H);
                drawNode(*m_d2dCv, root, CPoint(0, 0), true, RGB(255, 255, 255), &vp);
            }
            else
                m_d2dCv->text(CRect(0, 0, W, H), L"打开或新建 XML 后在此预览", RGB(170, 170, 170),
                             DT_CENTER | DT_VCENTER | DT_SINGLELINE, 0, L"");

            // 装饰层也走 D2D：若用 GDI 画在窗口 DC 上，下一次呈现会把它盖掉
            // 多选：每个被选中的对象都画自己的 8 个夹点（主选中红色，其余蓝色），
            // 拖任意一个夹点都按选区外接框统一缩放
            {
                XmlElement* root = (m_doc != nullptr) ? m_doc->root() : nullptr;
                drawSelectionCv(*m_d2dCv, m_selPath, CPoint(0, 0),
                                m_selPath != nullptr && m_selPath != root);
                const COLORREF multi = RGB(96, 160, 255);
                for (size_t i = 1; i < m_selNodes.size(); ++i)
                {
                    XmlElement* n = m_selNodes[i];
                    if (n == nullptr || n == root) continue;
                    drawSelectionCv(*m_d2dCv, n, CPoint(0, 0), true, multi);
                }
            }
            drawJigCv(*m_d2dCv);
            drawBandCv(*m_d2dCv);

            // 标尺在窗口坐标画，先恢复单位变换
            if (rt != nullptr) rt->SetTransform(D2D1::Matrix3x2F::Identity());
            if (m_showRuler) drawRulerCv(*m_d2dCv, 0, 0, W, H);

            m_d2dCv->end();
            const HRESULT hr = m_d2dCv->lastError();
            if (FAILED(hr))
            {
                m_memDC.SelectObject(oldBmp);
                return false;
            }
            // 必须先 BitBlt 再换回原位图：换回后内存 DC 已不是这张画好内容的位图，拷出来是空的
            dc.BitBlt(0, 0, W, H, &m_memDC, 0, 0, SRCCOPY);
            m_memDC.SelectObject(oldBmp);
            if (FAILED(hr)) diagLog("[paintD2D] end hr=0x%08X\n", static_cast<unsigned int>(hr));
            return true;
        }

        afx_msg void OnMButtonDown(UINT nFlags, CPoint point)
        {
            (void)nFlags;
            if (m_panning) return;
            m_panning = true;
            m_panStart = point;
            m_panX0 = m_panX;
            m_panY0 = m_panY;
            SetCapture();
            ::SetCursor(::LoadCursor(nullptr, IDC_SIZEALL));
        }

        afx_msg void OnMButtonUp(UINT nFlags, CPoint point)
        {
            (void)nFlags; (void)point;
            if (!m_panning) return;
            m_panning = false;
            if (::GetCapture() == m_hWnd) ReleaseCapture();
        }

        afx_msg void OnLButtonDown(UINT flags, CPoint point)
        {
            point = docPt(point);
            // 绘制 jig：点选控件箱后，在画布按下即定起点、拖动实时预览、松手按尺寸创建
            bool drawMode = false;
            if (onDrawBegin && !m_jig) drawMode = onDrawBegin(point);
            diagLog("[ldown] pt=%d,%d drawMode=%d\n", point.x, point.y, drawMode ? 1 : 0);
            if (drawMode)
            {
                m_jig = true;
                m_jigStart = point;
                m_jigRect = CRect(point.x, point.y, point.x, point.y);
                SetCapture();
                return;
            }
            if (onCreateNode)
            {
                if (onCreateNode(point))
                    return;   // 已创建控件，不进入选中/拖拽
            }
            XmlElement* root = m_doc ? m_doc->root() : nullptr;
            if (root == nullptr) return;

            if (m_selPath != nullptr)
            {
                // 夹点优先级最高：点中任一已选控件的夹点 → 缩放（多选按选区外接框统一缩放）；
                // 椭圆圆心夹点 = 整体移动，直线端点 = 拖单端改长度和角度
                XmlElement* hNode = nullptr;
                int h = hitHandleAny(point, hNode);
                if (h >= 0)
                {
                    diagLog("[grip] h=%d node=%ws x=%d y=%d w=%d h=%d pt=%d,%d\n", h,
                            hNode ? hNode->tag.c_str() : L"?", hNode ? hNode->previewX : 0,
                            hNode ? hNode->previewY : 0, hNode ? hNode->previewW : 0,
                            hNode ? hNode->previewH : 0, point.x, point.y);
                    if (hNode == m_selPath && isEllipseNode(hNode) && h == 0)
                    {
                        beginDrag(kMove, -1, point);
                        return;
                    }
                    beginDrag(kResize, h, point);
                    return;
                }
                // 命中测试：仅当点命中的最顶层控件就是已选控件本身时，才进入移动。
                // 否则（命中其他控件/空白）落到下方统一选中逻辑，避免选中容器后点
                // 其内部控件被误判为移动容器而“选不上”。
                Path hit;
                bool found = hitTestRoot(root, point, hit);
                XmlElement* hitN = nullptr;
                if (found)
                {
                    XmlElement* n = root; bool ok = true;
                    for (int i : hit)
                    {
                        if (i < 0 || i >= (int)n->children.size()) { ok = false; break; }
                        n = &n->children[i];
                    }
                    if (ok) hitN = n;
                }
                if (hitN == m_selPath)
                {
                    CRect rc(m_selPath->previewX, m_selPath->previewY,
                             m_selPath->previewX + m_selPath->previewW,
                             m_selPath->previewY + m_selPath->previewH);
                    if (rc.PtInRect(point))
                    {
                        // 已选中控件上按 Ctrl 拖动 = 复制一份跟着走（不复制就只是移动）
                        if ((::GetKeyState(VK_CONTROL) & 0x8000) != 0 && !m_sel.empty())
                        {
                            m_ctrlCopyPath = m_sel;
                            m_ctrlCopyPt = point;
                            m_ctrlCopyPending = true;
                            SetCapture();
                            return;
                        }
                        beginDrag(kMove, -1, point);
                        return;
                    }
                }
            }

            Path hit;
            bool hitted = hitTestRoot(root, point, hit);
            diagLog("[ldown] hit=%d pathlen=%d selEmpty=%d\n", hitted ? 1 : 0, (int)hit.size(),
                    m_sel.empty() ? 1 : 0);
            if (hitted && !hit.empty())          // 命中具体控件：选中
            {
                // Ctrl+按住拖动 = 复制出一份并跟着鼠标走（松手落在哪就是哪）；
                // Ctrl+单击不移动仍然是多选/取消多选，见 OnLButtonUp 的 pending 分支
                if ((::GetKeyState(VK_CONTROL) & 0x8000) != 0 && hit.size() > 0)
                {
                    XmlElement* cnode = Resolve(root, hit);
                    if (cnode != nullptr && cnode != root)
                    {
                        m_ctrlCopyPath = hit;
                        m_ctrlCopyPt = point;
                        m_ctrlCopyPending = true;
                        SetCapture();
                        return;
                    }
                }
                // Ctrl+点击：加入 / 移出多选（对齐、均匀分布靠这个集合）
                if ((::GetKeyState(VK_CONTROL) & 0x8000) != 0 && !m_selSet.empty())
                {
                    ctrlToggleSelection(hit);
                    return;
                }
                Path* p = new Path(std::move(hit));
                GetParent()->PostMessage(WM_CANVAS_SELECT, 0, reinterpret_cast<LPARAM>(p));
            }
            else                                  // 空白（含仅命中根窗口）：开始框选
            {
                m_banding = true;
                m_bandStart = point;
                m_bandRect = CRect(point.x, point.y, point.x, point.y);
                SetCapture();
            }
        }

        afx_msg void OnMouseMove(UINT nFlags, CPoint point)
        {
            // 中键拖动 = 平移视图（不进文档坐标，直接用窗口像素）
            if (m_panning)
            {
                m_panX = m_panX0 + (float)(point.x - m_panStart.x);
                m_panY = m_panY0 + (float)(point.y - m_panStart.y);
                invalidateSurface();
                return;
            }
            point = docPt(point);
            // 左键是否仍按住：真实鼠标看全局键状态；PostMessage 注入调试时消息里带 MK_LBUTTON
            const bool lbtnHeld = ((nFlags & MK_LBUTTON) != 0) ||
                                  ((::GetKeyState(VK_LBUTTON) & 0x8000) != 0);
            // Ctrl+拖动复制：越过 3px 阈值才真正复制，没动就退化成 Ctrl+单击多选
            if (m_ctrlCopyPending)
            {
                if (!lbtnHeld) { m_ctrlCopyPending = false; return; }
                if (abs(point.x - m_ctrlCopyPt.x) <= 3 && abs(point.y - m_ctrlCopyPt.y) <= 3) return;
                m_ctrlCopyPending = false;
                if (onCtrlDragCopy) onCtrlDragCopy(m_ctrlCopyPath, m_ctrlCopyPt);
                return;
            }
            if (m_jig)
            {
                // 左键已释放（异常状态）则直接完成绘制
                if (!lbtnHeld) { finishJig(point); return; }
                m_jigRect = CRect(m_jigStart, point);
                Invalidate();
                return;
            }
            if (m_banding)
            {
                if (!lbtnHeld) { finishBand(point); return; }
                m_bandCrossing = point.x < m_bandStart.x;   // 反向（右->左）= 相交选择
                m_bandRect = CRect(m_bandStart, point);
                Invalidate();
                return;
            }
            if (m_dragMode == kNone || m_selPath == nullptr)
                return;
            // 左键已释放则结束拖拽
            if (!lbtnHeld)
            {
                endDrag();
                return;
            }
            int dx = point.x - m_dragOrigin.x;
            int dy = point.y - m_dragOrigin.y;
            if (m_dragMode == kMove)
            {
                int nx = m_origX + dx, ny = m_origY + dy;
                static int dbgMove = 0;
                if (dbgMove < 200)
                {
                    diagLog("[dragmove] pt=%d,%d origin=%d,%d d=%d,%d orig=%d,%d nx=%d,%d prev=%d,%d\n",
                            point.x, point.y, m_dragOrigin.x, m_dragOrigin.y, dx, dy,
                            m_origX, m_origY, nx, ny,
                            (int)m_selPath->previewX, (int)m_selPath->previewY);
                    dbgMove++;
                }
                applyGeom(m_selPath, nx, ny, m_origW, m_origH, m_offX, m_offY);
            }
            else
            {
                // 圆弧：拖端点改角度而不是改方框 —— 起点跟手改 StartAngle，终点跟手改 SweepAngle
                if (m_arcDragIdx >= 0 && m_selPath != nullptr)
                {
                    // 圆心与半径在 beginDrag 时就钉住了，这里全程不再变，只把拖动点换算成角度
                    const double deg = atan2((double)point.y - m_arcCy,
                                             (double)point.x - m_arcCx) * 180.0 / 3.14159265358979323846;
                    const int hitAng = normAngle(deg);
                    int ns = m_arcStart0, nw = m_arcSweep0;
                    const bool ccw = (m_arcSweep0 < 0);
                    if (m_arcDragIdx == 0)                       // 拖起点：终点角度不动
                    {
                        const int endAng = m_arcStart0 + m_arcSweep0;
                        ns = hitAng;
                        nw = normAngle(endAng - ns);
                        if (ccw) nw = -nw;
                        if (nw == 0) nw = ccw ? -15 : 15;
                    }
                    else                                          // 拖终点：起点角度不动
                    {
                        nw = normAngle(hitAng - ns);
                        if (ccw) nw = -nw;
                        if (nw == 0) nw = ccw ? -15 : 15;
                    }
                    // 角度变了，弧的外接框跟着变，但圆心/半径仍取锚定值 —— 于是弧始终是同一个圆上的一段
                    CRect nb;
                    arcBoxFromArc(m_arcCx, m_arcCy, m_arcR, ns, nw, nb);
                    applyGeom(m_selPath, nb.left - m_offX, nb.top - m_offY,
                              nb.Width(), nb.Height(), m_offX, m_offY);
                    m_selPath->set(L"StartAngle", std::to_wstring(ns));
                    m_selPath->set(L"SweepAngle", std::to_wstring(nw));
                    Invalidate();
                    return;
                }
                // 直线：拖端点不是"按方框缩放"，而是把该端点直接跟到鼠标，
                // 另一端不动；长度、角度、以及走哪条对角线（Direction）随之重算。
                if (m_lineDragIdx >= 0 && m_selPath != nullptr)
                {
                    CPoint a = m_lineA, b = m_lineB;
                    CPoint& moved = (m_lineDragIdx == 0) ? a : b;
                    moved = point;
                    const int ddx = b.x - a.x;
                    const int ddy = b.y - a.y;
                    std::wstring dir = L"Down";
                    if (abs(ddy) <= 2) dir = L"H";
                    else if (abs(ddx) <= 2) dir = L"V";
                    else if ((ddx > 0) != (ddy > 0)) dir = L"Up";   // 斜率反向 → 左下→右上
                    CRect nb(a, b);
                    nb.NormalizeRect();
                    if (dir == L"H") { nb.top = a.y; nb.bottom = a.y + 1; }
                    if (dir == L"V") { nb.left = a.x; nb.right = a.x + 1; }
                    applyGeom(m_selPath, nb.left, nb.top, nb.Width(), nb.Height(), m_offX, m_offY);
                    m_selPath->set(L"Direction", dir);
                    Invalidate();
                    return;
                }
                // 8 手柄：0=TL 1=TC 2=TR 3=ML 4=MR 5=BL 6=BC 7=BR
                // 以选区外接框为基准统一缩放：单选时退化成原来的"改边"行为，
                // 多选时所有选中对象按同一比例一起放大/缩小（QW 语义）
                const int rh = (m_resizeBoxHandle >= 0) ? m_resizeBoxHandle : m_resizeHandle;
                CRect box = m_grpItems.empty()
                    ? CRect(m_origX + m_offX, m_origY + m_offY, m_origX + m_offX + m_origW, m_origY + m_offY + m_origH)
                    : m_grpBox;
                int nx = box.left, ny = box.top, nw = box.Width(), nh = box.Height();
                if (rh == 2 || rh == 4 || rh == 7) nw = box.Width() + dx;
                if (rh == 0 || rh == 3 || rh == 5) { nx = box.left + dx; nw = box.Width() - dx; }
                if (rh == 0 || rh == 1 || rh == 2) { ny = box.top + dy; nh = box.Height() - dy; }
                if (rh == 5 || rh == 6 || rh == 7) nh = box.Height() + dy;
                if (nw < 8) nw = 8;
                if (nh < 8) nh = 8;

                const double sx = (box.Width() > 0) ? (double)nw / box.Width() : 1.0;
                const double sy = (box.Height() > 0) ? (double)nh / box.Height() : 1.0;
                if (m_grpItems.empty())
                {
                    applyGeom(m_selPath, nx - m_offX, ny - m_offY, nw, nh, m_offX, m_offY);
                }
                else
                {
                    for (auto& it : m_grpItems)
                    {
                        const int px = nx + (int)((it.x + it.offX - box.left) * sx + 0.5);
                        const int py = ny + (int)((it.y + it.offY - box.top) * sy + 0.5);
                        int w2 = (int)(it.w * sx + 0.5); if (w2 < 8) w2 = 8;
                        int h2 = (int)(it.h * sy + 0.5); if (h2 < 8) h2 = 8;
                        applyGeom(it.n, px - it.offX, py - it.offY, w2, h2, it.offX, it.offY);
                    }
                }
            }
            Invalidate();
        }

        afx_msg void OnRButtonDown(UINT flags, CPoint point)
        {
            CPoint cli = point;                 // 弹菜单要用客户坐标；point 之后会被转成文档坐标
            point = docPt(point);
            XmlElement* root = m_doc ? m_doc->root() : nullptr;
            if (root == nullptr) return;
            Path hit;
            if (!hitTestRoot(root, point, hit) || hit.empty()) return;   // 空白处不弹菜单

            XmlElement* n = root;
            bool ok = true;
            for (int i : hit)
            {
                if (i < 0 || i >= (int)n->children.size()) { ok = false; break; }
                n = &n->children[i];
            }
            if (!ok) return;

            CPoint scr = cli;
            ClientToScreen(&scr);
            if (onContextMenu) onContextMenu(n, scr);
        }

        afx_msg void OnLButtonUp(UINT, CPoint point)
        {
            point = docPt(point);
            // 按下时按着 Ctrl 点中控件、但没拖出阈值 —— 当作 Ctrl+单击处理（多选切换）
            if (m_ctrlCopyPending)
            {
                m_ctrlCopyPending = false;
                if (::GetCapture() == m_hWnd) ReleaseCapture();
                ctrlToggleSelection(m_ctrlCopyPath);
                return;
            }
            if (m_jig) { finishJig(point); return; }
            if (m_banding) { finishBand(point); return; }
            if (m_dragMode != kNone)
                endDrag();
        }

        // Ctrl+点击的多选切换：已在集合里就移出，否则加入（集合只剩 1 个时退回单选）
        void ctrlToggleSelection(const Path& hit)
        {
            std::vector<Path> v = m_selSet;
            bool removed = false;
            for (size_t k = 0; k < v.size(); ++k)
                if (v[k] == hit) { v.erase(v.begin() + k); removed = true; break; }
            if (!removed) v.push_back(hit);
            if (v.empty())
            {
                Path* p = new Path();
                GetParent()->PostMessage(WM_CANVAS_SELECT, 0, reinterpret_cast<LPARAM>(p));
            }
            else if (v.size() == 1)
            {
                Path* p = new Path(v[0]);
                GetParent()->PostMessage(WM_CANVAS_SELECT, 0, reinterpret_cast<LPARAM>(p));
            }
            else
            {
                setSelectionSet(v);
                auto* vv = new std::vector<Path>(v);
                GetParent()->PostMessage(WM_CANVAS_SELECT_MULTI, 0, reinterpret_cast<LPARAM>(vv));
            }
        }

        // 图元专用夹点方案：
        //   Line      → 2 个（起点、终点），拖任一端改长度和角度，不给矩形外框夹点
        //   Arc       → 2 个（弧的两个端点），拖端点改起始角 / 扫掠角
        //   Ellipse   → 5 个（0 圆心 = 整体移动；1 上 / 2 下 / 3 左 / 4 右 = 改尺寸）
        //   其余控件   → 8 个方框夹点（维持原样）
        static bool isLineNode(const XmlElement* n) { return n != nullptr && tagIs(n, L"Line"); }
        static bool isEllipseNode(const XmlElement* n) { return n != nullptr && tagIs(n, L"Ellipse"); }
        static bool isArcNode(const XmlElement* n) { return n != nullptr && tagIs(n, L"Arc"); }
        // 两端口夹点的"开放图元"（直线、圆弧）
        static bool isOpenShapeNode(const XmlElement* n) { return isLineNode(n) || isArcNode(n); }

        // 圆弧两端点：起点角、起始角+扫掠角
        void arcEndpoints(const XmlElement* n, CPoint& a, CPoint& b) const
        {
            const CRect rc(n->previewX, n->previewY,
                           n->previewX + n->previewW, n->previewY + n->previewH);
            int sa = 180, sw = 180;
            arcParams(n, sa, sw);
            std::vector<POINT> pts = arcPoints(rc, sa, sw, 64);
            if (pts.empty()) { a = CPoint(rc.left, rc.top); b = CPoint(rc.right, rc.bottom); return; }
            a = CPoint(pts.front().x, pts.front().y);
            b = CPoint(pts.back().x, pts.back().y);
        }

        // 直线两端点：按 Direction 把外框对角线还原成实际线段
        void lineEndpoints(const XmlElement* n, CPoint& a, CPoint& b) const
        {
            const CRect rc(n->previewX, n->previewY,
                           n->previewX + n->previewW, n->previewY + n->previewH);
            const xmlgeom::Rect2 box((double)rc.left, (double)rc.top, (double)rc.right, (double)rc.bottom);
            const xmlgeom::Line2 line = xmlgeom::lineFromBox(box, xmlgeom::lineDirFromText(n->get(L"Direction")));
            a = CPoint((int)floor(line.a.x + 0.5), (int)floor(line.a.y + 0.5));
            b = CPoint((int)floor(line.b.x + 0.5), (int)floor(line.b.y + 0.5));
        }

        // 按图元类型给出夹点位置，返回个数（pts 至少 8 个）；几何全部由 xmlgeom 提供
        int handlePoints(const XmlElement* n, CPoint pts[8]) const
        {
            if (n == nullptr) return 0;
            const CRect rc(n->previewX, n->previewY,
                           n->previewX + n->previewW, n->previewY + n->previewH);
            const xmlgeom::Rect2 box((double)rc.left, (double)rc.top, (double)rc.right, (double)rc.bottom);
            xmlgeom::Point2 gp[8];
            int cnt = 0;
            if (isLineNode(n))
                cnt = xmlgeom::lineHandles(box, xmlgeom::lineDirFromText(n->get(L"Direction")), gp);
            else if (isArcNode(n))
            {
                int sa = 180, sw = 180;
                arcParams(n, sa, sw);
                cnt = xmlgeom::arcHandles(box, (double)sa, (double)sw, gp);
            }
            else if (isEllipseNode(n))
                cnt = xmlgeom::ellipseHandles(box, gp);
            else
                cnt = xmlgeom::boxHandles(box, gp);
            for (int i = 0; i < cnt; ++i)
                pts[i] = CPoint((int)floor(gp[i].x + 0.5), (int)floor(gp[i].y + 0.5));
            return cnt;
        }

        // 椭圆夹点换算成方框夹点索引（1 上→1 上中，2 下→6 下中，3 左→3 左中，4 右→4 右中）；
        // 圆心返回 -1（圆心不是缩放，是整体移动）
        static int boxHandleOf(const XmlElement* n, int h)
        {
            if (isEllipseNode(n))
            {
                static const int map[5] = { -1, 1, 6, 3, 4 };
                return (h >= 0 && h < 5) ? map[h] : -1;
            }
            return h;
        }

        // 8 个控制点命中（与 drawNode 选中态一致，含画布原点 20,20）
        // 所有被选中对象（主选中 + 多选成员）的夹点命中：多选时每个对象都有自己的一套夹点
        int hitHandleAny(CPoint p, XmlElement*& outNode)
        {
            outNode = nullptr;
            if (m_selPath == nullptr) return -1;
            int h = hitHandle(m_selPath, p);
            if (h >= 0) { outNode = m_selPath; return h; }
            for (size_t i = 1; i < m_selNodes.size(); ++i)
            {
                if (m_selNodes[i] == nullptr) continue;
                int hh = hitHandle(m_selNodes[i], p);
                if (hh >= 0) { outNode = m_selNodes[i]; return hh; }
            }
            return -1;
        }

        int hitHandle(XmlElement* n, CPoint p)
        {
            if (n == nullptr) return -1;
            CPoint pts[8];
            const int cnt = handlePoints(n, pts);
            for (int i = 0; i < cnt; ++i)
                if (abs(p.x - pts[i].x) <= 6 && abs(p.y - pts[i].y) <= 6)
                    return i;
            return -1;
        }

        void beginDrag(int mode, int handle, CPoint pt, bool undoAlready = false)
        {
            // 文档根（Window）是设计器的"画布本身"，不支持拖动/改尺寸
            if (m_selPath == nullptr || (m_doc != nullptr && m_selPath == m_doc->root()))
            { m_dragMode = kNone; return; }
            m_dragMode = static_cast<DragMode>(mode);
            m_resizeHandle = handle;
            m_resizeBoxHandle = boxHandleOf(m_selPath, handle);
            m_lineDragIdx = -1;
            m_arcDragIdx = -1;
            if (isLineNode(m_selPath))
            {
                lineEndpoints(m_selPath, m_lineA, m_lineB);
                if (handle == 0 || handle == 1) m_lineDragIdx = handle;   // 拖的是起点还是终点
            }
            else if (isArcNode(m_selPath))
            {
                arcParams(m_selPath, m_arcStart0, m_arcSweep0);
                if (handle == 0 || handle == 1) m_arcDragIdx = handle;    // 拖的是弧起点还是弧终点
                // 拖端点只调角度：此刻把圆心与半径钉住，整段拖动都不再变
                const int aox = (int)m_selPath->previewX - m_selPath->x;
                const int aoy = (int)m_selPath->previewY - m_selPath->y;
                const CRect arcRc(m_selPath->previewX, m_selPath->previewY,
                                  m_selPath->previewX + m_selPath->previewW,
                                  m_selPath->previewY + m_selPath->previewH);
                arcGeom(arcRc, m_arcStart0, m_arcSweep0, m_arcCx, m_arcCy, m_arcR);
                m_arcOffX = aox;
                m_arcOffY = aoy;
            }
            m_dragOrigin = pt;
            if (m_selPath != nullptr)
            {
                m_origX = m_selPath->x; m_origY = m_selPath->y;
                m_origW = m_selPath->w; m_origH = m_selPath->h;
                // 父容器偏移：画布绝对位置减自身相对位置
                m_offX = (int)m_selPath->previewX - m_selPath->x;
                m_offY = (int)m_selPath->previewY - m_selPath->y;
                diagLog("[drag] begin mode=%d handle=%d origin=%d,%d node x=%d y=%d w=%d h=%d prev=%d,%d %dx%d off=%d,%d\n",
                        mode, handle, pt.x, pt.y, m_origX, m_origY, m_origW, m_origH,
                        (int)m_selPath->previewX, (int)m_selPath->previewY,
                        (int)m_selPath->previewW, (int)m_selPath->previewH, m_offX, m_offY);
            }

            // 缩放：收集全部选中节点（主选中 + 多选成员），以外接框为统一缩放基准
            if (mode == kResize)
            {
                m_grpItems.clear();
                std::vector<XmlElement*> nodes;
                nodes.push_back(m_selPath);
                for (size_t i = 1; i < m_selNodes.size(); ++i)
                {
                    XmlElement* n = m_selNodes[i];
                    if (n == nullptr || n == m_selPath) continue;
                    nodes.push_back(n);
                }
                CRect box(0, 0, 0, 0);
                bool firstBox = true;
                for (auto* n : nodes)
                {
                    if (n == nullptr) continue;
                    if (m_doc != nullptr && n == m_doc->root()) continue;   // 根窗体不参与缩放
                    const CRect rc(n->previewX, n->previewY,
                                   n->previewX + n->previewW, n->previewY + n->previewH);
                    if (firstBox) { box = rc; firstBox = false; }
                    else box.UnionRect(box, rc);
                    GrpItem it;
                    it.n = n; it.x = n->x; it.y = n->y; it.w = n->w; it.h = n->h;
                    it.offX = (int)n->previewX - n->x;
                    it.offY = (int)n->previewY - n->y;
                    m_grpItems.push_back(it);
                }
                m_grpBox = box;
                diagLog("[drag] group n=%d box=%d,%d %dx%d\n", (int)m_grpItems.size(),
                        box.left, box.top, box.Width(), box.Height());
            }
            if (onEditBegin && !undoAlready) onEditBegin();   // DesignerPaneView：PushUndo
            SetCapture();
        }

        void endDrag()
        {
            m_dragMode = kNone;
            m_docBmpDirty = true;
            if (m_selPath != nullptr)
                diagLog("[drag] end node x=%d y=%d prev=%d,%d\n", m_selPath->x, m_selPath->y,
                        (int)m_selPath->previewX, (int)m_selPath->previewY);
            if (::GetCapture() == m_hWnd) ReleaseCapture();
            if (onEditEnd) onEditEnd();       // DesignerPaneView：重布局 + 属性面板 + 状态栏
        }

        // 绘制 jig 结束：把最终矩形交给 DesignerPaneView 创建控件
        void finishJig(CPoint point)
        {
            m_jig = false;
            if (::GetCapture() == m_hWnd) ReleaseCapture();
            CRect r(m_jigStart, point);
            r.NormalizeRect();
            m_jigRect.SetRectEmpty();
            if (onDrawEnd) onDrawEnd(r);
            m_pendingShape.clear();      // 一个图元放完，工具条回到未选中状态
            Invalidate();
        }

        // 框选结束：正向（左->右）全包含；反向（右->左）相交即选。取命中最深（最具体）的控件。
        // 纯容器不参与多选（选中它是为了往里加控件，不是拿来对齐的）
        static bool isPureContainerTag(const std::wstring& tag) { return uidesign::IsPureContainer(tag); }
        // np 的祖先是否已经在 hits 里（避免父子同时进多选）
        static bool hasLeafInside(const std::vector<Path>& hits, const Path& np)
        {
            for (auto& h : hits)
            {
                if (h.size() >= np.size()) continue;
                bool same = true;
                for (size_t i = 0; i < h.size(); ++i) if (h[i] != np[i]) { same = false; break; }
                if (same) return true;
            }
            return false;
        }

        void finishBand(CPoint point)
        {
            m_banding = false;
            if (::GetCapture() == m_hWnd) ReleaseCapture();
            CRect band(m_bandStart, point);
            band.NormalizeRect();
            bool crossing = point.x < m_bandStart.x;
            m_bandRect.SetRectEmpty();
            Invalidate();
            if (band.Width() < 3 && band.Height() < 3)
            {
                // 空白单击（没拖出框）= 取消选择
                clearSelection();
                Path* pp = new Path();      // 空路径 = 通知宿主清空选中
                GetParent()->PostMessage(WM_CANVAS_SELECT, 0, reinterpret_cast<LPARAM>(pp));
                return;
            }
            XmlElement* root = m_doc ? m_doc->root() : nullptr;
            if (root == nullptr) return;
            Path found; bool has = false;
            std::vector<Path> hits;
            std::function<void(XmlElement*, Path)> walk = [&](XmlElement* n, Path p) {
                for (int i = 0; i < (int)n->children.size(); ++i)
                {
                    XmlElement* c = &n->children[i];
                    CRect rc(c->previewX, c->previewY, c->previewX + c->previewW, c->previewY + c->previewH);
                    bool hit;
                    if (crossing)
                    {
                        CRect tmp; hit = (tmp.IntersectRect(rc, band) != FALSE);
                    }
                    else
                    {
                        hit = (rc.left >= band.left && rc.top >= band.top &&
                               rc.right <= band.right && rc.bottom <= band.bottom);
                    }
                    if (hit)
                    {
                        Path np = p; np.push_back(i);
                        if (!has || np.size() >= found.size()) { found = np; has = true; }
                        // 只把"可操作控件"收进多选：叶子节点 / 非纯容器
                        if (c->children.empty() || !isPureContainerTag(c->tag))
                        {
                            if (c->children.empty()) hits.push_back(np);
                            else if (!hasLeafInside(hits, np)) hits.push_back(np);
                        }
                    }
                    Path cp = p; cp.push_back(i);
                    walk(c, cp);
                }
            };
            Path empty;
            walk(root, empty);
            if (has)
            {
                if (hits.size() >= 2)
                {
                    auto* v = new std::vector<Path>(hits);
                    GetParent()->PostMessage(WM_CANVAS_SELECT_MULTI, 0, reinterpret_cast<LPARAM>(v));
                }
                else
                {
                    Path* pp = new Path(found);
                    GetParent()->PostMessage(WM_CANVAS_SELECT, 0, reinterpret_cast<LPARAM>(pp));
                }
            }
        }

        // 写回声明几何（x/y/w/h 成员 + preview 同步 + attrs Left/Top/Width/Height）
        // n.x/n.y 是相对父容器的坐标，previewX/previewY 是画布绝对坐标（父偏移 + 自身）。
        // 拖动时鼠标增量来自画布坐标，所以必须把父偏移加回去写 preview，否则控件会整块跳走
        // —— 这就是"鼠标坐标与按钮位置不符、按钮自己跑了"的根因。
        static void applyGeom(XmlElement* n, int x, int y, int w, int h, int offX, int offY)
        {
            n->hasX = n->hasY = n->hasW = n->hasH = true;
            n->x = x; n->y = y; n->w = w; n->h = h;
            n->previewX = x + offX; n->previewY = y + offY;
            n->previewW = w; n->previewH = h;
            n->set(L"Left", std::to_wstring(x));
            n->set(L"Top", std::to_wstring(y));
            n->set(L"Width", std::to_wstring(w));
            n->set(L"Height", std::to_wstring(h));
        }

        DECLARE_MESSAGE_MAP()

    public:
        // ---- Draw 工具条（外壳侧绘制，画布只保存"待放置图元"状态）----
        void SetPendingShapeTag(const std::wstring& tag, bool notify = true)
        {
            m_pendingShape = tag;
            if (notify && onPickCreate) onPickCreate(m_pendingShape);
            Invalidate(FALSE);
        }
        std::wstring PendingShapeTag() const { return m_pendingShape; }

        // 创建回调：点击画布时调用；返回 true 表示已创建（不再进入选中/拖拽）
        std::function<bool(CPoint)> onCreateNode;
        // 绘制 jig 回调：按下画布判定是否开始动态绘制；松手回传最终矩形（画布客户坐标）
        std::function<bool(CPoint)> onDrawBegin;
        std::function<void(CRect)> onDrawEnd;
        // 拖拽开始前回调（DesignerPaneView：PushUndo 快照）
        std::function<void()> onEditBegin;
        // 拖拽结束回调（DesignerPaneView：重布局 + 属性面板 + 状态栏）
        std::function<void()> onEditEnd;
        // 画布上右击某个控件：节点 + 屏幕坐标，由宿主弹菜单（复制/剪切/粘贴/删除）
        std::function<void(XmlElement*, CPoint)> onContextMenu;
        // Ctrl+拖动复制：宿主负责真复制（pushUndo / 唯一命名 / 刷新树和代码区），
        // 画布只负责判定"拖出阈值了"并回传源路径与按下点
        std::function<void(const Path&, CPoint)> onCtrlDragCopy;
        // 左侧 Draw 工具条点了某个图元：宿主记成"待放置 tag"，后续拖动即可画出来
        std::function<void(const std::wstring&)> onPickCreate;

        // 复制拖拽：宿主复制并选中副本后，让副本接着跟手（撤销快照由宿主统一压栈）
        void beginMoveDragNoUndo(CPoint pressPt) { beginDrag(kMove, -1, pressPt, true); }

    private:
        enum DragMode { kNone, kMove, kResize };
        DragMode m_dragMode = kNone;
        int m_resizeHandle = -1;
        // 图元专用：椭圆夹点换算出的方框夹点索引（-1 = 圆心/整体移动）；
        // 直线拖动端点时用的两端点与拖的是哪一端
        int m_resizeBoxHandle = -1;
        int m_lineDragIdx = -1;
        CPoint m_lineA{ 0, 0 }, m_lineB{ 0, 0 };
        // 圆弧：拖端点时记住原始角度，按"起点改 StartAngle / 终点改 SweepAngle"算
        int m_arcDragIdx = -1;
        int m_arcStart0 = 180, m_arcSweep0 = 180;
        double m_arcCx = 0, m_arcCy = 0, m_arcR = 1;   // 拖端点时锚定的圆心与半径（只改角度用）
        int m_arcOffX = 0, m_arcOffY = 0;
        // Ctrl+拖动复制的待定状态：按下时记下源节点与按下点，拖过阈值才真复制
        Path m_ctrlCopyPath;
        CPoint m_ctrlCopyPt;
        bool m_ctrlCopyPending = false;
        CPoint m_dragOrigin{ 0, 0 };
        int m_origX = 0, m_origY = 0, m_origW = 0, m_origH = 0;
        int m_offX = 0, m_offY = 0;   // 父容器在画布上的偏移（preview 与 x/y 的差值）

        // 多选统一缩放：拖任一夹点时，按选区外接框的比例把这批节点一起缩放
        struct GrpItem { XmlElement* n; int x, y, w, h, offX, offY; };
        std::vector<GrpItem> m_grpItems;
        CRect m_grpBox{ 0, 0, 0, 0 };

        // 绘制 jig 状态（按下起点 -> 拖动预览 -> 松手创建）
        bool m_jig = false;        CPoint m_jigStart{ 0, 0 };
        CRect m_jigRect{ 0, 0, 0, 0 };
        // 左侧 Draw 工具条当前选中的图元 tag（空 = 没选）
        std::wstring m_pendingShape;

        // 框选状态：正向（左->右）全包含选中；反向（右->左）接触到即选
        bool m_banding = false;
        CPoint m_bandStart{ 0, 0 };
        CRect m_bandRect{ 0, 0, 0, 0 };
        bool m_bandCrossing = false;

    private:
        // 解析颜色属性："#RRGGBB" / "#AARRGGBB" / 常见英文名；非法返回 def
        static COLORREF parseColor(const std::wstring& s, COLORREF def)
        {
            if (s.empty()) return def;
            if (s[0] == L'#')
            {
                std::wstring h = s.substr(1);
                if (h.size() == 6 || h.size() == 8)
                {
                    if (h.size() == 8) h = h.substr(2);   // 忽略 alpha
                    int r = (int)wcstol(h.substr(0, 2).c_str(), nullptr, 16);
                    int g = (int)wcstol(h.substr(2, 2).c_str(), nullptr, 16);
                    int b = (int)wcstol(h.substr(4, 2).c_str(), nullptr, 16);
                    return RGB(r, g, b);
                }
                return def;
            }
            std::wstring t = s;
            for (auto& c : t) c = (wchar_t)towlower(c);
            if (t == L"white") return RGB(255, 255, 255);
            if (t == L"black") return RGB(0, 0, 0);
            if (t == L"red") return RGB(220, 60, 60);
            if (t == L"green") return RGB(60, 170, 90);
            if (t == L"blue") return RGB(60, 110, 210);
            if (t == L"gray" || t == L"grey") return RGB(128, 128, 128);
            if (t == L"lightgray") return RGB(211, 211, 211);
            if (t == L"orange") return RGB(240, 150, 50);
            if (t == L"yellow") return RGB(240, 220, 80);
            return def;
        }

        // ---- canvas 版辅助绘制（与 GDI 版语义逐条一致，三端共用同一套外观规则）----
        static void drawRoundedCv(XmlCanvas& cv, const CRect& rc, COLORREF fill, COLORREF border, int radius = 6)
        {
            cv.fillRoundRect(rc, fill, border, radius);
        }

        // 3D 边框：内凹(上左深/下右亮) 或 外凸(上左亮/下右深)
        static void clearDarkenCv(XmlCanvas& cv, const CRect& rc, bool sunken, int thick)
        {
            const COLORREF dark = RGB(52, 56, 62);
            const COLORREF light = RGB(150, 158, 170);
            const COLORREF top = sunken ? dark : light;
            const COLORREF bottom = sunken ? light : dark;
            for (int i = 0; i < thick; ++i)
            {
                const int il = rc.left + i, it = rc.top + i, ir = rc.right - i, ib = rc.bottom - i;
                if (ir - il < 2 || ib - it < 2) break;
                cv.strokeLine(POINT{il, it}, POINT{ir, it}, top, 1);
                cv.strokeLine(POINT{il, it}, POINT{il, ib}, top, 1);
                cv.strokeLine(POINT{il, ib - 1}, POINT{ir, ib - 1}, bottom, 1);
                cv.strokeLine(POINT{ir - 1, it}, POINT{ir - 1, ib - 1}, bottom, 1);
            }
        }

        // 按 XML 的 BorderStyle/BorderThickness 画边框（覆盖控件默认描边）
        void applyBorderStyleCv(XmlCanvas& cv, const CRect& rc, COLORREF line, COLORREF fill,
                                const std::wstring& style, int thick)
        {
            if (_wcsicmp(style.c_str(), L"None") == 0)
            {
                cv.strokeRect(rc, fill, 1);   // 用背景色把默认描边盖掉
                return;
            }
            if (_wcsicmp(style.c_str(), L"Sunken") == 0 || _wcsicmp(style.c_str(), L"Raised") == 0)
            {
                clearDarkenCv(cv, rc, _wcsicmp(style.c_str(), L"Sunken") == 0, (std::max)(1, thick));
                return;
            }
            const int n = (std::max)(1, thick > 0 ? thick : 1);
            for (int i = 0; i < n && rc.Width() > i * 2 + 2 && rc.Height() > i * 2 + 2; ++i)
            {
                CRect r = rc;
                r.DeflateRect(i, i);
                cv.strokeRect(r, line, 1);
            }
        }

        // 按 XML 的 ScrollStyle 画滚动条示意
        void drawScrollBarsCv(XmlCanvas& cv, const CRect& rc, const std::wstring& style)
        {
            const int bar = 12;
            const bool vert = _wcsicmp(style.c_str(), L"Vertical") == 0 ||
                              _wcsicmp(style.c_str(), L"Both") == 0;
            const bool horz = _wcsicmp(style.c_str(), L"Horizontal") == 0 ||
                              _wcsicmp(style.c_str(), L"Both") == 0;
            if (rc.Width() < bar * 2 || rc.Height() < bar * 2) return;

            const COLORREF bg = RGB(66, 70, 78);
            const COLORREF fg = RGB(96, 102, 112);
            if (vert)
            {
                CRect v(rc.right - bar, rc.top, rc.right, rc.bottom - (horz ? bar : 0));
                cv.fillRect(v, bg);
                cv.fillRect(CRect(v.left + 2, v.top + 6, v.right - 2, v.top + 6 + (v.Height() - 40) / 2), fg);
            }
            if (horz)
            {
                CRect h(rc.left, rc.bottom - bar, rc.right - (vert ? bar : 0), rc.bottom);
                cv.fillRect(h, bg);
                cv.fillRect(CRect(h.left + 6, h.top + 2, h.left + 6 + (h.Width() - 40) / 2, h.bottom - 2), fg);
            }
        }

        // 只取对齐位：DT_VCENTER/SINGLELINE/END_ELLIPSIS 由 canvas 自己处理
        static UINT alignOnly(UINT f) { return f & (DT_LEFT | DT_CENTER | DT_RIGHT); }

        static void drawTextCv(XmlCanvas& cv, const CRect& rc, const std::wstring& s, COLORREF c,
                               UINT flags, int pt, const std::wstring& family)
        {
            if (s.empty()) return;
            cv.text(rc, s, c, alignOnly(flags), pt, family);
        }

        // 从预览矩形绘制单个节点（isRoot 用于根窗口背景）
        // 该节点是否由控件模板实体化出视觉层（此时控件本体不再自画文字）
        static bool hasTemplateChild(const XmlElement* node)
        {
            for (const auto& c : node->children)
                if (c.templateChild)
                    return true;
            return false;
        }

        // tag 判定一律大小写不敏感：解析器保留 XML 里的原始大小写，
        // 而 IsContainer 内部按小写比较，两边混用会静默漏掉分支。
        static bool tagIs(const XmlElement* node, const wchar_t* t)
        {
            if (node == nullptr || node->tag.c_str() == nullptr)
                return false;
            // 统一命名：<XButton> 与旧写法 <Button> 同一判定，去掉 X 前缀再比
            const std::wstring bare = xmlmfc::BareTag(node->tag);
            return _wcsicmp(bare.c_str(), t) == 0;
        }

        // 图标路径与设计器表面一致：Icon="res/xxx.png" 相对 exe 目录
        static std::wstring exeDirForCanvas()
        {
            wchar_t p[MAX_PATH] = { 0 };
            ::GetModuleFileNameW(nullptr, p, MAX_PATH);
            std::wstring s(p);
            const size_t k = s.find_last_of(L"\\/");
            return (k == std::wstring::npos) ? std::wstring() : s.substr(0, k);
        }

        // 未声明 Foreground 时的文字色：按所在背景明暗取可读色（白页上不能继续用浅色字）
        COLORREF textOn(COLORREF bg) const
        {
            const int lum = (GetRValue(bg) * 299 + GetGValue(bg) * 587 + GetBValue(bg) * 114) / 1000;
            return lum > 140 ? RGB(32, 36, 42) : RGB(228, 231, 236);
        }

        // 画布上控件的外观取"运行时原生控件"的浅色，不再用设计器外壳的深色：
        // 之前画布是深色卡片、预览器是浅色原生控件，同一个文档两边看起来不像一个东西，
        // 调色板统一放在 uidesign::cvdoc（文件头部），这里直接用。


// 画布里的图标写法与预览器一致：相对 exe 目录
inline std::wstring canvasExeDir()
{
    wchar_t path[MAX_PATH] = {};
    ::GetModuleFileNameW(nullptr, path, MAX_PATH);
    if (wchar_t* slash = wcsrchr(path, L'\\')) *(slash + 1) = 0;
    return std::wstring(path);
}

void drawNode(XmlCanvas& cv, XmlElement* node, CPoint origin, bool isRoot = false,
                      COLORREF inheritBg = RGB(255, 255, 255), const RECT* vp = nullptr)
        {
            // 统一坐标系：屏幕/客户坐标 = 文档坐标 + 画布原点(origin)
            CRect rc(node->previewX + origin.x, node->previewY + origin.y,
                     node->previewX + origin.x + node->previewW,
                     node->previewY + origin.y + node->previewH);
            // 视口裁剪：整棵子树都在可见矩形外时直接跳过（根窗是锚点，不裁）
            if (!isRoot && vp != nullptr &&
                (rc.right < vp->left || rc.bottom < vp->top ||
                 rc.left > vp->right || rc.top > vp->bottom))
                return;
            // 通用外观属性（catalog 暴露后，画布预览直接生效）
            COLORREF bgC = parseColor(node->get(L"Background"), (COLORREF)-1);
            if (bgC == (COLORREF)-1)
                bgC = parseColor(node->get(L"BackColor"), (COLORREF)-1);
            COLORREF fgC = parseColor(node->get(L"Foreground"), (COLORREF)-1);
            COLORREF bdC = parseColor(node->get(L"BorderColor"), (COLORREF)-1);
            int fontSize = _wtoi(node->get(L"FontSize").c_str());
            // XML 枚举属性：文字对齐 / 边框样式 / 滚动样式（未声明则用控件默认外观）
            const std::wstring bdStyle = node->get(L"BorderStyle");
            const std::wstring scrollStyle = node->get(L"ScrollStyle");
            const int bdThick = _wtoi(node->get(L"BorderThickness").c_str());
            UINT txtFlags = DT_LEFT;
            {
                const std::wstring a = node->get(L"TextAlign");
                if (_wcsicmp(a.c_str(), L"Center") == 0) txtFlags = DT_CENTER;
                else if (_wcsicmp(a.c_str(), L"Right") == 0) txtFlags = DT_RIGHT;
            }
            const int pt = fontSize > 0 ? fontSize : 0;
            const std::wstring fam = fontSize > 0 ? std::wstring(L"Microsoft YaHei") : std::wstring();

            if (isRoot)
            {
                rc = CRect(origin.x, origin.y, origin.x + node->w, origin.y + node->h);
                // 根窗口预览用白底：画布工作台是灰底，窗体必须保持白色，才能一眼看出"窗体内 / 窗体外的空白"。
                cv.fillRect(rc, bgC == (COLORREF)-1 ? RGB(255, 255, 255) : bgC);
                // 根窗体外框只在 XML 声明了 BorderColor 时才画：
                // 设计器自己的外壳是无边框窗口，画布再给它描一圈灰线就"多了一层外框"。
                if (bdC != (COLORREF)-1)
                    cv.strokeRect(rc, bdC, 1);
            }
            else if (tagIs(node, L"MenuBar"))
            {
                xmlctl::xmlDrawShellMenuBar(cv, rc, *node);
                return;
            }
            else if (tagIs(node, L"StatusBar"))
            {
                xmlctl::xmlDrawShellStatusBar(cv, rc, *node);
                return;
            }
            else if (tagIs(node, L"DrawBar"))
            {
                xmlctl::xmlDrawShellDrawBar(cv, rc, *node);
                return;
            }
            else if (tagIs(node, L"ChipBar"))
            {
                xmlctl::xmlDrawShellChipBar(cv, rc, *node);
                return;
            }
            else if (tagIs(node, L"TabStrip"))
            {
                xmlctl::xmlDrawShellTabStrip(cv, rc, *node);
                return;
            }
            else if (tagIs(node, L"Stage"))
            {
                xmlctl::xmlDrawShellStage(cv, rc, *node);
                return;
            }
            else if (tagIs(node, L"PanelHeader"))
            {
                xmlctl::xmlDrawShellPanelHeader(cv, rc, *node);
                return;
            }
            else if (tagIs(node, L"TreeView"))
            {
                // 预设行：XML 里写 Level/HasChildren/Expanded/Selected 即可在画布上看出行结构
                xmlapp::XmlAppearance ap;
                ap.backColor = node->get(L"Background");
                if (ap.backColor.empty()) ap.backColor = node->get(L"BackColor");
                ap.foreColor = node->get(L"Foreground");
                ap.borderColor = node->get(L"BorderColor");
                ap.borderStyle = node->get(L"BorderStyle");
                ap.fontSize = _wtoi(node->get(L"FontSize").c_str());
                const int lum = GetRValue(bgC == (COLORREF)-1 ? RGB(255, 255, 255) : bgC);
                xmlctl::xmlDrawTreeLike(cv, rc, *node, node->styled, ap,
                                          lum > 140 ? RGB(32, 36, 42) : RGB(228, 231, 236),
                                          RGB(120, 130, 150));
                return;
            }
            else if (tagIs(node, L"PropertyGrid"))
            {
                // 预设行：子节点的 Name/Value 即"属性—值"两列
                xmlapp::XmlAppearance ap;
                ap.backColor = node->get(L"Background");
                if (ap.backColor.empty()) ap.backColor = node->get(L"BackColor");
                ap.foreColor = node->get(L"Foreground");
                ap.borderColor = node->get(L"BorderColor");
                ap.borderStyle = node->get(L"BorderStyle");
                ap.fontSize = _wtoi(node->get(L"FontSize").c_str());
                const int lum2 = GetRValue(bgC == (COLORREF)-1 ? RGB(255, 255, 255) : bgC);
                xmlctl::xmlDrawPropRows(cv, rc, *node, node->styled, ap,
                                          lum2 > 140 ? RGB(32, 36, 42) : RGB(228, 231, 236),
                                          RGB(120, 130, 150));
                return;
            }
            else if (tagIs(node, L"ListBox") || tagIs(node, L"ListView"))
            {
                xmlapp::XmlAppearance ap;
                ap.backColor = node->get(L"Background");
                if (ap.backColor.empty()) ap.backColor = node->get(L"BackColor");
                ap.foreColor = node->get(L"Foreground");
                ap.borderColor = node->get(L"BorderColor");
                ap.borderStyle = node->get(L"BorderStyle");
                ap.fontSize = _wtoi(node->get(L"FontSize").c_str());
                const int inkLum = GetRValue(bgC == (COLORREF)-1 ? RGB(255, 255, 255) : bgC);
                xmlctl::xmlDrawListLike(cv, rc, *node, node->styled, ap,
                                          inkLum > 140 ? RGB(32, 36, 42) : RGB(228, 231, 236),
                                          RGB(120, 130, 150));
                return;
            }
            else if (tagIs(node, L"SplitView"))
            {
                const bool vertical = node->previewW <= node->previewH;
                xmlctl::xmlDrawSplitter(cv, rc, vertical, false, false,
                                          ::GetSysColor(COLOR_BTNFACE));
            }
            else if (tagIs(node, L"StatusBar"))
            {
                xmlctl::xmlDrawShellStatusBar(cv, rc, *node);
                return;
            }
            else if (false)
            {
                std::vector<xmlctl::StatusCell> cells;
                for (auto& c : node->children)
                {
                    xmlctl::StatusCell cell;
                    cell.text = c.get(L"Text");
                    if (cell.text.empty()) cell.text = c.get(L"Content");
                    if (cell.text.empty()) cell.text = c.text;
                    cell.width = c.previewW;
                    cells.push_back(cell);
                }
                xmlctl::xmlDrawStatusBar(cv, rc, cells, ::GetSysColor(COLOR_BTNFACE),
                                           RGB(32, 36, 42), 8, pt, fam);
            }
            else if (tagIs(node, L"PanelHeader"))
            {
                // 外壳构件：与设计器表面 / 预览器同一批绘制函数，画布预览才对得上
                xmlctl::xmlDrawPanelHeader(cv, rc, node->text, node->get(L"Hint"), 8);
            }
            else if (tagIs(node, L"TabStrip"))
            {
                xmlctl::xmlDrawShellTabStrip(cv, rc, *node);
                return;
            }
            else if (tagIs(node, L"DrawBar"))
            {
                xmlctl::xmlDrawShellDrawBar(cv, rc, *node);
                return;
            }
            else if (tagIs(node, L"Tab"))
            {
                const std::wstring act = node->get(L"Active");
                xmlctl::xmlDrawTab(cv, rc, node->text,
                                     !act.empty() && _wcsicmp(act.c_str(), L"True") == 0, false, 8);
            }
            else if (tagIs(node, L"Chip"))
            {
                const std::wstring chk = node->get(L"Checked");
                xmlctl::xmlDrawChip(cv, rc, node->text,
                                      !chk.empty() && _wcsicmp(chk.c_str(), L"True") == 0, false, 8);
            }
            else if (tagIs(node, L"Stage"))
            {
                xmlctl::xmlDrawShellStage(cv, rc, *node);
                return;
            }
            else if (tagIs(node, L"ChipBar"))
            {
                xmlctl::xmlDrawShellChipBar(cv, rc, *node);
                return;
            }
            else if (IsContainer(node->tag))
            {
                // 可见容器（TabControl/Border/Panel/ToolBar 等）画占位；纯布局容器保持透明，
                // 除非它在 XML 里显式声明了 Background / BorderColor（声明了就按声明渲染）
                const bool isPalettePage = tagIs(node, L"Page") || tagIs(node, L"page") ||
                                           tagIs(node, L"TabItem");
                const bool layoutOnly = isPalettePage ||
                                        (tagIs(node, L"Grid") && false) ||
                                        tagIs(node, L"StackPanel") ||
                                        tagIs(node, L"Canvas") ||
                                        tagIs(node, L"WrapPanel") ||
                                        tagIs(node, L"DockPanel") ||
                                        tagIs(node, L"GridPanel");
                if (!layoutOnly)
                {
                    drawRoundedCv(cv, rc,
                                  bgC == (COLORREF)-1 ? cvdoc::kSurface : bgC,
                                  bdC == (COLORREF)-1 ? cvdoc::kBorder : bdC, 4);
                    // 容器名用小字标在左上角，不压住子控件（模板视觉层不标名字）
                    if (!node->templateChild)
                    {
                        CRect lr = rc;
                        lr.left += 6;
                        lr.bottom = lr.top + 16;
                        drawTextCv(cv, lr, node->tag, cvdoc::kMuted,
                                   DT_LEFT | DT_VCENTER | DT_SINGLELINE, 0, std::wstring());
                    }
                }
                else
                {
                    // 布局容器：声明的背景/边框生效，未声明保持透明（不遮挡子控件）
                    if (bgC != (COLORREF)-1)
                        cv.fillRect(rc, bgC);
                    if (bdC != (COLORREF)-1)
                        cv.strokeRect(rc, bdC, 1);
                    // 侧栏页：顶部画一条页签，标出这页是哪个 tab（运行时由 CAdUiPaletteSet 呈现页签）
                    if (isPalettePage)
                    {
                        const bool hasBg = bgC != (COLORREF)-1;
                        const bool hasBd = bdC != (COLORREF)-1;
                        if (!hasBg && !hasBd)
                            cv.fillRect(rc, RGB(255, 255, 255));
                        CRect tab = rc;
                        tab.bottom = tab.top + 24;
                        cv.fillRect(tab, RGB(233, 238, 247));
                        tab.left += 8;
                        const std::wstring title = std::wstring(L"page \u201C") +
                                                   node->get(L"Header") + std::wstring(L"\u201D");
                        drawTextCv(cv, tab, title, RGB(64, 88, 128),
                                   DT_LEFT | DT_VCENTER | DT_SINGLELINE, 0, std::wstring());
                    }
                }
                if (tagIs(node, L"GroupBox"))
                {
                    drawRoundedCv(cv, rc, cvdoc::kSurface, cvdoc::kBorder, 4);
                    CRect title = rc;
                    title.bottom = title.top + 22;
                    title.left += 8;
                    drawTextCv(cv, title, node->text, cvdoc::kInk,
                               DT_LEFT | DT_VCENTER | DT_SINGLELINE, 0, std::wstring());
                }
            }
            // ---- 统一渲染器 ----
            // 按钮 / 文字 / 输入框 / 勾选 / 下拉 / 进度条：直接调 MfcXmlCore/XmlLeafPaint.h，
            // 预览器调的是同一个函数，所以画布与预览的圆角、边框、配色不会再分叉。
            else if (tagIs(node, L"Image") ||
                     tagIs(node, L"Button") || tagIs(node, L"Label") || tagIs(node, L"TextField") ||
                     tagIs(node, L"CheckBox") || tagIs(node, L"RadioButton") ||
                     tagIs(node, L"ComboBox") || tagIs(node, L"ProgressBar") ||
                     tagIs(node, L"Slider") || tagIs(node, L"RangeSlider") ||
                     tagIs(node, L"Switch") || tagIs(node, L"SpinBox") ||
                     tagIs(node, L"DatePicker") || tagIs(node, L"Dial") ||
                     tagIs(node, L"Tumbler") || tagIs(node, L"ScrollBar") ||
                     tagIs(node, L"BusyIndicator") ||
                     tagIs(node, L"Rectangle") || tagIs(node, L"Ellipse") || tagIs(node, L"Line") ||
                     tagIs(node, L"Divider") || tagIs(node, L"Hyperlink") || tagIs(node, L"Choice"))
            {
                xmlapp::XmlAppearance ap;
                ap.backColor = node->get(L"Background");
                if (ap.backColor.empty()) ap.backColor = node->get(L"BackColor");
                ap.foreColor = node->get(L"Foreground");
                ap.borderColor = node->get(L"BorderColor");
                ap.borderStyle = bdStyle;
                ap.fontSize = fontSize;
                {
                    const std::wstring op = node->get(L"Opacity");
                    ap.opacity = op.empty() ? 255 : _wtoi(op.c_str());
                }
                xmlleaf::drawLeaf(cv, node, rc, ap, node->styled, inheritBg, canvasExeDir());
            }

            else if (tagIs(node, L"Button"))
            {
                // 外壳按钮：声明了 Role（胶囊）或"图标 + 标题"时按设计器外壳的样子画，
                // 否则仍是画布通用按钮预览
                const std::wstring role = node->get(L"Role");
                const std::wstring icon = node->get(L"Icon");
                const bool pill = _wcsicmp(role.c_str(), L"Primary") == 0 ||
                                  _wcsicmp(role.c_str(), L"Ghost") == 0;
                const bool caption = !pill && !icon.empty() && !node->text.empty() &&
                                     node->previewH >= 36;
                if (pill)
                {
                    xmlctl::xmlDrawPillButton(cv, rc, node->text,
                                                _wcsicmp(role.c_str(), L"Primary") == 0, false, 8);
                }
                else if (caption)
                {
                    xmlctl::xmlDrawToolButtonCaption(cv, rc, exeDirForCanvas() + L"\\" + icon,
                                                       node->text, false, false, false,
                                                       xmlctl::ToolButtonStyle(), 8);
                }
                else
                {
                    const COLORREF btnBg = bgC == (COLORREF)-1 ? cvdoc::kButton : bgC;
                    drawRoundedCv(cv, rc, btnBg,
                                  bdC == (COLORREF)-1 ? cvdoc::kBorder : bdC);
                    if (!hasTemplateChild(node))
                    {
                        const UINT flags = node->has(L"TextAlign") ? txtFlags : DT_CENTER;
                        drawTextCv(cv, rc, node->text, fgC == (COLORREF)-1 ? textOn(btnBg) : fgC,
                                   flags | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS, pt, fam);
                    }
                }
            }
            else if (tagIs(node, L"Label") || tagIs(node, L"Label"))
            {
                CRect textRect = rc;
                if (txtFlags == DT_LEFT)
                    textRect.left += 4;
                drawTextCv(cv, textRect, node->text,
                           fgC == (COLORREF)-1 ? textOn(bgC == (COLORREF)-1 ? inheritBg : bgC) : fgC,
                           txtFlags | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS, pt, fam);
            }
            else if (tagIs(node, L"TextField"))
            {
                const COLORREF boxBg = bgC == (COLORREF)-1 ? cvdoc::kField : bgC;
                drawRoundedCv(cv, rc, boxBg, bdC == (COLORREF)-1 ? cvdoc::kBorder : bdC, 4);
                CRect textRect = rc; textRect.DeflateRect(8, 0);
                const std::wstring shown = (node == m_liveNode) ? m_liveBuf : node->text;
                drawTextCv(cv, textRect, shown, fgC == (COLORREF)-1 ? textOn(boxBg) : fgC,
                           txtFlags | DT_VCENTER | DT_SINGLELINE, pt, fam);

                // 就地编辑：在光标位置画一条竖线（量宽走 GDI，与 DWrite 同族误差 1px 内）
                if (node == m_liveNode)
                {
                    int caret = m_liveCaret;
                    if (caret < 0) caret = 0;
                    if (caret > (int)shown.size()) caret = (int)shown.size();
                    int cx = 0;
                    HDC hdc = ::GetDC(nullptr);
                    HFONT lf = ::CreateFontW(-13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                             OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                             DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI");
                    HGDIOBJ of = ::SelectObject(hdc, lf);
                    SIZE sz = { 0, 0 };
                    if (caret > 0)
                        ::GetTextExtentPoint32W(hdc, shown.substr(0, caret).c_str(), caret, &sz);
                    ::SelectObject(hdc, of);
                    ::DeleteObject(lf);
                    ::ReleaseDC(nullptr, hdc);
                    cx = sz.cx;
                    cv.fillRect(CRect(textRect.left + cx, textRect.top + 5,
                                      textRect.left + cx + 1, textRect.bottom - 5), RGB(120, 190, 255));
                }
            }
            else if (tagIs(node, L"CheckBox") || tagIs(node, L"RadioButton"))
            {
                CRect box(rc.left, rc.top + (rc.Height() - 14) / 2,
                          rc.left + 14, rc.top + (rc.Height() + 14) / 2);
                cv.fillRect(box, cvdoc::kField);
                cv.strokeRect(box, cvdoc::kBorder, 1);

                // 勾选态：CheckBox 画勾，RadioButton 画实心圆点
                const std::wstring ck = node->get(L"Checked");
                const bool on = (_wcsicmp(ck.c_str(), L"true") == 0 || ck == L"1");
                if (on)
                {
                    const COLORREF mark = cvdoc::kAccent;
                    if (tagIs(node, L"CheckBox"))
                    {
                        cv.strokeLine(POINT{ box.left + 3, box.top + 7 }, POINT{ box.left + 6, box.bottom - 4 }, mark, 2);
                        cv.strokeLine(POINT{ box.left + 6, box.bottom - 4 }, POINT{ box.right - 3, box.top + 4 }, mark, 2);
                    }
                    else
                    {
                        cv.fillRect(CRect(box.left + 4, box.top + 4, box.right - 4, box.bottom - 4), mark);
                    }
                }

                CRect tr = rc; tr.left += 20;
                drawTextCv(cv, tr, node->text,
                           fgC == (COLORREF)-1 ? textOn(bgC == (COLORREF)-1 ? inheritBg : bgC) : fgC,
                           DT_LEFT | DT_VCENTER | DT_SINGLELINE, pt, fam);
            }
            else if (tagIs(node, L"ComboBox"))
            {
                drawRoundedCv(cv, rc, cvdoc::kField, cvdoc::kBorder, 3);
                cv.fillRect(CRect(rc.right - 18, rc.top, rc.right, rc.bottom), cvdoc::kTrack);
                CRect tr = rc; tr.DeflateRect(6, 0); tr.right -= 18;
                drawTextCv(cv, tr, node->text, cvdoc::kInk,
                           DT_LEFT | DT_VCENTER | DT_SINGLELINE, pt, fam);
            }
            else if (tagIs(node, L"ListBox"))
            {
                drawRoundedCv(cv, rc, cvdoc::kField, cvdoc::kBorder, 3);
                for (int r = 0; r < 3; ++r)
                {
                    CRect row = rc; row.DeflateRect(6, 4); row.bottom = row.top + 16;
                    row.OffsetRect(0, r * 18);
                    drawTextCv(cv, row, (r == 0) ? node->text : std::wstring(),
                               (r == 0) ? cvdoc::kInk : cvdoc::kMuted,
                               DT_LEFT | DT_VCENTER | DT_SINGLELINE, 0, std::wstring());
                }
            }
            else if (tagIs(node, L"ListCtrl") || tagIs(node, L"GridLayout") ||
                     tagIs(node, L"DataGrid") || tagIs(node, L"TableView"))
            {
                // 表格类控件：把 XML 里的 Columns / Rows 真画出来。
                // 之前这里只是"空壳 + 控件名"，表头和数据一行都看不到，等于没法做表格界面。
                // 语法与引擎一致：Columns="名称:180|状态:100"，Rows="甲\t1|乙\t2"
                const std::wstring colsAttr = node->get(L"Columns");
                const std::wstring rowsAttr = node->get(L"Rows");
                const std::wstring sortAttr = node->get(L"Sort");

                const COLORREF tbg = bgC == (COLORREF)-1 ? cvdoc::kField : bgC;
                const int lum = (GetRValue(tbg) * 30 + GetGValue(tbg) * 59 + GetBValue(tbg) * 11) / 100;
                const bool dark = lum < 128;
                const COLORREF tink = fgC == (COLORREF)-1 ? (dark ? RGB(226, 231, 238) : cvdoc::kInk) : fgC;
                const COLORREF headBg = dark ? RGB(52, 58, 70) : cvdoc::kHead;
                const COLORREF gridC = dark ? RGB(72, 79, 92) : cvdoc::kGrid;
                drawRoundedCv(cv, rc, tbg, bdC == (COLORREF)-1 ? cvdoc::kBorder : bdC, 3);

                auto splitBy = [](const std::wstring& s, wchar_t sep) {
                    std::vector<std::wstring> out;
                    size_t start = 0;
                    while (true)
                    {
                        const size_t i = s.find(sep, start);
                        const std::wstring seg = (i == std::wstring::npos) ? s.substr(start) : s.substr(start, i - start);
                        if (!seg.empty()) out.push_back(seg);
                        if (i == std::wstring::npos) break;
                        start = i + 1;
                    }
                    return out;
                };

                struct Col { std::wstring title; int w; };
                std::vector<Col> cols;
                for (auto& seg : splitBy(colsAttr, L'|'))
                {
                    Col c;
                    c.title = seg;
                    c.w = 120;
                    const size_t colon = seg.rfind(L':');
                    if (colon != std::wstring::npos)
                    {
                        const std::wstring wt = seg.substr(colon + 1);
                        if (!wt.empty() && wt.find_first_not_of(L"0123456789") == std::wstring::npos)
                        {
                            c.w = (std::max)(20, _wtoi(wt.c_str()));
                            c.title = seg.substr(0, colon);
                        }
                    }
                    cols.push_back(c);
                }

                if (cols.empty())
                {
                    // 没声明列就仍是"空表格"，但标出控件类型，避免看起来像坏掉的空框
                    drawRoundedCv(cv, rc, cvdoc::kSurface, cvdoc::kBorder, 4);
                    drawTextCv(cv, rc, std::wstring(L"[") + node->tag + std::wstring(L"]"),
                               cvdoc::kMuted, DT_CENTER | DT_VCENTER | DT_SINGLELINE, 0, std::wstring());
                }
                else
                {
                    // 多列排序：Sort="数量:desc,状态:asc"
                    //   键可以是列标题（不区分大小写）或 0 基列序号；方向缺省 asc，可写 asc/desc；
                    //   多个键按书写顺序逐级比较（前者优先级高），比较是稳定排序。
                    //   两边都是数字时按数值比（12 > 3），否则按字符串比。
                    struct SortKey { int col; bool desc; };
                    std::vector<SortKey> sortKeys;
                    for (auto& seg : splitBy(sortAttr, L','))
                    {
                        std::wstring item = seg;
                        while (!item.empty() && (item.front() == L' ' || item.front() == L'\t')) item.erase(item.begin());
                        while (!item.empty() && (item.back() == L' ' || item.back() == L'\t')) item.pop_back();
                        if (item.empty()) continue;
                        bool desc = false;
                        const size_t colon = item.find(L':');
                        if (colon != std::wstring::npos)
                        {
                            std::wstring dir = item.substr(colon + 1);
                            for (auto& ch : dir) ch = (wchar_t)towlower(ch);
                            desc = (dir == L"desc" || dir == L"d" || dir == L"降序" || dir == L"-1");
                            item = item.substr(0, colon);
                        }
                        int colIdx = -1;
                        if (!item.empty() && item.find_first_not_of(L"0123456789") == std::wstring::npos)
                        {
                            colIdx = _wtoi(item.c_str());
                            if (colIdx < 0 || colIdx >= (int)cols.size()) colIdx = -1;
                        }
                        if (colIdx < 0)
                        {
                            for (size_t ci = 0; ci < cols.size(); ++ci)
                                if (_wcsicmp(cols[ci].title.c_str(), item.c_str()) == 0) { colIdx = (int)ci; break; }
                        }
                        if (colIdx >= 0) sortKeys.push_back({ colIdx, desc });
                    }

                    // 数据行 -> 单元格矩阵（行用 | 分隔，列用 \t 分隔）
                    std::vector<std::vector<std::wstring>> data;
                    for (auto& rtext : splitBy(rowsAttr, L'|'))
                        data.push_back(splitBy(rtext, L'\t'));

                    if (!sortKeys.empty() && data.size() > 1)
                    {
                        auto numOf = [](const std::wstring& s, double& out) -> bool {
                            if (s.empty()) return false;
                            wchar_t* end = nullptr;
                            out = wcstod(s.c_str(), &end);
                            return end != nullptr && *end == L'\0';
                        };
                        auto cellOf = [](const std::vector<std::wstring>& row, int ci) -> const std::wstring& {
                            static const std::wstring kEmpty;
                            return (ci >= 0 && ci < (int)row.size()) ? row[ci] : kEmpty;
                        };
                        std::stable_sort(data.begin(), data.end(),
                            [&](const std::vector<std::wstring>& A, const std::vector<std::wstring>& B) {
                                for (const auto& k : sortKeys)
                                {
                                    const std::wstring& a = cellOf(A, k.col);
                                    const std::wstring& b = cellOf(B, k.col);
                                    double na = 0, nb = 0;
                                    const bool an = numOf(a, na);
                                    const bool bn = numOf(b, nb);
                                    int cmp = 0;
                                    if (an && bn) cmp = (na < nb) ? -1 : ((na > nb) ? 1 : 0);
                                    else cmp = _wcsicmp(a.c_str(), b.c_str());
                                    if (cmp != 0) return k.desc ? (cmp > 0) : (cmp < 0);
                                }
                                return false;
                            });
                    }

                    const int padX = 4, padY = 4;
                    const int left = (int)rc.left + padX;
                    const int availW = (std::max)(1, (int)rc.Width() - padX * 2);
                    int total = 0;
                    for (auto& c : cols) total += c.w;
                    if (total > availW && total > 0)
                    {
                        // 列宽超宽时按比例压缩，不让最后一列跑出控件外
                        for (auto& c : cols) c.w = (std::max)(20, c.w * availW / total);
                        total = 0;
                        for (auto& c : cols) total += c.w;
                    }
                    // 末尾留白并给最后一列吃掉，表头底线与控件右边对齐
                    if (total < availW) cols.back().w += availW - total;

                    const int headH = (std::max)(20, pt > 0 ? pt + 12 : 22);
                    const int rowH = (std::max)(18, pt > 0 ? pt + 10 : 20);
                    const int top = (int)rc.top + padY;
                    const int bottom = (int)rc.bottom - padY;

                    cv.clipPush(rc);
                    // 表头
                    cv.fillRect(CRect(left, top, left + availW, top + headH), headBg);
                    int x = left;
                    for (size_t ci = 0; ci < cols.size(); ++ci)
                    {
                        auto& c = cols[ci];
                        CRect cell(x + 6, top, x + c.w - 4, top + headH);
                        cv.clipPush(cell);
                        drawTextCv(cv, cell, c.title, tink, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS, pt, fam);
                        cv.clipPop();
                        // 排序标记：▲升序 / ▼降序，多键时带优先级序号 1..N
                        for (size_t ki = 0; ki < sortKeys.size(); ++ki)
                        {
                            if (sortKeys[ki].col != (int)ci) continue;
                            std::wstring mark = sortKeys[ki].desc ? L"\u25BC" : L"\u25B2";
                            if (sortKeys.size() > 1)
                                mark += std::to_wstring(ki + 1);
                            CRect mr(x + c.w - 34, top, x + c.w - 4, top + headH);
                            drawTextCv(cv, mr, mark, tink, DT_RIGHT | DT_VCENTER | DT_SINGLELINE, 0, std::wstring());
                            break;
                        }
                        if (x > left)
                            cv.strokeLine(POINT{ x, top }, POINT{ x, bottom }, gridC, 1);
                        x += c.w;
                    }
                    cv.strokeLine(POINT{ left, top + headH }, POINT{ left + availW, top + headH }, gridC, 1);

                    // 数据行（已按 Sort 排好）
                    int y = top + headH;
                    for (auto& cells : data)
                    {
                        if (y + rowH > bottom) break;
                        x = left;
                        for (size_t ci = 0; ci < cols.size(); ++ci)
                        {
                            CRect cell(x + 6, y, x + cols[ci].w - 4, y + rowH);
                            cv.clipPush(cell);
                            const std::wstring val = ci < cells.size() ? cells[ci] : std::wstring();
                            if (!val.empty())
                                drawTextCv(cv, cell, val, tink, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS, pt, fam);
                            cv.clipPop();
                            x += cols[ci].w;
                        }
                        y += rowH;
                        cv.strokeLine(POINT{ left, y }, POINT{ left + availW, y }, gridC, 1);
                    }
                    cv.clipPop();
                }
            }
            else if (tagIs(node, L"Rectangle") || tagIs(node, L"Ellipse") || tagIs(node, L"Line") ||
                     tagIs(node, L"Arc"))
            {
                // 绘图图元：Background 填面、BorderColor/Foreground 描边、BorderThickness 线宽
                const COLORREF fill = bgC != (COLORREF)-1 ? bgC : cvdoc::kSurface;
                const COLORREF stroke = fgC != (COLORREF)-1 ? fgC
                                       : (bdC != (COLORREF)-1 ? bdC : cvdoc::kBorder);
                const int thick = bdThick > 0 ? bdThick : 1;
                if (tagIs(node, L"Arc"))
                {
                    // 圆弧：StartAngle/SweepAngle 决定弧度，包围盒决定长短轴（可椭）
                    int sa = 180, sw = 180;
                    arcParams(node, sa, sw);
                    cv.strokePath(arcPoints(rc, sa, sw, 64), stroke, thick, false);
                }
                else if (tagIs(node, L"Ellipse"))
                {
                    cv.fillEllipse(rc, fill, stroke);
                }
                else if (tagIs(node, L"Line"))
                {
                    // Direction 决定这根线走哪条对角线：Down=左上→右下（默认），Up=左下→右上，
                    // 另加 H/V 便于画纯水平、纯垂直的线
                    const std::wstring dir = node->get(L"Direction");
                    POINT a{ rc.left, rc.top }, b{ rc.right, rc.bottom };
                    if (_wcsicmp(dir.c_str(), L"Up") == 0) { a.y = rc.bottom; b.y = rc.top; }
                    else if (_wcsicmp(dir.c_str(), L"H") == 0 ||
                             _wcsicmp(dir.c_str(), L"Horizontal") == 0)
                    {
                        a.y = b.y = rc.top + rc.Height() / 2;
                    }
                    else if (_wcsicmp(dir.c_str(), L"V") == 0 ||
                             _wcsicmp(dir.c_str(), L"Vertical") == 0)
                    {
                        a.x = b.x = rc.left + rc.Width() / 2;
                    }
                    cv.strokeLine(a, b, stroke, thick);
                }
                else
                {
                    const int radius = _wtoi(node->get(L"Radius").c_str());
                    if (radius > 0) cv.fillRoundRect(rc, fill, stroke, radius);
                    else
                    {
                        cv.fillRect(rc, fill);
                        cv.strokeRect(rc, stroke, thick);
                    }
                }
            }
            else if (tagIs(node, L"ProgressBar"))
            {
                cv.fillRect(rc, cvdoc::kTrack);
                CRect fill = rc; fill.right = fill.left + rc.Width() / 2;
                cv.fillRect(fill, cvdoc::kAccent);
                cv.strokeRect(rc, cvdoc::kBorder, 1);
            }
            else if (tagIs(node, L"Image") || tagIs(node, L"ListView") ||
                     tagIs(node, L"TabBar") || tagIs(node, L"Border"))
            {
                drawRoundedCv(cv, rc, cvdoc::kSurface, cvdoc::kBorder, 4);
                drawTextCv(cv, rc, std::wstring(L"[") + node->tag + std::wstring(L"]"),
                           cvdoc::kMuted, DT_CENTER | DT_VCENTER | DT_SINGLELINE, 0, std::wstring());
            }
            else if (tagIs(node, L"TreeView") || tagIs(node, L"TreeTable"))
            {
                drawRoundedCv(cv, rc, cvdoc::kField, cvdoc::kBorder, 3);
                CRect body = rc;
                body.DeflateRect(1, 1);
                int y = body.top + 4;

                // TreeTable 有 Columns 时先画表头
                const std::wstring cols = node->get(L"Columns");
                if (!cols.empty())
                {
                    CRect head = body;
                    head.bottom = head.top + 24;
                    cv.fillRect(head, cvdoc::kHead);
                    cv.strokeLine(POINT{ head.left, head.bottom - 1 },
                                  POINT{ head.right, head.bottom - 1 }, cvdoc::kGrid, 1);
                    int x = head.left + 6;
                    for (auto& c : cvdoc::splitBar(cols))
                    {
                        std::wstring title = c;
                        const size_t colon = c.find(L':');
                        int w = 80;
                        if (colon != std::wstring::npos)
                        {
                            title = c.substr(0, colon);
                            const std::wstring wt = c.substr(colon + 1);
                            if (!wt.empty() && wt.find_first_not_of(L"0123456789") == std::wstring::npos)
                                w = (std::max)(24, _wtoi(wt.c_str()));
                        }
                        CRect cell(x, head.top, (std::min)(x + w, static_cast<int>(head.right)), head.bottom);
                        CRect tr = cell; tr.DeflateRect(4, 0);
                        drawTextCv(cv, tr, title, cvdoc::kInk,
                                   DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS, pt, fam);
                        x += w;
                    }
                    y = head.bottom + 2;
                }

                std::vector<std::pair<int, std::wstring>> rows;
                cvdoc::flattenNodes(node->get(L"Nodes"), rows);
                const int rowH = 20;
                cv.clipPush(body);
                for (auto& r : rows)
                {
                    if (y + rowH > body.bottom) break;
                    CRect row(body.left, y, body.right, y + rowH);
                    const int ind = 8 + r.first * 14;
                    CRect mark(row.left + ind - 10, row.top + 4, row.left + ind - 4, row.top + 10);
                    // 折叠箭头（与资源管理器一致），不是叉
                    const int midY2 = (mark.top + mark.bottom) / 2;
                    cv.strokeLine(POINT{ mark.left, mark.top }, POINT{ mark.left + 3, midY2 }, cvdoc::kMuted, 1);
                    cv.strokeLine(POINT{ mark.left + 3, midY2 }, POINT{ mark.left, mark.bottom }, cvdoc::kMuted, 1);
                    CRect tr = row; tr.left += ind;
                    drawTextCv(cv, tr, r.second, cvdoc::kInk,
                               DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS, pt, fam);
                    y += rowH;
                }
                cv.clipPop();
            }
            else if (tagIs(node, L"TabBar"))
            {
                // 页签条 + 内容区，与运行时原生 Tab 一致
                const std::vector<std::wstring> pages = cvdoc::splitBar(node->get(L"Pages"));
                CRect strip = rc;
                strip.bottom = strip.top + 26;
                cv.fillRect(rc, cvdoc::kField);
                int x = strip.left;
                const wchar_t* names[] = { L"常规", L"高级", L"关于" };
                for (size_t i = 0; i < (pages.empty() ? 3 : pages.size()); ++i)
                {
                    const std::wstring title = pages.empty() ? std::wstring(names[i]) : pages[i];
                    const int w = (std::min)(120, (int)(title.size() * 9 + 26));
                    CRect tab(x, strip.top, x + w, strip.bottom);
                    const bool active = (i == 0);
                    cv.fillRect(tab, active ? cvdoc::kField : cvdoc::kHead);
                    cv.strokeLine(POINT{ tab.left, tab.bottom - 1 }, POINT{ tab.right, tab.bottom - 1 },
                                  cvdoc::kBorder, 1);
                    drawTextCv(cv, tab, title, cvdoc::kInk, DT_CENTER | DT_VCENTER | DT_SINGLELINE, pt, fam);
                    x += w + 2;
                }
                CRect frame(rc.left, strip.bottom - 1, rc.right, rc.bottom);
                cv.strokeRect(frame, cvdoc::kBorder, 1);
                if (!pages.empty())
                    drawTextCv(cv, frame, pages[0], cvdoc::kMuted,
                               DT_CENTER | DT_VCENTER | DT_SINGLELINE, pt, fam);
            }
            else if (tagIs(node, L"StackView") || tagIs(node, L"SwipeView") || tagIs(node, L"Page"))
            {
                cv.fillRect(rc, cvdoc::kField);
                cv.strokeRect(rc, cvdoc::kBorder, 1);
                const int count = (std::max)(1, _wtoi(node->get(L"PageCount").c_str()));
                const int sel = (std::max)(0, (std::min)(count - 1, _wtoi(node->get(L"SelectedIndex").c_str())));
                const int d = 7;
                int x = rc.CenterPoint().x - (count * (d + 6) - 6) / 2;
                for (int i = 0; i < count; ++i)
                {
                    CRect dot(x, rc.bottom - 16, x + d, rc.bottom - 16 + d);
                    cv.fillRoundRect(dot, i == sel ? cvdoc::kAccent : cvdoc::kTrack,
                                     i == sel ? cvdoc::kAccent : cvdoc::kBorder, d / 2);
                    x += d + 6;
                }
            }
            else if (tagIs(node, L"PageIndicator"))
            {
                const int count = (std::max)(1, _wtoi(node->get(L"Count").c_str()));
                const int sel = (std::max)(0, (std::min)(count - 1, _wtoi(node->get(L"SelectedIndex").c_str())));
                const int d = 8;
                int x = rc.CenterPoint().x - (count * (d + 6) - 6) / 2;
                for (int i = 0; i < count; ++i)
                {
                    CRect dot(x, rc.CenterPoint().y - d / 2, x + d, rc.CenterPoint().y + d / 2);
                    cv.fillRoundRect(dot, i == sel ? cvdoc::kAccent : cvdoc::kTrack,
                                     i == sel ? cvdoc::kAccent : cvdoc::kBorder, d / 2);
                    x += d + 6;
                }
            }
            else if (tagIs(node, L"VirtualList"))
            {
                drawRoundedCv(cv, rc, cvdoc::kField, cvdoc::kBorder, 3);
                CRect body = rc; body.DeflateRect(1, 1);
                const int rowH = (std::max)(14, _wtoi(node->get(L"ItemHeight").c_str()));
                const int sel = _wtoi(node->get(L"SelectedIndex").c_str());
                std::vector<std::wstring> items = cvdoc::splitBar(node->get(L"Items"));
                int y = body.top + 1;
                cv.clipPush(body);
                for (size_t i = 0; i < items.size() && y + rowH <= body.bottom; ++i)
                {
                    CRect row(body.left, y, body.right, y + rowH);
                    if ((int)i == sel) cv.fillRect(row, RGB(219, 233, 249));
                    CRect tr = row; tr.left += 8;
                    drawTextCv(cv, tr, items[i], cvdoc::kInk,
                               DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS, pt, fam);
                    y += rowH;
                }
                cv.clipPop();
            }
            else if (tagIs(node, L"GridView"))
            {
                cv.fillRect(rc, cvdoc::kField);
                cv.strokeRect(rc, cvdoc::kBorder, 1);
                const int cw = (std::max)(24, _wtoi(node->get(L"CellWidth").c_str()));
                const int chh = (std::max)(20, _wtoi(node->get(L"CellHeight").c_str()));
                const int sel = _wtoi(node->get(L"SelectedIndex").c_str());
                std::vector<std::wstring> items = cvdoc::splitBar(node->get(L"Items"));
                int x = rc.left + 6, y = rc.top + 6, col = 0;
                for (size_t i = 0; i < items.size(); ++i)
                {
                    CRect tile(x, y, x + cw, y + chh);
                    if (tile.right > rc.right - 6) { x = rc.left + 6; y += chh + 6; col = 0; tile = CRect(x, y, x + cw, y + chh); }
                    if (tile.bottom > rc.bottom - 6) break;
                    cv.fillRoundRect(tile, (int)i == sel ? RGB(219, 233, 249) : cvdoc::kSurface,
                                     cvdoc::kBorder, 4);
                    drawTextCv(cv, tile, items[i], cvdoc::kInk,
                               DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS, pt, fam);
                    x += cw + 6;
                    ++col;
                }
            }
            else if (tagIs(node, L"PathView"))
            {
                cv.fillRect(rc, cvdoc::kSurface);
                cv.strokeRect(rc, cvdoc::kBorder, 1);
                std::vector<std::wstring> items = cvdoc::splitBar(node->get(L"Items"));
                const int sel = _wtoi(node->get(L"SelectedIndex").c_str());
                const int rowH = (std::min)(24, (int)((rc.Height() - 12) / (std::max)(1, (int)items.size())));
                int y = rc.top + 6;
                for (size_t i = 0; i < items.size(); ++i)
                {
                    const int shrink = (std::abs)((int)i - sel) * 8;
                    CRect row(rc.left + 12 + shrink, y, rc.right - 12 - shrink, y + rowH);
                    cv.fillRoundRect(row, (int)i == sel ? RGB(219, 233, 249) : cvdoc::kSurface,
                                     cvdoc::kBorder, 4);
                    drawTextCv(cv, row, items[i], cvdoc::kInk,
                               DT_CENTER | DT_VCENTER | DT_SINGLELINE, pt, fam);
                    y += rowH + 4;
                }
            }
            else if (tagIs(node, L"Dial"))
            {
                const int d = (std::min)(rc.Width(), rc.Height());
                CRect box(0, 0, d, d);
                box.OffsetRect(rc.left + (rc.Width() - d) / 2, rc.top + (rc.Height() - d) / 2);
                cv.fillRoundRect(box, cvdoc::kSurface, cvdoc::kBorder, d / 2);
                CRect inner = box; inner.DeflateRect(d / 8, d / 8);
                cv.fillRoundRect(inner, cvdoc::kField, cvdoc::kBorder, inner.Width() / 2);
                CRect knob(box.CenterPoint().x - d / 12, box.top + d / 6,
                           box.CenterPoint().x + d / 12, box.top + d / 6 + d / 6);
                cv.fillRoundRect(knob, cvdoc::kAccent, cvdoc::kAccent, d / 12);
                const std::wstring val = node->get(L"Value");
                if (!val.empty())
                    drawTextCv(cv, box, val, cvdoc::kInk,
                               DT_CENTER | DT_VCENTER | DT_SINGLELINE, pt, fam);
            }
            else if (tagIs(node, L"Tumbler"))
            {
                cv.fillRect(rc, cvdoc::kField);
                cv.strokeRect(rc, cvdoc::kBorder, 1);
                const int count = (std::max)(1, _wtoi(node->get(L"Count").c_str()));
                const int sel = (std::max)(0, (std::min)(count - 1, _wtoi(node->get(L"SelectedIndex").c_str())));
                CRect mid = rc;
                mid.top = rc.CenterPoint().y - 12;
                mid.bottom = rc.CenterPoint().y + 12;
                cv.fillRect(mid, RGB(219, 233, 249));
                wchar_t buf[16];
                swprintf_s(buf, L"%02d", sel);
                drawTextCv(cv, mid, buf, cvdoc::kInk, DT_CENTER | DT_VCENTER | DT_SINGLELINE, pt, fam);
                swprintf_s(buf, L"%02d", (sel + 1) % count);
                CRect below = rc; below.top = mid.bottom;
                drawTextCv(cv, below, buf, cvdoc::kMuted, DT_CENTER | DT_TOP | DT_SINGLELINE, pt, fam);
            }
            else if (tagIs(node, L"RangeSlider") || tagIs(node, L"Slider"))
            {
                // 轨道 + 手柄（RangeSlider 两个手柄，值来自 LowValue/HighValue）
                CRect track(rc.left, rc.CenterPoint().y - 2, rc.right, rc.CenterPoint().y + 2);
                cv.fillRect(track, cvdoc::kTrack);

                auto knobAt = [&](int pct) {
                    const int x = rc.left + (int)((rc.Width() - 10) * pct / 100.0);
                    CRect k(x, rc.CenterPoint().y - 7, x + 10, rc.CenterPoint().y + 7);
                    cv.fillRoundRect(k, cvdoc::kAccent, cvdoc::kAccent, 5);
                };
                if (tagIs(node, L"RangeSlider"))
                {
                    CRect hot = track;
                    hot.left = rc.left + (int)((rc.Width() - 10) * _wtoi(node->get(L"LowValue").c_str()) / 100.0);
                    hot.right = rc.left + (int)((rc.Width() - 10) * _wtoi(node->get(L"HighValue").c_str()) / 100.0) + 10;
                    cv.fillRect(hot, RGB(140, 190, 240));
                    knobAt(_wtoi(node->get(L"LowValue").c_str()));
                    knobAt(_wtoi(node->get(L"HighValue").c_str()));
                }
                else
                {
                    int v = _wtoi(node->get(L"Value").c_str());
                    const int maxV = (std::max)(1, _wtoi(node->get(L"Maximum").c_str()));
                    if (v > maxV) v = maxV;
                    CRect hot = track;
                    hot.left = rc.left;
                    hot.right = rc.left + (int)((rc.Width() - 10) * v / (double)maxV) + 10;
                    cv.fillRect(hot, RGB(140, 190, 240));
                    knobAt((int)(100.0 * v / maxV));
                }
            }
            else if (tagIs(node, L"DatePicker"))
            {
                drawRoundedCv(cv, rc, cvdoc::kField, cvdoc::kBorder, 3);
                cv.fillRect(CRect(rc.right - 18, rc.top + 1, rc.right - 1, rc.bottom - 1), cvdoc::kTrack);
                CRect tr = rc; tr.DeflateRect(6, 0); tr.right -= 20;
                drawTextCv(cv, tr, node->get(L"Value"), cvdoc::kInk,
                           DT_LEFT | DT_VCENTER | DT_SINGLELINE, pt, fam);
            }
            else if (tagIs(node, L"MonthGrid"))
            {
                drawRoundedCv(cv, rc, cvdoc::kField, cvdoc::kBorder, 3);
                CRect head = rc; head.bottom = head.top + 24;
                cv.fillRect(head, cvdoc::kHead);
                drawTextCv(cv, head, node->get(L"Value").empty() ? std::wstring(L"2026-11")
                                                                : node->get(L"Value").substr(0, 7),
                           cvdoc::kInk, DT_CENTER | DT_VCENTER | DT_SINGLELINE, pt, fam);
                // 7 列日期格：只画网格与首行数字，密度与运行时接近
                CRect grid = rc; grid.top = head.bottom + 2; grid.DeflateRect(4, 0);
                const int cellW = grid.Width() / 7;
                const int cellH = (std::min)(18, grid.Height() / 6);
                for (int r = 0; r < 6; ++r)
                {
                    for (int c = 0; c < 7; ++c)
                    {
                        CRect cell(grid.left + c * cellW, grid.top + r * cellH,
                                   grid.left + (c + 1) * cellW, grid.top + (r + 1) * cellH);
                        if (cell.bottom > rc.bottom - 2) break;
                        wchar_t buf[8];
                        swprintf_s(buf, L"%d", r * 7 + c + 1);
                        if (r * 7 + c + 1 <= 30)
                            drawTextCv(cv, cell, buf, cvdoc::kMuted,
                                       DT_CENTER | DT_VCENTER | DT_SINGLELINE, 0, std::wstring());
                    }
                }
            }
            else if (tagIs(node, L"MenuBar"))
            {
                xmlctl::xmlDrawShellMenuBar(cv, rc, *node);
            }
            else if (tagIs(node, L"ToolBar") || tagIs(node, L"Menu"))
            {
                const bool bar = !tagIs(node, L"Menu");
                if (bar) cv.fillRect(rc, cvdoc::kHead);
                else drawRoundedCv(cv, rc, cvdoc::kField, cvdoc::kBorder, 3);
                cv.strokeLine(POINT{ rc.left, rc.bottom - 1 }, POINT{ rc.right, rc.bottom - 1 },
                              cvdoc::kBorder, 1);
                const int sel = _wtoi(node->get(L"SelectedIndex").c_str());
                int x = rc.left + 4;
                std::vector<std::wstring> items = cvdoc::splitBar(node->get(L"Items"));
                for (size_t i = 0; i < items.size(); ++i)
                {
                    const int w = (std::min)(110, (int)(items[i].size() * 12 + 20));
                    if (x + w > rc.right - 4) break;
                    CRect item(x, rc.top + 2, x + w, rc.bottom - 2);
                    if ((int)i == sel && bar) cv.fillRect(item, RGB(219, 233, 249));
                    drawTextCv(cv, item, items[i], cvdoc::kInk,
                               DT_CENTER | DT_VCENTER | DT_SINGLELINE, pt, fam);
                    x += w + 3;
                }
            }
            else if (tagIs(node, L"ToolTip"))
            {
                drawRoundedCv(cv, rc, RGB(255, 255, 225), cvdoc::kBorder, 3);
                CRect tr = rc; tr.DeflateRect(6, 0);
                drawTextCv(cv, tr, node->text, cvdoc::kInk,
                           DT_LEFT | DT_VCENTER | DT_SINGLELINE, pt, fam);
            }
            else if (tagIs(node, L"Popup"))
            {
                cv.fillRect(rc, RGB(0, 0, 0));
                CRect panel = rc; panel.DeflateRect(4, 4);
                drawRoundedCv(cv, panel, cvdoc::kField, cvdoc::kBorder, 4);
                CRect title = panel; title.bottom = title.top + 26;
                cv.fillRect(title, cvdoc::kHead);
                CRect tr = title; tr.left += 8;
                drawTextCv(cv, tr, node->get(L"Title"), cvdoc::kInk,
                           DT_LEFT | DT_VCENTER | DT_SINGLELINE, pt, fam);
                CRect body = panel;
                body.top = title.bottom + 4;
                body.DeflateRect(8, 0);
                drawTextCv(cv, body, node->get(L"Content"), cvdoc::kInk,
                           DT_LEFT | DT_TOP | DT_WORDBREAK, pt, fam);
            }
            else if (tagIs(node, L"PowerCanvas") || tagIs(node, L"QCanvas"))
            {
                cv.fillRect(rc, RGB(255, 255, 255));
                cv.strokeRect(rc, cvdoc::kBorder, 1);
                // 淡网格，表明这是个画布而不是空框
                for (int x = rc.left + 24; x < rc.right; x += 24)
                    cv.strokeLine(POINT{ x, rc.top + 1 }, POINT{ x, rc.bottom - 1 }, RGB(238, 240, 243), 1);
                for (int y = rc.top + 24; y < rc.bottom; y += 24)
                    cv.strokeLine(POINT{ rc.left + 1, y }, POINT{ rc.right - 1, y }, RGB(238, 240, 243), 1);
                drawTextCv(cv, rc, node->tag, cvdoc::kMuted,
                           DT_CENTER | DT_VCENTER | DT_SINGLELINE, pt, fam);
            }
            else if (tagIs(node, L"Flickable"))
            {
                drawRoundedCv(cv, rc, cvdoc::kSurface, cvdoc::kBorder, 4);
                // 虚线视口框：表示可拖动的内容区
                const int inset = 10;
                for (int x = rc.left + inset; x < rc.right - inset; x += 8)
                    cv.strokeLine(POINT{ x, rc.top + inset }, POINT{ (std::min)(x + 4, static_cast<int>(rc.right) - inset), rc.top + inset },
                                  cvdoc::kBorder, 1);
                for (int x = rc.left + inset; x < rc.right - inset; x += 8)
                    cv.strokeLine(POINT{ x, rc.bottom - inset }, POINT{ (std::min)(x + 4, static_cast<int>(rc.right) - inset), rc.bottom - inset },
                                  cvdoc::kBorder, 1);
                for (int y = rc.top + inset; y < rc.bottom - inset; y += 8)
                    cv.strokeLine(POINT{ rc.left + inset, y }, POINT{ rc.left + inset, (std::min)(y + 4, static_cast<int>(rc.bottom) - inset) },
                                  cvdoc::kBorder, 1);
                for (int y = rc.top + inset; y < rc.bottom - inset; y += 8)
                    cv.strokeLine(POINT{ rc.right - inset, y }, POINT{ rc.right - inset, (std::min)(y + 4, static_cast<int>(rc.bottom) - inset) },
                                  cvdoc::kBorder, 1);
                const std::wstring off = node->get(L"OffsetX") + L"," + node->get(L"OffsetY");
                drawTextCv(cv, rc, (off == L",") ? node->tag : off, cvdoc::kMuted,
                           DT_CENTER | DT_VCENTER | DT_SINGLELINE, pt, fam);
            }
            else if (tagIs(node, L"CodeEditor"))
            {
                cv.fillRect(rc, cvdoc::kField);
                cv.strokeRect(rc, cvdoc::kBorder, 1);
                const int gutter = 26;
                cv.fillRect(CRect(rc.left, rc.top, rc.left + gutter, rc.bottom), cvdoc::kHead);
                cv.strokeLine(POINT{ rc.left + gutter, rc.top }, POINT{ rc.left + gutter, rc.bottom },
                              cvdoc::kBorder, 1);
                std::vector<std::wstring> lines;
                std::wstring txt = node->text;
                size_t start = 0;
                while (start <= txt.size())
                {
                    const size_t nl = txt.find(L'\n', start);
                    lines.push_back((nl == std::wstring::npos) ? txt.substr(start) : txt.substr(start, nl - start));
                    if (nl == std::wstring::npos) break;
                    start = nl + 1;
                }
                int y = rc.top + 3;
                for (size_t i = 0; i < lines.size() && y < rc.bottom - 12; ++i)
                {
                    CRect no(rc.left, y, rc.left + gutter - 4, y + 16);
                    wchar_t buf[8];
                    swprintf_s(buf, L"%d", (int)i + 1);
                    drawTextCv(cv, no, buf, cvdoc::kMuted, DT_RIGHT | DT_VCENTER | DT_SINGLELINE, 0, std::wstring());
                    CRect tr(rc.left + gutter + 6, y, rc.right - 4, y + 16);
                    drawTextCv(cv, tr, lines[i], cvdoc::kInk, DT_LEFT | DT_VCENTER | DT_SINGLELINE, pt, fam);
                    y += 16;
                }
            }
            else if (tagIs(node, L"Hyperlink"))
            {
                CRect tr = rc;
                drawTextCv(cv, tr, node->text.empty() ? node->get(L"Url") : node->text, cvdoc::kAccent,
                           DT_CENTER | DT_VCENTER | DT_SINGLELINE, pt, fam);
                const int y = rc.CenterPoint().y + 8;
                cv.strokeLine(POINT{ rc.left + 6, y }, POINT{ rc.right - 6, y }, cvdoc::kAccent, 1);
            }
            else if (tagIs(node, L"Choice"))
            {
                drawRoundedCv(cv, rc, cvdoc::kField, cvdoc::kBorder, 3);
                cv.fillRect(CRect(rc.right - 18, rc.top + 1, rc.right - 1, rc.bottom - 1), cvdoc::kTrack);
                CRect tr = rc; tr.DeflateRect(6, 0); tr.right -= 20;
                std::vector<std::wstring> items = cvdoc::splitBar(node->get(L"Items"));
                drawTextCv(cv, tr, items.empty() ? node->text : items[0], cvdoc::kInk,
                           DT_LEFT | DT_VCENTER | DT_SINGLELINE, pt, fam);
            }
            else if (tagIs(node, L"Divider"))
            {
                // 分隔线：一条横线，不再画成"文字卡片"
                const int midY = (rc.top + rc.bottom) / 2;
                cv.strokeLine(POINT{ rc.left, midY }, POINT{ rc.right, midY }, cvdoc::kBorder, 1);
            }
            else if (tagIs(node, L"Image"))
            {
                // 画布不加载图片文件，用浅底 + 图元标记表示"这里有一张位图"；
                // 运行时（预览器/正式界面）按 Image 属性真画图
                drawRoundedCv(cv, rc, cvdoc::kSurface, cvdoc::kBorder, 2);
                drawTextCv(cv, rc, std::wstring(L"\u25A3"), cvdoc::kMuted,
                           DT_CENTER | DT_VCENTER | DT_SINGLELINE, 0, std::wstring());
            }
            else if (tagIs(node, L"Switch"))
            {
                const std::wstring ck = node->get(L"Checked");
                const bool on = (_wcsicmp(ck.c_str(), L"true") == 0 || ck == L"1");
                CRect pill = rc;
                const int h = (std::min)(rc.Height(), 22);
                pill.top = (rc.top + rc.bottom - h) / 2;
                pill.bottom = pill.top + h;
                pill.right = (std::min)(rc.right, pill.left + 44);
                cv.fillRoundRect(pill, on ? cvdoc::kAccent : cvdoc::kTrack,
                                 on ? cvdoc::kAccent : cvdoc::kBorder, h / 2);
                CRect knob(0, 0, h - 4, h - 4);
                knob.OffsetRect(on ? pill.right - h + 2 : pill.left + 2, pill.top + 2);
                cv.fillRoundRect(knob, RGB(255, 255, 255), RGB(255, 255, 255), (h - 4) / 2);
                if (!node->text.empty())
                {
                    CRect tr = rc; tr.left = pill.right + 8;
                    drawTextCv(cv, tr, node->text, cvdoc::kInk,
                               DT_LEFT | DT_VCENTER | DT_SINGLELINE, pt, fam);
                }
            }
            else if (tagIs(node, L"SpinBox"))
            {
                drawRoundedCv(cv, rc, cvdoc::kField, cvdoc::kBorder, 3);
                CRect step(rc.right - 18, rc.top + 1, rc.right - 1, rc.bottom - 1);
                cv.fillRect(step, cvdoc::kTrack);
                cv.strokeLine(POINT{ step.left, (step.top + step.bottom) / 2 },
                              POINT{ step.right, (step.top + step.bottom) / 2 }, cvdoc::kBorder, 1);
                CRect tr = rc; tr.DeflateRect(6, 0); tr.right -= 20;
                std::wstring val = node->get(L"Value");
                if (val.empty()) val = node->text;
                drawTextCv(cv, tr, val, cvdoc::kInk, DT_LEFT | DT_VCENTER | DT_SINGLELINE, pt, fam);
            }
            else if (tagIs(node, L"ScrollBar"))
            {
                const bool vert = node->get(L"Orientation") != L"Horizontal";
                cv.fillRect(rc, cvdoc::kTrack);
                if (vert)
                {
                    const int th = (std::max)(12, rc.Height() / 3);
                    cv.fillRoundRect(CRect(rc.left, rc.top + 2, rc.right, rc.top + 2 + th),
                                     cvdoc::kButton, cvdoc::kBorder, 4);
                }
                else
                {
                    const int tw = (std::max)(12, rc.Width() / 3);
                    cv.fillRoundRect(CRect(rc.left + 2, rc.top, rc.left + 2 + tw, rc.bottom),
                                     cvdoc::kButton, cvdoc::kBorder, 4);
                }
            }
            else if (tagIs(node, L"BusyIndicator"))
            {
                const int d = (std::min)(rc.Width(), rc.Height());
                CRect c(0, 0, d, d);
                c.OffsetRect(rc.left + (rc.Width() - d) / 2, rc.top + (rc.Height() - d) / 2);
                cv.fillRoundRect(c, RGB(255, 255, 255), cvdoc::kBorder, d / 2);
                cv.strokeLine(POINT{ c.CenterPoint().x, c.top + 6 },
                              POINT{ c.left + d / 4, c.top + d / 4 }, cvdoc::kAccent, 2);
            }
            else
            {
                // MenuItem 这类外壳构件没有 Content，标题在 Header 里；都没有才退回标签名
                std::wstring cap = node->get(L"Header");
                if (cap.empty()) cap = node->text;
                if (cap.empty()) cap = node->tag;
                drawRoundedCv(cv, rc, cvdoc::kSurface, cvdoc::kBorder, 4);
                drawTextCv(cv, rc, cap, cvdoc::kInk,
                           DT_CENTER | DT_VCENTER | DT_SINGLELINE, 0, std::wstring());
            }

            // XML BorderStyle / ScrollStyle 生效（未声明则保持控件默认外观）
            if (!isRoot && !bdStyle.empty())
                applyBorderStyleCv(cv, rc, bdC == (COLORREF)-1 ? cvdoc::kBorder : bdC,
                                   bgC == (COLORREF)-1 ? cvdoc::kSurface : bgC, bdStyle, bdThick);
            if (!isRoot && !scrollStyle.empty())
                drawScrollBarsCv(cv, rc, scrollStyle);

            // 控件模板实体化出的视觉层也要画出来（所见即所得）
            if (IsContainer(node->tag) || tagIs(node, L"Window") || tagIs(node, L"GroupBox") ||
                tagIs(node, L"PanelHeader") || tagIs(node, L"TabStrip") ||
                tagIs(node, L"ChipBar") || tagIs(node, L"Stage") ||
                hasTemplateChild(node))
            {
                // 侧栏：画布只呈现第一页，其余页在对象树里仍可选中编辑
                bool firstPageDrawn = false;
                const std::vector<int> order = childDrawOrder(node);
                for (int ci : order)
                {
                    auto& child = node->children[ci];
                    const bool isPage = tagIs(&child, L"Page") || tagIs(&child, L"page") ||
                                        tagIs(&child, L"TabItem");
                    if (tagIs(node, L"PaletteSet") && isPage)
                    {
                        if (firstPageDrawn)
                            continue;
                        firstPageDrawn = true;
                    }
                    drawNode(cv, &child, origin, false,
                             bgC == (COLORREF)-1 ? inheritBg : bgC, vp);
                }
            }
        }

        // 虚线矩形：用整像素短段拼，三端与 D2D 都不依赖画笔样式
        void dashedRectCv(XmlCanvas& cv, const CRect& rc, COLORREF c)
        {
            const int dash = 3, gap = 3;
            for (int x = rc.left; x < rc.right; x += dash + gap)
            {
                const int x2 = (std::min)(x + dash, static_cast<int>(rc.right));
                cv.fillRect(CRect(x, rc.top, x2, rc.top + 1), c);
                cv.fillRect(CRect(x, rc.bottom - 1, x2, rc.bottom), c);
            }
            for (int y = rc.top; y < rc.bottom; y += dash + gap)
            {
                const int y2 = (std::min)(y + dash, static_cast<int>(rc.bottom));
                cv.fillRect(CRect(rc.left, y, rc.left + 1, y2), c);
                cv.fillRect(CRect(rc.right - 1, y, rc.right, y2), c);
            }
        }

        void selRectOf(XmlElement* node, CPoint origin, CRect& rc)
        {
            XmlElement* root = (m_doc != nullptr) ? m_doc->root() : nullptr;
            if (node == root)
                rc = CRect(origin.x, origin.y, origin.x + node->w, origin.y + node->h);
            else
                rc = CRect(node->previewX + origin.x, node->previewY + origin.y,
                           node->previewX + origin.x + node->previewW,
                           node->previewY + origin.y + node->previewH);
        }

        // 单个夹点方块（4px 半径）
        void gripCv(XmlCanvas& cv, CPoint p, bool round = false)
        {
            const int h = 4;
            CRect hr(p.x - h, p.y - h, p.x + h + 1, p.y + h + 1);
            if (round)
            {
                cv.fillEllipse(hr, RGB(245, 248, 255), RGB(60, 110, 200));
                return;
            }
            cv.fillRect(hr, RGB(245, 248, 255));
            cv.strokeRect(hr, RGB(60, 110, 200), 1);
        }

        void drawSelectionCv(XmlCanvas& cv, XmlElement* node, CPoint origin, bool handles = true,
                            COLORREF frame = RGB(255, 90, 90))
        {
            if (node == nullptr) return;
            CRect rc;
            selRectOf(node, origin, rc);
            // 直线 / 圆弧不画外框夹点：选中态把线条本身描亮，只给两端各一个夹点
            if (isOpenShapeNode(node))
            {
                CPoint a, b;
                if (isArcNode(node))
                {
                    int sa = 180, sw = 180;
                    arcParams(node, sa, sw);
                    const std::vector<POINT> pts = arcPoints(rc, sa, sw, 64);
                    cv.strokePath(pts, frame, 2, false);
                    if (!pts.empty()) { a = CPoint(pts.front().x, pts.front().y); b = CPoint(pts.back().x, pts.back().y); }
                    else { a = CPoint(rc.left, rc.top); b = CPoint(rc.right, rc.bottom); }
                }
                else
                {
                    lineEndpoints(node, a, b);
                    cv.strokeLine(a, b, frame, 2);
                }
                if (!handles) return;
                gripCv(cv, a);
                gripCv(cv, b);
                return;
            }
            dashedRectCv(cv, rc, frame);
            if (!handles) return;                       // 根窗体只给框，不给夹点
            CPoint pts[8];
            const int cnt = handlePoints(node, pts);
            for (int i = 0; i < cnt; ++i)
                gripCv(cv, pts[i], isEllipseNode(node) && i == 0);   // 圆心用圆点，和四个轴向夹点区分开
        }

        // ---- 左侧竖向 Draw 工具条：直线 / 矩形 / 圆 ----
        // 画在画布客户坐标（不跟缩放走）。点中即把待放置的 tag 交给宿主，
        // 之后按下拖动定尺寸 —— 与控件箱那条路径完全一致，只是入口挪到画布边上。
        // 反过来：给定圆心/半径与角度区间，算出这段弧的外接方框。
        // 与 arcGeom 互为逆运算，因此"圆心+半径不变、只改角度"时框会跟着变，弧始终落在同一个圆上。
        static void arcBoxFromArc(double cx, double cy, double R, int startDeg, int sweepDeg, CRect& rc)
        {
            xmlgeom::Arc2 arc;
            arc.cx = cx; arc.cy = cy; arc.radius = R;
            arc.startDeg = startDeg; arc.sweepDeg = (sweepDeg == 0) ? 180.0 : (double)sweepDeg;
            const xmlgeom::Rect2 box = xmlgeom::roundToInt(xmlgeom::arcBounds(arc));
            rc.SetRect((int)box.left, (int)box.top, (int)box.right, (int)box.bottom);
        }

        // 正圆弧几何：由"包围盒 + 起止角"反推圆心与半径，保证画出来是正圆的一段弧
        // （单一半径 R，弧居中内接于该框）。实现放在纯几何库 xmlgeom 里，绘制/夹点/拖动共用一份。
        static void arcGeom(const CRect& rc, int startDeg, int sweepDeg,
                            double& cx, double& cy, double& R)
        {
            const xmlgeom::Rect2 box((double)rc.left, (double)rc.top, (double)rc.right, (double)rc.bottom);
            const xmlgeom::Arc2 arc = xmlgeom::arcFromBox(box, (double)startDeg, (double)sweepDeg);
            cx = arc.cx;
            cy = arc.cy;
            R = arc.radius;
        }

        // 圆弧采样：rc 是期望弧占据的方框，圆心/半径由 arcGeom 定，弧是正圆的一段。
        static std::vector<POINT> arcPoints(const CRect& rc, int startDeg, int sweepDeg, int steps)
        {
            const xmlgeom::Rect2 box((double)rc.left, (double)rc.top, (double)rc.right, (double)rc.bottom);
            const xmlgeom::Arc2 arc = xmlgeom::arcFromBox(box, (double)startDeg, (double)sweepDeg);
            std::vector<xmlgeom::Point2> pts2;
            xmlgeom::sampleArc(arc, steps, pts2);
            std::vector<POINT> pts;
            pts.reserve(pts2.size());
            for (const xmlgeom::Point2& p : pts2)
                pts.push_back(POINT{ (LONG)floor(p.x + 0.5), (LONG)floor(p.y + 0.5) });
            return pts;
        }

        // 圆弧角度属性：未声明时给上半圈（180°→360°），与 catalog 的默认一致
        static void arcParams(const XmlElement* n, int& startDeg, int& sweepDeg)
        {
            startDeg = 180;
            sweepDeg = 180;
            const std::wstring s = n->get(L"StartAngle");
            const std::wstring w = n->get(L"SweepAngle");
            if (!s.empty()) startDeg = _wtoi(s.c_str());
            if (!w.empty()) sweepDeg = _wtoi(w.c_str());
            if (sweepDeg == 0) sweepDeg = 180;
        }

        static int normAngle(double a)
        {
            int v = (int)(a + (a >= 0 ? 0.5 : -0.5));
            v %= 360;
            if (v < 0) v += 360;
            return v;
        }

        void drawJigCv(XmlCanvas& cv)
        {
            if (!m_jig) return;
            CRect r = m_jigRect; r.NormalizeRect();
            dashedRectCv(cv, r, RGB(0, 120, 212));
            // 图元按最终形状给个轮廓预览，免得只能看虚线框猜结果
            if (m_pendingShape == L"Ellipse")
                cv.fillEllipse(r, RGB(0, 120, 212), RGB(0, 120, 212));
            else if (m_pendingShape == L"Line")
                cv.strokeLine(POINT{ r.left, r.top }, POINT{ r.right, r.bottom }, RGB(0, 120, 212), 2);
            else if (m_pendingShape == L"Arc")
                cv.strokePath(arcPoints(r, 180, 180, 48), RGB(0, 120, 212), 2, false);
            CString s; s.Format(L"%d x %d", r.Width(), r.Height());
            CRect tr = r; tr.OffsetRect(0, -18);
            cv.text(tr, std::wstring(s), RGB(0, 120, 212), DT_LEFT, 0, L"");
        }

        void drawBandCv(XmlCanvas& cv)
        {
            if (!m_banding) return;
            CRect r = m_bandRect; r.NormalizeRect();
            const COLORREF col = m_bandCrossing ? RGB(90, 200, 120) : RGB(0, 120, 212);
            if (m_bandCrossing) dashedRectCv(cv, r, col);
            else cv.strokeRect(r, col, 1);
            CString s = m_bandCrossing ? L"相交" : L"包含";
            CRect tr = r; tr.OffsetRect(0, -18);
            cv.text(tr, std::wstring(s), col, DT_LEFT, 0, L"");
        }

        // 窗口坐标 -> 文档坐标：缩放后所有鼠标消息入口先统一换算
        CPoint docPt(CPoint p) const
        {
            const float z = (m_zoom <= 0.0f) ? 1.0f : m_zoom;
            return CPoint((int)((p.x - m_panX) / z), (int)((p.y - m_panY) / z));
        }

        // 客户区矩形换算成文档坐标矩形：drawNode 视口裁剪用，外扩 1px 防止贴边节点被切掉
        RECT docViewport(int w, int h) const
        {
            const float z = (m_zoom <= 0.0f) ? 1.0f : m_zoom;
            RECT vp;
            vp.left = (int)((0.0f - m_panX) / z) - 1;
            vp.top = (int)((0.0f - m_panY) / z) - 1;
            vp.right = (int)((w - m_panX) / z) + 1;
            vp.bottom = (int)((h - m_panY) / z) + 1;
            return vp;
        }

        // 标尺：窗口坐标下的顶部 / 左侧刻度条，不参与缩放（在单位变换下画）
        // ox/oy 是画布矩形在目标表面上的偏移
        void drawRulerCv(XmlCanvas& cv, int ox, int oy, int W, int H)
        {
            const int band = 18;
            const COLORREF bandBg = RGB(238, 240, 243);
            const COLORREF tickCol = RGB(158, 165, 172);
            const COLORREF majorCol = RGB(96, 104, 112);
            const COLORREF textCol = RGB(90, 96, 104);
            RECT topRc = { ox, oy, ox + W, oy + band };
            RECT leftRc = { ox, oy + band, ox + band, oy + H };
            cv.fillRect(topRc, bandBg);
            cv.fillRect(leftRc, bandBg);
            cv.strokeLine(POINT{ ox, oy + band }, POINT{ ox + W, oy + band }, tickCol, 1);
            cv.strokeLine(POINT{ ox + band, oy + band }, POINT{ ox + band, oy + H }, tickCol, 1);

            const float z = (m_zoom <= 0.0f) ? 1.0f : m_zoom;
            // 每 10 文档单位短刻度、50 长刻度并标数字；文档 0 点对齐刻度条内侧
            const int startX = (int)((-m_panX / z) / 10.0f) * 10;
            const int startY = (int)((-m_panY / z) / 10.0f) * 10;
            for (int docX = (startX < 0 ? startX : 0); ox + band + (int)(m_panX + docX * z) < ox + W; docX += 10)
            {
                const int px = ox + band + (int)(m_panX + docX * z);
                if (px >= ox + W) break;
                const bool major = (docX % 50 == 0);
                cv.strokeLine(POINT{ px, oy + (major ? 4 : 12) }, POINT{ px, oy + band },
                              major ? majorCol : tickCol, 1);
                if (major && docX > 0)
                {
                    RECT tr = { px + 2, oy + 1, px + 46, oy + band };
                    cv.text(tr, std::to_wstring(docX), textCol, DT_LEFT | DT_VCENTER | DT_SINGLELINE, 11, L"");
                }
            }
            for (int docY = (startY < 0 ? startY : 0); oy + band + (int)(m_panY + docY * z) < oy + H; docY += 10)
            {
                const int py = oy + band + (int)(m_panY + docY * z);
                if (py >= oy + H) break;
                const bool major = (docY % 50 == 0);
                cv.strokeLine(POINT{ ox + (major ? 4 : 12), py }, POINT{ ox + band, py },
                              major ? majorCol : tickCol, 1);
                if (major && docY > 0)
                {
                    RECT tr = { ox + 1, py + 1, ox + band, py + 15 };
                    cv.text(tr, std::to_wstring(docY), textCol, DT_LEFT | DT_SINGLELINE, 11, L"");
                }
            }
        }

        // 把画布内容画进外部表面：设计器整窗只有一张 D2D 表面，画布不再自己上屏，
        // 否则父表面的不透明像素会盖住画布子窗口（DWM 直接合成，不受 WS_CLIPCHILDREN 约束）
        void paintInto(XmlCanvas& cv, int x, int y, int w, int h)
        {
            if (w <= 0 || h <= 0) return;
            RECT bg = { x, y, x + w, y + h };
            // 工作台底色用灰色：窗体（根窗口）以外的空白区一眼能和窗体区分开
            cv.fillRect(bg, RGB(160, 160, 160));

            // 网格点画在缩放到之前，保持 8px 固定间距（缩放只作用于文档内容）
            if (m_showGrid)
            {
                // 一次图案填充（D2D 后端走位图画刷），不再逐点 fillRect
                cv.dotPattern(bg, 8, RGB(148, 148, 148));
            }

            // 画布内容必须夹在自己的矩形里（此处尚未设缩放变换，clip 走设备像素）：
            // 窗口变窄/文档比画布大时，文档会溢出到相邻面板，重绘后留下一截表格/控件残影。
            cv.clipPush(bg);

            D2DCanvasBase* d2 = dynamic_cast<D2DCanvasBase*>(&cv);
            ID2D1RenderTarget* rt = (d2 != nullptr) ? d2->target() : nullptr;
            if (rt != nullptr)
                rt->SetTransform(D2D1::Matrix3x2F::Scale(m_zoom, m_zoom) *
                                 D2D1::Matrix3x2F::Translation((float)x + m_panX,
                                                               (float)y + m_panY));

            XmlElement* root = (m_doc != nullptr) ? m_doc->root() : nullptr;
            if (root != nullptr)
            {
                ensureDocCache(root, rt);
                if (rt != nullptr && m_docBmp != nullptr)
                {
                    // 文档层：缓存位图随当前 Scale*Translation 一起走，平移/缩放不再遍历节点
                    rt->DrawBitmap(m_docBmp, D2D1::RectF(0.0f, 0.0f,
                                                         static_cast<float>(m_docBmpW),
                                                         static_cast<float>(m_docBmpH)));
                }
                else
                {
                    const RECT vp = docViewport(w, h);
                    drawNode(cv, root, CPoint(0, 0), true, RGB(255, 255, 255), &vp);
                }
                // 根 Window 只画选中框，不给 8 个夹点（设计器窗体本身不可缩放/拖移）
                drawSelectionCv(cv, m_selPath, CPoint(0, 0),
                                m_selPath != nullptr && m_selPath != root);
                // 多选其余成员同样带夹点（蓝色），可统一拖拉缩放
                for (size_t i = 1; i < m_selNodes.size(); ++i)
                {
                    XmlElement* n = m_selNodes[i];
                    if (n == nullptr || n == root) continue;
                    drawSelectionCv(cv, n, CPoint(0, 0), true, RGB(96, 160, 255));
                }
                drawJigCv(cv);
                drawBandCv(cv);
            }
            else
            {
                cv.text(bg, L"打开或新建 XML 后在此预览", RGB(170, 170, 170),
                        DT_CENTER | DT_VCENTER | DT_SINGLELINE, 0, L"");
            }

            if (rt != nullptr) rt->SetTransform(D2D1::Matrix3x2F::Identity());
            if (m_showRuler) drawRulerCv(cv, x, y, w, h);
            cv.clipPop();       // 与开头的 clipPush(bg) 配对，恢复父表面的裁剪深度
        }

        // 命中测试入口：内部递归产生的索引是"叶子→根"的逆序，这里统一翻成正序，
        // 与 Resolve() 期望的 root→leaf 顺序一致（否则选中后属性表解析失败）
        bool hitTestRoot(XmlElement* root, CPoint point, Path& out)
        {
            out.clear();
            if (root == nullptr) return false;
            if (!hitTest(root, CPoint(0, 0), point, out)) return false;
            std::reverse(out.begin(), out.end());
            return true;
        }

        // 兄弟绘制先后由 Level 决定（大的后画 = 压在上面），同级保持声明序。
        // 与运行时 z 序、预览器叠放是同一套规则（见 PyMfcCore/controls/XmlMfcOrder.h）。
        static std::vector<int> childDrawOrder(XmlElement* node)
        {
            std::vector<int> levels;
            levels.reserve(node->children.size());
            for (auto& c : node->children)
            {
                const std::wstring lv = c.get(L"Level");
                if (!lv.empty())
                {
                    levels.push_back(_wtoi(lv.c_str()));
                    continue;
                }
                const std::wstring zi = c.get(L"ZIndex");
                levels.push_back(zi.empty() ? 0 : _wtoi(zi.c_str()));
            }
            return xmlmfc::sortDrawOrder(levels);
        }

        // 命中测试：返回命中最深节点的路径
        bool hitTest(XmlElement* node, CPoint origin, CPoint point, Path& out)
        {
            CRect rc(node->previewX + origin.x, node->previewY + origin.y,
                     node->previewX + origin.x + node->previewW,
                     node->previewY + origin.y + node->previewH);
            // 从"最上面"的那个兄弟开始试命中：Level 大的先算
            const std::vector<int> order = childDrawOrder(node);
            for (int oi = (int)order.size() - 1; oi >= 0; --oi)
            {
                const int i = order[oi];
                // 模板实体化出的视觉层不是文档结构，不可选中
                if (node->children[i].templateChild)
                    continue;
                if (hitTest(&node->children[i], origin, point, out))
                {
                    out.push_back(i);
                    return true;
                }
            }
            if (rc.PtInRect(point))
            {
                out.clear();
                return true;
            }
            return false;
        }

        XmlRuntime* m_doc = nullptr;

        // ---- 控件基础交互：画布上直接可用，不建子窗口（整窗仍是一张 D2D 表面）----
        XmlElement* m_liveNode = nullptr;   // 正在就地文本编辑的控件
        std::wstring m_liveBuf;
        int m_liveCaret = 0;

        static bool isTextControl(const XmlElement* n)
        {
            return n != nullptr && (tagIs(n, L"TextField") || tagIs(n, L"Label") || tagIs(n, L"Label"));
        }
        static bool isToggleControl(const XmlElement* n)
        {
            return n != nullptr && (tagIs(n, L"CheckBox") || tagIs(n, L"RadioButton") || tagIs(n, L"ToggleButton"));
        }
        static bool isListControl(const XmlElement* n)
        {
            if (n == nullptr) return false;
            return tagIs(n, L"ComboBox") || tagIs(n, L"ListBox") || tagIs(n, L"Choice") || tagIs(n, L"DatePicker");
        }

        // 画布自己就是一个独立的 D2D 子窗口：平移/缩放只让本窗口重画，
        // 不再让父表面（菜单/Ribbon/控件箱/对象树/属性表/代码区）跟着整窗重画 ——
        // 那正是"拖动不丝滑"的根因。父表面下一帧自然重绘时会用最新的 m_panX/m_zoom。
        void invalidateSurface()
        {
            Invalidate(FALSE);
        }

        void notifyDocChanged()
        {
            m_docBmpDirty = true;   // 文档内容变了，图层缓存作废
            CWnd* parent = GetParent();
            if (parent != nullptr && ::IsWindow(parent->m_hWnd))
                parent->PostMessage(WM_DOC_CHANGED, 0, 0);
        }

        void beginInlineText(XmlElement* n)
        {
            m_liveNode = n;
            m_liveBuf = n->text;
            m_liveCaret = (int)m_liveBuf.size();
            SetFocus();
            invalidateSurface();
        }

        void commitInlineText(bool apply)
        {
            if (m_liveNode == nullptr) return;
            if (apply)
            {
                m_liveNode->text = m_liveBuf;
                notifyDocChanged();
            }
            m_liveNode = nullptr;
            m_liveBuf.clear();
            m_liveCaret = 0;
            invalidateSurface();
        }

        static void splitListItems(const std::wstring& src, std::vector<std::wstring>& out)
        {
            out.clear();
            std::wstring cur;
            for (wchar_t c : src)
            {
                if (c == L'|' || c == L';' || c == L'\n')
                {
                    if (!cur.empty()) out.push_back(cur);
                    cur.clear();
                }
                else cur.push_back(c);
            }
            if (!cur.empty()) out.push_back(cur);
        }

        // 点击可交互控件的语义：下拉 / 勾选 / 就地文本编辑
        void activateControl(XmlElement* n, CPoint screenPt)
        {
            if (n == nullptr) return;
            if (isToggleControl(n))
            {
                const std::wstring v = n->get(L"Checked");
                const bool on = (_wcsicmp(v.c_str(), L"true") == 0 || v == L"1");
                n->set(L"Checked", on ? L"False" : L"True");
                notifyDocChanged();
                invalidateSurface();
                return;
            }
            if (isListControl(n))
            {
                std::wstring items = n->get(L"Items");
                if (items.empty()) items = n->get(L"Options");
                std::vector<std::wstring> parts;
                splitListItems(items, parts);
                if (parts.empty()) return;
                CMenu menu;
                menu.CreatePopupMenu();
                for (size_t i = 0; i < parts.size(); ++i)
                    menu.AppendMenuW(MF_STRING, (UINT)(i + 1), parts[i].c_str());
                const int cmd = (int)menu.TrackPopupMenu(
                    TPM_RETURNCMD | TPM_LEFTALIGN | TPM_TOPALIGN, screenPt.x, screenPt.y, this);
                if (cmd > 0 && cmd <= (int)parts.size())
                {
                    n->text = parts[cmd - 1];
                    n->set(L"SelectedIndex", std::to_wstring(cmd - 1));
                    notifyDocChanged();
                    invalidateSurface();
                }
                return;
            }
            if (isTextControl(n))
                beginInlineText(n);
        }

        afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
        {
            (void)nRepCnt;
            (void)nFlags;
            if (m_liveNode == nullptr)
            {
                CWnd::OnChar(nChar, nRepCnt, nFlags);
                return;
            }
            if (nChar == L'\r' || nChar == L'\n' || nChar == L'\x1b') return;   // 由 OnKeyDown 处理
            if (nChar == L'\b') return;
            if (nChar < 0x20) return;
            if (m_liveCaret < 0) m_liveCaret = 0;
            if (m_liveCaret > (int)m_liveBuf.size()) m_liveCaret = (int)m_liveBuf.size();
            m_liveBuf.insert(m_liveBuf.begin() + m_liveCaret, (wchar_t)nChar);
            ++m_liveCaret;
            invalidateSurface();
        }

        afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
        {
            (void)nRepCnt;
            (void)nFlags;
            if (m_liveNode == nullptr)
            {
                CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
                return;
            }
            switch (nChar)
            {
            case VK_RETURN: commitInlineText(true); break;
            case VK_ESCAPE: commitInlineText(false); break;
            case VK_BACK:
                if (m_liveCaret > 0)
                {
                    m_liveBuf.erase(m_liveBuf.begin() + (m_liveCaret - 1));
                    --m_liveCaret;
                    invalidateSurface();
                }
                break;
            case VK_DELETE:
                if (m_liveCaret < (int)m_liveBuf.size())
                {
                    m_liveBuf.erase(m_liveBuf.begin() + m_liveCaret);
                    invalidateSurface();
                }
                break;
            case VK_LEFT: if (m_liveCaret > 0) { --m_liveCaret; invalidateSurface(); } break;
            case VK_RIGHT: if (m_liveCaret < (int)m_liveBuf.size()) { ++m_liveCaret; invalidateSurface(); } break;
            case VK_HOME: m_liveCaret = 0; invalidateSurface(); break;
            case VK_END: m_liveCaret = (int)m_liveBuf.size(); invalidateSurface(); break;
            default: break;
            }
        }

        afx_msg void OnLButtonDblClk(UINT flags, CPoint point)
        {
            (void)flags;
            point = docPt(point);
            XmlElement* root = m_doc ? m_doc->root() : nullptr;
            if (root == nullptr) return;
            Path hit;
            const bool found = hitTestRoot(root, point, hit);
            diagLog("[dbl] pt=%d,%d hit=%d pathlen=%d\n", point.x, point.y, found ? 1 : 0, (int)hit.size());
            if (!found || hit.empty()) return;
            XmlElement* n = root;
            for (int i : hit)
            {
                if (i < 0 || i >= (int)n->children.size()) return;
                n = &n->children[i];
            }
            diagLog("[dbl] node=%s text=%d\n", diagNarrow(n->tag).c_str(), (int)n->text.size());
            CPoint scr = point;
            ClientToScreen(&scr);
            activateControl(n, scr);
        }

        Path m_sel;          // 选中路径（供 setSelected 传入）
        XmlElement* m_selPath = nullptr; // 选中节点指针（重绘时 Resolve 得到）
        std::vector<Path> m_selSet;       // 多选集合（对齐/分布用，首项=主选中）
        std::vector<XmlElement*> m_selNodes;
        bool m_showGrid = true;
        float m_zoom = 1.0f;
        float m_panX = 0.0f, m_panY = 0.0f;   // 视图平移（窗口像素）
        bool  m_panning = false;              // 中键拖动平移中
        CPoint m_panStart = CPoint(0, 0);     // 按下点（窗口坐标）
        float  m_panX0 = 0.0f, m_panY0 = 0.0f;
        bool m_autoFit = true;                // 打开文档时按视口自动缩放到可见                 // 画布缩放倍率（0.25 ~ 4.0）
        bool m_showRuler = false;            // 标尺开关（顶部/左侧刻度条）
        ID2D1Bitmap* m_bgD2DBmp = nullptr;   // 背景（白底+网格）烘成的 D2D 位图
        int m_bgD2DW = 0, m_bgD2DH = 0;
        bool m_bgD2DGrid = false;
        // 文档图层缓存：整棵文档树按"自然尺寸"烘成一张位图，平移/缩放只走位图变换，
        // 文档不变就不重画；选中框/夹点/拖拽框属于交互层，每帧单独画在缓存之上。
        ID2D1Bitmap* m_docBmp = nullptr;
        XmlElement*  m_docBmpRoot = nullptr;
        int m_docBmpW = 0, m_docBmpH = 0;
        bool m_docBmpDirty = true;
        D2DDcCanvas* m_d2dCv = nullptr;      // D2D 绘制目标（DCRenderTarget，画到内存 DC）
        CDC m_memDC;                         // 双缓冲内存 DC（D2D 画完后 BitBlt 上屏）
        CBitmap m_memBmp;
        int m_memW = 0, m_memH = 0;
    };
}
