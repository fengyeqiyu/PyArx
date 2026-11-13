#pragma once

// PyMfcWebView 公共头：MFC + pybind11 + Python。
// 纯 GUI 层：不链 AutoCAD/ObjectARX 库；只把 MFCWebView（静态库）包成 pyd，
// 让任何裸 Python 解释器都能 import 出一个 WebView 窗口。
#define WIN32_LEAN_AND_MEAN
#include <afxwin.h>
#include <afxext.h>
#include <afxcmn.h>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <functional>
#include <map>
#include <string>

namespace py = pybind11;

#pragma comment(lib, "python314.lib")
