// PyEdEnt.cpp - legacy ADS entity accessors + resbuf command chain
// (moved verbatim from PyEd.cpp; [NEW] wrappers appended).
#include "stdafx.h"
#include "acedCmdNF.h"
#include "PyEdEnt.h"
#include "PyEdCommon.h"
#include "TextFinder.h"
#include <array>
#include <string>

// Legacy ADS entity accessors (acdbEnt*), resbuf normalized through PyResbuf.
// These are thin editor-level wrappers; modern database editing belongs to PyDb.
PyResbuf PyEdEntGet(const PyAdsName& ename) {
    resbuf* rb = ::acdbEntGet(ename.data());
    if (!rb) return PyResbuf(); // safe default: empty chain, no throw
    return PyResbuf(rb); // take ownership; dtor releases the chain
}

bool PyEdEntMod(const TVList& dxf) {
    PyResbuf holder(dxf);
    return edWarn(::acdbEntMod(holder.data()), L"acdbEntMod");
}

// acedCmdS 命令链：直接由 C++ TVList（每项 TypedValue(code,value)）构造 resbuf。
// code 语义即 resbuf restype（RTSTR/RTREAL/RTSHORT/RTPOINT/RT3DPOINT…），无类型猜测。
static bool cmdFromList(const TVList& args) {
    PyResbuf holder(args);
    const int rt = ::acedCmdS(holder.data());
    return edWarn(rt, L"acedCmdS");
}

bool PyEdCmd(const TVList& args) { return cmdFromList(args); }

//-----------------------------------------------------------------------------
// [NEW] legacy ADS entity traversal / mutation (acedads.h)
//-----------------------------------------------------------------------------
PyAdsName PyEdEntNext(const PyAdsName& ename) {
    ads_name cur{}, res{};
    int64_t* pc = nullptr; // element pointer (ads_name* would be a pointer-to-array)
    if (!ename.isEmpty()) {
        std::memcpy(cur, ename.data(), sizeof(cur));
        pc = cur;
    }
    if (!edWarn(::acdbEntNext(pc, res), L"acdbEntNext"))
        return PyAdsName{}; // 末尾或失败：空 AdsName
    return PyAdsName(res);
}

PyAdsName PyEdEntLast() {
    ads_name res{};
    if (!edWarn(::acdbEntLast(res), L"acdbEntLast"))
        return PyAdsName{};
    return PyAdsName(res);
}

bool PyEdEntDel(const PyAdsName& ename) {
    return edWarn(::acdbEntDel(ename.data()), L"acdbEntDel");
}

PyAdsName PyEdHandEnt(const AcString& handle) {
    ads_name res{};
    if (!edWarn(::acdbHandEnt(handle.kwszPtr(), res), L"acdbHandEnt"))
        return PyAdsName{};
    return PyAdsName(res);
}

PyAdsName PyEdEntMakeX(const TVList& dxf) {
    PyResbuf holder(dxf);
    ads_name res{};
    if (!edWarn(::acdbEntMakeX(holder.data(), res), L"acdbEntMakeX"))
        return PyAdsName{};
    return PyAdsName(res);
}

bool PyEdEntUpd(const PyAdsName& ename) {
    return edWarn(::acdbEntUpd(ename.data()), L"acdbEntUpd");
}

PyResbuf PyEdVports() {
    resbuf* rb = nullptr;
    if (!edWarn(::acedVports(&rb), L"acedVports")) return PyResbuf();
    return PyResbuf(rb); // take ownership; dtor releases the chain
}

// resbuf 链 -> Python list[(code, value)]（简化版，覆盖常见 DXF 类型）
static py::list resbufToPyList(const PyResbuf& rb) {
    py::list out;
    if (rb.isNull()) return out;
    for (const auto& tv : rb.toList()) {
        py::object v;
        switch (tv.value.getType()) {
            case DxfValueType::kNull:   v = py::none(); break;
            case DxfValueType::kInt8:   v = py::int_(tv.value.asInt8()); break;
            case DxfValueType::kInt16:  v = py::int_(tv.value.asInt16()); break;
            case DxfValueType::kInt32:  v = py::int_(tv.value.asInt32()); break;
            case DxfValueType::kInt64:  v = py::int_(tv.value.asInt64()); break;
            case DxfValueType::kDouble: v = py::float_(tv.value.asDouble()); break;
            case DxfValueType::kString: v = py::cast(tv.value.asString()); break;
            case DxfValueType::kPoint2d: {
                auto p = tv.value.asPoint2d(); v = py::make_tuple(p[0], p[1]); break;
            }
            case DxfValueType::kPoint3d: {
                auto p = tv.value.asPoint3d(); v = py::make_tuple(p[0], p[1], p[2]); break;
            }
            case DxfValueType::kAdsName: v = py::cast(tv.value.asAdsName()); break;
            default: v = py::none(); break;
        }
        out.append(py::make_tuple(tv.code, v));
    }
    return out;
}

