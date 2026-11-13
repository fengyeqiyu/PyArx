// PyEdSel.cpp - thin legacy ADS selection-set wrappers (acedSS*). The
// PyEdSelectionSets OO class owns the set (RAII auto SSFree); these free
// functions accept/return the registered C++ types instead of tuples.
#include "stdafx.h"
#include "PyEdSel.h"
#include "PyEdCommon.h"

PyEdSelectionSets PyEdSSGet(const AcString& mode,
    const std::optional<AcGePoint3d>& pt1, const std::optional<AcGePoint3d>& pt2,
    const PyResbuf& filter) {
    ads_point p1{}, p2{};
    const ads_real* a = nullptr;
    const ads_real* b = nullptr;
    if (pt1) { p1[0] = pt1->x; p1[1] = pt1->y; p1[2] = pt1->z; a = p1; }
    if (pt2) { p2[0] = pt2->x; p2[1] = pt2->y; p2[2] = pt2->z; b = p2; }
    ads_name ss{};
    if (!edWarn(::acedSSGet(mode.isEmpty() ? nullptr : mode.kwszPtr(), a, b,
        filter.isNull() ? nullptr : filter.data(), ss), L"acedSSGet"))
        return PyEdSelectionSets(); // failure: empty, non-owning
    return PyEdSelectionSets(ss);   // take ownership; dtor auto SSFree
}

PyAdsName PyEdSSName(const PyEdSelectionSets& ss, int i) {
    ads_name name{}, ent{};
    std::memcpy(name, ss.ads(), sizeof(name));
    if (!edWarn(::acedSSName(name, i, ent), L"acedSSName"))
        return PyAdsName();
    return PyAdsName(ent);
}

bool PyEdSSMemb(const PyAdsName& ename, const PyEdSelectionSets& ss) {
    ads_name e{}, s{};
    std::memcpy(e, ename.data(), sizeof(e));
    std::memcpy(s, ss.ads(), sizeof(s));
    return ::acedSSMemb(e, s) == RTNORM;
}

// acedSSAdd: with ss=nullptr creates a new owned set; with an existing set the
// entity is added in place and the returned wrapper does NOT take ownership
// (avoids a double SSFree with the original owner).
PyEdSelectionSets PyEdSSAdd(const PyAdsName& ename, const PyEdSelectionSets* ss) {
    ads_name result{}, e{}, existing{};
    std::memcpy(e, ename.data(), sizeof(e));
    int64_t* ps = nullptr; // element pointer (ads_name* would be a pointer-to-array)
    if (ss) {
        std::memcpy(existing, ss->ads(), sizeof(existing));
        ps = existing; // array name decays to int64_t*
    }
    if (!edWarn(::acedSSAdd(e, ps, result), L"acedSSAdd"))
        return PyEdSelectionSets();
    if (ss) {
        PyEdSelectionSets out(result);
        out.detach(); // existing set stays owned by the caller
        return out;
    }
    return PyEdSelectionSets(result); // new set: take ownership
}

bool PyEdSSDel(const PyAdsName& ename, PyEdSelectionSets& ss) {
    ads_name e{}, s{};
    std::memcpy(e, ename.data(), sizeof(e));
    std::memcpy(s, ss.ads(), sizeof(s));
    return edWarn(::acedSSDel(e, s), L"acedSSDel");
}

// 读取首选集/组选择集：遍历返回的 resbuf 链取 RTENAME -> ObjectId，内部 acutRelRb 释放。
std::tuple<py::list, py::list> PyEdSSGetFirst() {
    resbuf* g = nullptr;
    resbuf* p = nullptr;
    py::list gset, pset;
    auto drain = [](resbuf* rb, py::list& out) {
        for (resbuf* c = rb; c; c = c->rbnext) {
            if (c->restype == RTENAME) {
                AcDbObjectId id;
                if (::acdbGetObjectId(id, c->resval.rlname) == Acad::eOk)
                    out.append(py::int_(id.isNull() ? (uint64_t)0 : (uint64_t)id.asOldId()));
            }
        }
        if (rb) ::acutRelRb(rb);
    };
    if (::acedSSGetFirst(&g, &p) == RTNORM) {
        drain(g, gset);
        drain(p, pset);
    } else {
        if (g) ::acutRelRb(g);
        if (p) ::acutRelRb(p);
    }
    return { gset, pset };
}

// Python-facing shim: filter 直接为 C++ TvList（TypedValue 组码链）；空链无过滤。
PyEdSelectionSets PyEdSSGetPy(const AcString& mode,
    const std::optional<AcGePoint3d>& pt1, const std::optional<AcGePoint3d>& pt2,
    const TVList& filter) {
    PyResbuf f(filter);
    return PyEdSSGet(mode, pt1, pt2, f);
}
