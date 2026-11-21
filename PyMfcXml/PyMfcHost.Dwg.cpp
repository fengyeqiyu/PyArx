// PyMfcHost.Dwg.cpp —— 拆分自 PyMfcHost.cpp（按职责分组；接口声明仍在 PyMfcHost.h）。
#include "PyMfcHostInternal.h"

HBITMAP PyMfcHost::loadBitmapFromFile(const std::wstring& path)
{
    if (path.empty())
        return nullptr;
    ensureWicCom();
    IWICImagingFactory* factory = nullptr;
    HRESULT hr = ::CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                    IID_PPV_ARGS(&factory));
    if (FAILED(hr) || factory == nullptr)
        return nullptr;

    HBITMAP result = nullptr;
    IWICBitmapDecoder* decoder = nullptr;
    IWICBitmapFrameDecode* frame = nullptr;
    IWICFormatConverter* conv = nullptr;
    hr = factory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ,
                                            WICDecodeMetadataCacheOnDemand, &decoder);
    if (SUCCEEDED(hr) && decoder != nullptr)
        hr = decoder->GetFrame(0, &frame);
    if (SUCCEEDED(hr) && frame != nullptr)
        hr = factory->CreateFormatConverter(&conv);
    if (SUCCEEDED(hr) && conv != nullptr)
        hr = conv->Initialize(frame, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone,
                              nullptr, 0.0, WICBitmapPaletteTypeCustom);
    if (SUCCEEDED(hr) && conv != nullptr)
    {
        UINT w = 0, h = 0;
        if (SUCCEEDED(conv->GetSize(&w, &h)) && w > 0 && h > 0)
        {
            BITMAPINFO bi = {};
            bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bi.bmiHeader.biWidth = static_cast<LONG>(w);
            bi.bmiHeader.biHeight = -static_cast<LONG>(h);   // top-down
            bi.bmiHeader.biPlanes = 1;
            bi.bmiHeader.biBitCount = 32;
            bi.bmiHeader.biCompression = BI_RGB;
            void* bits = nullptr;
            HDC screen = ::GetDC(nullptr);
            HBITMAP dib = ::CreateDIBSection(screen, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
            ::ReleaseDC(nullptr, screen);
            if (dib != nullptr && bits != nullptr)
            {
                const UINT stride = w * 4;
                if (SUCCEEDED(conv->CopyPixels(nullptr, stride, stride * h,
                                               static_cast<BYTE*>(bits))))
                    result = dib;
                else
                    ::DeleteObject(dib);
            }
        }
    }
    if (conv != nullptr)
        conv->Release();
    if (frame != nullptr)
        frame->Release();
    if (decoder != nullptr)
        decoder->Release();
    factory->Release();
    return result;
}

void PyMfcHost::fitDwgCanvas(int id)
{
        DwgCanvasState& st = m_dwgCv[id];
        float w = 0.0f, h = 0.0f;
        if (st.mode == 1) { w = static_cast<float>(st.plan.width); h = static_cast<float>(st.plan.height); }
        else if (st.mode == 2) { w = st.svg.width; h = st.svg.height; }
        RECT rc = {0, 0, 1, 1};
        if (HWND h2 = findByIdDeep(m_hWnd, id))
            ::GetClientRect(h2, &rc);
        if (w <= 0.0f || h <= 0.0f || rc.right <= 2 || rc.bottom <= 2)
        {
            debugLogf(L"[dwg-fit] id=%d skip client=%dx%d plan=%gx%g",
                      id, rc.right, rc.bottom, w, h);
            return;
        }
        if (st.mode == 2)
        {
            // SVG 场景按声明宽高等比适配客户区并居中：面板 SVG 与控件同尺寸时
            // 缩放≈1、偏移≈0，仍是 1:1；大工程图 SVG 很大时缩到视口内，避免只
            // 显示左上角一片。地板放宽到 0.001，容纳大坐标 DWG。
            const float kw = static_cast<float>(rc.right) / w;
            const float kh = static_cast<float>(rc.bottom) / h;
            const float raw = (std::min)(kw, kh);
            st.zoom = raw < 1e-6f ? 1e-6f : raw;
            st.ox = static_cast<int>(lroundf((rc.right - w * st.zoom) * 0.5f));
            st.oy = static_cast<int>(lroundf((rc.bottom - h * st.zoom) * 0.5f));
            debugLogf(L"[dwg-fit] id=%d svg fit client=%dx%d svg=%gx%g rawZoom=%.4f fitZoom=%.4f ox=%d oy=%d",
                      id, rc.right, rc.bottom, w, h, raw, st.zoom, st.ox, st.oy);
            return;
        }
        const float kw = (static_cast<float>(rc.right) - 8.0f) / w;
        const float kh = (static_cast<float>(rc.bottom) - 8.0f) / h;
        const float raw = (std::min)(kw, kh);
        if (m_newSurface)
        {
            // 新骨架：不封顶 1.0；地板 0.05，上限 4.0，偏移允许负/居中。
            st.zoom = clampDwgZoom(raw);
            st.ox = static_cast<int>((rc.right - w * st.zoom) * 0.5f);
            st.oy = static_cast<int>((rc.bottom - h * st.zoom) * 0.5f);
        }
        else
        {
            // =0 旧行为：HTML clampK [0.2, 4]，fit 封顶 1.0，偏移不允许为负。
            st.zoom = (std::min)((std::min)(kw, kh), 1.0f);
            if (st.zoom < 0.2f) st.zoom = 0.2f;
            st.ox = static_cast<int>((rc.right - w * st.zoom) * 0.5f);
            st.oy = static_cast<int>((rc.bottom - h * st.zoom) * 0.5f);
            if (st.ox < 0) st.ox = 0;
            if (st.oy < 0) st.oy = 0;
        }
        debugLogf(L"[dwg-fit] id=%d client=%dx%d plan=%gx%g rawZoom=%.4f fitZoom=%.4f ox=%d oy=%d",
                  id, rc.right, rc.bottom, w, h, raw, st.zoom, st.ox, st.oy);
    }

