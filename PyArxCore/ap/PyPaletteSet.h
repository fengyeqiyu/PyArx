#pragma once
//-----------------------------------------------------------------------------
// PyPaletteSet.h — PyArxCore 的侧栏（可停靠面板）封装。
// 依赖 ObjectARX 原生 AcUi 接口（CAdUiPaletteSet / CAdUiPalette），
// 属 CAD 应用级能力，故收进 PyArxCore（而非 GUI 层 PyMfc）。
//
// 对齐 PyMfc 的 PyMfcPaletteSet（已迁出 GUI 层）：
//   Create() -> new CAdUiPaletteSet + Create(name, style, rect, frame, PSS_*)
//   AddPage(title) -> CAdUiPalette 页 + 控件宿主（宿主由宿主层提供）
//   Show/Hide -> frame->ShowControlBar
//   GetOrCreatePalette(name,w,h) -> 按名复用（防 arxload 累积）
//
// 说明：侧栏页内的控件宿主由 GUI 层（MfcXml）创建后回填（见 PyPageHandle），
// PyArxCore 自身不创建任何宿主，也不假设宿主用什么渲染。
// 编码：UTF-8 带 BOM。
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include <adui.h>
#include <aduiPaletteSet.h>
#include <aduiPalette.h>
#include <aced.h>
#include <rxmfcapi.h>

#include <map>
#include <string>
#include <vector>

namespace PyArx
{
    // 页内控件宿主句柄：由宿主层（MfcXml 等）创建并回填，
    // PyArxCore 只负责把它挂到 CAdUiPalette 上，不创建宿主。
    struct PyPageHandle
    {
        HWND hwnd = nullptr;          // 宿主窗口 HWND（供 CAdUiPalette 挂载）
    };

    class PyPaletteImpl : public CAdUiPalette
    {
        DECLARE_DYNAMIC(PyPaletteImpl)
    public:
        PyPaletteImpl() = default;
        ~PyPaletteImpl() override
        {
            // 宿主由 GUI 层所有，这里只销毁 CAdUiPalette 自身。
        }

        std::wstring Title() const { return m_title; }
        void SetTitle(const std::wstring& title) { m_title = title; }

        void SetHost(HWND hwnd) { m_hostHwnd = hwnd; }
        HWND HostHwnd() const { return m_hostHwnd; }

        // 页窗口本身：宿主层可把任意 HWND 挂到这块客户区上（默认空置）
        HWND Hwnd() const { return GetSafeHwnd(); }

        void ResizeHost()
        {
            if (m_hostHwnd == nullptr || !::IsWindow(m_hostHwnd))
                return;
            CRect rc;
            GetClientRect(&rc);
            ::MoveWindow(m_hostHwnd, 0, 0, rc.Width(), rc.Height(), TRUE);
        }

    protected:
        afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
        afx_msg void OnSize(UINT nType, int cx, int cy);
        DECLARE_MESSAGE_MAP()

    private:
        std::wstring m_title;
        HWND m_hostHwnd = nullptr;
    };

    // 侧栏外壳：在 CAdUiPaletteSet 之上加「尺寸上限」。
    // CAD 停靠布局会把侧栏纵向拉满整个框架高度，内容只占顶部一小块时
    // 就变成一条整屏高的空白侧栏；这里把最大尺寸收敛到调用方声明的宽高。
    class PyPaletteSetImpl : public CAdUiPaletteSet
    {
        DECLARE_DYNAMIC(PyPaletteSetImpl)
    public:
        PyPaletteSetImpl() = default;

        void SetSizeLimit(int width, int height)
        {
            m_maxWidth = width;
            m_maxHeight = height;
        }

        int MaxWidth() const { return m_maxWidth; }
        int MaxHeight() const { return m_maxHeight; }

    protected:
        // 布局引擎按此上限裁剪侧栏尺寸。
        // 注意：只在「浮动」时把宽高都锁到声明尺寸；一旦停靠，长边必须交给停靠站点
        // 拉满（左/右停靠拉高、上/下停靠拉宽），否则侧栏只占一小块、下面整片空白，
        // 网页也就跟着停在 340×535 不再长高。
        void GetMaximumSize(CSize& size) override
        {
            CAdUiPaletteSet::GetMaximumSize(size);
            if (!IsFloating())
                return;
            if (m_maxWidth > 0 && (size.cx <= 0 || size.cx > m_maxWidth))
                size.cx = m_maxWidth;
            if (m_maxHeight > 0 && (size.cy <= 0 || size.cy > m_maxHeight))
                size.cy = m_maxHeight;
        }

        // 停靠时 MFC 通过 CalcFixedLayout 问「这条 bar 有多长」。
        // bHorz=TRUE 表示停靠在上下边：厚度（高）锁声明值、长度（宽）交给基类拉满；
        // bHorz=FALSE 表示停在左右边：厚度（宽）锁声明值、长度（高）交给基类拉满。
        CSize CalcFixedLayout(BOOL bStretch, BOOL bHorz) override
        {
            CSize size = CAdUiPaletteSet::CalcFixedLayout(bStretch, bHorz);
            if (bHorz)
            {
                if (m_maxHeight > 0)
                    size.cy = m_maxHeight;
            }
            else
            {
                if (m_maxWidth > 0)
                    size.cx = m_maxWidth;
            }
            return size;
        }

        void GetMinimumSize(CSize& size) override
        {
            CAdUiPaletteSet::GetMinimumSize(size);
            const int minWidth = 180;
            const int minHeight = 220;
            if (size.cx < minWidth)
                size.cx = minWidth;
            if (size.cy < minHeight)
                size.cy = minHeight;
        }

    private:
        int m_maxWidth = 0;
        int m_maxHeight = 0;
    };

    class PyPaletteSet
    {
    public:
        PyPaletteSet(const std::wstring& name, int width, int height)
            : m_name(name), m_width(width), m_height(height)
        {
        }

        ~PyPaletteSet()
        {
            // 由进程级注册表持有到 CAD 进程结束
        }

        const std::wstring& Name() const { return m_name; }

        // 创建原生侧栏（幂等）
        bool Create();

        // 添加一页，返回页对象（页内控件宿主由 GUI 层回填）
        PyPaletteImpl* AddPage(const std::wstring& title);

        void Show();
        void Hide();

        CAdUiPaletteSet* Impl() const { return m_impl; }
        int PageCount() const { return static_cast<int>(m_pages.size()); }

    private:
        std::wstring m_name;
        int m_width = 280;
        int m_height = 600;
        PyPaletteSetImpl* m_impl = nullptr;
        std::vector<PyPaletteImpl*> m_pages;
        std::vector<std::wstring> m_pageTitles;
    };

    // 进程级注册表：按名复用侧栏（防 arxload 累积）
    class PyPaletteRegistry
    {
    public:
        static PyPaletteRegistry& Get()
        {
            static PyPaletteRegistry inst;
            return inst;
        }

        PyPaletteSet* GetOrCreate(const std::wstring& name, int width, int height)
        {
            auto it = m_map.find(name);
            if (it != m_map.end())
                return it->second.get();
            auto p = std::make_unique<PyPaletteSet>(name, width, height);
            PyPaletteSet* raw = p.get();
            m_map[name] = std::move(p);
            return raw;
        }

        void DestroyAll()
        {
            m_map.clear();
        }

    private:
        std::map<std::wstring, std::unique_ptr<PyPaletteSet>> m_map;
    };
}

void bind_PyPaletteSet(pym& m);
