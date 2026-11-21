// PyUtils.h - PyStr 命名空间精简版。
// 字符串转换（toPy/toAc/toW）与 AcArray 转换工具（AcArrayToList/ListToAcArray/
// ListToObjectIdArray/ListToPoint2dArray 等）已全部移除：
//   - AcString ↔ Python str 由 PyString.h 的 type_caster<AcString> 完成；
//   - AcArray<T> ↔ list 由 PyArrayCaster.h 的 type_caster<AcArray<T>> 完成
//     （含 AcDbObjectIdArray 的 oldId 特化）。
// 所有绑定参数/返回值统一使用真实 C++ 类型，靠 type_caster 自动转换，不手工转。
#pragma once

#include "stdafx.h"
#include <string>
// 保留数组 typedef 头：其他编译单元 include PyUtils.h 后可能直接使用这些类型。
#include <gedblar.h>
#include <geintarr.h>
#include <dbintar.h>
#include <gept2dar.h>
#include <gept3dar.h>
#include <gevc2dar.h>
#include <gevc3dar.h>
#include <dbidar.h>

namespace PyStr {

// Empty checks for the three common source types.
inline bool isEmpty(const AcString& s) { return s.isEmpty(); }
inline bool isEmpty(const ACHAR* p) { return p == nullptr || p[0] == L'\0'; }
inline bool isEmpty(const std::wstring& ws) { return ws.empty(); }

} // namespace PyStr
