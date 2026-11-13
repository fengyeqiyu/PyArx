//-----------------------------------------------------------------------------
// PyPaletteSet.cpp — PyArxCore 的侧栏（可停靠面板）实现。
// 依赖 ObjectARX 原生 AcUi 接口（CAdUiPaletteSet / CAdUiPalette），
// 属 CAD 应用级能力，故收进 PyArxCore（而非 GUI 层 PyMfc）。
//
// 页内控件宿主：由 GUI 层（MfcXaml）创建并回填，PyArxCore 只负责把宿主窗口
// 挂到 CAdUiPalette 上，自身不创建任何渲染宿主。
// 编码：UTF-8 带 BOM。
//-----------------------------------------------------------------------------

#include "PyPaletteSet.h"

#include <stdexcept>
#include <thread>

namespace PyArx
{
    IMPLEMENT_DYNAMIC(PyPaletteImpl, CAdUiPalette)
    IMPLEMENT_DYNAMIC(PyPaletteSetImpl, CAdUiPaletteSet)

    BEGIN_MESSAGE_MAP(PyPaletteImpl, CAdUiPalette)
        ON_WM_CREATE()
        ON_WM_SIZE()
    END_MESSAGE_MAP()

    static bool ClassStartsWith(HWND wnd, const char* prefix)
    {
        char cls[128] = { 0 };
        ::GetClassNameA(wnd, cls, _countof(cls));
        return ::strncmp(cls, prefix, ::strlen(prefix)) == 0;
    }

    // 竖直停靠态钉尺寸：只钉「厚度」（宽度 = 声明宽度），长度（高度）跟随停靠条客户区。
    //
    // 历史：以前这里把条子按「声明宽 × 声明高」整块钉死，于是 CAD 停靠后条子明明
    // 占到 1268 高，内容窗却只有 562，网页下面留一大片空白——用户看到的正是这个。
    // 停靠的语义应该是：厚度固定、长度拉满；浮动才用声明尺寸。
    static void PinDockedSizeAsync(HWND setWnd, int width, int fallbackHeight)
    {
        if (setWnd == nullptr || !::IsWindow(setWnd))
            return;

        std::thread([setWnd, width, fallbackHeight]()
        {
            for (int i = 0; i < 60; ++i)
            {
                ::Sleep(200);
                if (!::IsWindow(setWnd))
                    return;

                HWND root = ::GetAncestor(setWnd, GA_ROOT);
                if (root == nullptr || !::IsWindow(root))
                    return;
                if (!ClassStartsWith(root, "AfxMDIFrame") && !ClassStartsWith(root, "AfxFrame"))
                    return;   // 已变成浮动，交给浮动路径

                // 停靠条是外层 control bar；它的客户区高度就是停靠站点给的可用长度
                HWND bar = ::GetParent(setWnd);
                int availHeight = 0;
                if (bar != nullptr && ::IsWindow(bar))
                {
                    RECT rcClient{};
                    ::GetClientRect(bar, &rcClient);
                    availHeight = rcClient.bottom - rcClient.top;
                }

                // 条子的位置由 CAD 布局算（父窗口是框架，用父客户区坐标），这里只压宽度
                ::SetWindowPos(setWnd, nullptr, 0, 0, width,
                               availHeight > 0 ? availHeight : fallbackHeight,
                               SWP_NOZORDER | SWP_NOACTIVATE);

                if (i % 5 == 0)
                {
                    RECT rcBar{};
                    if (bar != nullptr && ::IsWindow(bar))
                        ::GetWindowRect(bar, &rcBar);
                    FILE* fp = ::fopen("G:\\PyArx\\docs\\_pal_fix.log", "a");
                    if (fp != nullptr)
                    {
                        ::fprintf(fp, "dock i=%d width=%d availH=%d bar=%ld,%ld %ldx%ld\n",
                                  i, width, availHeight,
                                  rcBar.left, rcBar.top,
                                  rcBar.right - rcBar.left, rcBar.bottom - rcBar.top);
                        ::fclose(fp);
                    }
                }
            }
        }).detach();
    }

