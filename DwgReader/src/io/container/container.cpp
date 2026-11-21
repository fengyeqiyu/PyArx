#include "dwg_reader/io/container/container.h"

#include <fstream>
#include <stdexcept>

#include "dwg_reader/io/codec/decompress.h"
#include "dwg_reader/value/version.h"

namespace dwr {

namespace {

constexpr uint32_t SIG_PAGE_MAP = 0x41630E3Bu;
constexpr uint32_t SIG_SECTION_MAP = 0x4163003Bu;
constexpr uint32_t SIG_DATA_PAGE = 0x4163043Bu;
constexpr uint32_t SEC_MASK = 0x4164536Bu;
constexpr uint64_t FIRST_SECTION_ADDR = 0x100;

uint32_t le32(const uint8_t* p) {
    return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
           (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
}
uint16_t le16(const uint8_t* p) {
    return static_cast<uint16_t>(p[0]) | (static_cast<uint16_t>(p[1]) << 8);
}
uint64_t le64(const uint8_t* p) {
    uint64_t v = 0;
    for (int i = 0; i < 8; ++i) v |= static_cast<uint64_t>(p[i]) << (8 * i);
    return v;
}
int32_t lei32(const uint8_t* p) { return static_cast<int32_t>(le32(p)); }

std::vector<uint8_t> magic(int n) {
    std::vector<uint8_t> out;
    out.reserve(n);
    uint32_t seed = 1;
    for (int i = 0; i < n; ++i) {
        seed = (seed * 0x343FDu + 0x269EC3u) & 0xFFFFFFFFu;
        out.push_back(static_cast<uint8_t>((seed >> 16) & 0xFF));
    }
    return out;
}

size_t findSigFrom(const std::vector<uint8_t>& data, uint32_t sig, size_t from) {
    uint8_t pat[4] = {static_cast<uint8_t>(sig & 0xFF), static_cast<uint8_t>((sig >> 8) & 0xFF),
                      static_cast<uint8_t>((sig >> 16) & 0xFF), static_cast<uint8_t>((sig >> 24) & 0xFF)};
    for (size_t i = from; i + 4 <= data.size(); ++i) {
        if (data[i] == pat[0] && data[i + 1] == pat[1] && data[i + 2] == pat[2] &&
            data[i + 3] == pat[3]) {
            return i;
        }
    }
    return std::string::npos;
}

std::string cstr(const uint8_t* p, size_t maxLen) {
    size_t n = 0;
    while (n < maxLen && p[n] != 0) ++n;
    return std::string(reinterpret_cast<const char*>(p), n);
}

}  // namespace

uint64_t Container::pageAddr(int pageId) const {
    auto it = pageMap.find(pageId);
    if (it != pageMap.end() && it->second.address)
        return it->second.address;
    throw std::runtime_error("page id not in page map");
}

std::vector<uint8_t> Container::readSystemPage(uint32_t sig) const {
    // 同一签名可能命中多处（数据页 / 旧页残留），逐个尝试放到第一个能解出
    // 合理长度 payload 的命中。R2013 等文件页残留较多，必须择优。
    size_t from = 0;
    while (from + 4 <= data.size()) {
        size_t hit = findSigFrom(data, sig, from);
        from = (hit == std::string::npos) ? std::string::npos : hit + 1;
        if (hit == std::string::npos) break;
        const uint8_t* p = data.data() + hit;
        if (hit + 0x14 > data.size()) continue;
        uint32_t dsize = le32(p + 4);
        uint32_t csize = le32(p + 8);
        uint32_t ctype = le32(p + 12);
        if (csize == 0) continue;
        if (hit + 0x14 + csize > data.size()) continue;

        const uint8_t* payload = p + 0x14;
        if (ctype == 2) {
            // R2004 族：头部两个长度字段的先后顺序在个别写入器上相反
            // （实测 2004/2018 样本 comp 在前、decomp 在后），统一按
            // 「解压到 max(两长度)+余量」处理，再校验长度合理性。
            uint32_t cap = (dsize > csize ? dsize : csize) * 8 + 0x1000;
            if (cap > (1u << 24)) cap = (1u << 24);
            std::vector<uint8_t> out;
            try {
                out = decompress_to_vec(payload, csize, cap, hit + 0x14);
            } catch (...) {
                out.clear();
            }
            if (!out.empty()) return out;
            continue;  // 本命中解压失败，试下一处
        }
        return std::vector<uint8_t>(payload, payload + dsize);
    }
    throw std::runtime_error("system page signature not found");
}

std::vector<uint8_t> Container::readDataPage(int pageId, DataPageHdr& hdr) const {
    uint64_t addr = pageAddr(pageId);
    const uint8_t* p = data.data() + addr;
    uint32_t mask = SEC_MASK ^ static_cast<uint32_t>(addr);
    uint32_t raw[8];
    for (int i = 0; i < 8; ++i) raw[i] = le32(p + 4 * i) ^ mask;

    // R2004 页头按 0x4164536B^页地址 掩码；R2010+ 沿用同一排布，但部分写入器不掩码，
    // 两种口径都试，命中 0x4163043B 的那个为准。
    if (raw[0] != SIG_DATA_PAGE) {
        for (int i = 0; i < 8; ++i) raw[i] = le32(p + 4 * i);
    }
    uint32_t ptype = raw[0];
    if (ptype != SIG_DATA_PAGE) throw std::runtime_error("data page bad type");

    hdr.sectionType = raw[1];
    hdr.dataSize = raw[2];
    hdr.pageSize = raw[3];
    hdr.startOffset = raw[4];
    hdr.compressed = hdr.dataSize < hdr.pageSize;

    const uint8_t* payload = p + 0x20;
    return std::vector<uint8_t>(payload, payload + hdr.dataSize);
}

std::vector<uint8_t> Container::readSection(const std::string& name) const {
    auto it = sections.find(name);
    if (it == sections.end()) throw std::runtime_error("section not found: " + name);
    const SectionDesc& s = it->second;
    size_t limit = static_cast<size_t>(s.maxPageSize) * s.pageCount;
    std::vector<uint8_t> buf(limit, 0);

    for (const auto& pg : s.pages) {
        DataPageHdr hdr;
        std::vector<uint8_t> payload = readDataPage(pg.pageId, hdr);
        size_t off = static_cast<size_t>(pg.startOffset);
        if (hdr.compressed) {
            decompress(payload.data(), payload.size(), limit, buf.data(), off, 0);
        } else {
            size_t n = std::min<size_t>(hdr.pageSize, payload.size());
            for (size_t k = 0; k < n; ++k) buf[off + k] = payload[k];
        }
    }
    return buf;
}

Container openDwg(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("cannot open: " + path);
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)),
                              std::istreambuf_iterator<char>());
    if (data.size() < 0x80 + 0x6C) throw std::runtime_error("file too small");
    char code[7] = {};
    for (int i = 0; i < 6; ++i) code[i] = static_cast<char>(data[i]);
    DwgVersion ver = versionFromCode(code);
    if (ver < DwgVersion::kAc1018 || ver > DwgVersion::kAc1032) {
        throw std::runtime_error("unsupported version for R2004 container: " + std::string(code));
    }

    Container c;
    c.data = std::move(data);
    c.version = code;
    c.maintenance = c.data[0x0B];
    c.codepage = le16(c.data.data() + 0x13);
    c.security = le32(c.data.data() + 0x18);

    std::vector<uint8_t> mg = magic(0x6C);
    std::vector<uint8_t> hdr(0x6C);
    for (int i = 0; i < 0x6C; ++i) hdr[i] = c.data[0x80 + i] ^ mg[i];
    if (std::string(reinterpret_cast<const char*>(hdr.data()), 11) != "AcFssFcAJMB") {
        throw std::runtime_error("file header magic mismatch; decrypt failed");
    }

    // --- System Section Page Map（每项 8 字节 {id, size}，负 id 为 gap，额外 16 字节） ---
    try {
        std::vector<uint8_t> pm = c.readSystemPage(SIG_PAGE_MAP);
        size_t pos = 0;
        uint64_t addr = FIRST_SECTION_ADDR;
        int gaps = 0;
        while (pos + 8 <= pm.size()) {
            int32_t pageId = lei32(pm.data() + pos);
            int32_t sectionSize = lei32(pm.data() + pos + 4);
            pos += 8;
            if (pageId == 0 && sectionSize == 0) break;  // 尾部 0 填充
            PageEntry e;
            e.pageId = pageId;
            e.sectionSize = sectionSize;
            e.address = addr;
            if (pageId < 0) {
                e.isGap = true;
                if (pos + 16 <= pm.size()) {
                    e.parent = lei32(pm.data() + pos);
                    e.left = lei32(pm.data() + pos + 4);
                    e.right = lei32(pm.data() + pos + 8);
                    pos += 16;
                }
                ++gaps;
                c.gaps.push_back(e);  // gap 单独存放，避免与真实页号冲突
            } else {
                c.pageMap[pageId] = e;
            }
            addr += static_cast<uint64_t>(sectionSize);  // gap 也占地址空间
        }
        c.lastPageAddress = addr;
        c.diag += "gaps=" + std::to_string(gaps) + " end=" + std::to_string(addr) + "; ";
    } catch (const std::exception& e) {
        c.diag += std::string("page_map: ") + e.what() + "; ";
    }

    // --- Section Map ---
    std::vector<uint8_t> sm;
    try {
        sm = c.readSystemPage(SIG_SECTION_MAP);
    } catch (const std::exception& e) {
        c.diag += std::string("section_map: ") + e.what() + "; ";
        sm.clear();
    }
    c.diag += "pageMap=" + std::to_string(c.pageMap.size()) +
              " sectionMapBuf=" + std::to_string(sm.size()) + "; ";
    if (c.pageMap.size() <= 40) {
        c.diag += "pmIds=";
        for (const auto& kv : c.pageMap) c.diag += std::to_string(kv.first) + ",";
        c.diag += "; ";
    }
    if (sm.size() >= 4) {
        uint32_t num = le32(sm.data());
        size_t pos = 0x14;
        for (uint32_t i = 0; i < num; ++i) {
            SectionDesc d;
            d.size = le64(sm.data() + pos);
            d.pageCount = le32(sm.data() + pos + 8);
            d.maxPageSize = le32(sm.data() + pos + 12);
            d.unknown = le32(sm.data() + pos + 16);
            d.compressed = le32(sm.data() + pos + 20);
            d.sectionId = le32(sm.data() + pos + 24);
            d.encrypted = le32(sm.data() + pos + 28);
            pos += 0x20;
            d.name = cstr(sm.data() + pos, 0x40);
            pos += 0x40;
            for (uint32_t p = 0; p < d.pageCount; ++p) {
                PageRef ref;
                ref.pageId = static_cast<int>(le32(sm.data() + pos));
                ref.dataSize = le32(sm.data() + pos + 4);
                ref.startOffset = le64(sm.data() + pos + 8);
                pos += 0x10;
                d.pages.push_back(ref);
            }
            c.sections[d.name] = std::move(d);
        }
        auto it = c.sections.find("AcDb:AcDbObjects");
        if (it != c.sections.end()) {
            c.diag += "objPages=";
            for (const auto& pg : it->second.pages) c.diag += std::to_string(pg.pageId) + ",";
            c.diag += "; ";
        }
    }

    return c;
}

}  // namespace dwr
