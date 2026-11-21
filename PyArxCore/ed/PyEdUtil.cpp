// PyEdUtil.cpp - pure editor math/format utilities ([NEW] SDK completion).
#include "stdafx.h"
#include "PyEdUtil.h"
#include "PyEdCommon.h"
#include "PyResbuf.h"

std::tuple<double, double, double> PyEdTrans(const AcGePoint3d& pt,
    const TypedValue& from, const TypedValue& to, int disp) {
    ads_point p{ pt.x, pt.y, pt.z };
    // from/to 各为单节点描述符：RTSHORT=坐标系码，RTENAME=实体空间。
    // 直接由 TypedValue 经 PyResbuf 构造 resbuf 节点，复用统一的 code->resval 映射。
    PyResbuf f(TVList{ from });
    PyResbuf t(TVList{ to });
    ads_point res{};
    const int rt = ::acedTrans(p, f.data(), t.data(), disp, res);
    if (!edWarn(rt, L"acedTrans")) return { 0.0, 0.0, 0.0 };
    return { res[0], res[1], res[2] };
}

py::object PyEdInters(const AcGePoint3d& p1, const AcGePoint3d& p2,
    const AcGePoint3d& p3, const AcGePoint3d& p4, bool extend) {
    ads_point a{ p1.x, p1.y, p1.z }, b{ p2.x, p2.y, p2.z };
    ads_point c{ p3.x, p3.y, p3.z }, d{ p4.x, p4.y, p4.z };
    ads_point res{};
    if (::acdbInters(a, b, c, d, extend ? 1 : 0, res) != RTNORM)
        return py::none();
    return py::make_tuple(res[0], res[1], res[2]);
}

AcString PyEdRToS(double val, int unit, int prec) {
    AcString s;
    if (!edWarn(::acdbRToS(val, s, unit, prec), L"acdbRToS")) return AcString();
    return s;
}

AcString PyEdAngToS(double angle, int unit, int prec) {
    AcString s;
    if (!edWarn(::acdbAngToS(angle, s, unit, prec), L"acdbAngToS")) return AcString();
    return s;
}

double PyEdDisToF(const AcString& s, int unit) {
    double v = 0.0;
    if (!edWarn(::acdbDisToF(s.kwszPtr(), unit, &v), L"acdbDisToF")) return 0.0;
    return v;
}

double PyEdAngToF(const AcString& s, int unit) {
    double v = 0.0;
    if (!edWarn(::acdbAngToF(s.kwszPtr(), unit, &v), L"acdbAngToF")) return 0.0;
    return v;
}

// acedTextBox：文字 DXF TvList -> 包围盒两角点；失败返回 None，不抛异常。
py::object PyEdTextBox(const TVList& dxf) {
    PyResbuf rb(dxf);
    ads_point p1{}, p2{};
    if (::acedTextBox(rb.data(), p1, p2) != RTNORM)
        return py::none();
    return py::make_tuple(
        py::make_tuple(p1[0], p1[1], p1[2]),
        py::make_tuple(p2[0], p2[1], p2[2]));
}

// ---- acut* 实用函数 ----
void PyAcutPrintf(const AcString& fmt) {
    ::acutPrintf(L"%ls", fmt.kwszPtr());
}

double PyAcutCvUnit(double value, const AcString& oldunit, const AcString& newunit) {
    double res = 0.0;
    if (!edWarn(::acutCvUnit(value, oldunit.kwszPtr(), newunit.kwszPtr(), &res), L"acutCvUnit"))
        return 0.0;
    return res;
}

double PyAcutAngle(const AcGePoint3d& p1, const AcGePoint3d& p2) {
    ads_point a{ p1.x, p1.y, p1.z }, b{ p2.x, p2.y, p2.z };
    return ::acutAngle(a, b);
}

double PyAcutDistance(const AcGePoint3d& p1, const AcGePoint3d& p2) {
    ads_point a{ p1.x, p1.y, p1.z }, b{ p2.x, p2.y, p2.z };
    return ::acutDistance(a, b);
}

AcGePoint3d PyAcutPolar(const AcGePoint3d& pt, double angle, double dist) {
    ads_point p{ pt.x, pt.y, pt.z }, res{};
    ::acutPolar(p, angle, dist, res);
    return AcGePoint3d(res[0], res[1], res[2]);
}

AcString PyAcdbRawAngToS(double angle, int unit, int prec) {
    AcString s;
    if (!edWarn(::acdbRawAngToS(angle, s, unit, prec), L"acdbRawAngToS")) return AcString();
    return s;
}

double PyAcdbRawAngToF(const AcString& s, int unit) {
    double v = 0.0;
    if (!edWarn(::acdbRawAngToF(s.kwszPtr(), unit, &v), L"acdbRawAngToF")) return 0.0;
    return v;
}

bool PyAcdbSNValid(const AcString& s, bool pipetest) {
    return ::acdbSNValid(s.kwszPtr(), pipetest ? 1 : 0) == RTNORM;
}
