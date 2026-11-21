// PyEdCommon.h - shared inline conversion / status helpers used by every PyEd
// free-function slice. Originally an anonymous-namespace block in PyEd.cpp;
// promoted to an inline header so each slice TU sees the same definitions
// (ODR-safe: inline). No logic changed.
#pragma once

#include "stdafx.h"
#include "PyResbuf.h"
#include "PyUtils.h"
#include "PyEdPromptOptions.h"

namespace py = pybind11;

namespace pyed_common {

// Map ADS status codes. RTNORM(5100) = success; non-throwing path: print a
// [PyArx] warning to the CAD command line and return false (never throws).
inline bool edWarn(int rt, const wchar_t* what) {
    if (rt == RTNORM) return true;
    acutPrintf(L"[PyArx] aced status=%d [%ls]\n", rt, what ? what : L"");
    return false;
}

// tuple/list of 2 or 3 floats -> ads_point; returns false when obj is None.
inline bool pointFromPy(const py::handle& obj, ads_point out) {
    if (obj.is_none()) return false;
    auto seq = obj.cast<py::sequence>();
    const size_t n = seq.size();
    out[0] = seq[0].cast<double>();
    out[1] = seq[1].cast<double>();
    out[2] = (n >= 3) ? seq[2].cast<double>() : 0.0;
    return true;
}

// tuple of 2 ints -> ads_name; returns false if conversion fails.
inline bool nameFromPy(const py::handle& obj, ads_name out) {
    if (obj.is_none()) return false;
    auto t = obj.cast<py::sequence>();
    if (t.size() != 2) return false;
    out[0] = t[0].cast<int64_t>();
    out[1] = t[1].cast<int64_t>();
    return true;
}

inline py::object pointToPy(const ads_point p) {
    return py::make_tuple(p[0], p[1], p[2]);
}
inline py::object nameToPy(const ads_name n) {
    return py::make_tuple(n[0], n[1]);
}

// Run acedInitGet with the requested flags / keywords before a getxxx() call.
inline void edInitGet(const PromptOptions& o) {
    const int flags = static_cast<int>(o.Flags);
    if (flags != 0 || !o.KeyWords.isEmpty())
        ::acedInitGet(flags, o.KeyWords.isEmpty() ? nullptr : o.KeyWords.kwszPtr());
}

// After RTKWORD, fetch the entered keyword string.
inline AcString edKeyword() {
    AcString s;
    ::acedGetInput(s);
    return s;
}

inline void adsToObjectId(const ads_name n, AcDbObjectId& id) {
    if (::acdbGetObjectId(id, n) != Acad::eOk) id = AcDbObjectId();
}

// Python filter list of (code, value) pairs -> TVList -> owned PyResbuf chain.
inline TVList tvListFromPy(const py::object& o) {
    TVList tv;
    if (o.is_none()) return tv;
    for (auto item : o.cast<py::sequence>()) {
        auto kv = item.cast<py::sequence>();
        const short code = kv[0].cast<short>();
        auto val = kv[1];
        if (py::isinstance<py::str>(val))
            tv.emplace_back(code, val.cast<AcString>());
        else if (py::isinstance<py::float_>(val))
            tv.emplace_back(code, val.cast<double>());
        else
            tv.emplace_back(code, val.cast<int32_t>());
    }
    return tv;
}

} // namespace pyed_common

using namespace pyed_common;
