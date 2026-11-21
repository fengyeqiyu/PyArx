#include "dwg_reader/db/index/object_index.h"

#include "dwg_reader/io/codec/bitstream.h"

namespace dwr {

namespace {

/// 读对象头（按版本口径）。addr 为对象记录起点（前缀之后的数据起点）。
///   - R13/R14  ：type=BS、handle=H（bitsize 在实体公共段里，不在对象头）
///   - R2000..R2007：type=BS、bitsize=RL、handle=H
///   - R2010+   ：size=MS、handlestream=UMC、type=BOT、handle=H，
///                bitsize = size*8 - handlestream
bool readObjectHead(const uint8_t* objects, size_t len, uint64_t addr,
                    DwgVersion ver, ObjectRef& out) {
    if (addr >= len) return false;
    BitStream b(objects, len);
    b.byte_ = static_cast<size_t>(addr);
    b.bit_ = 0;
    out.address = addr;
    out.recordSize = 0;
    out.handlestream = 0;

    if (ver >= DwgVersion::kAc1015 && ver < DwgVersion::kAc1018) {
        // R2000/R2002：MS size → BS type → RL bitsize → H handle
        // （libredwg decode.c:4496-4519 / dwg_decode_entity 的 VERSIONS(R_2000,R_2007) 分支）
        uint32_t size = b.read_ms();
        out.recordSize = size;
        out.typeCode = b.read_bs();
        out.bitsize = b.read_rl();
        out.handle = b.read_handle();
        return true;
    }
    if (ver < DwgVersion::kAc1018) {
        // 经典布局（R13/R14/R2000/R2002）：MS size → type BS → handle H。
        // R13/R14 的 bitsize 在实体公共段里（不在这里），这里只按 size 给上界。
        uint32_t size = b.read_ms();
        out.recordSize = size;
        out.typeCode = b.read_bs();
        out.handle = b.read_handle();
        out.bitsize = size * 8;
        return true;
    }
    if (hasHandleStreamSize(ver)) {
        uint32_t size = b.read_ms();
        uint32_t hdl = static_cast<uint32_t>(b.read_umc());
        out.recordSize = size;
        out.handlestream = hdl;
        out.typeCode = b.read_bot();
        out.handle = b.read_handle();
        out.bitsize = size * 8 - hdl;
        return true;
    }
    out.typeCode = b.read_bs();
    out.bitsize = b.read_rl();
    out.handle = b.read_handle();
    return true;
}

}  // namespace

void ObjectIndex::build(const uint8_t* objects, size_t len,
                        const std::vector<HandleEntry>& pairs, DwgVersion ver) {
    m_objects.clear();
    m_byHandle.clear();
    m_realigned = 0;
    m_mismatched = 0;
    if (!objects || len == 0) return;

    // 目录地址语义随版本不同：
    //  R2004/R2007：地址即记录起点（type BS 处）；
    //  R2010+    ：地址指向记录内 MS(size) 之后 2 字节处，起点 = 地址 - 2。
    // 仍保留 ±探测兜底，用于少数写入器偏移异常（自校验条件：头内 handle == 目录 handle）。
    const bool r2010 = ver >= DwgVersion::kAc1024;
    static const int kOffR2010[] = {-2, 0, 2, -4};
    static const int kOffOld[] = {0, 2, -2, 4};
    const int* offsets = r2010 ? kOffR2010 : kOffOld;

    m_objects.reserve(pairs.size());
    for (const auto& e : pairs) {
        if (e.address >= len) break;

        ObjectRef ref;
        bool matched = false;
        for (int k = 0; k < 4; ++k) {
            const int off = offsets[k];
            if (off < 0 && static_cast<int64_t>(e.address) + off < 0) continue;
            const uint64_t a = static_cast<uint64_t>(static_cast<int64_t>(e.address) + off);
            if (!readObjectHead(objects, len, a, ver, ref)) continue;
            if (ref.handle == e.handle) {
                if (off != offsets[0]) ++m_realigned;  // 仅统计偏离版本基准偏移的个案
                matched = true;
                break;
            }
        }
        if (!matched) {
            readObjectHead(objects, len, e.address, ver, ref);
            ref.handle = e.handle;  // 仍对不上时以目录为准
            ++m_mismatched;
        }

        m_byHandle[e.handle] = m_objects.size();
        m_objects.push_back(ref);
    }
}

void ObjectIndex::build(const uint8_t* objects, size_t len,
                        const std::vector<uint64_t>& addresses, DwgVersion ver) {
    m_objects.clear();
    m_byHandle.clear();
    m_realigned = 0;
    m_mismatched = 0;
    if (!objects || len == 0) return;

    m_objects.reserve(addresses.size());
    for (uint64_t addr : addresses) {
        if (addr >= len) break;
        ObjectRef ref;
        if (!readObjectHead(objects, len, addr, ver, ref)) break;
        m_byHandle[ref.handle] = m_objects.size();
        m_objects.push_back(ref);
    }
}

const ObjectRef* ObjectIndex::findByHandle(uint64_t handle) const {
    auto it = m_byHandle.find(handle);
    if (it == m_byHandle.end()) return nullptr;
    return &m_objects[it->second];
}

}  // namespace dwr
