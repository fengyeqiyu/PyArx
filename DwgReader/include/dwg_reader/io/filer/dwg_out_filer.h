#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "dwg_reader/export.h"

namespace dwr {

/// DWG 写出侧的低层字节流（ARX `AcDbDwgFiler` 的对应物）。
/// 读侧对称实现见 `io/codec/bitstream.h`（BitReader）。
/// 目前只覆盖经典布局（R13..R2002）所需原语；R2004+ 的句柄流/字符串流分离待扩。
class DWREADER_API DwgOutFiler {
public:
    virtual ~DwgOutFiler() = default;

    virtual void writeBool(bool v) = 0;
    virtual void writeU8(uint8_t v) = 0;
    virtual void writeI16(int16_t v) = 0;
    virtual void writeI32(int32_t v) = 0;
    virtual void writeI64(int64_t v) = 0;
    virtual void writeDouble(double v) = 0;
    /// 句柄：MC（7 位一组、0x40 符号位），与 `read_handle` 逆运算。
    virtual void writeHandle(uint64_t v) = 0;
    /// UMC / MC 原始编码，供句柄表使用。
    virtual void writeUmc(uint64_t v) = 0;
    virtual void writeMc(int64_t v) = 0;
    /// 字符串：BS 长度 + 原始字节（R2000 及以前为 codepage 字节，R2007+ 待扩 TU）。
    virtual void writeString(const std::string& s) = 0;
    virtual void writeBytes(const uint8_t* p, size_t n) = 0;
};

/// 内存缓冲实现：所有写入落到 `bytes`。
class DWREADER_API MemoryOutFiler : public DwgOutFiler {
public:
    std::vector<uint8_t> bytes;

    void writeBool(bool v) override;
    void writeU8(uint8_t v) override;
    void writeI16(int16_t v) override;
    void writeI32(int32_t v) override;
    void writeI64(int64_t v) override;
    void writeDouble(double v) override;
    void writeHandle(uint64_t v) override;
    void writeUmc(uint64_t v) override;
    void writeMc(int64_t v) override;
    void writeString(const std::string& s) override;
    void writeBytes(const uint8_t* p, size_t n) override;

    // 小工具（非 filer 协议）
    void u8(uint8_t v) { bytes.push_back(v); }
    void u16le(uint16_t v);
    void u32le(uint32_t v);
    void u16be(uint16_t v);
    void padTo(size_t n, uint8_t fill = 0);
    void patchU32(size_t off, uint32_t v);
    size_t size() const { return bytes.size(); }
};

}  // namespace dwr
