#pragma once

// PyMfcCore ???:MFC + pybind11 + Python?
// ? GUI ?:???? AutoCAD/ObjectARX ?(??? CAD ??? PyArxCore)?
#define WIN32_LEAN_AND_MEAN
#include <afxwin.h>
#include <afxext.h>
#include <afxcmn.h>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <map>
#include <string>

namespace py = pybind11;

// Python import lib: free-threaded build -> python315t.lib, GIL build -> python315.lib
#ifdef Py_GIL_DISABLED
#pragma comment(lib, "python315t.lib")
#else
#pragma comment(lib, "python315.lib")
#endif