void PyMfcHost::addDwgCanvas(int id, int x, int y, int w, int h)
{
        // 登记早于创建：创建过程里就有首帧 WM_PAINT，晚登记会按"普通标签"画一次
        m_dwgCv[id];
        HWND hwnd = createControl(L"STATIC", L"", SS_OWNERDRAW | WS_CLIPSIBLINGS, x, y, w, h, id);
        if (hwnd != nullptr)
        {
            ::SetPropW(hwnd, kDwgHostProp, reinterpret_cast<HANDLE>(this));
            // XML 里 Data 属性可能先于建窗落到状态里（那时客户区还是 1×1，算不出 fit）：
            // 窗口一建好就重算一次视图，否则首帧停在 1:1 左上角。
            DwgCanvasState& st = m_dwgCv[id];
            if (st.mode != 0)
                fitDwgCanvas(id);
            debugLogf(L"[dwg-cv] id=%d hwnd=%p mode=%d zoom=%.2f", id,
                      static_cast<void*>(hwnd), st.mode, st.zoom);
        }
    }

void PyMfcHost::addRibbon(int id, int x, int y, int w, int h)
{
    // 与 DwgCanvas 同一套自绘 STATIC：状态留宿主，模型/布局/绘制/命中全走控件库 xmlctl。
    m_ribbon[id];
    HWND hwnd = createControl(L"STATIC", L"", SS_OWNERDRAW | WS_CLIPSIBLINGS, x, y, w, h, id);
    if (hwnd != nullptr)
        ::SetPropW(hwnd, kDwgHostProp, reinterpret_cast<HANDLE>(this));
}

void PyMfcHost::setRibbonElement(int id, const XmlElement* el)
{
    RibbonState& st = m_ribbon[id];
    if (el != nullptr)
        xmlctl::ribbonParse(*const_cast<XmlElement*>(el), st.model);
    if (xmlctl::ribbonIconBase().empty())
        xmlctl::ribbonIconBase() = pymfcDir();
    ::InvalidateRect(findByIdDeep(m_hWnd, id), nullptr, TRUE);
}

void PyMfcHost::addFileTab(int id, int x, int y, int w, int h)
{
    // 与 Ribbon 同一套自绘 STATIC：状态留宿主，模型/布局/绘制/命中全走控件库 xmlctl。
    m_fileTabs[id];
    HWND hwnd = createControl(L"STATIC", L"", SS_OWNERDRAW | WS_CLIPSIBLINGS, x, y, w, h, id);
    if (hwnd != nullptr)
        ::SetPropW(hwnd, kDwgHostProp, reinterpret_cast<HANDLE>(this));
}

void PyMfcHost::setFileTabElement(int id, const XmlElement* el)
{
    FileTabState& st = m_fileTabs[id];
    if (el != nullptr)
        xmlctl::filetabParse(*const_cast<XmlElement*>(el), st.model);
    ::InvalidateRect(findByIdDeep(m_hWnd, id), nullptr, TRUE);
}

void PyMfcHost::setDwgLayout(int id, const std::wstring& json)
{
        DwgCanvasState& st = m_dwgCv[id];
        std::vector<xmlplan::PowerNode> nodes;
        std::wstring title, err;
        bool hz = false;
        if (!xmlplan::parseJson(json, nodes, title, err, &hz))
        {
            debugLogf(L"[dwg-cv] id=%d json 解析失败: %ls", id, err.c_str());
            return;
        }
        xmlplan::PowerPlanStyle style;
        if (!xmlplan::buildPlan(nodes, st.plan, style, err, hz))
        {
            debugLogf(L"[dwg-cv] id=%d 布局失败: %ls", id, err.c_str());
            return;
        }
        st.mode = 1;
        st.sel.clear();
        fitDwgCanvas(id);
        debugLogf(L"[dwg-cv] id=%d plan nodes=%d boxes=%d links=%d canvas=%dx%d dir=%ls zoom=%.2f",
                  id, static_cast<int>(st.plan.nodes.size()),
                  static_cast<int>(st.plan.boxes.size()),
                  static_cast<int>(st.plan.links.size()),
                  st.plan.width, st.plan.height, st.plan.dir.c_str(), st.zoom);
        ::InvalidateRect(findByIdDeep(m_hWnd, id), nullptr, TRUE);
    }

void PyMfcHost::setDwgScene(int id, const std::wstring& json)
{
        // 坐标由外部（Python/JSON 引擎）算好；这里只解析 + 建视图，不跑布局算法
        DwgCanvasState& st = m_dwgCv[id];
        xmlplan::PowerPlan plan;
        std::wstring err;
        if (!xmlplan::parseScene(json, plan, err))
        {
            debugLogf(L"[dwg-cv] id=%d 场景解析失败: %ls", id, err.c_str());
            return;
        }
        st.plan = std::move(plan);
        st.mode = 1;
        st.sel.clear();
        fitDwgCanvas(id);
        debugLogf(L"[dwg-cv] id=%d scene nodes=%d boxes=%d links=%d canvas=%dx%d dir=%ls zoom=%.2f",
                  id, static_cast<int>(st.plan.nodes.size()),
                  static_cast<int>(st.plan.boxes.size()),
                  static_cast<int>(st.plan.links.size()),
                  st.plan.width, st.plan.height, st.plan.dir.c_str(), st.zoom);
        ::InvalidateRect(findByIdDeep(m_hWnd, id), nullptr, TRUE);
    }

