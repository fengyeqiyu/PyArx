#include "dwg_reader/io/filer/dwg_out_filer.h"

#include <cstring>

namespace dwr {

void MemoryOutFiler::writeBool(bool v) { bytes.push_back(v ? 1 : 0); }
void MemoryOutFiler::writeU8(uint8_t v) { bytes.push_back(v); }

void MemoryOutFiler::writeI16(int16_t v) {
    bytes.push_back(static_cast<uint8_t>(v & 0xFF));
    bytes.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
}

void MemoryOutFiler::writeI32(int32_t v) {
    for (int i = 0; i < 4; ++i) bytes.push_back(static_cast<uint8_t>((v >> (8 * i)) & 0xFF));
}

void MemoryOutFiler::writeI64(int64_t v) {
    for (int i = 0; i < 8; ++i)
        bytes.push_back(static_cast<uint8_t>((v >> (8 * i)) & 0xFF));
}

void MemoryOutFiler::writeDouble(double v) {
    uint64_t bits;
    std::memcpy(&bits, &v, 8);
    for (int i = 0; i < 8; ++i)
        bytes.push_back(static_cast<uint8_t>((bits >> (8 * i)) & 0xFF));
}

void MemoryOutFiler::writeHandle(uint64_t v) { writeUmc(v); }

void MemoryOutFiler::writeUmc(uint64_t v) {
    while (v >= 0x80) {
        bytes.push_back(static_cast<uint8_t>(v & 0x7F) | 0x80);
        v >>= 7;
    }
    bytes.push_back(static_cast<uint8_t>(v));
}

void MemoryOutFiler::writeMc(int64_t v) {
    bool neg = v < 0;
    uint64_t a = neg ? static_cast<uint64_t>(-v) : static_cast<uint64_t>(v);
    while (a >= 0x40) {
        bytes.push_back(static_cast<uint8_t>((a & 0x7F) | 0x80));
        a >>= 7;
    }
    uint8_t last = static_cast<uint8_t>(a);
    if (neg) last |= 0x40;
    bytes.push_back(last);
}

void MemoryOutFiler::writeString(const std::string& s) {
    writeI16(static_cast<int16_t>(s.size()));
    writeBytes(reinterpret_cast<const uint8_t*>(s.data()), s.size());
}

void MemoryOutFiler::writeBytes(const uint8_t* p, size_t n) {
    if (p && n) bytes.insert(bytes.end(), p, p + n);
}

void MemoryOutFiler::u16le(uint16_t v) { writeI16(static_cast<int16_t>(v)); }
void MemoryOutFiler::u32le(uint32_t v) { writeI32(static_cast<int32_t>(v)); }

void MemoryOutFiler::u16be(uint16_t v) {
    bytes.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
    bytes.push_back(static_cast<uint8_t>(v & 0xFF));
}

void MemoryOutFiler::padTo(size_t n, uint8_t fill) {
    while (bytes.size() < n) bytes.push_back(fill);
}

void MemoryOutFiler::patchU32(size_t off, uint32_t v) {
    for (int i = 0; i < 4; ++i)
        if (off + i < bytes.size()) bytes[off + i] = static_cast<uint8_t>((v >> (8 * i)) & 0xFF);
}

}  // namespace dwr
