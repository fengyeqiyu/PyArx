#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

#include "dwg_reader/db/entity/entity.h"
#include "dwg_reader/db/index/object_index.h"
#include "dwg_reader/value/version.h"

namespace dwr {

class BitStream;

/// 跳过 EED 扩展实体数据段（按组码跳读，位流回退到段尾）。
void decodeEed(BitStream& b, const uint8_t* data, size_t len);

/// 读公共 dat 段（EED 之后、类型几何之前）。字段集随版本变化：
/// R13/R14 带 bitsize 与 isbylayerlt；R2004+ 用 ENC 颜色编码；
/// R2007+ 增 material_flags / shadow_flags；R2010+ 增三组 visualstyle 位。
CommonData readCommon(BitStream& b, DwgVersion ver);

/// 读对象句柄流（数据流 bitsize 之后），回填 common.handles。
void readHandles(BitStream& h, CommonData& common, DwgVersion ver);

/// 解码单个对象；未注册类型返回 nullptr。
std::unique_ptr<Entity> decodeObject(const uint8_t* objects, size_t len,
                                     const ObjectRef& ref, DwgVersion ver);

/// R2007+ 独立字符串流的实际起止（位偏移，相对对象记录数据起点）。
/// 返回 false 表示该对象无字符串流。
bool locateStringStream(const uint8_t* objects, size_t len, const ObjectRef& ref,
                        uint64_t& startBit, uint32_t& sizeBits);

}  // namespace dwr
