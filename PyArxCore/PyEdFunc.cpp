// PyEdFunc.cpp - non-interactive editor/session services (moved from PyEd.cpp
// verbatim; [NEW] wrappers appended at the bottom following the same edWarn
// non-throwing convention).
#include "stdafx.h"
#include "acedCmdNF.h"
#include "PyEdFunc.h"
#include "PyErrorStatusException.h"
#include "PyResbuf.h"
#include "PyUtils.h"
#include "PyEdCommon.h"
#include "PyEngineCompat.h"

//-----------------------------------------------------------------------------
// Non-interactive wrappers (moved verbatim from PyEd.cpp)
//-----------------------------------------------------------------------------
bool PyEdPrompt(const AcString& s) {
    return edWarn(::acedPrompt(s.kwszPtr()), L"acedPrompt");
}

bool PyEdAlert(const AcString& s) {
    return edWarn(::acedAlert(s.kwszPtr()), L"acedAlert");
}

py::object PyEdGetVar(const AcString& sym) {
    // One owned node; acedGetVar fills restype/resval in place; PyResbuf frees it.
    PyEngine::dbgLog(L"GetVar: acutNewRb 前");
    PyResbuf holder(acutNewRb(RTSTR));
    PyEngine::dbgLog(L"GetVar: acutNewRb 完成");
    if (!edWarn(::acedGetVar(sym.kwszPtr(), holder.data()), L"acedGetVar")) {
        PyEngine::dbgLog(L"GetVar: acedGetVar 失败返回");
        return py::none();
    }
    PyEngine::dbgLog(L"GetVar: acedGetVar 成功");
    const resbuf* rb = holder.data();
    PyEngine::dbgLog(L"GetVar: switch 前 restype=%d", rb->restype);
    switch (rb->restype) {
    case RTSHORT:
    case RTLONG:   return py::int_(rb->resval.rint);
    case RTREAL:   return py::float_(rb->resval.rreal);
    case RTSTR:    {
        // GetVar 非交互路径：AcString 经 type_caster<AcString> 自动转 py::str。
        return py::cast(AcString(rb->resval.rstring));
    }
    case RTPOINT:  return py::make_tuple(rb->resval.rpoint[0], rb->resval.rpoint[1]);
    case RT3DPOINT:return py::make_tuple(rb->resval.rpoint[0], rb->resval.rpoint[1], rb->resval.rpoint[2]);
    case RTENAME:  return py::make_tuple(rb->resval.rlname[0], rb->resval.rlname[1]);
    case RTPICKS:  return py::make_tuple(rb->resval.rlname[0], rb->resval.rlname[1]);
    case RTNIL:
    default:       return py::none();
    }
}

bool PyEdSetVar(const AcString& sym, const TypedValue& value) {
    PyResbuf holder(TVList{ value });
    return edWarn(::acedSetVar(sym.kwszPtr(), holder.data()), L"acedSetVar");
}

AcString PyEdGetEnv(const AcString& sym) {
    AcString r;
    if (!edWarn(::acedGetEnv(sym.kwszPtr(), r), L"acedGetEnv")) return AcString();
    return r;
}

bool PyEdSetEnv(const AcString& sym, const AcString& val) {
    return edWarn(::acedSetEnv(sym.kwszPtr(), val.kwszPtr()), L"acedSetEnv");
}

AcString PyEdFindFile(const AcString& fname) {
    wchar_t buf[1024] = { 0 };
    const int rt = ::acedFindFile(fname.kwszPtr(), buf, _countof(buf));
    if (rt != RTNORM) return AcString(); // not found
    return AcString(buf);
}

AcString PyEdFindTrustedFile(const AcString& fname) {
    wchar_t buf[1024] = { 0 };
    const int rt = ::acedFindTrustedFile(fname.kwszPtr(), buf, _countof(buf));
    if (rt != RTNORM) return AcString(); // not found
    return AcString(buf);
}

