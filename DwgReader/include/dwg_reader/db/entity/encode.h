#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "dwg_reader/db/entity/entity.h"
#include "dwg_reader/value/version.h"

namespace dwr {

class BitStream;
class BitWriter;

/// 写公共 dat 段（`readCommon` 的镜像）。当前实现覆盖 R13/R14 口径：
/// `B preview` + `RL bitsize` + `BB entmode` + `BL numReactors` +
/// `B isbylayerlt` + `CMC color` + `BD ltypeScale` + `BS invisible`。
void writeCommonR13R14(BitWriter& w, const CommonData& c, uint32_t bitsize);

/// 写单个句柄引用：首字节 = (code<<4)|字节数，随后大批小端序句柄字节。
void writeHandleRef(BitWriter& w, int code, uint64_t v);

/// 写句柄流（`readHandles` 的镜像，R13/R14 口径）。
void writeHandlesR13R14(BitWriter& w, const CommonData& c);

/// 对象记录重编码（R13/R14）：type BS + handle + preview + bitsize + [EED 原样位拷贝]
/// + 公共段 + 几何段 + 句柄流。两遍写：先量长度得到 bitsize，再正式写。
/// - `recBit`：记录在对象区中的起始位（= 对象地址 * 8）
/// - `eedStartBit`/`eedEndBit`：对象头之后到公共段之前的位区间（EED 原样搬运）
/// 返回 false 表示该实体类型未实现写侧。
bool encodeObjectR13R14(const Entity& e, const uint8_t* data, size_t len,
                        uint64_t recBit, uint64_t eedStartBit, uint64_t eedEndBit,
                        std::vector<uint8_t>& out, uint32_t* outBitsize,
                        uint32_t* outPayloadBits);

/// 写 R2000（AC1015）公共段：无 `RL bitsize`，多 ltype_flags/plotstyle_flags 与 lineweight。
void writeCommonR2000(BitWriter& w, const CommonData& c);

/// 写 R2000 句柄流（prev/next 在 layer 之后、ltype 之前）。
void writeHandlesR2000(BitWriter& w, const CommonData& c);

/// R2000 对象记录重编码：MS size → BS type → H handle → [EED] → 公共段 → 几何段
/// → 句柄流 → 补位 → CRC16-IBM。`outDataBits` 为「数据段（含对象头/EED/公共/几何）位长」。
bool encodeObjectR2000(const Entity& e, const uint8_t* data, size_t len,
                       uint64_t recBit, uint64_t eedStartBit, uint64_t eedEndBit,
                       std::vector<uint8_t>& out, uint32_t* outDataBits);

/// 按位拷贝 `[fromBit,toBit)` 到 `w`。
void copyBits(BitWriter& w, const uint8_t* data, size_t len, uint64_t fromBit, uint64_t toBit);

}  // namespace dwr
