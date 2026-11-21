#include "dwg_reader/db/index/handle_map.h"

#include "dwg_reader/io/codec/bitstream.h"

namespace dwr {

void HandleMap::build(const uint8_t* data, size_t len, DwgVersion ver) {
    (void)ver;
    m_addresses.clear();
    m_entries.clear();
    m_map.clear();
    m_pages = 0;
    if (!data || len == 0) return;

    BitStream bs(data, len);

    while (bs.byte_ + 2 <= len) {
        const size_t start = bs.byte_;
        int page_size = bs.read_rs_be();
        if (page_size <= 2) break;

        const size_t end = start + static_cast<size_t>(page_size);
        if (end > len) break;

        // 每页页首归零：首条 UMC 即该页首个对象的绝对句柄增量
        uint64_t last_handle = 0;
        int64_t last_offset = 0;

        while (bs.byte_ < end) {
            const size_t oldpos = bs.byte_;
            uint64_t handle_delta = bs.read_umc();
            int offset_delta = bs.read_mc();
            if (bs.byte_ == oldpos) break;  // 位流无进展，防死循环

            last_handle += handle_delta;
            last_offset += offset_delta;
            if (last_offset < 0) break;

            const uint64_t address =
                static_cast<uint64_t>(last_offset) + ADDR_BASE;
            m_entries.push_back(HandleEntry{last_handle, address});
            m_addresses.push_back(address);
            m_map[last_handle] = address;
        }

        ++m_pages;
        // 页尾 CRC（2 字节 BE）
        bs.byte_ = end + 2;
        if (bs.byte_ >= len) break;
    }
}

bool HandleMap::lookup(uint64_t handle, uint64_t& address) const {
    auto it = m_map.find(handle);
    if (it == m_map.end()) return false;
    address = it->second;
    return true;
}

void HandleMap::addMapping(uint64_t handle, uint64_t address) {
    m_map[handle] = address;
}

}  // namespace dwr