// 文件选择对话框：RTNORM 且有路径才返回；取消/失败返回空串，不抛异常。
AcString PyEdGetFileD(const AcString& title, const AcString& defName,
    const AcString& ext, int flags) {
    resbuf* rb = ::acutNewRb(RTSTR);
    if (!rb) return AcString();
    AcString out;
    const int rt = ::acedGetFileD(title.kwszPtr(),
        defName.isEmpty() ? nullptr : defName.kwszPtr(),
        ext.isEmpty() ? nullptr : ext.kwszPtr(), flags, rb);
    if (rt == RTNORM && rb->resval.rstring && *rb->resval.rstring)
        out = rb->resval.rstring;
    ::acutRelRb(rb);
    return out;
}

// acedGetSym：读取 LISP 符号值，首节点转 Python，链由 PyResbuf 释放；失败 None。
py::object PyEdGetSym(const AcString& name) {
    resbuf* val = nullptr;
    if (!edWarn(::acedGetSym(name.kwszPtr(), &val), L"acedGetSym"))
        return py::none();
    PyResbuf holder(val); // 接管并负责释放
    const resbuf* rb = holder.data();
    if (!rb) return py::none();
    switch (rb->restype) {
    case RTSHORT:
    case RTLONG:   return py::int_(rb->resval.rint);
    case RTREAL:   return py::float_(rb->resval.rreal);
    case RTSTR:    return py::cast(AcString(rb->resval.rstring));
    case RTPOINT:  return py::make_tuple(rb->resval.rpoint[0], rb->resval.rpoint[1]);
    case RT3DPOINT:return py::make_tuple(rb->resval.rpoint[0], rb->resval.rpoint[1], rb->resval.rpoint[2]);
    case RTENAME:
    case RTPICKS:  return py::make_tuple(rb->resval.rlname[0], rb->resval.rlname[1]);
    case RTNIL:
    default:       return py::none();
    }
}

// acedPutSym：写 LISP 符号；入参 TypedValue（code=RTSHORT/RTREAL/RTSTR…），内部构造并释放 resbuf。
bool PyEdPutSym(const AcString& name, const TypedValue& value) {
    PyResbuf holder(TVList{ value });
    return edWarn(::acedPutSym(name.kwszPtr(), holder.data()), L"acedPutSym");
}

// acedHelp：调起帮助；失败返回 false。
bool PyEdHelp(const AcString& file, const AcString& topic, int cmd) {
    return edWarn(::acedHelp(file.kwszPtr(), topic.kwszPtr(), cmd), L"acedHelp");
}

// acedHelpForExternal：按函数名调起外部帮助；失败返回 false。
bool PyEdHelpForExternal(const AcString& funcName) {
    return edWarn(::acedHelpForExternal(funcName.kwszPtr()), L"acedHelpForExternal");
}

// acedFNSplit：拆分路径为 (目录, 文件名, 扩展名)；失败返回三个空串。
py::tuple PyEdFNSplit(const AcString& path) {
    wchar_t pre[1024] = { 0 }, name[1024] = { 0 }, ext[1024] = { 0 };
    if (::acedFNSplit(path.kwszPtr(), pre, _countof(pre), name, _countof(name),
                      ext, _countof(ext)) != RTNORM) {
        return py::make_tuple(AcString(), AcString(), AcString());
    }
    return py::make_tuple(AcString(pre), AcString(name), AcString(ext));
}

// acedInvoke：args 为 TVList（首项 TypedValue(RTSTR, 函数名)，其余参数）；失败 None，结果链由 PyResbuf 释放。
py::object PyEdInvoke(const TVList& args) {
    if (args.empty()) {
        acutPrintf(L"[PyArx] acedInvoke: 参数列表不能为空（首项为函数名）\n");
        return py::none();
    }
    PyResbuf in(args);
    resbuf* out = nullptr;
    if (!edWarn(::acedInvoke(in.data(), &out), L"acedInvoke"))
        return py::none();
    PyResbuf holder(out);
    const resbuf* rb = holder.data();
    if (!rb) return py::none();
    switch (rb->restype) {
    case RTSHORT:
    case RTLONG:   return py::int_(rb->resval.rint);
    case RTREAL:   return py::float_(rb->resval.rreal);
    case RTSTR:    return py::cast(AcString(rb->resval.rstring));
    case RTPOINT:  return py::make_tuple(rb->resval.rpoint[0], rb->resval.rpoint[1]);
    case RT3DPOINT:return py::make_tuple(rb->resval.rpoint[0], rb->resval.rpoint[1], rb->resval.rpoint[2]);
    case RTENAME:
    case RTPICKS:  return py::make_tuple(rb->resval.rlname[0], rb->resval.rlname[1]);
    case RTNIL:
    default:       return py::none();
    }
}

