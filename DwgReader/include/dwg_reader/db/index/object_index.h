#pragma once

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "dwg_reader/db/index/handle_map.h"
#include "dwg_reader/value/version.h"

namespace dwr {

/// 对象头索引条目：主流地址、句柄、类型码、主流位长（bitsize）。
struct ObjectRef {
    uint64_t address = 0;      ///< 对象数据起点（记录前缀之后）
    uint64_t handle = 0;
    int typeCode = 0;
    uint32_t bitsize = 0;      ///< 主流（数据流）位长
    uint32_t recordSize = 0;   ///< R2010+ 记录 MS 长度（含数据+句柄+字符串流）
    uint32_t handlestream = 0; ///< R2010+ 句柄流位长（UMC，记录尾部）
};

/// 对象头表：按句柄目录给出的地址序列，在主流缓冲逐条读对象头，重建
/// addr↔handle↔typeCode 索引。对象头随版本变化：
///   - R13..R2007：type=BS、bitsize=RL、handle=H
///   - R2010+    ：size=MS、handlestream=UMC、type=BOT、handle=H（bitsize=size*8-hdl）
class ObjectIndex {
public:
    /// objects=解压后的 AcDb:AcDbObjects 主流缓冲；pairs=HandleMap 有序条目。
    /// 读头后校验「头内 handle == 目录 handle」，不一致时按对象记录前缀长度
    /// 差异做 ±2 对齐重试（实测命中率见 docs/arx-dwg-read-pipeline.md）。
    void build(const uint8_t* objects, size_t len,
               const std::vector<HandleEntry>& pairs, DwgVersion ver);

    /// 无句柄目录时的兼容入口：只按地址序列读头。
    void build(const uint8_t* objects, size_t len,
               const std::vector<uint64_t>& addresses, DwgVersion ver);

    const std::vector<ObjectRef>& objects() const { return m_objects; }
    size_t size() const { return m_objects.size(); }

    /// 因前缀长度异常做过 ±2 对齐的条目数（诊断用）。
    uint32_t realigned() const { return m_realigned; }

    /// 头内 handle 与目录 handle 仍不一致的条目数（诊断用）。
    uint32_t mismatched() const { return m_mismatched; }

    const ObjectRef* findByHandle(uint64_t handle) const;

private:
    std::vector<ObjectRef> m_objects;
    std::unordered_map<uint64_t, size_t> m_byHandle;
    uint32_t m_realigned = 0;
    uint32_t m_mismatched = 0;
};

}  // namespace dwr
