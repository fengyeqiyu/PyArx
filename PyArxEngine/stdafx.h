#pragma once
// PyArxEngine stdafx.h - ????(arx/crx)?????
// ??:Python ?????(Python.h/PyConfig)+ ObjectARX ??(acrxEntryPoint/aced/acut)?
// ??:UTF-8 ? BOM?

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

// MFC ??????(?? MFC ??:afxwin.h ? windows.h ??)
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

// ---- ??? ----
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

// ---- ObjectARX ?????(????)----
#pragma comment(lib, "rxapi.lib")
#pragma comment(lib, "acdb24.lib")
#pragma comment(lib, "accore.lib")
#pragma comment(lib, "acgeoment.lib")
#pragma comment(lib, "acpal.lib")   // acHeapAlloc/acHeapFree(AcHeapOpers.h ?? operator new/delete)

// Python import lib: free-threaded build -> python315t.lib, GIL build -> python315.lib
#ifdef Py_GIL_DISABLED
#pragma comment(lib, "python315t.lib")
#else
#pragma comment(lib, "python315.lib")
#endif

// ---- ObjectARX ??(arxHeaders.h ???? AcDb/AcGe/AcEd ?)----
#include "arxHeaders.h"

// ---- Python ?? ----
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <frameobject.h>
#include <structmember.h>

#include <pybind11/pybind11.h>
#include <pybind11/embed.h>
#include <pybind11/functional.h>   // PyCmdBinder:py::function ? std::function
namespace py = pybind11;
namespace fs = std::filesystem;
using Path = std::filesystem::path;
using pym = pybind11::module_;
