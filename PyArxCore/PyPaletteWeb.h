#pragma once
//-----------------------------------------------------------------------------
// PyPaletteWeb.h — 侧栏页的 Web 宿主。
//
// 取代原「PyMfcCore.guiload(XAML)」那条链路：页窗口的客户区整块交给
// MFCWebView 的 WebShell 渲染，页内控件全部由 HTML 画，MFC 只留窗口外壳。
// 一个页面一个实例；按「侧栏名 + 页名」复用，避免 arxload 累积。
// 编码：UTF-8 带 BOM。
//-----------------------------------------------------------------------------

#include "stdafx.h"

#include <memory>
#include <string>

namespace webmfc
{
    class WebShell;   // MFCWebView 的壳，头文件不把 WebView 依赖外溢到 PyArxCore
}

namespace PyArx
{
    class PyPaletteImpl;

    class PyPaletteWeb
    {
    public:
        PyPaletteWeb();
        ~PyPaletteWeb();

        // 把页窗口（CAdUiPalette）整块客户区交给 WebView
        bool attachPage(PyPaletteImpl* page, std::wstring& err);

        // 载入本地 HTML 文件
        bool loadPage(const std::wstring& htmlPath, std::wstring& err);

        // 往页面推 JSON 消息 / 执行 JS
        bool emit(const std::wstring& json);
        bool eval(const std::wstring& js);

        void detach();
        bool valid() const;

    private:
        // 页面发上来的 cad.* 命令：直接调 ObjectARX，回执用 cad.result 推回页面
        // onCommand 只做异常兜底，真正的路由在 onCommandInner
        bool onCommand(const std::wstring& json);
        bool onCommandInner(const std::wstring& json);
        void reply(bool ok, const std::wstring& text);

        std::unique_ptr<webmfc::WebShell> m_shell;
    };
}