    // CAD 在 Show() 之后会再排两次：mini frame 先被按传入点翻倍摆放，约 10 秒后
    // 又挪回正确位置；同时 control bar 的客户区坐标被写成屏幕坐标，于是 bar 落在
    // 框外（表现为一片空白）。这里在工作线程里持续钉住
    // 「mini frame 在 (x,y)、control bar 在其客户区 (0,0)」这一状态。
    static void CorrectFloatPositionAsync(HWND setWnd, int x, int y, int width, int height)
    {
        if (setWnd == nullptr || !::IsWindow(setWnd))
            return;

        std::thread([setWnd, x, y, width, height]()
        {
            int borderDx = -1;
            int borderDy = -1;

            for (int i = 0; i < 80; ++i)
            {
                ::Sleep(200);
                if (!::IsWindow(setWnd))
                    return;

                // 侧栏整体所在的顶层窗口（浮动时的 mini frame）
                HWND mini = ::GetAncestor(setWnd, GA_ROOT);
                if (mini == nullptr || !::IsWindow(mini))
                    return;

                // 用户把侧栏拖回停靠位时 GA_ROOT 变成 CAD 主框架，立刻收手，
                // 否则会把主框架当成侧栏去摆位。
                if (ClassStartsWith(mini, "AfxMDIFrame") || ClassStartsWith(mini, "AfxFrame"))
                    return;

                RECT rcMini;
                ::GetWindowRect(mini, &rcMini);
                if (borderDx < 0)
                {
                    borderDx = (rcMini.right - rcMini.left) - width;
                    borderDy = (rcMini.bottom - rcMini.top) - height;
                    if (borderDx < 0)
                        borderDx = 0;
                    if (borderDy < 0)
                        borderDy = 0;
                }

                if (rcMini.left != x || rcMini.top != y ||
                    (rcMini.right - rcMini.left) != width + borderDx ||
                    (rcMini.bottom - rcMini.top) != height + borderDy)
                {
                    ::SetWindowPos(mini, nullptr, x, y, width + borderDx, height + borderDy,
                                   SWP_NOZORDER | SWP_NOACTIVATE);
                }

                // mini frame 下的 control bar 必须回到客户区原点
                HWND child = ::GetWindow(mini, GW_CHILD);
                while (child != nullptr)
                {
                    if (::IsWindow(child) && ClassStartsWith(child, "AfxControlBar"))
                    {
                        ::SetWindowPos(child, nullptr, 0, 0, width, height,
                                       SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
                    }
                    child = ::GetWindow(child, GW_HWNDNEXT);
                }

                if (i % 10 == 0)
                {
                    RECT rcBar;
                    ::GetWindowRect(setWnd, &rcBar);
                    FILE* fp = ::fopen("G:\\PyArx\\docs\\_pal_fix.log", "a");
                    if (fp != nullptr)
                    {
                        ::fprintf(fp, "i=%d miniHwnd=%p mini=%ld,%ld %ldx%ld set=%ld,%ld %ldx%ld\n",
                                  i, (void*)mini,
                                  rcMini.left, rcMini.top,
                                  rcMini.right - rcMini.left, rcMini.bottom - rcMini.top,
                                  rcBar.left, rcBar.top,
                                  rcBar.right - rcBar.left, rcBar.bottom - rcBar.top);
                        ::fclose(fp);
                    }
                }
            }
        }).detach();
    }


    int PyPaletteImpl::OnCreate(LPCREATESTRUCT lpCreateStruct)
    {
        AFX_MANAGE_STATE(AfxGetAppModuleState());
        if (CAdUiPalette::OnCreate(lpCreateStruct) == -1)
            return -1;
        if (lpCreateStruct != nullptr && lpCreateStruct->lpszName != nullptr)
            m_title = lpCreateStruct->lpszName;
        return 0;
    }

    void PyPaletteImpl::OnSize(UINT nType, int cx, int cy)
    {
        CAdUiPalette::OnSize(nType, cx, cy);
        ResizeHost();
    }

    bool PyPaletteSet::Create()
    {
        AFX_MANAGE_STATE(AfxGetAppModuleState());
        if (m_impl != nullptr)
            return true;

        CMDIFrameWnd* frame = static_cast<CMDIFrameWnd*>(::acedGetAcadFrame());
        if (frame == nullptr)
            return false;

        auto* impl = new PyPaletteSetImpl();
        impl->SetSizeLimit(m_width, m_height);
        // PSS_SINGLE_COL_VERT_DOCK / PSS_SINGLE_ROW_NOVERTRESIZE 是 adui 内部的位，
        // 加了也压不动竖直停靠高度，还挡住手动纵向拖，所以不用。
        if (!impl->Create(m_name.c_str(), WS_OVERLAPPED | WS_DLGFRAME,
                          CRect(0, 0, m_width, m_height), frame,
                          PSS_AUTO_ROLLUP | PSS_CLOSE_BUTTON | PSS_SNAP))
        {
            delete impl;
            return false;
        }
        impl->EnableDocking(CBRS_ALIGN_ANY);
        impl->SetName(m_name.c_str());
        m_impl = impl;
        return true;
    }

    PyPaletteImpl* PyPaletteSet::AddPage(const std::wstring& title)
    {
        // 重新 arxload 后复用同名页面，避免 tab 与控件累积
        for (size_t i = 0; i < m_pages.size(); ++i)
        {
            if (m_pageTitles[i] == title)
                return m_pages[i];
        }

        if (!Create() || m_impl == nullptr)
            return nullptr;

        auto* page = new PyPaletteImpl();
        page->SetTitle(title);
        if (!page->Create(WS_CHILD | WS_VISIBLE, title.c_str(), m_impl, 0))
        {
            delete page;
            return nullptr;
        }
        if (m_impl->AddPalette(page) == -1)
        {
            delete page;
            return nullptr;
        }

        m_pages.push_back(page);
        m_pageTitles.push_back(title);
        return page;
    }

    void PyPaletteSet::Show()
    {
        AFX_MANAGE_STATE(AfxGetAppModuleState());
        if (m_impl == nullptr)
            return;
        CMDIFrameWnd* frame = static_cast<CMDIFrameWnd*>(::acedGetAcadFrame());
        if (frame == nullptr)
            return;

        frame->ShowControlBar(m_impl, TRUE, FALSE);

        // 停靠尺寸：厚度用声明值，长度交给停靠站点。
        // CAD 内部用 m_VertDockSize / m_HorzDockSize 记竖直、水平停靠时的高宽，
        // CalcFixedLayout 会读这两个字段，必须在恢复停靠之前写进去。
        // 竖直停靠（左/右）的高度取框架客户区高度——「停靠后拉满」正是要的效果；
        // 水平停靠（上/下）才用声明的厚度。
        CRect frameClient;
        frame->GetClientRect(&frameClient);
        m_impl->SetHorzDockSize(CSize(m_width, m_height));
        m_impl->SetVertDockSize(CSize(m_width, frameClient.Height()));

        // 默认形态是 CAD 原生竖直停靠（左侧）。RestoreControlBar 走 CAD 自己
        // 记录的停靠矩形（用户手动停靠过一次就会记住旧尺寸）。
        m_impl->RestoreControlBar(AFX_IDW_DOCKBAR_LEFT, nullptr);

        // 再用显式停靠矩形摆一次：竖向停靠时 pRect 的高度就是条子长度，
        // 这里直接给满框架客户区，避免恢复出旧的 340×560。
        CRect rcDock(0, 0, m_width, frameClient.Height());
        m_impl->DockControlBar(AFX_IDW_DOCKBAR_LEFT, &rcDock);

        // CAD 每次 RecalcLayout 都可能把条子/面板摆回旧尺寸，摆好后持续把宽度钉住
        // （高度由 PinDockedSizeAsync 每轮按停靠条客户区取，不再写死）。
        PinDockedSizeAsync(m_impl->GetSafeHwnd(), m_width, m_height + 2);

        // 停靠/浮动切换后页窗口父窗口可能变化，Web 宿主尺寸按客户区重算
        for (auto* page : m_pages)
        {
            if (!::IsWindow(page->GetSafeHwnd()))
                continue;
            page->ResizeHost();
            page->RedrawWindow(nullptr, nullptr,
                               RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_UPDATENOW);
        }
    }

    void PyPaletteSet::Hide()
    {
        AFX_MANAGE_STATE(AfxGetAppModuleState());
        if (m_impl == nullptr)
            return;
        CMDIFrameWnd* frame = static_cast<CMDIFrameWnd*>(::acedGetAcadFrame());
        if (frame != nullptr)
            frame->ShowControlBar(m_impl, FALSE, FALSE);
    }
} // namespace PyArx

// ===========================================================================
// 绑定
// ===========================================================================
void bind_PyPaletteSet(pym& m)
{
    namespace pa = PyArx;

    py::class_<pa::PyPaletteImpl>(m, "Palette")
        .def_property_readonly("title", &pa::PyPaletteImpl::Title)
        .def_property_readonly("hwnd", [](pa::PyPaletteImpl& p) {
            return static_cast<long long>(reinterpret_cast<std::intptr_t>(p.Hwnd()));
        }, "页窗口句柄：宿主窗口挂到这里（MfcXaml 侧传 host.addXxx 的父窗口）")
        .def("setHost", [](pa::PyPaletteImpl& p, long long hwnd) {
            p.SetHost(reinterpret_cast<HWND>(hwnd));
        }, py::arg("hwnd"))
        .def("getHost", [](pa::PyPaletteImpl& p) {
            return static_cast<long long>(reinterpret_cast<std::intptr_t>(p.HostHwnd()));
        })
        .def("resizeHost", &pa::PyPaletteImpl::ResizeHost);

    py::class_<pa::PyPaletteSet>(m, "PaletteSet")
        .def_property_readonly("name", &pa::PyPaletteSet::Name)
        .def("create", &pa::PyPaletteSet::Create)
        .def("addPage", &pa::PyPaletteSet::AddPage, py::arg("title"),
             py::return_value_policy::reference)
        .def("show", &pa::PyPaletteSet::Show)
        .def("hide", &pa::PyPaletteSet::Hide)
        .def_property_readonly("pageCount", &pa::PyPaletteSet::PageCount);

    // 模块级工厂 + 注册表
    m.def("getPalette", [](const std::wstring& name, int width, int height) {
        return pa::PyPaletteRegistry::Get().GetOrCreate(name, width, height);
    }, py::arg("name"), py::arg("width") = 280, py::arg("height") = 600,
       py::return_value_policy::reference,
       "按名获取/创建侧栏（复用同名实例，防 arxload 累积）");

    m.def("destroyAllPalettes", []() {
        pa::PyPaletteRegistry::Get().DestroyAll();
    }, "销毁全部侧栏（一般只在 arxunload 时调用）");
}
