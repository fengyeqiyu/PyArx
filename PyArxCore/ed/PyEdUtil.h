// PyEdUtil.h - pure (non-interactive) editor math/format utilities:
// coordinate transform (acedTrans), segment intersection (acdbInters),
// number/angle formatting and parsing (acdbRToS/acdbAngToS/acdbDisToF/acdbAngToF).
#pragma once

#include "stdafx.h"
#include "PyDxfTypeValue.h"
#include <tuple>

// acedTrans: pt transformed between coordinate systems. from/to 为 TypedValue：
// RTSHORT=坐标系码(0 WCS,1 UCS,2 DCS,3 PSDCS)，RTENAME=实体空间描述符；
// 默认 WCS(0)。disp: 0=结果为点，1=结果为位移向量。
std::tuple<double, double, double> PyEdTrans(const AcGePoint3d& pt,
    const TypedValue& from = TypedValue{ (short)0, (int16_t)0 },
    const TypedValue& to = TypedValue{ (short)0, (int16_t)0 },
    int disp = 0);

// acdbInters: intersection of two segments; extend=true treats them as infinite
// lines. Returns None when no intersection.
py::object PyEdInters(const AcGePoint3d& p1, const AcGePoint3d& p2,
    const AcGePoint3d& p3, const AcGePoint3d& p4, bool extend);

// acdbRToS: real -> localized string (unit=-1/prec=-1 use current settings).
AcString    PyEdRToS(double val, int unit = -1, int prec = -1);
// acdbAngToS: angle (radians) -> localized string.
AcString    PyEdAngToS(double angle, int unit = -1, int prec = -1);
// acdbDisToF: localized distance string -> double (0.0 on parse failure).
double PyEdDisToF(const AcString& s, int unit);
// acdbAngToF: localized angle string -> radians (0.0 on parse failure).
double PyEdAngToF(const AcString& s, int unit);

// acedTextBox: 由文字实体 DXF TvList（10 位置 / 40 高度 / 50 旋转 / 1 内容）
// 计算包围盒，返回 ((x,y,z),(x,y,z))；失败返回 None。
py::object PyEdTextBox(const TVList& dxf);

// ---- acut* 实用函数（acutads.h）----
void PyAcutPrintf(const AcString& fmt);
double PyAcutCvUnit(double value, const AcString& oldunit, const AcString& newunit);
double PyAcutAngle(const AcGePoint3d& p1, const AcGePoint3d& p2);
double PyAcutDistance(const AcGePoint3d& p1, const AcGePoint3d& p2);
AcGePoint3d PyAcutPolar(const AcGePoint3d& pt, double angle, double dist);

// ---- acdb 原始角度格式（不跟随当前角度制式）----
AcString    PyAcdbRawAngToS(double angle, int unit = -1, int prec = -1);
double PyAcdbRawAngToF(const AcString& s, int unit);

// ---- 符号名合法性验证 ----
bool PyAcdbSNValid(const AcString& s, bool pipetest);
