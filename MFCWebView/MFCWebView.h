#pragma once
// MFCWebView 统一入口：MFC 提供窗口外壳，客户区整块交给 web 渲染。
//
//   #include "MFCWebView.h"
//   m_shell.attach(m_hWnd, L"", err);     // 宿主窗口客户区全部交给 WebView2
//   m_shell.loadPage(L"ui\\index.html");
//
// 页面侧：
//   chrome.webview.postMessage({ cmd: 'window.drag' })            // 自绘标题条拖动
//   chrome.webview.postMessage({ cmd: 'dialog.open', filter: 'XAML|*.xaml', id: 'open1' })
//   chrome.webview.addEventListener('message', e => { ... })      // 收 shell.ready / dialog.open ...
#include "WebViewHost.h"
#include "WebShell.h"