// acedUpdate：重生成指定视口区域；p1/p2 均 None 时刷新当前视口全部。
bool PyEdUpdate(const std::optional<AcGePoint3d>& p1, const std::optional<AcGePoint3d>& p2) {
    ads_point a{}, b{};
    ads_real* pa = nullptr;
    ads_real* pb = nullptr;
    if (p1) { a[0] = p1->x; a[1] = p1->y; a[2] = p1->z; pa = a; }
    if (p2) { b[0] = p2->x; b[1] = p2->y; b[2] = p2->z; pb = b; }
    return edWarn(::acedUpdate(0, pa, pb), L"acedUpdate");
}

bool PyEdInitGet(int val, const AcString& kwl) {
    return edWarn(::acedInitGet(val, kwl.isEmpty() ? nullptr : kwl.kwszPtr()), L"acedInitGet");
}

bool PyEdUsrBrk() { return ::acedUsrBrk() != 0; }
int  PyEdIsDragging() { return ::acedIsDragging(); }
bool PyEdIsUpdateDisplayPaused() { return ::acedIsUpdateDisplayPaused(); }
void PyEdUpdateDisplayPause(bool bEnable) { ::acedUpdateDisplayPause(bEnable); }
bool PyEdFiberWorld() { return ::acedFiberWorld(); }

bool PyEdLoadPartialMenu(const AcString& file) {
    return ::acedLoadPartialMenu(file.kwszPtr()) ? true : false;
}

int PyEdGetCommandVersion() { return ::acedGetCommandVersion(); }

bool PyEdMspace() { return PyWarnIfError(::acedMspace(), L"acedMspace"); }
bool PyEdPspace() { return PyWarnIfError(::acedPspace(), L"acedPspace"); }

bool PyEdSetCurrentVPort(int vpnumber) {
    return PyWarnIfError(::acedSetCurrentVPort(vpnumber), L"acedSetCurrentVPort");
}

intptr_t PyEdRegCmdsPtr() {
    // acedRegCmds is a macro -> AcEdCommandStack::cast(...); return opaque ptr.
    return reinterpret_cast<intptr_t>(acedRegCmds);
}

bool PyEdArxLoad(const AcString& app) {
    return edWarn(::acedArxLoad(app.kwszPtr()), L"acedArxLoad");
}
bool PyEdArxUnload(const AcString& app) {
    return edWarn(::acedArxUnload(app.kwszPtr()), L"acedArxUnload");
}

void PyEdRetStr(const AcString& s) { ::acedRetStr(s.kwszPtr()); }
void PyEdRetInt(int v) { ::acedRetInt(v); }
void PyEdRetReal(double v) { ::acedRetReal(v); }
void PyEdRetT() { ::acedRetT(); }
void PyEdRetNil() { ::acedRetNil(); }
void PyEdRetVoid() { ::acedRetVoid(); }

bool PyEdDefun(const AcString& name, int fcode) {
    return edWarn(::acedDefun(name.kwszPtr(), fcode), L"acedDefun");
}
bool PyEdMenucmd(const AcString& s) {
    return edWarn(::acedMenuCmd(s.kwszPtr()), L"acedMenuCmd");
}

//-----------------------------------------------------------------------------
// [NEW] SDK-completion wrappers (non-throwing edWarn / safe-default convention)
//-----------------------------------------------------------------------------
AcString PyEdGetInput() {
    AcString s;
    ::acedGetInput(s); // valid only right after an RTKWORD result; never throws
    return s;
}

