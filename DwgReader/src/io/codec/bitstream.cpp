#include "dwg_reader/io/codec/bitstream.h"

#include <cstring>

namespace dwr {

void BitStream::advance(int bits) {
    size_t total = byte_ * 8 + bit_ + bits;
    byte_ = total >> 3;
    bit_ = static_cast<int>(total & 7);
}

int BitStream::read_b() {
    if (byte_ >= len_) return 0;
    uint8_t byte = data_[byte_];
    int result = (byte & (0x80 >> bit_)) >> (7 - bit_);
    advance(1);
    return result;
}

int BitStream::read_bb() {
    if (byte_ >= len_) return 0;
    uint8_t byte = data_[byte_];
    int result;
    if (bit_ < 7) {
        result = (byte & (0xC0 >> bit_)) >> (6 - bit_);
    } else {
        result = (byte & 0x01) << 1;
        if (byte_ < len_ - 1) {
            byte = data_[byte_ + 1];
            result |= (byte & 0x80) >> 7;
        }
    }
    advance(2);
    return result;
}

int BitStream::read_rc() {
    if (byte_ >= len_) return 0;
    uint8_t byte = data_[byte_];
    int result;
    if (bit_ == 0) {
        result = byte;
        ++byte_;
    } else {
        result = byte << bit_;
        uint8_t nxt = (byte_ + 1 < len_) ? data_[byte_ + 1] : 0;
        result |= nxt >> (8 - bit_);
        advance(8);
    }
    return result & 0xFF;
}

int BitStream::read_rs() {
    int lo = read_rc();
    int hi = read_rc();
    return (hi << 8) | lo;
}

uint32_t BitStream::read_rl() {
    uint32_t w1 = static_cast<uint32_t>(read_rs());
    uint32_t w2 = static_cast<uint32_t>(read_rs());
    return ((w2 << 16) | w1) & 0xFFFFFFFFu;
}

uint64_t BitStream::read_rll() {
    uint64_t lo = read_rl();
    uint64_t hi = read_rl();
    return ((hi << 32) | lo) & 0xFFFFFFFFFFFFFFFFull;
}

uint64_t BitStream::read_rll_be() {
    uint64_t v = 0;
    for (int i = 0; i < 8; ++i) v = (v << 8) | read_rc();
    return v & 0xFFFFFFFFFFFFFFFFull;
}

double BitStream::read_rd() {
    uint64_t u = read_rll();
    double d;
    std::memcpy(&d, &u, sizeof(d));
    return d;
}

double BitStream::read_bd() {
    int code = read_bb();
    if (code == 0) return read_rd();
    if (code == 1) return 1.0;
    return 0.0;
}

uint32_t BitStream::read_bl() {
    int code = read_bb();
    if (code == 0) return read_rl();
    if (code == 1) return static_cast<uint32_t>(read_rc()) & 0xFFu;
    if (code == 2) return 0;
    return 256;
}

double BitStream::read_dd(double default_value) {
    int code = read_bb();
    if (code == 0) return default_value;
    if (code == 3) return read_rd();

    uint64_t u;
    std::memcpy(&u, &default_value, sizeof(u));
    if (code == 2) {
        int b4 = read_rc(), b5 = read_rc();
        int b0 = read_rc(), b1 = read_rc(), b2 = read_rc(), b3 = read_rc();
        uint8_t arr[8] = {static_cast<uint8_t>(b0), static_cast<uint8_t>(b1),
                          static_cast<uint8_t>(b2), static_cast<uint8_t>(b3),
                          static_cast<uint8_t>(b4), static_cast<uint8_t>(b5),
                          static_cast<uint8_t>((u >> 48) & 0xFF),
                          static_cast<uint8_t>((u >> 56) & 0xFF)};
        u = 0;
        for (int k = 7; k >= 0; --k) u = (u << 8) | arr[k];
    } else {
        int b0 = read_rc(), b1 = read_rc(), b2 = read_rc(), b3 = read_rc();
        uint8_t arr[8] = {static_cast<uint8_t>(b0), static_cast<uint8_t>(b1),
                          static_cast<uint8_t>(b2), static_cast<uint8_t>(b3),
                          static_cast<uint8_t>((u >> 32) & 0xFF),
                          static_cast<uint8_t>((u >> 40) & 0xFF),
                          static_cast<uint8_t>((u >> 48) & 0xFF),
                          static_cast<uint8_t>((u >> 56) & 0xFF)};
        u = 0;
        for (int k = 7; k >= 0; --k) u = (u << 8) | arr[k];
    }
    double d;
    std::memcpy(&d, &u, sizeof(d));
    return d;
}

void BitStream::read_2rd(double& x, double& y) {
    x = read_rd();
    y = read_rd();
}

void BitStream::read_2bd(double& x, double& y) {
    x = read_bd();
    y = read_bd();
}

void BitStream::read_3bd(double& x, double& y, double& z) {
    x = read_bd();
    y = read_bd();
    z = read_bd();
}

void BitStream::read_be(double& x, double& y, double& z) {
    if (read_b()) {
        x = 0.0; y = 0.0; z = 1.0;
    } else {
        read_3bd(x, y, z);
    }
}

double BitStream::read_bt() {
    if (read_b()) return 0.0;
    return read_bd();
}

std::string BitStream::read_tf(int n) {
    if (n < 0) n = 0;
    if (n > 65536) n = 65536;  // 错位保护：荒谬长度钳制，避免超大分配
    std::string s;
    s.reserve(n);
    for (int i = 0; i < n; ++i) s.push_back(static_cast<char>(read_rc()));
    return s;
}

std::string BitStream::read_tv() {
    int n = read_bs();
    return read_tf(n);
}

int BitStream::read_rs_be() {
    int hi = read_rc();
    int lo = read_rc();
    return (hi << 8) | lo;
}

int BitStream::read_ms() {
    int result = 0;
    int j = 0;
    for (int round = 0; round < 2; ++round) {
        int word = read_rs();
        if (!(word & 0x8000)) {
            result |= word << j;
            return result;
        }
        result |= (word & 0x7FFF) << j;
        j += 15;
    }
    return 0;
}

int BitStream::read_bs() {
    int code = read_bb();
    if (code == 0) return read_rs();
    if (code == 1) return read_rc() & 0xFF;
    if (code == 2) return 0;
    return 256;
}

int BitStream::read_bot() {
    int code = read_bb();
    if (code == 0) return read_rc();
    if (code == 1) return read_rc() + 0x1F0;
    return read_rs();
}

uint64_t BitStream::read_umc() {
    uint64_t result = 0;
    int j = 0;
    for (int i = 7; i >= 0; --i) {
        int b = read_rc();
        if (!(b & 0x80)) {
            result |= static_cast<uint64_t>(b) << j;
            return result;
        }
        result |= static_cast<uint64_t>(b & 0x7F) << j;
        j += 7;
    }
    return 0;
}

uint64_t BitStream::read_handle() {
    int c = read_rc();
    int sz = c & 0x0f;
    uint64_t v = 0;
    for (int i = 0; i < sz; ++i) v = (v << 8) | static_cast<uint64_t>(read_rc());
    return v;
}

int BitStream::read_mc() {
    int negative = 0;
    int result = 0;
    int j = 0;
    for (int i = 4; i >= 0; --i, j += 7) {
        int b = read_rc();
        if (!(b & 0x80)) {
            if (b & 0x40) {
                negative = 1;
                b &= 0xBF;
            }
            result |= b << j;
            return negative ? -result : result;
        }
        b &= 0x7F;
        result |= b << j;
    }
    return 0;
}

void BitStream::read_cmc(int& index, uint32_t& rgb, int& flag) {
    index = read_bs();
    rgb = read_bl();
    flag = read_rc();
    if (flag < 4) {
        if (flag & 1) read_tv();
        if (flag & 2) read_tv();
    }
}

}  // namespace dwr
