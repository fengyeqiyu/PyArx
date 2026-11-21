#include "dwg_reader/io/container/classic_container.h"

#include <array>
#include <fstream>
#include <stdexcept>

namespace dwr {

namespace {

constexpr std::array<const char*, 6> kSectionNames = {
    "AcDb:Header", "AcDb:Classes", "AcDb:Handles",
    "AcDb:ObjFreeSpace", "AcDb:Template", "AcDb:AuxHeader"};

uint32_t rd32(const uint8_t* p) {
    return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
           (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
}

uint16_t rd16be(const uint8_t* p) {
    return static_cast<uint16_t>((static_cast<uint16_t>(p[0]) << 8) | p[1]);
}

/// UMC：7 位一组，低字节在前，最高位 0x80 为继续位（ODA §4.3 / libredwg bit_read_UMC）。
uint64_t readUMC(const std::vector<uint8_t>& d, size_t& pos, size_t end) {
    uint64_t value = 0;
    int shift = 0;
    while (pos < end && shift < 64) {
        uint8_t b = d[pos++];
        if (b & 0x80) {
            value |= static_cast<uint64_t>(b & 0x7F) << shift;
            shift += 7;
        } else {
            value |= static_cast<uint64_t>(b) << shift;
            return value;
        }
    }
    return value;
}

/// MC：同 UMC，但终止字节的 0x40 位为符号位（负），参与数值的部分清掉该位。
int64_t readMC(const std::vector<uint8_t>& d, size_t& pos, size_t end) {
    uint64_t value = 0;
    int shift = 0;
    while (pos < end && shift < 64) {
        uint8_t b = d[pos++];
        if (b & 0x80) {
            value |= static_cast<uint64_t>(b & 0x7F) << shift;
            shift += 7;
        } else {
            bool negative = (b & 0x40) != 0;
            if (negative) b &= 0xBF;
            value |= static_cast<uint64_t>(b) << shift;
            return negative ? -static_cast<int64_t>(value) : static_cast<int64_t>(value);
        }
    }
    return static_cast<int64_t>(value);
}

}  // namespace

const ClassicSection* ClassicContainer::section(const std::string& name) const {
    for (const auto& s : sections) {
        if (s.name == name) return &s;
    }
    return nullptr;
}

ClassicContainer openClassicDwg(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("cannot open: " + path);
    std::vector<uint8_t> raw((std::istreambuf_iterator<char>(in)),
                             std::istreambuf_iterator<char>());
    if (raw.size() < 0x40) throw std::runtime_error("file too small");

    char code[7] = {};
    for (int i = 0; i < 6; ++i) code[i] = static_cast<char>(raw[i]);
    ClassicContainer c;
    c.version = code;
    c.ver = versionFromCode(code);
    if (c.ver == DwgVersion::kUnknown)
        throw std::runtime_error("unrecognized version code: " + c.version);

    // 文件头（R13/R14/R2000 同构，但写入器的 codepage 位置差 1 字节）：
    //   libredwg 样本：codepage RS@0x12 → sections RL@0x14 → 节定位器@0x18
    //   真 AutoCAD   ：codepage RS@0x13 → sections RL@0x15 → 节定位器@0x19
    // 用「节数合理 + 各节 address/size 不越界」判哪个布局成立。
    c.thumbnailAddress = rd32(raw.data() + 0x0C);
    size_t hdrBase = 0;  // codepage 所在偏移
    bool layoutOk = false;
    for (size_t cand : {size_t(0x12), size_t(0x13)}) {
        int cp = static_cast<int>(raw[cand]) | (static_cast<int>(raw[cand + 1]) << 8);
        int secs = static_cast<int>(rd32(raw.data() + cand + 2));
        if (secs < 3 || secs > 12) continue;
        const size_t locator = cand + 6;
        const size_t locEnd = locator + static_cast<size_t>(secs) * 9;
        bool ok = locEnd <= raw.size();
        for (int i = 0; ok && i < secs; ++i) {
            const uint64_t a = rd32(raw.data() + locator + i * 9 + 1);
            const uint32_t sz = rd32(raw.data() + locator + i * 9 + 5);
            if (a == 0 && sz == 0) continue;  // 可选节缺席（ObjFreeSpace/Template 等）
            ok = (static_cast<uint64_t>(a) + sz <= raw.size()) && a >= locEnd && sz > 0;
        }
        if (ok) {
            c.codepage = cp;
            c.declaredSections = secs;
            hdrBase = cand;
            layoutOk = true;
            break;
        }
    }
    if (!layoutOk) throw std::runtime_error("classic header layout not recognized");
    int sections = c.declaredSections;
    if (sections <= 0 || sections > 32)
        throw std::runtime_error("invalid section locator count: " + std::to_string(sections));

    size_t pos = hdrBase + 6;
    for (int i = 0; i < sections; ++i) {
        if (pos + 9 > raw.size()) throw std::runtime_error("section locator overflow");
        ClassicSection s;
        s.number = raw[pos];
        s.address = rd32(raw.data() + pos + 1);
        s.size = rd32(raw.data() + pos + 5);
        pos += 9;
        if (s.address + s.size > raw.size())
            throw std::runtime_error("section[" + std::to_string(i) + "] out of file bounds");
        s.name = (i < static_cast<int>(kSectionNames.size()))
                     ? kSectionNames[i]
                     : std::string("section") + std::to_string(i);
        c.sections.push_back(s);
    }
    c.data = std::move(raw);
    c.diag = "sections=" + std::to_string(c.sections.size()) + "; ";

    // Handles 节：逐页 RS_BE 页长 + 若干 {UMC handle 增量, MC 地址增量} + 页尾 RS_BE CRC。
    const ClassicSection* hdl = c.section("AcDb:Handles");
    if (!hdl) throw std::runtime_error("AcDb:Handles section not found");

    size_t pageStart = static_cast<size_t>(hdl->address);
    const size_t hdlEnd = pageStart + hdl->size;
    uint64_t lastHandle = 0;
    while (pageStart + 2 <= hdlEnd && pageStart + 2 <= c.data.size()) {
        int pageSize = rd16be(c.data.data() + pageStart);
        if (pageSize <= 2 || static_cast<size_t>(pageSize) > hdlEnd - pageStart) break;
        const size_t pageEnd = pageStart + static_cast<size_t>(pageSize);
        size_t p = pageStart + 2;
        uint64_t lastOffset = 0;
        uint32_t count = 0;
        while (p + 1 < pageEnd) {  // 最短条目 2 字节（UMC 1 + MC 1），末尾不留半条
            uint64_t handleOff = readUMC(c.data, p, pageEnd);
            int64_t addrOff = readMC(c.data, p, pageEnd);
            lastHandle += handleOff;
            lastOffset = static_cast<uint64_t>(static_cast<int64_t>(lastOffset) + addrOff);
            if (lastOffset >= c.data.size()) break;
            c.objectAddresses.push_back(lastOffset);
            c.objectHandles.push_back(lastHandle);
            ++count;
            if (p >= pageEnd) break;
        }
        if (count) c.handlePageEntries.push_back(count);
        pageStart = pageEnd + 2;  // 跳过页尾 CRC
        if (pageStart >= hdlEnd) break;
    }
    c.diag += "objects=" + std::to_string(c.objectAddresses.size()) + "; ";

    c.headerBase = static_cast<int>(hdrBase);
    if (!c.objectAddresses.empty()) {
        c.objectBase = c.objectAddresses.front();
        for (uint64_t a : c.objectAddresses)
            if (a < c.objectBase) c.objectBase = a;
        c.objectEnd = hdl->address;  // 对象区在 Handles 节之前结束
        c.diag += "objRange=" + std::to_string(c.objectBase) + ".." +
                  std::to_string(c.objectEnd) + "; ";
    }
    return c;
}

}  // namespace dwr
