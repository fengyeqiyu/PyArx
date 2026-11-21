#include "dwg_reader/io/container/container1021.h"

#include <fstream>
#include <stdexcept>

#include "dwg_reader/io/codec/bitstream.h"
#include "dwg_reader/io/codec/decompress1021.h"

namespace dwr {

namespace {

constexpr size_t HEADER_FIXED = 0x3D8;
constexpr uint64_t PAGES_MAP_BASE = 0x480;
constexpr int RS_K_SYSTEM = 239;
constexpr int RS_K_DATA = 251;
constexpr int RS_N = 255;

// Dwg_R2007_Header 34 个 LE64 字段顺序（对照 probe_dump1021.HDR_FIELDS）。
enum HdrField {
    kHeaderSize = 0,
    kFileSize = 1,
    kPagesMapCrcComp = 2,
    kPagesMapCorrection = 3,
    kPagesMapCrcSeed = 4,
    kPagesMap2Offset = 5,
    kPagesMap2Id = 6,
    kPagesMapOffset = 7,
    kPagesMapId = 8,
    kHeader2Offset = 9,
    kPagesMapSizeComp = 10,
    kPagesMapSizeUncomp = 11,
    kPagesAmount = 12,
    kPagesMaxId = 13,
    kUnknown1 = 14,
    kUnknown2 = 15,
    kPagesMapCrcUncomp = 16,
    kUnknown3 = 17,
    kUnknown4 = 18,
    kUnknown5 = 19,
    kNumSections = 20,
    kSectionsMapCrcUncomp = 21,
    kSectionsMapSizeComp = 22,
    kSectionsMap2Id = 23,
    kSectionsMapId = 24,
    kSectionsMapSizeUncomp = 25,
    kSectionsMapCrcComp = 26,
    kSectionsMapCorrection = 27,
    kSectionsMapCrcSeed = 28,
    kStreamVersion = 29,
    kCrcSeed = 30,
    kCrcSeedEncoded = 31,
    kRandomSeed = 32,
    kHeaderCrc = 33,
};

uint32_t le32(const uint8_t* p) {
    return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
           (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
}

uint64_t le64(const uint8_t* p) {
    uint64_t v = 0;
    for (int i = 0; i < 8; ++i) v |= static_cast<uint64_t>(p[i]) << (8 * i);
    return v;
}

std::string utf16leToString(const uint8_t* p, size_t units) {
    std::string out;
    for (size_t i = 0; i < units; ++i) {
        uint16_t code = static_cast<uint16_t>(p[2 * i]) |
                        (static_cast<uint16_t>(p[2 * i + 1]) << 8);
        if (code == 0) continue;
        if (code < 0x80) {
            out.push_back(static_cast<char>(code));
        } else if (code < 0x800) {
            out.push_back(static_cast<char>(0xC0 | (code >> 6)));
            out.push_back(static_cast<char>(0x80 | (code & 0x3F)));
        } else {
            out.push_back(static_cast<char>(0xE0 | (code >> 12)));
            out.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (code & 0x3F)));
        }
    }
    return out;
}

// 读 R2007 文件头：RS 解交织 → 解压 34 个 LE64 字段。
std::vector<uint64_t> readHeader(const std::vector<uint8_t>& data) {
    if (data.size() < 0x80 + HEADER_FIXED) throw std::runtime_error("file too small");
    auto pedata = decodeRs1021(data.data() + 0x80, HEADER_FIXED, 3, RS_K_SYSTEM);
    uint32_t comprLen = le32(pedata.data() + 24);
    const size_t outSize = 34 * 8;
    std::vector<uint8_t> hdr;
    size_t avail = pedata.size() - 32;
    if (comprLen > 0) {
        hdr = decompressR2007(outSize, pedata.data() + 32, std::min<size_t>(avail, comprLen));
    } else {
        hdr.assign(pedata.begin() + 32, pedata.begin() + 32 + outSize);
    }
    std::vector<uint64_t> fields(34);
    for (int i = 0; i < 34; ++i) fields[i] = le64(hdr.data() + 8 * i);
    return fields;
}

uint32_t pageSizeIfRsCoded(uint32_t compSize) {
    size_t pesize = (compSize + 7) & ~static_cast<uint32_t>(7);
    int bc = static_cast<int>((pesize + RS_K_DATA - 1) / RS_K_DATA);
    return static_cast<uint32_t>((static_cast<size_t>(bc) * RS_N + 31) & ~static_cast<size_t>(31));
}

}  // namespace

const Page1021* Container1021::findPage(int pageId) const {
    auto it = pages.find(pageId);
    return it == pages.end() ? nullptr : &it->second;
}

std::vector<uint8_t> Container1021::readSystemPage(uint64_t addr, uint32_t compSize,
                                                    uint32_t uncompSize,
                                                    uint32_t correction) const {
    size_t pesize = ((compSize + 7) & ~static_cast<uint32_t>(7)) * correction;
    int bc = static_cast<int>((pesize + RS_K_SYSTEM - 1) / RS_K_SYSTEM);
    if (bc <= 0) throw std::runtime_error("invalid system page block_count");
    size_t pageSize = (static_cast<size_t>(bc) * RS_N + 7) & ~static_cast<size_t>(7);
    if (addr + pageSize > data.size()) throw std::runtime_error("system page overrun");

    auto pedata = decodeRs1021(data.data() + addr, pageSize, bc, RS_K_SYSTEM);
    size_t pedataSize = static_cast<size_t>(bc) * RS_K_SYSTEM;
    if (compSize < uncompSize) {
        return decompressR2007(uncompSize, pedata.data(), std::min<size_t>(pedataSize, compSize));
    }
    return std::vector<uint8_t>(pedata.begin(), pedata.begin() + uncompSize);
}

