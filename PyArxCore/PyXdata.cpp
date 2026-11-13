#include "stdafx.h"
#include "PyXdata.h"
#include "stdafx.h"

#include <acutads.h>
#include "stdafx.h"
#include <acedads.h>
#include "stdafx.h"
#include <acdb.h>

// ============================================================
// 白名单（静态成员，直接引用类内枚举）
// ============================================================
bool PyXdata::isAllowedCode(short c) noexcept {
    switch (c) {
    case kRegAppCode:
    case kWstrCode:
    case kLayerCode:
    case kArrayCode:
    case kPointCode:
    case kDispCode:
    case kDirCode:
    case kDoubleCode:
    case kDistCode:
    case kScaleCode:
    case kInt16Code:
    case kInt32Code:
    case kHandleCode:
        return true;
    default:
        return false;
    }
}

// ============================================================
// 私有工具
// ============================================================
void PyXdata::ensureRegAppHeader(const AcString& name) {
    if (empty() || front().code != kRegAppCode) {
        insert(begin(), TypedValue(kRegAppCode, name));
    }
}

// ============================================================
// 构造
// ============================================================
PyXdata::PyXdata(const AcString& appName) {
    acdbRegApp(appName);
    emplace_back(kRegAppCode, appName);
}

PyXdata::PyXdata(const AcString& appName, const TVList& values)
    : TVList(values) {
    acdbRegApp(appName);
    ensureRegAppHeader(appName);
}

// ============================================================
// AppName
// ============================================================
const AcString& PyXdata::getRegApp() const noexcept {
    return front().value.asString();
}

void PyXdata::setRegApp(const AcString& name) {
    acdbRegApp(name);

    if (!empty() && front().code == kRegAppCode) {
        front().value = DxfValue::fromString(name);
    }
    else {
        ensureRegAppHeader(name);
    }
}

// ============================================================
// 状态
// ============================================================
void PyXdata::setEmpty() noexcept {
    AcString name = empty() ? AcString() : front().value.asString();
    TVList::clear();
    emplace_back(kRegAppCode, name);
}

// ============================================================
// 校验
// ============================================================
bool PyXdata::checkIsValid() const noexcept {
    if (front().code != kRegAppCode) return false;

    for (const auto& tv : *this) {
        if (!isAllowedCode(tv.code)) return false;
    }
    return true;
}

// ============================================================
// 追加
// ============================================================
PyXdata& PyXdata::addShort(short v) { emplace_back(kInt16Code, (int16_t)v); return *this; }
PyXdata& PyXdata::addInt(int v) { emplace_back(kInt32Code, (int32_t)v); return *this; }
PyXdata& PyXdata::addDouble(double v) { emplace_back(kDoubleCode, v);          return *this; }
PyXdata& PyXdata::addDist(double v) { emplace_back(kDistCode, v);          return *this; }
PyXdata& PyXdata::addScale(double v) { emplace_back(kScaleCode, v);          return *this; }
PyXdata& PyXdata::addString(const AcString& s) { emplace_back(kWstrCode, s);          return *this; }
PyXdata& PyXdata::addLayer(const AcString& s) { emplace_back(kLayerCode, s);          return *this; }
PyXdata& PyXdata::addArray(const std::array<double, 3>& a)
{
    emplace_back(kArrayCode, a);          return *this;
}

PyXdata& PyXdata::addPoint(const AcGePoint3d& pt) {
    emplace_back(kPointCode, std::array<double, 3>{ pt.x, pt.y, pt.z });
    return *this;
}

PyXdata& PyXdata::addDisp(const AcGeVector3d& v) {
    emplace_back(kDispCode, std::array<double, 3>{ v.x, v.y, v.z });
    return *this;
}

PyXdata& PyXdata::addDir(const AcGeVector3d& v) {
    emplace_back(kDirCode, std::array<double, 3>{ v.x, v.y, v.z });
    return *this;
}

PyXdata& PyXdata::addHandle(const AcDbHandle& h) {
    uint64_t v = h.get64BitVal();
    std::array<uint8_t, 8> bytes{};
    std::memcpy(bytes.data(), &v, 8);
    emplace_back(kHandleCode, bytes);
    return *this;
}

// ============================================================
// 读写
// ============================================================
void PyXdata::write(AcDbObject* pObj) const {
    PyResbuf rb(*this);
    pObj->setXData(rb.data());
}

void PyXdata::clear(AcDbObject* pObj) {
    setEmpty();
    PyResbuf rb(*this);
    pObj->setXData(rb.data());   // 只有 1001 → AutoCAD 删除该 AppName 的 XData
}

void PyXdata::read(AcDbObject* pObj) {
    const AcString name = getRegApp();
    setEmpty();

    resbuf* xdata = pObj->xData(name);
    if (!xdata) return;

    PyResbuf rb(xdata);
    TVList::operator=(rb.toList());
    ensureRegAppHeader(name);
}

// ============================================================
// 输出
// ============================================================
void PyXdata::print() const {
    acutPrintf(L"%s\n", TVList::toString());
}