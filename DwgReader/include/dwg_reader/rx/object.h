#pragma once

#include <cstdint>
#include <string>

#include "dwg_reader/value/types.h"

namespace dwr {

/// 自研对象根基类（蓝本 AcRxObject）。
/// 类型标识 / 句柄 / 类名上移至此；db 实体与非图形对象均继承本类。
class RxObject {
public:
    virtual ~RxObject() = default;

    TypeCode typeCode = TypeCode::kText;
    uint64_t handle = 0;
    std::string dxfName;

    RxObject() = default;
    RxObject(uint64_t h, TypeCode tc) : typeCode(tc), handle(h) {}

    /// 运行时类名（蓝本 AcRxObject::isKindOf 的 RTTI 入口，子类可覆写）。
    virtual const char* className() const { return dxfName.c_str(); }

protected:
    RxObject(const RxObject&) = default;
    RxObject(RxObject&&) = default;
    RxObject& operator=(const RxObject&) = default;
};

}  // namespace dwr