std::vector<uint8_t> Container1021::readSection(const std::string& name) const {
    auto it = sections.find(name);
    if (it == sections.end()) throw std::runtime_error("section not found: " + name);
    const Section1021& sec = it->second;
    std::vector<uint8_t> buf(sec.dataSize, 0);

    for (const auto& sp : sec.pages) {
        const Page1021* page = findPage(sp.pageId);
        if (!page) throw std::runtime_error("page not in page map");

        std::vector<uint8_t> pageBytes;
        if (sp.compSize != sp.uncompSize || page->size == pageSizeIfRsCoded(sp.compSize)) {
            // 数据页 RS k=251。
            size_t pesize = (sp.compSize + 7) & ~static_cast<uint32_t>(7);
            int bc = static_cast<int>((pesize + RS_K_DATA - 1) / RS_K_DATA);
            size_t pedataSize = static_cast<size_t>(bc) * RS_K_DATA;
            auto pedata = decodeRs1021(data.data() + page->offset, page->size, bc, RS_K_DATA);
            if (sp.compSize < sp.uncompSize) {
                pageBytes = decompressR2007(sp.uncompSize, pedata.data(),
                                            std::min<size_t>(pedataSize, sp.compSize));
            } else {
                pageBytes.assign(pedata.begin(), pedata.begin() + sp.uncompSize);
            }
        } else {
            pageBytes.assign(data.begin() + page->offset,
                             data.begin() + page->offset + sp.uncompSize);
        }
        std::copy(pageBytes.begin(), pageBytes.end(), buf.begin() + sp.offset);
    }
    return buf;
}

Container1021 openDwg1021(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("cannot open: " + path);
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)),
                              std::istreambuf_iterator<char>());
    if (data.size() < 6 || std::string(reinterpret_cast<const char*>(data.data()), 6) != "AC1021") {
        throw std::runtime_error("unsupported version; AC1021 expected");
    }

    Container1021 c;
    c.data = std::move(data);
    const auto hdr = readHeader(c.data);

    // --- Pages Map ---
    uint64_t pmAddr = PAGES_MAP_BASE + hdr[kPagesMapOffset];
    std::vector<uint8_t> pm = c.readSystemPage(
        pmAddr, static_cast<uint32_t>(hdr[kPagesMapSizeComp]),
        static_cast<uint32_t>(hdr[kPagesMapSizeUncomp]),
        static_cast<uint32_t>(hdr[kPagesMapCorrection]));

    {
        BitStream bs(pm.data(), pm.size());
        uint64_t offset = PAGES_MAP_BASE;
        uint64_t maxPages = hdr[kPagesAmount];
        while (bs.byte_ < pm.size() && (maxPages == 0 || c.pages.size() < maxPages)) {
            uint64_t size = bs.read_rll();
            uint64_t pid = bs.read_rll();
            c.pages[static_cast<int>(pid)] = Page1021{
                static_cast<uint32_t>(size), static_cast<int>(pid), offset};
            offset += size;
        }
    }

    // --- Sections Map ---
    const Page1021* secPage = c.findPage(static_cast<int>(hdr[kSectionsMapId]));
    if (!secPage) throw std::runtime_error("sections map page not found");
    std::vector<uint8_t> sm = c.readSystemPage(
        secPage->offset, static_cast<uint32_t>(hdr[kSectionsMapSizeComp]),
        static_cast<uint32_t>(hdr[kSectionsMapSizeUncomp]),
        static_cast<uint32_t>(hdr[kSectionsMapCorrection]));

    {
        BitStream bs(sm.data(), sm.size());
        while (bs.byte_ < sm.size()) {
            Section1021 s;
            s.dataSize = bs.read_rll();
            s.maxSize = bs.read_rll();
            s.encrypted = static_cast<uint32_t>(bs.read_rll());
            s.hashcode = bs.read_rll();
            s.nameLength = static_cast<uint32_t>(bs.read_rll());
            s.unknown = static_cast<uint32_t>(bs.read_rll());
            s.encoded = static_cast<uint32_t>(bs.read_rll());
            s.numPages = static_cast<uint32_t>(bs.read_rll());

            uint32_t nameUnits = s.nameLength & 1 ? s.nameLength + 1 : s.nameLength;
            std::vector<uint8_t> nameRaw(nameUnits * 2);
            for (uint32_t k = 0; k < nameUnits; ++k) nameRaw[k] = static_cast<uint8_t>(bs.read_rc());
            s.name = utf16leToString(nameRaw.data(), nameUnits);

            for (uint32_t p = 0; p < s.numPages; ++p) {
                SectionPage1021 sp;
                sp.offset = bs.read_rll();
                sp.size = static_cast<uint32_t>(bs.read_rll());
                sp.pageId = static_cast<int>(bs.read_rll());
                sp.uncompSize = static_cast<uint32_t>(bs.read_rll());
                sp.compSize = static_cast<uint32_t>(bs.read_rll());
                sp.checksum = static_cast<uint32_t>(bs.read_rll());
                sp.crc = static_cast<uint32_t>(bs.read_rll());
                s.pages.push_back(sp);
            }
            if (s.dataSize > 0) c.sections[s.name] = std::move(s);
        }
    }

    return c;
}

}  // namespace dwr
