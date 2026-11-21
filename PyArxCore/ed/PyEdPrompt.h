// PyEdPrompt.h - 统一交互提示封装（PyEdPrompt 风格内联自由函数）。
// 合并自原 PyEdAds.h（多模式选择集）、PyEdPrompts.h（Maple* 输入/拾取）、
// PyEdPromptInfra（Result 家族）。结果结构只采用 PyEdResult.h 一套；
// SDK 调用原样不改，失败不抛异常、不崩溃，返回安全默认值。
#pragma once

#include "stdafx.h"
#include "PyEdResult.h"
#include "PyEdPromptOptions.h"
#include "PyResbuf.h"
#include "PyEdCommon.h"
#include "PyAdsName.h"
#include "PyEdSelectionSets.h"
#include <gepnt3d.h>

// ===========================================================================
// 便捷构造：提示词字符串 -> PromptOptions（自动补前导换行，PyEdPrompts 风格）
// ===========================================================================
inline PromptOptions pyedOpts(const AcString& message) {
    PromptOptions o;
    o.setMessage(message);
    return o;
}

// ===========================================================================
// 以下交互输入包装函数（pyedGetInt/Real/String/Keyword/Point/Corner/Dist/
// Angle、pyedOsnap、pyedEntSel/NEntSel、pyedSelect*）均调用 acedGetXxx /
// acedEntSel / acedNEntSelP / acedSSGet，会阻塞等待用户键盘输入或图形点取，
// 无头 accoreconsole 无显示/交互通道，故仅在定义 PyEdModule（GUI）时编译。
// pyedSetFirst（acedSSSetFirst 首选集，被非交互 sssetfirst 绑定引用）保留在外。
// ===========================================================================
#ifdef PyEdModule

// ---------------------------------------------------------------------------
// 整数输入
// ---------------------------------------------------------------------------
inline IntResult pyedGetInt(const PromptOptions& o) {
    IntResult r; edInitGet(o);
    int v = 0;
    const int rt = ::acedGetInt(o.Message.kwszPtr(), v);
    r.status = static_cast<PromptStatus>(rt);
    if (rt == RTNORM) r.value = v;
    return r;
}
inline IntResult pyedGetInt(const AcString& message = AcString(L"请输入整数:")) {
    return pyedGetInt(pyedOpts(message));
}

// ---------------------------------------------------------------------------
// 实数输入
// ---------------------------------------------------------------------------
inline DoubleResult pyedGetReal(const PromptOptions& o) {
    DoubleResult r; edInitGet(o);
    double v = 0.0;
    const int rt = ::acedGetReal(o.Message.kwszPtr(), &v);
    r.status = static_cast<PromptStatus>(rt);
    if (rt == RTNORM) r.value = v;
    return r;
}
inline DoubleResult pyedGetReal(const AcString& message = AcString(L"输入实数:")) {
    return pyedGetReal(pyedOpts(message));
}

// ---------------------------------------------------------------------------
// 字符串输入（cronly=1 仅允许键盘输入，不允许点取）
// ---------------------------------------------------------------------------
inline StringResult pyedGetString(int cronly, const PromptOptions& o) {
    StringResult r; edInitGet(o);
    AcString s;
    const int rt = ::acedGetString(cronly, o.Message.kwszPtr(), s);
    r.status = static_cast<PromptStatus>(rt);
    if (rt == RTNORM) r.value = s;
    return r;
}
inline StringResult pyedGetString(const AcString& message = AcString(L"请输入字符:"), int cronly = 0) {
    return pyedGetString(cronly, pyedOpts(message));
}

// ---------------------------------------------------------------------------
// 关键字输入
// ---------------------------------------------------------------------------
inline StringResult pyedGetKeyword(const PromptOptions& o) {
    StringResult r; edInitGet(o);
    AcString s;
    const int rt = ::acedGetKword(o.Message.kwszPtr(), s);
    r.status = static_cast<PromptStatus>(rt);
    if (rt == RTNORM) r.value = s;
    return r;
}
inline StringResult pyedGetKeyword(const AcString& message = AcString(L"请输入关键字:")) {
    return pyedGetKeyword(pyedOpts(message));
}

