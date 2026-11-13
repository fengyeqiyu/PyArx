#pragma once
// PyArxEngine stdafx.h — 宿主模块（arx/crx）预编译头。
// 职责：Python 运行时嵌入（Python.h/PyConfig）+ ObjectARX 入口（acrxEntryPoint/aced/acut）。
// 编码：UTF-8 带 BOM。

#pragma warning(disable: 4786 4996 26812)

#define STRICT

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN
#endif

#ifndef WINVER
#define WINVER 0x0A00
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif

// MFC 必须最先包含（标准 MFC 顺序：afxwin.h 在 windows.h 之前）
#include <afxwin.h>
#include <afxext.h>
#include <afxcmn.h>
#include <tlhelp32.h>

#include <string>
#include <vector>
#include <memory>
#include <format>
#include <filesystem>
#include <unordered_map>

// ---- 系统库 ----
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "uuid.lib")
#pragma comment(lib, "rpcrt4.lib")
#pragma comment(lib, "version.lib")

// ---- ObjectARX 动态导入库（宿主入口）----
#pragma comment(lib, "rxapi.lib")
#pragma comment(lib, "acdb24.lib")
#pragma comment(lib, "accore.lib")
#pragma comment(lib, "acgeoment.lib")
#pragma comment(lib, "acpal.lib")   // acHeapAlloc/acHeapFree（AcHeapOpers.h 全局 operator new/delete）

// ---- Python 3.14 动态链接导入库（python314.dll）----
#pragma comment(lib, "python314.lib")

// ---- ObjectARX 头链（arxHeaders.h 引入完整 AcDb/AcGe/AcEd 链）----
#include "arxHeaders.h"

// ---- Python 嵌入 ----
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <frameobject.h>
#include <structmember.h>

#include <pybind11/pybind11.h>
#include <pybind11/embed.h>
#include <pybind11/functional.h>   // PyCmdBinder：py::function → std::function
namespace py = pybind11;
namespace fs = std::filesystem;
using Path = std::filesystem::path;
using pym = pybind11::module_;
