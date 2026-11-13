#pragma once

// PyMfcCore 公共头：MFC + pybind11 + Python。
// 纯 GUI 层：不链任何 AutoCAD/ObjectARX 库（侧栏等 CAD 能力在 PyArxCore）。
#define WIN32_LEAN_AND_MEAN
#include <afxwin.h>
#include <afxext.h>
#include <afxcmn.h>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <map>
#include <string>

namespace py = pybind11;

#pragma comment(lib, "python314.lib")