void PyMfcHost::copyDwgScene(int toId, int fromId)
{
    auto it = m_dwgCv.find(fromId);
    if (it == m_dwgCv.end())
        return;
    DwgCanvasState& st = m_dwgCv[toId];
    st.plan = it->second.plan;      // 结构体整体拷贝（字符串/向量都是深拷贝）
    st.mode = 1;
    st.sel.clear();
    fitDwgCanvas(toId);
    debugLogf(L"[dwg-cv] id=%d <- copy of %d (nodes=%d links=%d)",
              toId, fromId, static_cast<int>(st.plan.nodes.size()),
              static_cast<int>(st.plan.links.size()));
}

void PyMfcHost::setDwgSvg(int id, const std::wstring& svgText)
{
        // 转 UTF-8 后交给解析器（内部按 UTF-8 处理实体与文本）
        const std::string utf8 = wideToUtf8Text(svgText);
        DwgCanvasState& st = m_dwgCv[id];
        // 仅首次喂图才按 1:1 初始化视图；之后平移/缩放产生的 ox/oy/zoom 必须保留，
        // 否则 Python 重绘 SVG 喂回时会把用户视图清零，表现为"拖完/缩放立马跳回原位"。
        const bool firstLoad = st.svg.shapes.empty() && st.svg.texts.empty() && st.svg.width <= 0.0f;
        std::wstring err;
        if (!xmlsvg::parseSvgText(utf8, st.svg, err))
        {
            debugLogf(L"[dwg-cv] id=%d svg 解析失败: %ls", id, err.c_str());
            return;
        }
        st.mode = 2;
        if (firstLoad)
            fitDwgCanvas(id);
        debugLogf(L"[dwg-cv] id=%d svg shapes=%d texts=%d size=%.0fx%.0f zoom=%.2f",
                  id, static_cast<int>(st.svg.shapes.size()),
                  static_cast<int>(st.svg.texts.size()),
                  st.svg.width, st.svg.height, st.zoom);
        ::InvalidateRect(findByIdDeep(m_hWnd, id), nullptr, TRUE);
    }

void PyMfcHost::setDwgXml(int id, const std::wstring& sceneXml)
{
        const std::string utf8 = wideToUtf8Text(sceneXml);
        DwgCanvasState& st = m_dwgCv[id];
        const bool firstLoad = st.svg.shapes.empty() && st.svg.texts.empty() && st.svg.width <= 0.0f;
        std::wstring err;
        if (!xmlsvg::parseXmlSceneText(utf8, st.svg, err))
        {
            debugLogf(L"[dwg-cv] id=%d xml 场景解析失败: %ls", id, err.c_str());
            return;
        }
        st.mode = 2;
        if (firstLoad)
            fitDwgCanvas(id);
        debugLogf(L"[dwg-cv] id=%d xml shapes=%d texts=%d size=%.0fx%.0f zoom=%.2f",
                  id, static_cast<int>(st.svg.shapes.size()),
                  static_cast<int>(st.svg.texts.size()),
                  st.svg.width, st.svg.height, st.zoom);
        ::InvalidateRect(findByIdDeep(m_hWnd, id), nullptr, TRUE);
    }

void PyMfcHost::setDwgOverlay(int id, const std::wstring& svgText)
{
        const std::string utf8 = wideToUtf8Text(svgText);
        DwgCanvasState& st = m_dwgCv[id];
        st.ovl.shapes.clear();
        st.ovl.texts.clear();
        st.ovl.width = st.ovl.height = 0.0f;
        if (!utf8.empty())
        {
            std::wstring err;
            xmlsvg::SvgScene parsed;
            if (xmlsvg::parseSvgText(utf8, parsed, err))
            {
                st.ovl.shapes.swap(parsed.shapes);
                st.ovl.texts.swap(parsed.texts);
                st.ovl.width = parsed.width;
                st.ovl.height = parsed.height;
            }
            else
            {
                debugLogf(L"[dwg-cv] id=%d overlay 解析失败: %ls", id, err.c_str());
            }
        }
        // 只重绘，不动 zoom/ox/oy，也不重新 fit
        ::InvalidateRect(findByIdDeep(m_hWnd, id), nullptr, FALSE);
}

void PyMfcHost::setDwgOverlayXml(int id, const std::wstring& sceneXml)
{
        const std::string utf8 = wideToUtf8Text(sceneXml);
        DwgCanvasState& st = m_dwgCv[id];
        st.ovl.shapes.clear();
        st.ovl.texts.clear();
        st.ovl.width = st.ovl.height = 0.0f;
        if (!utf8.empty())
        {
            std::wstring err;
            xmlsvg::SvgScene parsed;
            if (xmlsvg::parseXmlSceneText(utf8, parsed, err))
            {
                st.ovl.shapes.swap(parsed.shapes);
                st.ovl.texts.swap(parsed.texts);
                st.ovl.width = parsed.width;
                st.ovl.height = parsed.height;
            }
            else
            {
                debugLogf(L"[dwg-cv] id=%d overlay(xml) 解析失败: %ls", id, err.c_str());
            }
        }
        ::InvalidateRect(findByIdDeep(m_hWnd, id), nullptr, FALSE);
}

void PyMfcHost::setDwgView(int id, float zoom, int offsetX, int offsetY)
{        DwgCanvasState& st = m_dwgCv[id];
        // 缩放不设上下限，仅保留极小地板防止除零。
        st.zoom = zoom < 1e-6f ? 1e-6f : zoom;
        st.ox = offsetX;
        st.oy = offsetY;
        ::InvalidateRect(findByIdDeep(m_hWnd, id), nullptr, FALSE);
    }

float PyMfcHost::getDwgZoom(int id) const
{
        auto it = m_dwgCv.find(id);
        return it != m_dwgCv.end() ? it->second.zoom : 0.0f;
    }