// ---- acdbEntGetX / acdbRegApp ----
py::object PyEdEntGetX(const PyAdsName& ename, const TVList& args) {
    PyResbuf argRb(args);
    resbuf* rb = ::acdbEntGetX(ename.data(), argRb.data());
    if (!rb) return py::none();
    PyResbuf holder(rb);
    return resbufToPyList(holder);
}

bool PyEdRegApp(const AcString& appname) {
    return edWarn(::acdbRegApp(appname.kwszPtr()), L"acdbRegApp");
}

// ---- acdb 符号表 / 字典 ----
py::object PyEdTblNext(const AcString& tblname, bool rewind) {
    resbuf* rb = ::acdbTblNext(tblname.kwszPtr(), rewind ? 1 : 0);
    if (!rb) return py::none();
    PyResbuf holder(rb);
    return resbufToPyList(holder);
}

py::object PyEdTblSearch(const AcString& tblname, const AcString& sym, bool setnext) {
    resbuf* rb = ::acdbTblSearch(tblname.kwszPtr(), sym.kwszPtr(), setnext ? 1 : 0);
    if (!rb) return py::none();
    PyResbuf holder(rb);
    return resbufToPyList(holder);
}

PyAdsName PyEdTblObjName(const AcString& tblname, const AcString& sym) {
    ads_name res{};
    if (!edWarn(::acdbTblObjName(tblname.kwszPtr(), sym.kwszPtr(), res), L"acdbTblObjName"))
        return PyAdsName{};
    return PyAdsName(res);
}

PyAdsName PyEdNamedObjDict() {
    ads_name res{};
    if (!edWarn(::acdbNamedObjDict(res), L"acdbNamedObjDict"))
        return PyAdsName{};
    return PyAdsName(res);
}

py::object PyEdDictSearch(const PyAdsName& dict, const AcString& sym, bool setnext) {
    resbuf* rb = ::acdbDictSearch(dict.data(), sym.kwszPtr(), setnext ? 1 : 0);
    if (!rb) return py::none();
    PyResbuf holder(rb);
    return resbufToPyList(holder);
}

py::object PyEdDictNext(const PyAdsName& dict, bool rewind) {
    resbuf* rb = ::acdbDictNext(dict.data(), rewind ? 1 : 0);
    if (!rb) return py::none();
    PyResbuf holder(rb);
    return resbufToPyList(holder);
}

bool PyEdDictRename(const PyAdsName& dict, const AcString& oldsym, const AcString& newsym) {
    return edWarn(::acdbDictRename(dict.data(), oldsym.kwszPtr(), newsym.kwszPtr()), L"acdbDictRename");
}

bool PyEdDictRemove(const PyAdsName& dict, const AcString& sym) {
    return edWarn(::acdbDictRemove(dict.data(), sym.kwszPtr()), L"acdbDictRemove");
}

PyAdsName PyEdDictAdd(const PyAdsName& dict, const AcString& sym, const PyAdsName& obj) {
    if (!edWarn(::acdbDictAdd(dict.data(), sym.kwszPtr(), obj.data()), L"acdbDictAdd"))
        return PyAdsName{};
    return obj;  // acdbDictAdd 无返回名，入参 obj 即新项名
}

//-----------------------------------------------------------------------------
// 批量文字搜索（AcDbText / AcDbMText，大小写无关）：建一次索引，按模式查询。
// mode: prefix(默认) 前缀 / contains 任意位置包含 / full 整串全等。
// 空 ids 或空关键字直接返回空列表，不做无谓的图元打开。
//-----------------------------------------------------------------------------
AcDbObjectIdArray PyEdTextFind(const AcDbObjectIdArray& ids, const AcString& keyword, const std::string& mode) {
    AcDbObjectIdArray result;
    if (ids.isEmpty() || keyword.isEmpty()) return result;
    QTextFind finder(ids);
    std::vector<AcDbObjectId> hits;
    if (mode == "contains" || mode == "any")
        hits = finder.matchAnyPos(keyword);
    else if (mode == "full" || mode == "equals")
        hits = finder.matchFull(keyword);
    else
        hits = finder.matchStart(keyword);
    for (const AcDbObjectId& id : hits) result.append(id);
    return result;
}
