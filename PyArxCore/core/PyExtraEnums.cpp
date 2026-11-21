#include "stdafx.h"
#include "PyAdsCode.h"
#include "PyCommandFlags.h"   // CommandFlags（原 PyRxCmd.h，命令管理已迁 PyArxEngine）
#include <aced.h>
#include <dbmain.h>
#include <dbLoftOptions.h>
#include "PyEdResult.h"
#include <adscodes.h>

// resbuf 数据类型枚举（封装 adscodes.h RT* 宏）
enum class ResbufType : int {
    kNone = RTNONE,
    kReal = RTREAL,
    kPoint = RTPOINT,
    kShort = RTSHORT,
    kAngle = RTANG,
    kString = RTSTR,
    kEntityName = RTENAME,
    kPickSet = RTPICKS,
    kOrientation = RTORINT,
    kLong = RTLONG,
    kVoid = RTVOID,
    kListBegin = RTLB,
    kListEnd = RTLE,
    kDottedPair = RTDOTE,
    kNil = RTNIL,
    kDxf0 = RTDXF0,
    kT = RTT,
    kResbuf = RTRESBUF,
    kModeless = RTMODELESS,
    kLongPtr = RTLONG_PTR,
    kInt64 = RTINT64,
    k3dPoint = RT3DPOINT,
    kCoroutineInfo = RTCOROUTINEINFO,
};

// ADS 函数返回码枚举（封装 adscodes.h RTNORM/RTERROR 等）
enum class AdsReturn : int {
    kNorm = RTNORM,
    kError = RTERROR,
    kCancel = RTCAN,
    kReject = RTREJ,
    kFail = RTFAIL,
    kKeyword = RTKWORD,
    kInputTruncated = RTINPUTTRUNCATED,
};

// 从各子模块迁移到 Enum 模块的枚举
void bind_PyExtraEnums(pym& m) {
    // CommandFlags（原 PyCmdBinder.cpp）
    py::enum_<CommandFlags>(m, "CommandFlags", py::arithmetic())
        .value("Modal", CommandFlags::Modal)
        .value("Transparent", CommandFlags::Transparent)
        .value("UsePickSet", CommandFlags::UsePickSet)
        .value("Redraw", CommandFlags::Redraw)
        .value("NoPerspective", CommandFlags::NoPerspective)
        .value("NoMultiple", CommandFlags::NoMultiple)
        .value("NoTileMode", CommandFlags::NoTileMode)
        .value("NoPaperSpace", CommandFlags::NoPaperSpace)
        .value("NoOem", CommandFlags::NoOem)
        .value("Undefined", CommandFlags::Undefined)
        .value("InProgress", CommandFlags::InProgress)
        .value("Defun", CommandFlags::Defun)
        .value("NoNewStack", CommandFlags::NoNewStack)
        .value("NoInternalLock", CommandFlags::NoInternalLock)
        .value("DocReadLock", CommandFlags::DocReadLock)
        .value("DocExclusiveLock", CommandFlags::DocExclusiveLock)
        .value("Session", CommandFlags::Session)
        .value("Interruptible", CommandFlags::Interruptible)
        .value("NoHistory", CommandFlags::NoHistory)
        .value("NoUndoMarker", CommandFlags::NoUndoMarker)
        .value("NoBlockEditor", CommandFlags::NoBlockEditor)
        .value("NoActionRecording", CommandFlags::NoActionRecording)
        .value("ActionMacro", CommandFlags::ActionMacro)
        .value("NoInferConstraint", CommandFlags::NoInferConstraint)
        .export_values();

    // ContextMenuMode（原 PyEdBinder.cpp）
    py::enum_<AcadContextMenuMode>(m, "ContextMenuMode")
        .value("kDefault", kDefault)
        .value("kEdit", kEdit)
        .value("kCommand", kCommand)
        .value("kHotGrip", kHotGrip)
        .value("kOsnap", kOsnap);

    // PromptStatus（PyEdResult 用，值与 AdsReturn 一致，不重复注册）

    // OptionFlags（原 PyEdBinder.cpp）
    py::enum_<OptionFlags>(m, "OptionFlags", py::arithmetic())
        .value("None", OptionFlags::None)
        .value("NoNull", OptionFlags::NoNull)
        .value("NoZero", OptionFlags::NoZero)
        .value("NoNegative", OptionFlags::NoNegative)
        .value("UseDashLine", OptionFlags::UseDashLine)
        .value("NoLimit", OptionFlags::NoLimit)
        .value("Only2D", OptionFlags::Only2D)
        .value("DDISTFIRST", OptionFlags::DDISTFIRST)
        .value("TrackUcs", OptionFlags::TrackUcs)
        .value("NoOrthoZ", OptionFlags::NoOrthoZ)
        .value("NoOsnap", OptionFlags::NoOsnap)
        .value("Other", OptionFlags::Other);

    // NormalOption（原 PyDbLoftOptions.cpp）
    py::enum_<AcDbLoftOptions::NormalOption>(m, "LoftNormalOption")
        .value("kNoNormal", AcDbLoftOptions::kNoNormal)
        .value("kFirstNormal", AcDbLoftOptions::kFirstNormal)
        .value("kLastNormal", AcDbLoftOptions::kLastNormal)
        .value("kEndsNormal", AcDbLoftOptions::kEndsNormal)
        .value("kAllNormal", AcDbLoftOptions::kAllNormal)
        .value("kUseDraftAngles", AcDbLoftOptions::kUseDraftAngles);

    // resbuf 数据类型（adscodes.h RT* 宏分类封装）
    py::enum_<ResbufType>(m, "ResbufType")
        .value("kNone", ResbufType::kNone)
        .value("kReal", ResbufType::kReal)
        .value("kPoint", ResbufType::kPoint)
        .value("kShort", ResbufType::kShort)
        .value("kAngle", ResbufType::kAngle)
        .value("kString", ResbufType::kString)
        .value("kEntityName", ResbufType::kEntityName)
        .value("kPickSet", ResbufType::kPickSet)
        .value("kOrientation", ResbufType::kOrientation)
        .value("kLong", ResbufType::kLong)
        .value("kVoid", ResbufType::kVoid)
        .value("kListBegin", ResbufType::kListBegin)
        .value("kListEnd", ResbufType::kListEnd)
        .value("kDottedPair", ResbufType::kDottedPair)
        .value("kNil", ResbufType::kNil)
        .value("kDxf0", ResbufType::kDxf0)
        .value("kT", ResbufType::kT)
        .value("kResbuf", ResbufType::kResbuf)
        .value("kModeless", ResbufType::kModeless)
        .value("kLongPtr", ResbufType::kLongPtr)
        .value("kInt64", ResbufType::kInt64)
        .value("k3dPoint", ResbufType::k3dPoint)
        .value("kCoroutineInfo", ResbufType::kCoroutineInfo)
        .export_values();

    // ADS 函数返回码（adscodes.h 负值/RTNORM 分类封装）
    py::enum_<AdsReturn>(m, "AdsReturn")
        .value("kNorm", AdsReturn::kNorm)
        .value("kError", AdsReturn::kError)
        .value("kCancel", AdsReturn::kCancel)
        .value("kReject", AdsReturn::kReject)
        .value("kFail", AdsReturn::kFail)
        .value("kKeyword", AdsReturn::kKeyword)
        .value("kInputTruncated", AdsReturn::kInputTruncated)
        .export_values();
}
