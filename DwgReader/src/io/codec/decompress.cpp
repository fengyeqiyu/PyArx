#include "dwg_reader/io/codec/decompress.h"

#include <utility>

namespace dwr {

namespace {

struct State {
    const uint8_t* src;
    size_t pos = 0;
    size_t end;
    uint8_t* out;
    size_t limit;
    size_t base;
    size_t out_pos;

    State(const uint8_t* s, size_t src_len, size_t lim, uint8_t* o, size_t op, size_t b)
        : src(s), end(src_len), out(o), limit(lim), base(b), out_pos(op) {}

    uint8_t next() {
        if (pos >= end) throw DecompressError("input overrun");
        return src[pos++];
    }

    // 拷 n 个字面字节，再读一字节作为下一 opcode1。
    uint8_t copy_bytes(size_t n) {
        if (n == 0) return next();
        size_t e = pos + n;
        if (e > end) throw DecompressError("literal run past input");
        if (out_pos + n > limit) throw DecompressError("literal run past output");
        for (size_t k = 0; k < n; ++k) out[out_pos + k] = src[pos + k];
        out_pos += n;
        pos = e;
        return next();
    }

    void emit_match(size_t length, size_t offset) {
        if (offset == 0) {
            if (out_pos == 0) throw DecompressError("offset 0 match on empty output");
            uint8_t last = out[out_pos - 1];
            if (out_pos + length > limit) throw DecompressError("match past output");
            for (size_t k = 0; k < length; ++k) out[out_pos + k] = last;
            out_pos += length;
            return;
        }
        size_t start = out_pos - offset;
        if (start > out_pos) throw DecompressError("back-reference before start");
        if (out_pos + length > limit) throw DecompressError("match past output");
        for (size_t k = 0; k < length; ++k) {
            out[out_pos] = out[start];
            ++start;
            ++out_pos;
        }
    }
};

size_t read_literal_length(State& st, int opcode) {
    int low = opcode & 0x0F;
    if (low == 0) {
        int last = 0;
        while (true) {
            last = st.next();
            if (last == 0 && st.pos < st.end) low += 0xFF;
            else break;
        }
        low += 0xF + last;
    }
    return low + 3;
}

size_t compressed_bytes(State& st, int opcode, int bits) {
    int n = opcode & bits;
    if (n == 0) {
        int last = 0;
        while (true) {
            last = st.next();
            if (last == 0 && st.pos < st.end) n += 0xFF;
            else break;
        }
        n += last + bits;
    }
    return n + 2;
}

std::pair<size_t, int> two_byte_offset(State& st, int plus, int high = 0) {
    int first = st.next();
    int second = st.next();
    size_t offset = static_cast<size_t>(high | (first >> 2) | (second << 6));
    offset += plus;
    return {offset, first & 0x03};
}

}  // namespace

size_t decompress(const uint8_t* src, size_t src_len, size_t limit,
                  uint8_t* out, size_t out_pos, size_t base) {
    State st(src, src_len, limit, out, out_pos, base);
    if (st.end == 0) return st.out_pos;

    int opcode1 = st.next();
    if ((opcode1 & 0xF0) == 0) {
        size_t lit_length = read_literal_length(st, opcode1);
        opcode1 = st.copy_bytes(lit_length);
    } else {
        throw DecompressError("first byte is not a literal length");
    }

    while (st.pos < st.end && st.out_pos < st.limit && opcode1 != 0x11) {
        size_t comp_bytes = 0;
        size_t comp_offset = 0;
        int lit_count = 0;

        if (opcode1 == 0x11) break;
        if (0x10 <= opcode1 && opcode1 <= 0x1F) {
            comp_bytes = compressed_bytes(st, opcode1, 7);
            int high = (opcode1 & 8) << 11;
            auto [off, lit] = two_byte_offset(st, 0x4000, high);
            comp_offset = off;
            lit_count = lit;
        } else if (0x20 <= opcode1 && opcode1 <= 0x3F) {
            comp_bytes = compressed_bytes(st, opcode1, 0x1F);
            auto [off, lit] = two_byte_offset(st, 1);
            comp_offset = off;
            lit_count = lit;
        } else {
            comp_bytes = (opcode1 >> 4) - 1;
            int opcode2 = st.next();
            comp_offset = (((opcode1 >> 2) & 3) | (opcode2 << 2)) + 1;
            lit_count = opcode1 & 0x03;
        }

        st.emit_match(comp_bytes, comp_offset);

        if (lit_count == 0) {
            opcode1 = st.next();
            if ((opcode1 & 0xF0) == 0) lit_count = static_cast<int>(read_literal_length(st, opcode1));
        }
        if (lit_count) {
            if (st.out_pos + static_cast<size_t>(lit_count) <= st.limit) {
                opcode1 = st.copy_bytes(lit_count);
            } else {
                break;
            }
        }
    }
    return st.out_pos;
}

std::vector<uint8_t> decompress_to_vec(const uint8_t* src, size_t src_len,
                                       size_t max_out, size_t base) {
    std::vector<uint8_t> out(max_out, 0);
    decompress(src, src_len, max_out, out.data(), 0, base);
    return out;
}

}  // namespace dwr
