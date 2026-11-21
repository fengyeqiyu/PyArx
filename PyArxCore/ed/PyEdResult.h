// PyEdResult.h - PromptStatus / OptionFlags enums, Result<T> family and the
// entity / sub-entity payload value structs shared by the PyEd interactive
// helper family. Pure header (template + POD); no .cpp required.
//
// Moved verbatim from PyEdBinder.h during the PyEdFuncs split; no logic changed.
#pragma once

#include "stdafx.h"
#include "PyResbuf.h"
#include <tuple>

class PyEdSelectionSets; // 供 SSResult = PyEdResult<PyEdSelectionSets> 别名使用

// ===========================================================================
// Interactive prompt infrastructure (PromptStatus / Result family)
// ===========================================================================
enum class PromptStatus : int {
    kCancel   = -5002,
    kNone     = 5000,
    kError    = -5001,
    kKeyword  = -5005,
    kOK       = 5100,
    kModeless = 5027,
    kOther    = 5028
};

enum class OptionFlags : int {
    None         = 0,
    NoNull       = 1,     // RSG_NONULL
    NoZero       = 2,     // RSG_NZERO
    NoNegative   = 4,     // RSG_NNEG
    UseDashLine  = 8,     // RSG_DASH
    NoLimit      = 32,    // RSG_NOLIM
    Only2D       = 64,    // RSG_2D
    DDISTFIRST   = 128,   // RSG_DDISTFIRST
    TrackUcs     = 256,
    NoOrthoZ     = 512,
    NoOsnap      = 1024,
    Other        = 0x8000
};

template<typename T>
struct PyEdResult {
    T value{};
    PromptStatus status = PromptStatus::kError;

    // 隐式转换：直接取用底层值（如 int r = someIntResult;）
    operator T() const { return value; }

    bool isOk() const      { return status == PromptStatus::kOK; }
    bool isCanceled() const { return status == PromptStatus::kCancel; }
    bool isModeless() const { return status == PromptStatus::kModeless; }
    bool isError() const   { return status == PromptStatus::kError; }
    bool isNone() const    { return status == PromptStatus::kNone; }
    bool isOther() const   { return status == PromptStatus::kOther; }
    bool isKeyWord() const  { return status == PromptStatus::kKeyword; }
};

struct EntityValue {
    AcDbObjectId id;
    AcGePoint3d pickedPoint;
};

struct SubEntityValue {
    AcDbObjectId id;
    AcGePoint3d pickedPoint;
    AcGeMatrix3d xform;
    AcDbObjectId container;
    PyResbuf refStack; // nentsel reference path chain, owned by PyResbuf
};

using IntResult       = PyEdResult<int>;
using DoubleResult    = PyEdResult<double>;
using StringResult    = PyEdResult<AcString>;
using PointResult     = PyEdResult<AcGePoint3d>;
using EntityResult    = PyEdResult<EntityValue>;
using SubEntityResult = PyEdResult<SubEntityValue>;
using SSResult        = PyEdResult<PyEdSelectionSets>;   // 选择集结果：value 为拥有式选择集