// ---------------------------------------------------------------------------
// 点输入（可带基点）
// ---------------------------------------------------------------------------
inline PointResult pyedGetPoint(const AcGePoint3d& basept, const PromptOptions& o) {
    PointResult r; edInitGet(o);
    ads_point base{ basept.x, basept.y, basept.z };
    ads_point res{};
    const int rt = ::acedGetPoint(base, o.Message.kwszPtr(), res);
    r.status = static_cast<PromptStatus>(rt);
    if (rt == RTNORM) r.value = AcGePoint3d(res[0], res[1], res[2]);
    return r;
}
inline PointResult pyedGetPoint(const PromptOptions& o) {
    return pyedGetPoint(AcGePoint3d::kOrigin, o);
}
inline PointResult pyedGetPoint(const AcString& message = AcString(L"请选择一个点:")) {
    return pyedGetPoint(pyedOpts(message));
}

// ---------------------------------------------------------------------------
// 对角点（窗口第二角）
// ---------------------------------------------------------------------------
inline PointResult pyedGetCorner(const AcGePoint3d& basept, const PromptOptions& o) {
    PointResult r; edInitGet(o);
    ads_point base{ basept.x, basept.y, basept.z };
    ads_point res{};
    const int rt = ::acedGetCorner(base, o.Message.kwszPtr(), res);
    r.status = static_cast<PromptStatus>(rt);
    if (rt == RTNORM) r.value = AcGePoint3d(res[0], res[1], res[2]);
    return r;
}
inline PointResult pyedGetCorner(const AcGePoint3d& basept, const AcString& message = AcString(L"选择对角:")) {
    return pyedGetCorner(basept, pyedOpts(message));
}

// ---------------------------------------------------------------------------
// 距离输入
// ---------------------------------------------------------------------------
inline DoubleResult pyedGetDist(const AcGePoint3d& basept, const PromptOptions& o) {
    DoubleResult r; edInitGet(o);
    ads_point base{ basept.x, basept.y, basept.z };
    double v = 0.0;
    const int rt = ::acedGetDist(base, o.Message.kwszPtr(), &v);
    r.status = static_cast<PromptStatus>(rt);
    if (rt == RTNORM) r.value = v;
    return r;
}
inline DoubleResult pyedGetDist(const AcGePoint3d& basept, const AcString& message = AcString(L"输入目标距离:")) {
    return pyedGetDist(basept, pyedOpts(message));
}

// ---------------------------------------------------------------------------
// 角度输入
// ---------------------------------------------------------------------------
inline DoubleResult pyedGetAngle(const AcGePoint3d& basept, const PromptOptions& o) {
    DoubleResult r; edInitGet(o);
    ads_point base{ basept.x, basept.y, basept.z };
    double v = 0.0;
    const int rt = ::acedGetAngle(base, o.Message.kwszPtr(), &v);
    r.status = static_cast<PromptStatus>(rt);
    if (rt == RTNORM) r.value = v;
    return r;
}
inline DoubleResult pyedGetAngle(const AcGePoint3d& basept, const AcString& message = AcString(L"输入角度:")) {
    return pyedGetAngle(basept, pyedOpts(message));
}

// ---------------------------------------------------------------------------
// 对象捕捉：对给定点按捕捉模式（如 L"end,mid,cen"）求捕捉点，不交互点取
// ---------------------------------------------------------------------------
inline PointResult pyedOsnap(const AcGePoint3d& pt, const AcString& mode) {
    PointResult r;
    ads_point base{ pt.x, pt.y, pt.z };
    ads_point res{};
    const int rt = ::acedOsnap(base, mode.kwszPtr(), res);
    r.status = static_cast<PromptStatus>(rt);
    if (rt == RTNORM) r.value = AcGePoint3d(res[0], res[1], res[2]);
    return r;
}

// ---------------------------------------------------------------------------
// 拾取单个实体（acedEntSel）
// ---------------------------------------------------------------------------
inline EntityResult pyedEntSel(const PromptOptions& o) {
    EntityResult r; edInitGet(o);
    PyAdsName ent; ads_point pt{};
    const int rt = ::acedEntSel(o.Message.kwszPtr(), ent.data(), pt);
    r.status = static_cast<PromptStatus>(rt);
    if (rt == RTNORM) {
        adsToObjectId(ent.data(), r.value.id);
        r.value.pickedPoint = AcGePoint3d(pt[0], pt[1], pt[2]);
    }
    return r;
}
inline EntityResult pyedEntSel(const AcString& message = AcString(L"请选择图元:")) {
    return pyedEntSel(pyedOpts(message));
}