AcString PyEdGetAppName() {
    const ACHAR* p = ::acedGetAppName();
    return p ? AcString(p) : AcString();
}

AcString PyEdGetCName(const AcString& cmd) {
    AcString s;
    if (!edWarn(::acedGetCName(cmd.kwszPtr(), s), L"acedGetCName")) return AcString();
    return s;
}

AcString PyEdGetCfg(const AcString& sym) {
    wchar_t buf[1024] = { 0 };
    if (!edWarn(::acedGetCfg(sym.kwszPtr(), buf, _countof(buf)), L"acedGetCfg"))
        return AcString();
    return AcString(buf);
}

bool PyEdSetCfg(const AcString& sym, const AcString& val) {
    return edWarn(::acedSetCfg(sym.kwszPtr(), val.kwszPtr()), L"acedSetCfg");
}

bool PyEdUndef(const AcString& name, int fcode) {
    return edWarn(::acedUndef(name.kwszPtr(), fcode), L"acedUndef");
}

int PyEdGetFunCode() { return ::acedGetFunCode(); }

py::list PyEdArxLoaded() {
    py::list out;
    resbuf* rb = ::acedArxLoaded();
    if (!rb) return out;
    for (resbuf* cur = rb; cur; cur = cur->rbnext) {
        if (cur->restype == RTSTR && cur->resval.rstring)
            out.append(py::cast(AcString(cur->resval.rstring)));
    }
    ::acutRelRb(rb); // caller owns the returned chain
    return out;
}

bool PyEdIsMenuGroupLoaded(const AcString& group) {
    return ::acedIsMenuGroupLoaded(group.kwszPtr()) != Adesk::kFalse;
}

bool PyEdUnloadPartialMenu(const AcString& file) {
    return ::acedUnloadPartialMenu(file.kwszPtr());
}

void PyEdReloadMenus(bool incremental) { ::acedReloadMenus(incremental); }

void PyEdPostCommandPrompt() { ::acedPostCommandPrompt(); }

void PyEdUpdateDisplay() { ::acedUpdateDisplay(); }

bool PyEdIsInBackgroundMode() { return ::acedIsInBackgroundMode(); }

void PyEdEnableUsrbrk() { ::acedEnableUsrbrk(); }
void PyEdDisableUsrbrk() { ::acedDisableUsrbrk(); }
bool PyEdIsUsrbrkDisabled() { return ::acedIsUsrbrkDisabled(); }

bool PyEdGraphScr() { return edWarn(::acedGraphScr(), L"acedGraphScr"); }
bool PyEdTextScr() { return edWarn(::acedTextScr(), L"acedTextScr"); }
bool PyEdTextPage() { return edWarn(::acedTextPage(), L"acedTextPage"); }

bool PyEdRedraw(const std::optional<PyAdsName>& ename, int mode) {
    ads_name n{};
    const int64_t* pn = nullptr; // element pointer; nullptr = whole screen
    if (ename && !ename->isEmpty()) {
        std::memcpy(n, ename->data(), sizeof(n));
        pn = n;
    }
    return edWarn(::acedRedraw(pn, mode), L"acedRedraw");
}

bool PyEdRetPoint(const AcGePoint3d& pt) {
    ads_point p{ pt.x, pt.y, pt.z };
    return edWarn(::acedRetPoint(p), L"acedRetPoint");
}

bool PyEdRetName(const PyAdsName& ename, int type) {
    ads_name n{};
    std::memcpy(n, ename.data(), sizeof(n));
    return edWarn(::acedRetName(n, type), L"acedRetName");
}

bool PyEdRetList(const PyResbuf& rbuf) {
    return edWarn(::acedRetList(rbuf.data()), L"acedRetList");
}

bool PyEdRetVal(const PyResbuf& rbuf) {
    return edWarn(::acedRetVal(rbuf.data()), L"acedRetVal");
}

bool PyEdWcMatch(const AcString& text, const AcString& pattern) {
    return ::acutWcMatch(text.kwszPtr(), pattern.kwszPtr()) == RTNORM;
}
