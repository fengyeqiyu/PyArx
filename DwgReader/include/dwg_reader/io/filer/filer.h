#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace dwr {

/// DWG 序列化 Filer 基类（蓝本 AcDbDwgFiler）。
/// 读侧当前由 io/codec/BitStream 承担；后续把读位流也收敛到 Filer 接口。
/// 写侧派生类负责把对象树编码回位流（组码 / 句柄 / 几何段）。
class DwgFiler {
public:
    virtual ~DwgFiler() = default;

    // ---- 读位流预留（当前实现见 BitStream，逐步收敛）----
    virtual bool readBool() = 0;
    virtual double readDouble() = 0;
    virtual uint64_t readHandle() = 0;
    virtual std::string readString() = 0;

    // ---- 写位流预留（M5 实现）----
    virtual void writeBool(bool v) = 0;
    virtual void writeDouble(double v) = 0;
    virtual void writeHandle(uint64_t v) = 0;
    virtual void writeString(const std::string& v) = 0;
};

}  // namespace dwr
