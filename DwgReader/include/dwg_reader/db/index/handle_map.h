#pragma once

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "dwg_reader/export.h"
#include "dwg_reader/value/version.h"

namespace dwr {

/// 句柄目录条目：句柄 + 对象流内数据地址。
struct HandleEntry {
    uint64_t handle = 0;
    uint64_t address = 0;
};

/// AcDb:Handles 差分目录：UMC 句柄增量 + MC 地址增量，逐页重建。
/// 口径（按 AutoCAD 2024 行为 + ARX 基准反推）：
///   - 句柄增量与地址增量均在每页页首归零，页内累加；
///   - 地址 = 页内累加偏移 + ADDR_BASE（对象记录 2 字节前缀之后的数据起点）；
///   - 页结构：RS(size,BE) + N×[UMC handle增量 + MC offset增量] + RS(crc,BE)。
class DWREADER_API HandleMap {
public:
    static constexpr uint64_t ADDR_BASE = 2;

    /// 解析 Handles 节解压缓冲；解析失败的尾部空页跳过。
    void build(const uint8_t* data, size_t len, DwgVersion ver);

    /// 按句柄查对象地址；命中返回 true。
    bool lookup(uint64_t handle, uint64_t& address) const;

    /// 回填 handle→address（手工修补用；build 已直接建表）。
    void addMapping(uint64_t handle, uint64_t address);

    size_t size() const { return m_entries.size(); }

    /// 有序对象地址序列（与句柄目录条目顺序一致）。
    const std::vector<uint64_t>& addresses() const { return m_addresses; }

    /// 有序 (handle, address) 条目：ObjectIndex 按此顺序读对象头。
    const std::vector<HandleEntry>& pairs() const { return m_entries; }

    uint32_t pages() const { return m_pages; }

    const std::unordered_map<uint64_t, uint64_t>& entries() const { return m_map; }

private:
    std::vector<uint64_t> m_addresses;
    std::vector<HandleEntry> m_entries;
    std::unordered_map<uint64_t, uint64_t> m_map;
    uint32_t m_pages = 0;
};

}  // namespace dwr
