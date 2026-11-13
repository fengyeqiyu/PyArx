#pragma once

#include "stdafx.h"
#include "PyDxfTypeValue.h"
#include "PyResbuf.h"

class PyXdata : public TVList {
public:
    // ---- 组码常量 ----
    enum Code : short {
        kRegAppCode = 1001,   // 应用名（1001 头）
        kWstrCode = 1000,   // 字符串
        kLayerCode = 1003,   // 图层名
        kArrayCode = 1010,   // 三维点数组
        kPointCode = 1011,   // 三维点（世界坐标）
        kDispCode = 1012,   // 三维位移向量
        kDirCode = 1013,   // 三维方向向量
        kDoubleCode = 1040,   // 实数
        kDistCode = 1041,   // 距离
        kScaleCode = 1042,   // 比例
        kInt16Code = 1070,   // 16 位整数
        kInt32Code = 1071,   // 32 位整数
        kHandleCode = 1005,   // 句柄
    };

    PyXdata() = default;
    explicit PyXdata(const AcString& appName);
    PyXdata(const AcString& appName, const TVList& values);

    // ---- AppName（走 1001 头）----
    const AcString& getRegApp() const noexcept;
    void setRegApp(const AcString& name);

    // 仅保留 1001 头
    void setEmpty() noexcept;

    // 组码白名单校验
    bool checkIsValid() const noexcept;

    // ---- 追加（链式）----
    PyXdata& addShort(short v);
    PyXdata& addInt(int v);
    PyXdata& addDouble(double v);
    PyXdata& addDist(double v);
    PyXdata& addScale(double v);
    PyXdata& addString(const AcString& s);
    PyXdata& addLayer(const AcString& s);
    PyXdata& addArray(const std::array<double, 3>& a);
    PyXdata& addPoint(const AcGePoint3d& pt);
    PyXdata& addDisp(const AcGeVector3d& v);
    PyXdata& addDir(const AcGeVector3d& v);
    PyXdata& addHandle(const AcDbHandle& h);

    // ---- 读写 ----
    void write(AcDbObject* pObj) const;
    void clear(AcDbObject* pObj);
    void read(AcDbObject* pObj);

    void print() const;

private:
    void ensureRegAppHeader(const AcString& name);
    static bool isAllowedCode(short c) noexcept;   // 白名单
};