std::wstring PyMfcHost::getDwgSelection(int id) const
{
        auto it = m_dwgCv.find(id);
        return it != m_dwgCv.end() ? it->second.sel : std::wstring();
    }

void PyMfcHost::setDwgOptions(int id, bool showCable, bool mini, int miniOf)
{
        DwgCanvasState& st = m_dwgCv[id];
        st.showCable = showCable;
        st.mini = mini;
        st.miniOf = miniOf;
        ::InvalidateRect(findByIdDeep(m_hWnd, id), nullptr, TRUE);
        if (miniOf >= 0)
            ::InvalidateRect(::GetDlgItem(m_hWnd, miniOf), nullptr, TRUE);
    }

void PyMfcHost::getDwgView(int id, float& zoom, int& ox, int& oy) const
{
        zoom = 0.0f;
        ox = oy = 0;
        auto it = m_dwgCv.find(id);
        if (it == m_dwgCv.end()) return;
        zoom = it->second.zoom;
        ox = it->second.ox;
        oy = it->second.oy;
    }

void PyMfcHost::setDetailCard(int id, py::list rect, py::list texts)
{
    auto it = m_dwgCv.find(id);
    if (it == m_dwgCv.end()) return;
    DwgCanvasState& st = it->second;
    if (rect.size() >= 4)
    {
        const int x = py::cast<int>(rect[0]);
        const int y = py::cast<int>(rect[1]);
        const int w = py::cast<int>(rect[2]);
        const int h = py::cast<int>(rect[3]);
        st.detailRc = RECT{ x, y, x + w, y + h };
    }
    st.detailVisible = true;
    st.detailTexts.clear();
    for (py::handle h : texts)
    {
        py::sequence sq = py::reinterpret_borrow<py::sequence>(h);
        if (sq.size() < 8) continue;
        DetailText dt;
        const int x = py::cast<int>(sq[0]);
        const int y = py::cast<int>(sq[1]);
        const int w = py::cast<int>(sq[2]);
        const int h = py::cast<int>(sq[3]);
        dt.rc = RECT{ x, y, x + w, y + h };
        dt.text = py::cast<std::wstring>(sq[4]);
        dt.size = py::cast<int>(sq[5]);
        dt.bold = py::cast<bool>(sq[6]);
        py::sequence rgb = py::reinterpret_borrow<py::sequence>(sq[7]);
        const int r = py::cast<int>(rgb[0]);
        const int g = py::cast<int>(rgb[1]);
        const int b = py::cast<int>(rgb[2]);
        dt.color = RGB(r, g, b);
        st.detailTexts.push_back(std::move(dt));
    }
    if (HWND hw = findByIdDeep(m_hWnd, id))
        ::InvalidateRect(hw, nullptr, FALSE);
}

void PyMfcHost::clearDetailCard(int id)
{
    auto it = m_dwgCv.find(id);
    if (it == m_dwgCv.end()) return;
    DwgCanvasState& st = it->second;
    st.detailVisible = false;
    st.detailRc = RECT{ 0, 0, 0, 0 };
    st.detailTexts.clear();
    if (HWND hw = findByIdDeep(m_hWnd, id))
        ::InvalidateRect(hw, nullptr, FALSE);
}

bool PyMfcHost::detailHit(int id, int clientX, int clientY) const
{
    auto it = m_dwgCv.find(id);
    if (it == m_dwgCv.end() || !it->second.detailVisible) return false;
    const RECT& rc = it->second.detailRc;
    return clientX >= rc.left && clientX < rc.right && clientY >= rc.top && clientY < rc.bottom;
}

std::wstring PyMfcHost::dwgHit(int id, int clientX, int clientY)
{
        auto it = m_dwgCv.find(id);
        if (it == m_dwgCv.end() || it->second.mode != 1)
            return {};
        DwgCanvasState& st = it->second;
        if (detailHit(id, clientX, clientY))
            return {};
        const float mx = (static_cast<float>(clientX) - st.ox) / st.zoom;
        const float my = (static_cast<float>(clientY) - st.oy) / st.zoom;
        debugLogf(L"[dwg-hit] id=%d client=%d,%d zoom=%.4f off=%d,%d model=%.1f,%.1f boxes=%d",
                  id, clientX, clientY, st.zoom, st.ox, st.oy, mx, my,
                  static_cast<int>(st.plan.boxes.size()));
        for (const xmlplan::PowerBox& b : st.plan.boxes)
        {
            if (mx >= b.x && mx <= b.x + b.w && my >= b.y && my <= b.y + b.h)
                return b.code;
        }
        // 连线命中：贝塞尔折线到光标的最近距离（容差按屏幕 8px 折算成模型单位）。
        // 点中一条线 = 选中它的下游节点：卡片描边 + 父链边高亮 + 详情一起对上。
        const float tol = 8.0f / (st.zoom > 0.01f ? st.zoom : 0.01f);
        for (const xmlplan::PowerLink& l : st.plan.links)
        {
            // 与绘制路径（PyMfcCommon.h 折线）保持一致：4 个拐点直接连成正交折线，
            // 不再做贝塞尔采样；命中距离按折线各线段点到线段计算。
            std::vector<xmlgeom::Point2> pts;
            for (int q = 0; q < 4; ++q)
            {
                xmlgeom::Point2 p(l.bez[q * 2], l.bez[q * 2 + 1]);
                if (pts.empty() || pts.back().x != p.x || pts.back().y != p.y)
                    pts.push_back(p);
            }
            float best = 3.4e38f;
            for (size_t i = 1; i < pts.size(); ++i)
            {
                const float ax = static_cast<float>(pts[i - 1].x);
                const float ay = static_cast<float>(pts[i - 1].y);
                const float bx = static_cast<float>(pts[i].x);
                const float by = static_cast<float>(pts[i].y);
                const float vx = bx - ax, vy = by - ay;
                const float len2 = vx * vx + vy * vy;
                float tt = 0.0f;
                if (len2 > 1e-6f)
                    tt = ((mx - ax) * vx + (my - ay) * vy) / len2;
                if (tt < 0.0f) tt = 0.0f;
                else if (tt > 1.0f) tt = 1.0f;
                const float dx = mx - (ax + vx * tt);
                const float dy = my - (ay + vy * tt);
                const float d2 = dx * dx + dy * dy;
                if (d2 < best) best = d2;
            }
            if (best <= tol * tol)
            {
                debugLogf(L"[dwg-hit] 命中连线 %ls>%ls", l.from.c_str(), l.to.c_str());
                return l.to;
            }
        }
        return {};
    }