// ---------------------------------------------------------------------------
// 拾取子实体（acedNEntSelP）：引用路径链所有权交给 PyResbuf，无需手动释放
// ---------------------------------------------------------------------------
inline SubEntityResult pyedNEntSel(const PromptOptions& o) {
    SubEntityResult r; edInitGet(o);
    PyAdsName ent; ads_point pt{};
    ads_matrix mat{};
    resbuf* pRef = nullptr;
    const int rt = ::acedNEntSelP(o.Message.kwszPtr(), ent.data(), pt, 1, mat, &pRef);
    r.status = static_cast<PromptStatus>(rt);
    r.value.refStack.reset(pRef); // 转移所有权，由 PyResbuf 管理释放
    if (rt == RTNORM) {
        adsToObjectId(ent.data(), r.value.id);
        r.value.pickedPoint = AcGePoint3d(pt[0], pt[1], pt[2]);
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                r.value.xform(i, j) = (j < 3) ? mat[i][j] : (i == 3 ? 1.0 : 0.0);
        r.value.container = AcDbObjectId();
    }
    return r;
}
inline SubEntityResult pyedNEntSel(const AcString& message = AcString(L"请选择子图元:")) {
    return pyedNEntSel(pyedOpts(message));
}

// ---------------------------------------------------------------------------
// 选择集：mode 为 ADS 选择模式（":A" 全选 / "X" 全部 / ":W" 窗口 / ":C" 交叉 /
// ":P" 前一个 / "L" 最后 / ":WP" 多边形窗口 / ":CP" 交叉多边形）。
// 栈上 PyEdSelectionSets 自析构，离开作用域自动 acedSSFree。
// ---------------------------------------------------------------------------
inline SSResult pyedSelect(const AcString& mode, const PromptOptions& o,
                           const PyResbuf& filter = PyResbuf()) {
    SSResult r;
    PyEdSelectionSets ss;
    const int rt = ::acedSSGet(mode.kwszPtr(), nullptr, nullptr,
        filter.isNull() ? nullptr : filter.data(), ss.ads());
    // 选择集结果状态简化为 bool 语义：要么 kOK，要么 kCancel（其余一律视为取消）
    r.status = (rt == RTNORM) ? PromptStatus::kOK : PromptStatus::kCancel;
    if (rt != RTNORM) return r;
    ss.attach();
    r.value = std::move(ss); // value 直接为拥有式选择集（move 转移所有权）
    return r;
}
inline SSResult pyedSelect(const PromptOptions& o, const PyResbuf& filter = PyResbuf()) {
    return pyedSelect(L":A", o, filter);
}

// 多模式选择集：结果直接写入调用方持有的 PyEdSelectionSets（自析构）。
inline bool pyedSelectByMode(const AcString& mode, const PromptOptions& o,
                             const PyResbuf& filter, PyEdSelectionSets& ss) {
    edInitGet(o);
    const bool ok = ::acedSSGet(mode.kwszPtr(), nullptr, nullptr,
        filter.isNull() ? nullptr : filter.data(), ss.ads()) == RTNORM;
    if (ok) ss.attach();
    return ok;
}

// Python-facing shim：filter 为 C++ TvList（TypedValue 组码链），包装为 owned PyResbuf。
inline SSResult pyedSelectPy(const PromptOptions& o, const TVList& filter, const AcString& mode) {
    PyResbuf f(filter);
    return pyedSelect(mode, o, f);
}

#endif // PyEdModule —— 交互输入包装函数结束

// ---------------------------------------------------------------------------
// 设为首选集（栈上建集，自析构）
// ---------------------------------------------------------------------------
inline bool pyedSetFirst(const py::list& objectIds) {
    PyEdSelectionSets ss;
    if (::acedSSAdd(nullptr, nullptr, ss.ads()) != RTNORM) return false;
    ss.attach();
    AcArray<AcDbObjectId> ids = py::cast<AcDbObjectIdArray>(objectIds);
    for (int i = 0; i < ids.length(); ++i) {
        PyAdsName n;
        if (::acdbGetAdsName(n.data(), ids[i]) == Acad::eOk)
            ::acedSSAdd(n.data(), ss.ads(), ss.ads());
    }
    PyAdsName nothing;
    return edWarn(::acedSSSetFirst(ss.ads(), nothing.data()), L"acedSSSetFirst");
}
