// PyEdSel.h - thin legacy ADS selection-set wrappers (acedSS*). The
// object-oriented PyEdSelectionSets class owns the set (RAII auto-free); these
// free functions accept/return the registered C++ types instead of tuples.
#pragma once

#include "stdafx.h"
#include "PyResbuf.h"
#include "PyAdsName.h"
#include "PyEdSelectionSets.h"
#include <optional>

// acedSSGet: on success return an owning PyEdSelectionSets (dtor auto SSFree);
// on failure return an empty non-owning object. pt1/pt2 optional fence points.
PyEdSelectionSets PyEdSSGet(const AcString& mode,
    const std::optional<AcGePoint3d>& pt1, const std::optional<AcGePoint3d>& pt2,
    const PyResbuf& filter = PyResbuf());
PyAdsName PyEdSSName(const PyEdSelectionSets& ss, int i);
bool      PyEdSSMemb(const PyAdsName& ename, const PyEdSelectionSets& ss);
// acedSSAdd: append ename; pass nullptr to create a new owned set.
PyEdSelectionSets PyEdSSAdd(const PyAdsName& ename, const PyEdSelectionSets* ss = nullptr);
bool      PyEdSSDel(const PyAdsName& ename, PyEdSelectionSets& ss);

// 读取首选集/组选择集：返回 (组选择集 ObjectId 列表, 首选集 ObjectId 列表)，内部释放 resbuf。
std::tuple<py::list, py::list> PyEdSSGetFirst();

// Python-facing shim: filter 直接传 C++ TvList（TypedValue 组码链）；空链表示无过滤。
PyEdSelectionSets PyEdSSGetPy(const AcString& mode,
    const std::optional<AcGePoint3d>& pt1, const std::optional<AcGePoint3d>& pt2,
    const TVList& filter = TVList{});