bool PyMfcHost::dwgCardRect(int id, const std::wstring& code, RECT& out) const
{
        out = RECT{0, 0, 0, 0};
        auto it = m_dwgCv.find(id);
        if (it == m_dwgCv.end() || it->second.mode != 1)
            return false;
        const DwgCanvasState& st = it->second;
        for (const xmlplan::PowerBox& b : st.plan.boxes)
        {
            if (_wcsicmp(b.code.c_str(), code.c_str()) != 0)
                continue;
            out.left = st.ox + static_cast<LONG>(lroundf(b.x * st.zoom));
            out.top = st.oy + static_cast<LONG>(lroundf(b.y * st.zoom));
            out.right = st.ox + static_cast<LONG>(lroundf((b.x + b.w) * st.zoom));
            out.bottom = st.oy + static_cast<LONG>(lroundf((b.y + b.h) * st.zoom));
            return true;
        }
        return false;
    }

bool PyMfcHost::saveControlPng(int id, const std::wstring& path)
{
        HWND c = findByIdDeep(m_hWnd, id);
        if (c == nullptr || !::IsWindow(c))
            return false;
        RECT wr = {};
        if (!::GetWindowRect(c, &wr))
            return false;
        const int w = wr.right - wr.left;
        const int hh = wr.bottom - wr.top;
        if (w <= 0 || hh <= 0)
            return false;

        HDC screen = ::GetDC(nullptr);
        HDC mem = ::CreateCompatibleDC(screen);
        BITMAPINFO bi = {};
        bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bi.bmiHeader.biWidth = w;
        bi.bmiHeader.biHeight = -hh;          // 自顶向下，与 wicWritePng 行序一致
        bi.bmiHeader.biPlanes = 1;
        bi.bmiHeader.biBitCount = 32;
        bi.bmiHeader.biCompression = BI_RGB;
        void* bits = nullptr;
        HBITMAP bmp = ::CreateDIBSection(screen, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
        HGDIOBJ oldBmp = (bmp != nullptr) ? ::SelectObject(mem, bmp) : nullptr;

        RECT full = { 0, 0, w, hh };
        HBRUSH bg = ::CreateSolidBrush(RGB(0xF3, 0xF5, 0xF8));
        ::FillRect(mem, &full, bg);
        ::DeleteObject(bg);

        // PW_RENDERFULLCONTENT(2)：内容渲染型窗口也能抓
        ::PrintWindow(c, mem, 2);
        ::GdiFlush();

        bool wrote = false;
        if (bits != nullptr)
            wrote = wicWritePng(path, w, hh, bits);

        if (oldBmp != nullptr)
            ::SelectObject(mem, oldBmp);
        if (bmp != nullptr)
            ::DeleteObject(bmp);
        ::DeleteDC(mem);
        ::ReleaseDC(nullptr, screen);
        return wrote;
    }

bool PyMfcHost::selectDwgItem(int id, const std::wstring& code)
{
        auto it = m_dwgCv.find(id);
        if (it == m_dwgCv.end() || it->second.mode != 1)
            return false;
        DwgCanvasState& st = it->second;
        const xmlplan::PowerBox* hit = nullptr;
        for (const xmlplan::PowerBox& b : st.plan.boxes)
        {
            if (_wcsicmp(b.code.c_str(), code.c_str()) == 0)
            {
                hit = &b;
                break;
            }
        }
        if (hit == nullptr)
            return false;

        st.sel = hit->code;
        RECT rc = {0, 0, 1, 1};
        HWND hwnd = findByIdDeep(m_hWnd, id);
        if (hwnd != nullptr)
            ::GetClientRect(hwnd, &rc);
        st.ox = static_cast<int>(lroundf(rc.right * 0.5f -
                                         (hit->x + hit->w * 0.5f) * st.zoom));
        st.oy = static_cast<int>(lroundf(rc.bottom * 0.5f -
                                         (hit->y + hit->h * 0.5f) * st.zoom));
        if (hwnd != nullptr)
            ::InvalidateRect(hwnd, nullptr, FALSE);
        return true;
    }

void PyMfcHost::fireRibbonButton(int id, const std::wstring& name)
{
    auto it = m_click.find(id);
    if (it == m_click.end() || !it->second) return;
    py::gil_scoped_acquire gil;
    try
    {
        std::string utf8;
        if (!name.empty())
        {
            const int n = ::WideCharToMultiByte(CP_UTF8, 0, name.c_str(),
                                                static_cast<int>(name.size()), nullptr, 0, nullptr, nullptr);
            if (n > 0)
            {
                utf8.resize(static_cast<size_t>(n));
                ::WideCharToMultiByte(CP_UTF8, 0, name.c_str(), static_cast<int>(name.size()),
                                      utf8.data(), n, nullptr, nullptr);
            }
        }
        it->second(py::str(utf8));
    }
    catch (py::error_already_set& e)
    {
        debugLogf(L"[ribbon] Python 回调抛异常: %hs", e.what());
        e.restore();
        PyErr_Print();
    }
    catch (...)
    {
        debugLogf(L"[ribbon] Python 回调抛未知异常");
    }
}

bool PyMfcHost::ribbonMouse(int id, UINT msg, int x, int y, WPARAM /*flags*/)
{
    auto it = m_ribbon.find(id);
    if (it == m_ribbon.end()) return false;
    RibbonState& st = it->second;
    HWND hw = findByIdDeep(m_hWnd, id);
    const POINT pt{ x, y };
    const xmlctl::XRibbonHit hit = xmlctl::ribbonHitTest(st.lay, pt);

    auto buttonName = [&]() -> std::wstring
    {
        const xmlctl::XRibbonTab* tab = st.model.current();
        if (tab == nullptr || hit.group < 0 || hit.group >= (int)tab->groups.size()) return {};
        const xmlctl::XRibbonGroup& g = tab->groups[(size_t)hit.group];
        if (hit.button < 0 || hit.button >= (int)g.buttons.size()) return {};
        return g.buttons[(size_t)hit.button].name;
    };

    switch (msg)
    {
    case WM_MOUSEMOVE:
        st.hotTab = hit.tab;
        st.hotBtn = (hit.button >= 0) ? buttonName() : std::wstring();
        if (hw != nullptr) ::InvalidateRect(hw, nullptr, FALSE);
        return true;
    case WM_LBUTTONDOWN:
        st.pressed = true;
        st.pressedBtn = (hit.button >= 0) ? buttonName() : std::wstring();
        if (hw != nullptr) { ::SetCapture(hw); ::InvalidateRect(hw, nullptr, FALSE); }
        return true;
    case WM_LBUTTONUP:
    {
        const bool wasPressed = st.pressed;
        st.pressed = false;
        st.pressedBtn.clear();
        if (hw != nullptr) ::ReleaseCapture();
        if (wasPressed)
        {
            if (hit.tab >= 0)
            {
                st.model.selected = hit.tab;
                st.hotTab = hit.tab;
            }
            else if (hit.button >= 0)
            {
                fireRibbonButton(id, buttonName());
            }
        }
        if (hw != nullptr) ::InvalidateRect(hw, nullptr, FALSE);
        return true;
    }
    case WM_MOUSELEAVE:
        st.hotTab = -1;
        st.hotBtn.clear();
        if (hw != nullptr) ::InvalidateRect(hw, nullptr, FALSE);
        return true;
    default:
        return true;
    }
}

std::wstring PyMfcHost::getFileTabInfo(int id) const
{
    auto it = m_fileTabs.find(id);
    if (it == m_fileTabs.end()) return L"{}";
    const FileTabState& st = it->second;
    std::wstring s = L"{";
    s += L"\"active\":" + std::to_wstring(st.model.activeIndex);
    s += L",\"showAdd\":" + std::wstring(st.model.showAdd ? L"true" : L"false");
    s += L",\"tabs\":[";
    for (size_t i = 0; i < st.lay.tabs.size(); ++i)
    {
        const RECT& r = st.lay.tabs[i];
        if (i) s += L",";
        s += L"[" + std::to_wstring(r.left) + L"," + std::to_wstring(r.top) + L"," +
             std::to_wstring(r.right - r.left) + L"," + std::to_wstring(r.bottom - r.top) + L"]";
    }
    s += L"],\"close\":[";
    for (size_t i = 0; i < st.lay.closeBoxes.size(); ++i)
    {
        const RECT& r = st.lay.closeBoxes[i];
        if (i) s += L",";
        s += L"[" + std::to_wstring(r.left) + L"," + std::to_wstring(r.top) + L"," +
             std::to_wstring(r.right - r.left) + L"," + std::to_wstring(r.bottom - r.top) + L"]";
    }
    s += L"]";
    if (st.lay.addRc.right > st.lay.addRc.left)
        s += L",\"add\":[" + std::to_wstring(st.lay.addRc.left) + L"," +
             std::to_wstring(st.lay.addRc.top) + L"," +
             std::to_wstring(st.lay.addRc.right - st.lay.addRc.left) + L"," +
             std::to_wstring(st.lay.addRc.bottom - st.lay.addRc.top) + L"]";
    s += L",\"scrollX\":" + std::to_wstring(st.scrollX);
    s += L",\"totalWidth\":" + std::to_wstring(st.lay.totalWidth);
    s += L"}";
    return s;
}

void PyMfcHost::fireFileTab(int id, const std::wstring& msg)
{
    auto it = m_click.find(id);
    if (it == m_click.end() || !it->second) return;
    py::gil_scoped_acquire gil;
    try
    {
        std::string utf8;
        if (!msg.empty())
        {
            const int n = ::WideCharToMultiByte(CP_UTF8, 0, msg.c_str(),
                                                static_cast<int>(msg.size()), nullptr, 0, nullptr, nullptr);
            if (n > 0)
            {
                utf8.resize(static_cast<size_t>(n));
                ::WideCharToMultiByte(CP_UTF8, 0, msg.c_str(), static_cast<int>(msg.size()),
                                      utf8.data(), n, nullptr, nullptr);
            }
        }
        it->second(py::str(utf8));
    }
    catch (py::error_already_set& e)
    {
        debugLogf(L"[filetab] Python 回调抛异常: %hs", e.what());
        e.restore();
        PyErr_Print();
    }
    catch (...)
    {
        debugLogf(L"[filetab] Python 回调抛未知异常");
    }
}

bool PyMfcHost::fileTabMouse(int id, UINT msg, int x, int y, WPARAM flags)
{
    auto it = m_fileTabs.find(id);
    if (it == m_fileTabs.end()) return false;
    FileTabState& st = it->second;
    HWND hw = findByIdDeep(m_hWnd, id);
    const POINT pt{ x, y };
    const xmlctl::FileTabHit hit = xmlctl::filetabHitTest(st.lay, pt);

    switch (msg)
    {
    case WM_MOUSEMOVE:
        st.hotTab   = (hit.region == 1) ? hit.tab : -1;
        st.hotClose = (hit.region == 2) ? hit.tab : -1;
        st.hotAdd   = (hit.region == 3);
        if (hw != nullptr) ::InvalidateRect(hw, nullptr, FALSE);
        return true;
    case WM_LBUTTONDOWN:
        st.pressed = true;
        if (hw != nullptr) { ::SetCapture(hw); ::InvalidateRect(hw, nullptr, FALSE); }
        return true;
    case WM_LBUTTONUP:
    {
        const bool wasPressed = st.pressed;
        st.pressed = false;
        if (hw != nullptr) ::ReleaseCapture();
        if (wasPressed)
        {
            if (hit.region == 3)
            {
                fireFileTab(id, L"add");
            }
            else if (hit.region == 2 && hit.tab >= 0)
            {
                fireFileTab(id, L"close:" + std::to_wstring(hit.tab));
            }
            else if (hit.region == 1 && hit.tab >= 0)
            {
                st.model.activeIndex = hit.tab;
                st.hotTab = hit.tab;
                fireFileTab(id, L"activate:" + std::to_wstring(hit.tab));
            }
        }
        if (hw != nullptr) ::InvalidateRect(hw, nullptr, FALSE);
        return true;
    }
    case WM_MOUSEWHEEL:
    {
        const int notches = GET_WHEEL_DELTA_WPARAM(flags) / WHEEL_DELTA;
        st.scrollX -= notches * 24;
        if (st.scrollX < 0) st.scrollX = 0;
        if (hw != nullptr) ::InvalidateRect(hw, nullptr, FALSE);
        return true;
    }
    case WM_MOUSELEAVE:
        st.hotTab = -1;
        st.hotClose = -1;
        st.hotAdd = false;
        if (hw != nullptr) ::InvalidateRect(hw, nullptr, FALSE);
        return true;
    default:
        return true;
    }
}

void PyMfcHost::fireDwgSelect(int id, const std::wstring& /*code*/)
{
        auto it = m_click.find(id);
        if (it != m_click.end())
            invokePython(it->second);
    }

void PyMfcHost::addView3D(int id, int x, int y, int w, int h)
{
    m_view3D[id];
    HWND hwnd = createControl(L"STATIC", L"", SS_OWNERDRAW, x, y, w, h, id);
    if (hwnd != nullptr)
    {
        ::SetPropW(hwnd, kView3DHostProp, reinterpret_cast<HANDLE>(this));
        View3DState& st = m_view3D[id];
        if (!st.prims.empty())
            view3DFit(id);
        debugLogf(L"[view3d] id=%d hwnd=%p prims=%d yaw=%.1f pitch=%.1f",
                  id, static_cast<void*>(hwnd), static_cast<int>(st.prims.size()),
                  st.yaw, st.pitch);
    }
}

void PyMfcHost::setView3DModel(int id, const std::wstring& geom)
{
    View3DState& st = m_view3D[id];
    st.src = geom;
    st.prims.clear();

    size_t pos = 0;
    while (pos < geom.size())
    {
        while (pos < geom.size() && (geom[pos] == L';' || geom[pos] == L'\n' ||
                                     geom[pos] == L'\r' || geom[pos] == L' ' || geom[pos] == L'\t'))
            ++pos;
        if (pos >= geom.size()) break;
        size_t end = pos;
        while (end < geom.size() && geom[end] != L';' && geom[end] != L'\n' && geom[end] != L'\r')
            ++end;
        const std::wstring rec = geom.substr(pos, end - pos);
        pos = end;

        std::wstringstream ss(rec);
        std::wstring head;
        ss >> head;
        V3Prim pr;
        bool ok = true;
        int need = 0;
        if (head == L"box")
        {
            pr.kind = 0;
            need = 6;
        }
        else if (head == L"line")
        {
            pr.kind = 1;
            need = 6;
        }
        else if (head == L"tri")
        {
            pr.kind = 2;
            need = 9;
        }
        else
            ok = false;
        for (int i = 0; ok && i < need; ++i)
        {
            std::wstring tok;
            if (!(ss >> tok)) { ok = false; break; }
            wchar_t* endp = nullptr;
            const double v = wcstod(tok.c_str(), &endp);
            if (endp == tok.c_str()) { ok = false; break; }
            pr.p[i] = static_cast<float>(v);
        }
        std::wstring col;
        ss >> col;
        if (ok && !col.empty())
            v3ParseColor(col, pr.color);
        if (ok && pr.kind == 1)
        {
            std::wstring wtok;
            if (ss >> wtok)
                pr.width = static_cast<float>(wcstod(wtok.c_str(), nullptr));
        }
        if (ok)
            st.prims.push_back(pr);
    }

    // 包围盒
    float lo[3] = { 1e18f, 1e18f, 1e18f }, hi[3] = { -1e18f, -1e18f, -1e18f };
    auto acc = [&](float x, float y, float z) {
        lo[0] = (std::min)(lo[0], x); hi[0] = (std::max)(hi[0], x);
        lo[1] = (std::min)(lo[1], y); hi[1] = (std::max)(hi[1], y);
        lo[2] = (std::min)(lo[2], z); hi[2] = (std::max)(hi[2], z);
    };
    for (const V3Prim& pr : st.prims)
    {
        if (pr.kind == 0)
        {
            acc(pr.p[0], pr.p[1], pr.p[2]);
            acc(pr.p[3], pr.p[4], pr.p[5]);
        }
        else if (pr.kind == 1)
        {
            acc(pr.p[0], pr.p[1], pr.p[2]);
            acc(pr.p[3], pr.p[4], pr.p[5]);
        }
        else
        {
            acc(pr.p[0], pr.p[1], pr.p[2]);
            acc(pr.p[3], pr.p[4], pr.p[5]);
            acc(pr.p[6], pr.p[7], pr.p[8]);
        }
    }
    if (!st.prims.empty())
    {
        for (int i = 0; i < 3; ++i) { st.lo[i] = lo[i]; st.hi[i] = hi[i]; }
    }
    st.empty = st.prims.empty();
    st.autoFit = true;
    if (!st.empty)
        view3DFit(id);
    debugLogf(L"[view3d] id=%d prims=%d bbox=(%.1f,%.1f,%.1f)-(%.1f,%.1f,%.1f)",
              id, static_cast<int>(st.prims.size()), st.lo[0], st.lo[1], st.lo[2],
              st.hi[0], st.hi[1], st.hi[2]);
    ::InvalidateRect(findByIdDeep(m_hWnd, id), nullptr, TRUE);
}

void PyMfcHost::view3DAddRecord(int id, const std::wstring& rec)
{
    View3DState& st = m_view3D[id];
    st.src += rec;
    if (st.src.size() > 32u * 1024u * 1024u)
        st.src = rec;
    setView3DModel(id, st.src);
}

void PyMfcHost::view3DFit(int id)
{
    auto it = m_view3D.find(id);
    if (it == m_view3D.end()) return;
    View3DState& st = it->second;
    st.yaw = 35.0f;
    st.pitch = 22.0f;
    st.zoom = 1.0f;
    st.panX = st.panY = 0;
    st.autoFit = true;
    ::InvalidateRect(findByIdDeep(m_hWnd, id), nullptr, TRUE);
}

void PyMfcHost::setView3DCamera(int id, float yaw, float pitch, float zoom, int panX, int panY)
{
    View3DState& st = m_view3D[id];
    st.yaw = yaw;
    st.pitch = (std::max)(-89.0f, (std::min)(89.0f, pitch));
    st.zoom = (std::max)(0.05f, (std::min)(40.0f, zoom));
    st.panX = panX;
    st.panY = panY;
    st.autoFit = false;
    ::InvalidateRect(findByIdDeep(m_hWnd, id), nullptr, TRUE);
}

void PyMfcHost::setView3DStyle(int id, COLORREF bg, bool axes, bool grid)
{
    View3DState& st = m_view3D[id];
    st.bg = bg;
    st.axes = axes;
    st.grid = grid;
    ::InvalidateRect(findByIdDeep(m_hWnd, id), nullptr, TRUE);
}

bool PyMfcHost::setImage(int id, const std::wstring& path)
{
        m_images[id] = path;
        // 路径解析顺序：原样（绝对/相对当前目录）→ pyd 所在目录（与 Icon="res/x.png" 一致）。
        // 只按后一种解析会让 XML 里写的 res/xxx.png 找不到文件（XML 旁边才有 res/）。
        std::wstring full = path;
        if (!full.empty() && !(full.size() > 2 && full[1] == L':') &&
            full[0] != L'\\' && full[0] != L'/')
        {
            if (::GetFileAttributesW(full.c_str()) == INVALID_FILE_ATTRIBUTES)
            {
                const std::wstring alt = pymfcDir() + L"\\" + full;
                if (::GetFileAttributesW(alt.c_str()) != INVALID_FILE_ATTRIBUTES)
                    full = alt;
            }
        }
        HWND h = findByIdDeep(m_hWnd, id);
        if (h == nullptr)
            return false;
        // 自绘图片盒子：不装载 HBITMAP，只在自绘时按 object-fit 用 WIC 解码这张图。
        if (m_imageBoxes.count(id) != 0)
        {
            setControlIcon(id, full);            // 外观通道也要知道这张图（背景图/图标复用）
            ::InvalidateRect(h, nullptr, TRUE);
            ::UpdateWindow(h);
            return !full.empty();
        }
        HBITMAP bmp = full.empty() ? nullptr : loadBitmapFromFile(full);
        wchar_t cls[64] = {};
        ::GetClassNameW(h, cls, 64);
        if (_wcsicmp(cls, L"Static") == 0)
            ::SendMessageW(h, STM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>(bmp));
        else if (_wcsicmp(cls, L"Button") == 0)
        {
            if (bmp != nullptr)
                ::SetWindowLongPtrW(h, GWL_STYLE,
                                    ::GetWindowLongPtrW(h, GWL_STYLE) | BS_BITMAP);
            ::SendMessageW(h, BM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>(bmp));
        }
        // 自绘画布那条通路也要知道这张图，否则 owner-draw 的外观里仍然没图
        setControlIcon(id, full);
        if (bmp != nullptr)
        {
            auto it = m_ownedBitmaps.find(id);
            if (it != m_ownedBitmaps.end() && it->second != nullptr && it->second != bmp)
                ::DeleteObject(it->second);
            m_ownedBitmaps[id] = bmp;
        }
        ::InvalidateRect(h, nullptr, TRUE);
        ::UpdateWindow(h);
        return bmp != nullptr;
    }

void PyMfcHost::setImageFit(int id, const std::wstring& fit)
{
        auto it = m_imageBoxes.find(id);
        if (it == m_imageBoxes.end()) return;
        std::wstring lc = fit;
        for (auto& ch : lc)
            if (ch >= L'A' && ch <= L'Z') ch = static_cast<wchar_t>(ch + 32);
        it->second = lc;
        HWND h = findByIdDeep(m_hWnd, id);
        if (h != nullptr) ::InvalidateRect(h, nullptr, TRUE);
    }

void PyMfcHost::setImagePosition(int id, const std::wstring& pos)
{
        if (m_imageBoxes.count(id) == 0) return;
        m_imagePos[id] = pos;
        HWND h = findByIdDeep(m_hWnd, id);
        if (h != nullptr) ::InvalidateRect(h, nullptr, TRUE);
    }
