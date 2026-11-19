#pragma once

// ?????:?????? pybind11 ???,??? TU ????? internals ?????
#include "HotModuleName.h"

#pragma pack(push, 8)
#pragma warning(disable: 4786 4996 26812)

#define PyEdModule

#define STRICT

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN
#endif

#ifndef WINVER
#define WINVER _WIN32_WINNT_WIN10
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT _WIN32_WINNT_WIN10
#endif
#ifndef _WIN32_WINDOWS
#define _WIN32_WINDOWS _WIN32_WINNT_WIN10
#endif
#ifndef _WIN32_IE
#define _WIN32_IE _WIN32_WINNT_WIN10
#endif

#define SETBIT(flags, bit, value) ((value) ? (flags |= (bit)) : (flags &= ~(bit)))
#define GETBIT(flags, bit) (((flags) & (bit)) ? true : false)

#if _MSC_VER >= 1920
#pragma comment(lib, "AcPal.lib")
#endif
// AcUi / AdUi(PyApp / PyPaletteSet ?? CAdUiPaletteSet ??)
#pragma comment(lib, "adui24.lib")
#pragma comment(lib, "acui24.lib")
#pragma comment(lib, "AcMPolygonObj.lib")

// ObjectARX ?????(??)
#pragma comment(lib, "acgeoment.lib")
#pragma comment(lib, "acgex24.lib")
#pragma comment(lib, "rxapi.lib")
#pragma comment(lib, "acdb24.lib")
#pragma comment(lib, "accore.lib")
// Python import lib: free-threaded build -> python315t.lib, GIL build -> python315.lib
#ifdef Py_GIL_DISABLED
#pragma comment(lib, "python315t.lib")
#else
#pragma comment(lib, "python315.lib")
#endif
#pragma comment(lib, "rpcrt4.lib")
#pragma comment(lib, "version.lib")
#pragma comment(lib, "pathcch.lib")
#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "winspool.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "uuid.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "kernel32.lib")

// ???
#include <map>
#include <vector>
#include <array>
#include <functional>
#include <memory>
#include <variant>
#include <type_traits>
#include <string>
#include <format>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <set>
#include <span>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <cwctype>
#include <ranges>
#include <queue>
#include <execution>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
// MFC ??(PyApp / PyPaletteSet ??? CAdUiPaletteSet ? AcUi/MFC ??)?
// ???? windows.h ??,??? winsock2.h ??,?? ObjectARX ???
#include <afxwin.h>
#include <afxext.h>
#include <afxcmn.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <windows.h>
#include <ole2.h>
#include <unknwn.h>

#define _BREP_SUPPORT_
#define _ARX_CUSTOM_DRAG_N_DROP_

#include "arxHeaders.h"
#include "rxvalue.h"
#include "axlock.h"

#define PY_SSIZE_T_CLEAN

#include <Python.h>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include <pybind11/operators.h>
#include <pybind11/numpy.h>
#include <pybind11/iostream.h>
namespace py = pybind11;
using pym = pybind11::module_;

namespace fs = std::filesystem;
using Path = std::filesystem::path;

#include "PyString.h"
#include "PyArrayCaster.h"
#include <source_location>

#define BindClass(ClassName) \
 void bind_##ClassName(pybind11::module& m)

#define ImpBindClass(ClassName) \
 void bind_##ClassName(pybind11::module& m) { \
 py::class_<ClassName>(m, #ClassName)

#define RegClass(ClassName)\
 bind_##ClassName(m)

#define PyWrapper_H(cls) \
public: \
 virtual Ac##cls* impObj() override;  \
 virtual Ac##cls* impObj() const override; \
 static PyRxClass desc();\
 static Py##cls cast(const PyRxObject& rxobj); \
 static const AcString className(); \
 static const AcString appName(); \
 static const AcString dxfName(); \
 static const AcString name();

#define PyWrapper_Cpp(cls) \
Ac##cls* Py##cls::impObj() { \
 return static_cast<Ac##cls*>(m_ptr); \
} \
Ac##cls* Py##cls::impObj() const { \
 return static_cast<Ac##cls*>(m_ptr); \
} \
PyRxClass Py##cls::desc() { \
 return Ac##cls::desc(); \
} \
Py##cls Py##cls::cast(const PyRxObject& rxobj) { \
 return Ac##cls::cast(rxobj.impObj()); \
} \
const AcString Py##cls::className() { \
 return AcString(Ac##cls::className()); \
} \
const AcString Py##cls::appName() { \
 return AcString(Ac##cls::desc()->appName()); \
} \
const AcString Py##cls::dxfName() { \
 return AcString(Ac##cls::desc()->dxfName()); \
} \
const AcString Py##cls::name() { \
 return AcString(Ac##cls::desc()->name()); \
}

#define PyWrapper_BIND( cls) \
 .def_static("desc", &Py##cls::desc) \
 .def_static("cast", &Py##cls::cast) \
 .def_static("className", &Py##cls::className) \
 .def_static("appName", &Py##cls::appName) \
 .def_static("dxfName", &Py##cls::dxfName)

typedef std::array<uint8_t, 8> PyHandle;
typedef std::tuple<int, py::object> PyDxfCodeGroup;
typedef std::vector<AcDbObjectId> PyDbObjectIdArray;

template <typename OutType, typename InType>
AcArray<OutType> VecToAcArray(const std::vector<InType>& vec) {
	AcArray<OutType> arr;
	arr.setLogicalLength(vec.size());
	for (auto& obj : vec) {
		if constexpr (std::is_same_v<InType, AcDbObjectId>) {
			arr.append(obj);
		} else {
			arr.append(obj.impObj());
		}
	}
	return arr;
}

template <typename OutType, typename InType>
std::vector<OutType> AcArrayToVec(const AcArray<InType>& arr) {
	std::vector<OutType> vec;
	vec.reserve(arr.length());
	for (auto& item : arr) {
		vec.emplace_back(item);
	}
	return vec;
}

inline bool CheckErrorStatus(const Acad::ErrorStatus& es, AcString msg = L"", const std::source_location& loc = std::source_location::current()) {
	if (es == Acad::eOk) return true;
	AcString estr = acadErrorStatusText(es);
	AcString filename = AcString(loc.file_name());
	AcString funcname = AcString(loc.function_name());
	acutPrintf(L"[Error]\nfileName=%s\n, funcName=%s, ErrorStatus=%s\n", filename, funcname, estr, msg);
	return false;
}
#pragma pack(pop)
