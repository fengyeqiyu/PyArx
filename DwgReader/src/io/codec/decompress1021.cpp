#include "dwg_reader/io/codec/decompress1021.h"

#include <cstring>
#include <stdexcept>
#include <utility>

namespace dwr {

std::vector<uint8_t> decodeRs1021(const uint8_t* src, size_t src_len,
                                  int block_count, int data_size) {
    const size_t out_len = static_cast<size_t>(block_count) * data_size;
    if (out_len > src_len) throw std::runtime_error("decode_rs overflow");
    std::vector<uint8_t> dst(out_len, 0);
    for (int i = 0; i < block_count; ++i) {
        for (int j = 0; j < data_size; ++j) {
            dst[static_cast<size_t>(i) * data_size + j] =
                src[static_cast<size_t>(j) * block_count + i];
        }
    }
    return dst;
}

namespace {

size_t putAt(uint8_t* dst, size_t dp, const uint8_t* src, size_t sp,
             size_t offset, int width) {
    switch (width) {
        case 1:
            dst[dp] = src[sp + offset];
            return dp + 1;
        case 2:
            dst[dp] = src[sp + offset + 1];
            dst[dp + 1] = src[sp + offset];
            return dp + 2;
        case 3:
            dst[dp] = src[sp + offset + 2];
            dst[dp + 1] = src[sp + offset + 1];
            dst[dp + 2] = src[sp + offset];
            return dp + 3;
        case 4:
            std::memcpy(dst + dp, src + sp + offset, 4);
            return dp + 4;
        case 8:
            std::memcpy(dst + dp, src + sp + offset, 8);
            return dp + 8;
        case 16:
            std::memcpy(dst + dp, src + sp + offset + 8, 8);
            std::memcpy(dst + dp + 8, src + sp + offset, 8);
            return dp + 16;
        default:
            throw std::runtime_error("copy width");
    }
}

// literal 段拷贝，返回 {新 dp, 新 sp}。对应 copy_compressed_bytes。
std::pair<size_t, size_t> copyCompressedAt(uint8_t* dst, size_t dp,
                                           const uint8_t* src, size_t sp,
                                           size_t length) {
    while (length >= 32) {
        dp = putAt(dst, dp, src, sp, 16, 16);
        dp = putAt(dst, dp, src, sp, 0, 16);
        sp += 32;
        length -= 32;
    }
    switch (length) {
        case 0: break;
        case 1: dp = putAt(dst, dp, src, sp, 0, 1); break;
        case 2: dp = putAt(dst, dp, src, sp, 0, 2); break;
        case 3: dp = putAt(dst, dp, src, sp, 0, 3); break;
        case 4: dp = putAt(dst, dp, src, sp, 0, 4); break;
        case 5:
            dp = putAt(dst, dp, src, sp, 4, 1); dp = putAt(dst, dp, src, sp, 0, 4); break;
        case 6:
            dp = putAt(dst, dp, src, sp, 5, 1); dp = putAt(dst, dp, src, sp, 1, 4);
            dp = putAt(dst, dp, src, sp, 0, 1); break;
        case 7:
            dp = putAt(dst, dp, src, sp, 5, 2); dp = putAt(dst, dp, src, sp, 1, 4);
            dp = putAt(dst, dp, src, sp, 0, 1); break;
        case 8: dp = putAt(dst, dp, src, sp, 0, 8); break;
        case 9:
            dp = putAt(dst, dp, src, sp, 8, 1); dp = putAt(dst, dp, src, sp, 0, 8); break;
        case 10:
            dp = putAt(dst, dp, src, sp, 9, 1); dp = putAt(dst, dp, src, sp, 1, 8);
            dp = putAt(dst, dp, src, sp, 0, 1); break;
        case 11:
            dp = putAt(dst, dp, src, sp, 9, 2); dp = putAt(dst, dp, src, sp, 1, 8);
            dp = putAt(dst, dp, src, sp, 0, 1); break;
        case 12:
            dp = putAt(dst, dp, src, sp, 8, 4); dp = putAt(dst, dp, src, sp, 0, 8); break;
        case 13:
            dp = putAt(dst, dp, src, sp, 12, 1); dp = putAt(dst, dp, src, sp, 8, 4);
            dp = putAt(dst, dp, src, sp, 0, 8); break;
        case 14:
            dp = putAt(dst, dp, src, sp, 13, 1); dp = putAt(dst, dp, src, sp, 9, 4);
            dp = putAt(dst, dp, src, sp, 1, 8); dp = putAt(dst, dp, src, sp, 0, 1); break;
        case 15:
            dp = putAt(dst, dp, src, sp, 13, 2); dp = putAt(dst, dp, src, sp, 9, 4);
            dp = putAt(dst, dp, src, sp, 1, 8); dp = putAt(dst, dp, src, sp, 0, 1); break;
        case 16: dp = putAt(dst, dp, src, sp, 0, 16); break;
        case 17:
            dp = putAt(dst, dp, src, sp, 9, 8); dp = putAt(dst, dp, src, sp, 8, 1);
            dp = putAt(dst, dp, src, sp, 0, 8); break;
        case 18:
            dp = putAt(dst, dp, src, sp, 17, 1); dp = putAt(dst, dp, src, sp, 1, 16);
            dp = putAt(dst, dp, src, sp, 0, 1); break;
        case 19:
            dp = putAt(dst, dp, src, sp, 16, 3); dp = putAt(dst, dp, src, sp, 0, 16); break;
        case 20:
            dp = putAt(dst, dp, src, sp, 16, 4); dp = putAt(dst, dp, src, sp, 0, 16); break;
        case 21:
            dp = putAt(dst, dp, src, sp, 20, 1); dp = putAt(dst, dp, src, sp, 16, 4);
            dp = putAt(dst, dp, src, sp, 0, 16); break;
        case 22:
            dp = putAt(dst, dp, src, sp, 20, 2); dp = putAt(dst, dp, src, sp, 16, 4);
            dp = putAt(dst, dp, src, sp, 0, 16); break;
        case 23:
            dp = putAt(dst, dp, src, sp, 20, 3); dp = putAt(dst, dp, src, sp, 16, 4);
            dp = putAt(dst, dp, src, sp, 0, 16); break;
        case 24:
            dp = putAt(dst, dp, src, sp, 16, 8); dp = putAt(dst, dp, src, sp, 0, 16); break;
        case 25:
            dp = putAt(dst, dp, src, sp, 17, 8); dp = putAt(dst, dp, src, sp, 16, 1);
            dp = putAt(dst, dp, src, sp, 0, 16); break;
        case 26:
            dp = putAt(dst, dp, src, sp, 25, 1); dp = putAt(dst, dp, src, sp, 17, 8);
            dp = putAt(dst, dp, src, sp, 16, 1); dp = putAt(dst, dp, src, sp, 0, 16); break;
        case 27:
            dp = putAt(dst, dp, src, sp, 25, 2); dp = putAt(dst, dp, src, sp, 17, 8);
            dp = putAt(dst, dp, src, sp, 16, 1); dp = putAt(dst, dp, src, sp, 0, 16); break;
        case 28:
            dp = putAt(dst, dp, src, sp, 24, 4); dp = putAt(dst, dp, src, sp, 16, 8);
            dp = putAt(dst, dp, src, sp, 0, 16); break;
        case 29:
            dp = putAt(dst, dp, src, sp, 28, 1); dp = putAt(dst, dp, src, sp, 24, 4);
            dp = putAt(dst, dp, src, sp, 16, 8); dp = putAt(dst, dp, src, sp, 0, 16); break;
        case 30:
            dp = putAt(dst, dp, src, sp, 28, 2); dp = putAt(dst, dp, src, sp, 24, 4);
            dp = putAt(dst, dp, src, sp, 16, 8); dp = putAt(dst, dp, src, sp, 0, 16); break;
        case 31:
            dp = putAt(dst, dp, src, sp, 30, 1); dp = putAt(dst, dp, src, sp, 26, 4);
            dp = putAt(dst, dp, src, sp, 18, 8); dp = putAt(dst, dp, src, sp, 2, 16);
            dp = putAt(dst, dp, src, sp, 0, 2); break;
        default:
            throw std::runtime_error("wrong literal length");
    }
    return {dp, sp + length};
}

// read_literal_length，返回 {length, 新 sp}。
std::pair<size_t, size_t> readLiteralLength(const uint8_t* src, size_t sp,
                                            size_t src_end, int opcode) {
    size_t length = static_cast<size_t>(opcode + 8);
    if (length == 0x17) {
        if (sp >= src_end) throw std::runtime_error("literal length: read past end");
        int n = src[sp++];
        length += n;
        if (n == 0xFF) {
            while (true) {
                if (src_end - sp < 2) throw std::runtime_error("literal length: read past end (16)");
                n = src[sp] | (src[sp + 1] << 8);
                sp += 2;
                length += n;
                if (n != 0xFFFF) break;
            }
        }
    }
    return {length, sp};
}

// read_instructions，返回 {opcode, offset, length, 新 sp}。
struct Instr { int opcode; size_t offset; size_t length; size_t sp; };

Instr readInstructions(const uint8_t* src, size_t sp, size_t src_end, int opcode) {
    auto need = [&](int n) {
        if (src_end - sp < static_cast<size_t>(n)) throw std::runtime_error("instructions: read past end");
    };
    int hi = opcode >> 4;
    size_t offset = 0, length = 0;
    if (hi == 0) {
        length = static_cast<size_t>((opcode & 0xF) + 0x13);
        need(2);
        offset = src[sp++];
        opcode = src[sp++];
        length = (((opcode >> 3) & 0x10) + length);
        offset = (((opcode & 0x78) << 5) + 1 + offset);
    } else if (hi == 1) {
        length = static_cast<size_t>((opcode & 0xF) + 3);
        need(2);
        offset = src[sp++];
        opcode = src[sp++];
        offset = (((opcode & 0xF8) << 5) + 1 + offset);
    } else if (hi == 2) {
        need(2);
        offset = src[sp++];
        offset = (((src[sp] << 8) & 0xFF00) | offset);
        sp++;
        length = opcode & 7;
        if ((opcode & 8) == 0) {
            need(1);
            opcode = src[sp++];
            length = ((opcode & 0xF8) + length);
        } else {
            offset += 1;
            need(1);
            length = ((src[sp] << 3) + length);
            sp++;
            need(1);
            opcode = src[sp++];
            length = ((((opcode & 0xF8) << 8) + length) + 0x100);
        }
    } else {
        length = opcode >> 4;
        offset = opcode & 15;
        need(1);
        opcode = src[sp++];
        offset = ((((opcode & 0xF8) << 1) + offset) + 1);
    }
    return {opcode, offset, length, sp};
}

}  // namespace

std::vector<uint8_t> decompressR2007(size_t dst_size, const uint8_t* src,
                                     size_t src_size) {
    if (dst_size == 0 || src_size < 2) throw std::runtime_error("invalid argument to decompress_r2007");

    std::vector<uint8_t> dst(dst_size, 0);
    size_t dp = 0, sp = 0, src_end = src_size;
    size_t length = 0, offset = 0;
    int opcode = src[sp++];

    if ((opcode & 0xF0) == 0x20) {
        sp += 2;
        length = src[sp] & 0x07;
        sp++;
        if (length == 0) throw std::runtime_error("zero length");
    }

    while (sp < src_end) {
        if (length == 0) {
            auto [lit, nsp] = readLiteralLength(src, sp, src_end, opcode);
            length = lit;
            sp = nsp;
        }
        if (dp + length > dst_size) throw std::runtime_error("literal length overflow (dst)");
        if (sp + length > src_end) throw std::runtime_error("literal length overflow (src)");
        auto [ndp, nsp] = copyCompressedAt(dst.data(), dp, src, sp, length);
        dp = ndp;
        sp = nsp;

        length = 0;
        if (sp >= src_end) break;
        opcode = src[sp++];
        Instr ins = readInstructions(src, sp, src_end, opcode);
        opcode = ins.opcode;
        offset = ins.offset;
        length = ins.length;
        sp = ins.sp;

        while (true) {
            if (offset > dp) throw std::runtime_error("offset underflow");
            for (size_t k = 0; k < length; ++k) {
                dst[dp] = dst[dp - offset];
                ++dp;
            }
            length = opcode & 7;
            if (length != 0 || sp >= src_end) break;
            opcode = src[sp++];
            if ((opcode >> 4) == 0) break;
            if ((opcode >> 4) == 0x0F) opcode &= 0xF;
            ins = readInstructions(src, sp, src_end, opcode);
            opcode = ins.opcode;
            offset = ins.offset;
            length = ins.length;
            sp = ins.sp;
        }
    }
    return dst;
}

}  // namespace dwr
